/*############################################################################*/
/*#                                                                          #*/
/*#  A renderer for ADM streams.                                             #*/
/*#  CAdmRenderer - ADM Renderer                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmRenderer.cpp                                          #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmRenderer.h"
#include<type_traits>
#include<iostream>

namespace admrender {

	CAdmRenderer::CAdmRenderer()
	{
		m_RenderLayout = OutputLayout::Stereo;
		m_nSamples = 0;
	}

	CAdmRenderer::~CAdmRenderer()
	{
		DeallocateBuffers(m_hoaObjectDirect, m_nAmbiChannels);
		DeallocateBuffers(m_hoaObjectDiffuse, m_nAmbiChannels);
		DeallocateBuffers(m_hoaDecodedOut, m_nOutputChannels);
		DeallocateBuffers(m_speakerOut, m_nOutputChannels);
		DeallocateBuffers(m_speakerOutDirect, m_nOutputChannels);
		DeallocateBuffers(m_speakerOutDiffuse, m_nOutputChannels);
	}

	bool CAdmRenderer::Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, const StreamInformation& channelInfo, std::string HRTFPath, std::vector<Screen> reproductionScreen)
	{
		// Set the output layout
		m_RenderLayout = outputTarget;
		// Set the order to be used for the HOA rendering
		m_HoaOrder = hoaOrder;
		m_nAmbiChannels = (m_HoaOrder + 1) * (m_HoaOrder + 1);
		if (m_HoaOrder > 3)
			return false; // only accepts orders 0 to 3
		// Set the maximum number of samples expected in a frame
		m_nSamples = nSamples;
		// Store the channel information
		m_channelInformation = channelInfo;
		// Configure the B-format buffers
		bool bHoaOutConfig = m_hoaAudioOut.Configure(hoaOrder, true, nSamples);
		if (!bHoaOutConfig)
			return false;

		// Set up the output layout
		Amblib_SpeakerSetUps ambiLayout = Amblib_SpeakerSetUps::kAmblib_Stereo;
		switch (m_RenderLayout)
		{
		case OutputLayout::Stereo:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_Stereo;
			m_outputLayout = GetMatchingLayout("0+2+0");
			break;
		case OutputLayout::Quad:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_Quad;
			m_outputLayout = GetMatchingLayout("0+4+0");
			m_hoaDecMap = { 0,1,2,3 }; // One-to-one mapping
			break;
		case OutputLayout::FivePointOne:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_51;
			m_outputLayout = GetMatchingLayout("0+5+0");
			m_hoaDecMap = { 0,1,4,5,2,3 }; // M+030, M-030, M+110, M-110, M+000, LFE to M+030, M-030, LFE, M+000, M+110, M-110
			break;
		case OutputLayout::FivePointZero:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_50;
			m_outputLayout = getLayoutWithoutLFE(GetMatchingLayout("0+5+0"));
			m_outputLayout.hasLFE = false;
			m_hoaDecMap = { 0,1,4,2,3 }; // M+030, M-030, M+110, M-110, M+000 to M+030, M-030, M+000, M+110, M-110
			break;
		case OutputLayout::SevenPointOne:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_71;
			m_outputLayout = GetMatchingLayout("0+7+0");
			m_hoaDecMap = { 0,1,6,7,2,3,4,5 }; // M+030, M-030, M+090, M-090, M+135, M-135, M+000, LFE to M+030, M-030, M+000, LFE, M+090, M-090, M+135, M-135
			break;
		case OutputLayout::SevenPointZero:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_70;
			m_outputLayout = getLayoutWithoutLFE(GetMatchingLayout("0+7+0"));
			m_outputLayout.hasLFE = false;
			m_hoaDecMap = { 0,1,6,2,3,4,5 }; // M+030, M-030, M+090, M-090, M+135, M-135, M+000 to M+030, M-030, M+000, M+090, M-090, M+135, M-135
			break;
		case OutputLayout::Binaural:
			ambiLayout = Amblib_SpeakerSetUps::kAmblib_Dodecahedron;
			// This is only used in the setting the decorrelator when using binaural output
			switch (hoaOrder)
			{
			case 0: m_outputLayout = GetMatchingLayout("0+2+0"); break;
			case 1: m_outputLayout = GetMatchingLayout("1OA"); break;
			case 2: m_outputLayout = GetMatchingLayout("2OA"); break;
			case 3: m_outputLayout = GetMatchingLayout("3OA"); break;
			default:
				break;
			}

			break;
		default:
			break;
		}

		unsigned int nInternalCh = (unsigned int)m_outputLayout.channels.size();

		if (reproductionScreen.size() > 0)
		{
			m_outputLayout.reproductionScreen = reproductionScreen;
			m_objMetaDataTmp.referenceScreen = reproductionScreen;
		}

		// Clear the vectors containing the HOA and panning objects so that if the renderer is
		// reconfigured the mappings will be correct
		m_hoaEncoders.clear();
		m_pannerTrackInd.clear();
		m_objectMetadata.clear();
		m_channelToObjMap.clear();

		// Set up required processors based on channelInfo
		unsigned int nObject = 0;
		int iObj = 0;
		for (unsigned int iCh = 0; iCh < channelInfo.nChannels; ++iCh)
		{
			switch (channelInfo.typeDefinition[iCh])
			{
			case TypeDefinition::DirectSpeakers:
				m_pannerTrackInd.push_back({ iCh,TypeDefinition::DirectSpeakers });
				if (m_RenderLayout == OutputLayout::Binaural)
				{
					m_hoaEncoders.push_back(std::vector<CAmbisonicEncoder>(1, CAmbisonicEncoder()));
					m_hoaEncoders[nObject++][0].Configure(hoaOrder, true, 0);
				}
				break;
			case TypeDefinition::Matrix:
				break;
			case TypeDefinition::Objects:
				m_pannerTrackInd.push_back({ iCh,TypeDefinition::Objects });
				m_gainInterpDirect.push_back(CGainInterp(nInternalCh));
				m_gainInterpDiffuse.push_back(CGainInterp(nInternalCh));
				m_objectMetadata.push_back(ObjectMetadata());
				if (reproductionScreen.size() > 0)
					m_objectMetadata.back().referenceScreen = reproductionScreen;
				m_channelToObjMap.insert(std::pair<int, int>(iCh, iObj++));
				break;
			case TypeDefinition::HOA:
				break;
			case TypeDefinition::Binaural:
				break;
			default:
				break;
			}
		}

		// Set up the gain calculator
		m_objectGainCalc = std::make_unique<CGainCalculator>(m_outputLayout);
		//Set up the direct gain calculator if output is not binaural
		if (m_RenderLayout != OutputLayout::Binaural)
			m_directSpeakerGainCalc = std::make_unique<CAdmDirectSpeakersGainCalc>(m_outputLayout);
		// Set up the decorrelator
		bool bDecorConfig = m_decorrelate.Configure(m_outputLayout, nSamples);
		if (!bDecorConfig)
			return false;
		// Set up the Householder matrix matrix to apply to the diffuse gains when output is binaural
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			m_scatteringMatrix.resize(nInternalCh);
			for (size_t iCh = 0; iCh < nInternalCh; ++iCh)
			{
				m_scatteringMatrix[iCh].resize(nInternalCh);
				for (size_t jCh = 0; jCh < nInternalCh; ++jCh)
					m_scatteringMatrix[iCh][jCh] = (iCh == jCh ? (double)nInternalCh - 2. : -2.) / (double)nInternalCh;
			}
		}

		bool bHoaDecoderConfig = m_hoaDecoder.Configure(hoaOrder, true, nSamples, nSampleRate, ambiLayout);
		if (!bHoaDecoderConfig)
			return false;

		m_nOutputChannels = m_hoaDecoder.GetSpeakerCount();
		if (m_RenderLayout == OutputLayout::Binaural)
			m_nOutputChannels = 2;

		unsigned int tailLength = 0;
		bool bBinConf = m_hoaBinaural.Configure(hoaOrder, true, nSampleRate, nSamples, tailLength, HRTFPath);
		if (!bBinConf)
			return false;

		// If the output layout has an LFE then get its index

		for (unsigned int iSpk = 0; iSpk < nInternalCh; ++iSpk)
			if (!m_outputLayout.channels[iSpk].isLFE)
				m_mapNoLfeToLfe.push_back(iSpk);

		// Set up the buffers holding the direct and diffuse speaker signals
		AllocateBuffers(m_speakerOut, m_nOutputChannels, nSamples);
		AllocateBuffers(m_speakerOutDirect, m_nOutputChannels, nSamples);
		AllocateBuffers(m_speakerOutDiffuse, m_nOutputChannels, nSamples);
		AllocateBuffers(m_hoaObjectDirect, m_nAmbiChannels, nSamples);
		AllocateBuffers(m_hoaObjectDiffuse, m_nAmbiChannels, nSamples);
		AllocateBuffers(m_hoaDecodedOut, m_nOutputChannels, nSamples);

		// A buffer of zeros to use to clear the HOA buffer
		m_pZeros = std::make_unique<float[]>(nSamples);
		memset(m_pZeros.get(), 0, m_nSamples * sizeof(float));

		// Allocate vectors used during gain calculations
		auto nOutputChannelsNoLFE = getLayoutWithoutLFE(m_outputLayout).channels.size();
		m_directGainsNoLFE.resize(nOutputChannelsNoLFE);
		m_diffuseGainsNoLFE.resize(nOutputChannelsNoLFE);
		m_directGains.resize(nInternalCh);
		m_diffuseGains.resize(nInternalCh);
		m_directSpeakerGains.resize(m_nOutputChannels);

		return true;
	}


	void CAdmRenderer::Reset()
	{
		m_decorrelate.Reset();
		ClearOutputBuffer();
		ClearObjectDirectBuffer();
		ClearObjectDiffuseBuffer();
		ClearHoaBuffer();

		for (size_t i = 0; i < m_gainInterpDirect.size(); ++i)
		{
			m_gainInterpDiffuse[i].Reset();
			m_gainInterpDirect[i].Reset();
		}
	}

	void CAdmRenderer::AddObject(float* pIn, unsigned int nSamples, const ObjectMetadata& metadata, unsigned int nOffset)
	{
		// convert from cartesian to polar metadata (if required)
		toPolar(metadata, m_objMetaDataTmp);

		// Map from the track index to the corresponding panner index
		int nObjectInd = GetMatchingIndex(m_pannerTrackInd, m_objMetaDataTmp.trackInd, TypeDefinition::Objects);

		if (nObjectInd == -1) // this track was not declared at construction. Stopping here.
		{
			std::cerr << "AdmRender Warning: Expected a track index that was declared an Object in construction. Input will not be rendered." << std::endl;
			return;
		}

		// Check if the metadata has changed
		int iObj = m_channelToObjMap[nObjectInd];
		bool newMetadata = !(m_objMetaDataTmp == m_objectMetadata[iObj]);
		if (newMetadata)
		{
			// Store the metadata
			m_objectMetadata[iObj] = m_objMetaDataTmp;
			// Calculate a new gain vector with this metadata
			m_objectGainCalc->CalculateGains(m_objMetaDataTmp, m_directGainsNoLFE, m_diffuseGainsNoLFE);

			// Apply scattering to the diffuse gains when output is binaural
			if (m_RenderLayout == OutputLayout::Binaural)
				multiplyMatVec(m_scatteringMatrix, m_diffuseGainsNoLFE, m_diffuseGainsNoLFE);

			// Advance the index of any speakers after the LFE to leave a gap for the LFE
			if (m_outputLayout.hasLFE)
			{
				m_directGains.resize(m_nOutputChannels, 0.);
				m_diffuseGains.resize(m_nOutputChannels, 0.);
				for (unsigned int i = 0; i < m_nOutputChannels - 1; ++i)
				{
					m_directGains[m_mapNoLfeToLfe[i]] = m_directGainsNoLFE[i];
					m_diffuseGains[m_mapNoLfeToLfe[i]] = m_diffuseGainsNoLFE[i];
				}
			}
			else
			{
				m_directGains = m_directGainsNoLFE;
				m_diffuseGains = m_diffuseGainsNoLFE;
			}

			// Get the interpolation time
			unsigned int interpLength = 0;
			if (m_objMetaDataTmp.jumpPosition.flag)
				interpLength = m_objMetaDataTmp.jumpPosition.interpolationLength;
			else
				interpLength = m_objMetaDataTmp.blockLength;

			// Set the gains in the interpolators
			m_gainInterpDirect[iObj].SetGainVector(m_directGains, interpLength);
			m_gainInterpDiffuse[iObj].SetGainVector(m_diffuseGains, interpLength);
		}

		if (m_RenderLayout == OutputLayout::Binaural)
		{
			m_gainInterpDirect[iObj].ProcessAccumul(pIn, m_hoaObjectDirect, nSamples, nOffset);
			m_gainInterpDiffuse[iObj].ProcessAccumul(pIn, m_hoaObjectDiffuse, nSamples, nOffset);
		}
		else
		{
			m_gainInterpDirect[iObj].ProcessAccumul(pIn, m_speakerOutDirect, nSamples, nOffset);
			m_gainInterpDiffuse[iObj].ProcessAccumul(pIn, m_speakerOutDiffuse, nSamples, nOffset);
		}
	}

	void CAdmRenderer::AddHoa(float** pHoaIn, unsigned int nSamples, const HoaMetadata& metadata, unsigned int nOffset)
	{
		if (metadata.normalization != "SN3D")
		{
			std::cerr << "AdmRender Warning: Only SN3D normalisation supported. HOA signal not added to rendering." << std::endl;
			return;
		}

		unsigned int nHoaCh = (unsigned int)metadata.orders.size();
		for (unsigned int iHoaCh = 0; iHoaCh < nHoaCh; ++iHoaCh)
		{
			int order = metadata.orders[iHoaCh];
			int degree = metadata.degrees[iHoaCh];
			// which HOA channel to write to based on the order and degree
			unsigned int iHoaChWrite = order * (order + 1) + degree;
			m_hoaAudioOut.AddStream(pHoaIn[iHoaCh], iHoaChWrite, nSamples, nOffset);
		}
	}

	void CAdmRenderer::AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, const DirectSpeakerMetadata& metadata, unsigned int nOffset)
	{
		// If no matching output speaker then pan using the point source panner
		// Set the position of the source from the metadata
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			PolarPoint newPos;
			// Get the speaker position based on the nominal speaker direction
			auto spkPosIt = bs2094Positions.find(GetNominalSpeakerLabel(metadata.speakerLabel));
			if (spkPosIt != bs2094Positions.end())
			{
				newPos.fAzimuth = DegreesToRadians((float)spkPosIt->second.azimuth);
				newPos.fElevation = DegreesToRadians((float)spkPosIt->second.elevation);
				newPos.fDistance = (float)spkPosIt->second.distance;
			}
			else
			{
				// If there is no matching speaker name then take the direction from the metadata
				newPos.fAzimuth = DegreesToRadians((float)metadata.polarPosition.azimuth);
				newPos.fElevation = DegreesToRadians((float)metadata.polarPosition.elevation);
				newPos.fDistance = (float)metadata.polarPosition.distance;
			}
			int nDirectSpeakerInd = GetMatchingIndex(m_pannerTrackInd, metadata.trackInd, TypeDefinition::DirectSpeakers);
			m_hoaEncoders[nDirectSpeakerInd][0].SetPosition(newPos);
			// Encode the audio and add it to the buffer for output
			m_hoaEncoders[nDirectSpeakerInd][0].ProcessAccumul(pDirSpkIn, nSamples, &m_hoaAudioOut, nOffset);
		}
		else
		{
			// Get the gain vector to be applied to the DirectSpeaker channel
			m_directSpeakerGainCalc->calculateGains(metadata, m_directSpeakerGains);
			for (int iSpk = 0; iSpk < (int)m_nOutputChannels; ++iSpk)
				if (m_directSpeakerGains[iSpk] != 0.)
					for (int iSample = 0; iSample < (int)nSamples; ++iSample)
						m_speakerOut[iSpk][iSample + nOffset] += pDirSpkIn[iSample] * (float)m_directSpeakerGains[iSpk];
		}
	}

	void CAdmRenderer::AddBinaural(float** pBinIn, unsigned int nSamples, unsigned int nOffset)
	{
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// Add the binaural signals directly to the output buffer with no processing
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					m_speakerOut[iSpk][iSample + nOffset] += pBinIn[iSpk][iSample];
		}
	}

	void CAdmRenderer::GetRenderedAudio(float** pRender, unsigned int nSamples)
	{
		// Clear the input buffers
		for (int iCh = 0; iCh < (int)m_nOutputChannels; ++iCh)
			memset(pRender[iCh], 0, nSamples * sizeof(float));
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// Apply diffuseness filters and compensation delay
			m_decorrelate.Process(m_hoaObjectDirect, m_hoaObjectDiffuse, nSamples);

			// Add the direct and diffuse signals to the HOA and DirectSpeaker signals
			for (unsigned int iCh = 0; iCh < m_hoaAudioOut.GetChannelCount(); ++iCh)
			{
				m_hoaAudioOut.AddStream(&m_hoaObjectDirect[iCh][0], iCh, m_hoaAudioOut.GetSampleCount());
				m_hoaAudioOut.AddStream(&m_hoaObjectDiffuse[iCh][0], iCh, m_hoaAudioOut.GetSampleCount());
			}
			// For binaural, everything has been rendered to HOA since we don't have any real loudspeaker system to target
			m_hoaBinaural.Process(&m_hoaAudioOut, pRender, nSamples);
			// Add any Binaural type signals to the output buffer
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					pRender[iSpk][iSample] += m_speakerOut[iSpk][iSample];
		}
		else
		{
			// Apply diffuseness filters and compensation delay
			m_decorrelate.Process(m_speakerOutDirect, m_speakerOutDiffuse, nSamples);

			// Decode the HOA stream to the output buffer
			m_hoaDecoder.Process(&m_hoaAudioOut, nSamples, m_hoaDecodedOut);
			// Remap the decoded signal to the ADM channel order
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					pRender[iSpk][iSample] = m_hoaDecodedOut[m_hoaDecMap[iSpk]][iSample];
			// Add the signals that have already been routed to the speaker layout to the output buffer
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					pRender[iSpk][iSample] += m_speakerOut[iSpk][iSample] + m_speakerOutDirect[iSpk][iSample] + m_speakerOutDiffuse[iSpk][iSample];
		}

		// Clear the HOA data for the next frame
		ClearHoaBuffer();
		// Clear the output buffer
		ClearOutputBuffer();
		// Clear the Object direct signal data
		ClearObjectDirectBuffer();
		// Clear the data in the diffuse buffers
		ClearObjectDiffuseBuffer();
	}

	void CAdmRenderer::ClearHoaBuffer()
	{
		unsigned int nHoaSamples = m_hoaAudioOut.GetSampleCount();
		for (unsigned int iHoaCh = 0; iHoaCh < m_hoaAudioOut.GetChannelCount(); ++iHoaCh)
		{
			m_hoaAudioOut.InsertStream(m_pZeros.get(), iHoaCh, nHoaSamples);
		}
	}

	void CAdmRenderer::ClearOutputBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			for (unsigned int iSamp = 0; iSamp < m_nSamples; ++iSamp)
				m_speakerOut[iCh][iSamp] = 0.f;
		for (unsigned int iAmbiCh = 0; iAmbiCh < m_nAmbiChannels; ++iAmbiCh)
			for (unsigned int iSamp = 0; iSamp < m_nSamples; ++iSamp)
				m_hoaObjectDiffuse[iAmbiCh][iSamp] = 0.f;
	}

	void CAdmRenderer::ClearObjectDirectBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			for (unsigned int iSamp = 0; iSamp < m_nSamples; ++iSamp)
				m_speakerOutDirect[iCh][iSamp] = 0.f;
		for (unsigned int iAmbiCh = 0; iAmbiCh < m_nAmbiChannels; ++iAmbiCh)
			for (unsigned int iSamp = 0; iSamp < m_nSamples; ++iSamp)
				m_hoaObjectDirect[iAmbiCh][iSamp] = 0.f;
	}

	void CAdmRenderer::ClearObjectDiffuseBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			for (unsigned int iSamp = 0; iSamp < m_nSamples; ++iSamp)
				m_speakerOutDiffuse[iCh][iSamp] = 0.f;
	}

	int CAdmRenderer::GetMatchingIndex(const std::vector<std::pair<unsigned int, TypeDefinition>>& vector, unsigned int nElement, TypeDefinition trackType)
	{
		// Map from the track index to the corresponding panner index
		int nInd = 0;
		for (unsigned int i = 0; i < vector.size(); i++)
			if (vector[i].first == nElement && vector[i].second == trackType) {
				nInd = (int)i;
				return nInd;
			}

		return -1;
	}

	void CAdmRenderer::AllocateBuffers(float**& buffers, unsigned nCh, unsigned nSamples)
	{
		DeallocateBuffers(buffers, nCh);
		buffers = new float* [nCh];
		for (unsigned int iCh = 0; iCh < nCh; ++iCh)
		{
			buffers[iCh] = new float[nSamples];
			memset(buffers[iCh], 0, m_nSamples * sizeof(float));
		}
	}

	void CAdmRenderer::DeallocateBuffers(float**& buffers, unsigned nCh)
	{
		if (buffers)
		{
			for (unsigned int iCh = 0; iCh < nCh; ++iCh)
				if (buffers[iCh])
					delete buffers[iCh];
			delete[] buffers;
		}
	}

}
