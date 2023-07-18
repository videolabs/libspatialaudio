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

	}

	CAdmRenderer::~CAdmRenderer()
	{
	}

	bool CAdmRenderer::Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo, std::string HRTFPath, std::vector<Screen> reproductionScreen)
	{
		// Set the output layout
		m_RenderLayout = outputTarget;
		// Set the order to be used for the HOA rendering
		m_HoaOrder = hoaOrder;
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
		unsigned int ambiLayout = kAmblib_Stereo;
		switch (m_RenderLayout)
		{
		case OutputLayout::Stereo:
			ambiLayout = kAmblib_Stereo;
			m_outputLayout = GetMatchingLayout("0+2+0");
			break;
		case OutputLayout::Quad:
			ambiLayout = kAmblib_Quad;
			m_outputLayout = GetMatchingLayout("0+4+0");
			break;
		case OutputLayout::FivePointOne:
			ambiLayout = kAmblib_51;
			m_outputLayout = GetMatchingLayout("0+5+0");
			break;
		case OutputLayout::FivePointZero:
			ambiLayout = kAmblib_50;
			m_outputLayout = GetMatchingLayout("0+5+0");
			m_outputLayout.hasLFE = false;
			break;
		case OutputLayout::SevenPointOne:
			ambiLayout = kAmblib_71;
			m_outputLayout = GetMatchingLayout("0+7+0");
			break;
		case OutputLayout::SevenPointZero:
			ambiLayout = kAmblib_70;
			m_outputLayout = GetMatchingLayout("0+7+0");
			m_outputLayout.hasLFE = false;
			break;
		case OutputLayout::Binaural:
			ambiLayout = kAmblib_Dodecahedron;
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

		if (reproductionScreen.size() > 0)
			m_outputLayout.reproductionScreen = reproductionScreen;

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
				m_gainInterpDirect.push_back(CGainInterp());
				m_gainInterpDiffuse.push_back(CGainInterp());
				m_objectMetadata.push_back(ObjectMetadata());
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
			size_t nCh = m_outputLayout.channels.size();
			m_scatteringMatrix.resize(nCh);
			for (size_t iCh = 0; iCh < nCh; ++iCh)
			{
				m_scatteringMatrix[iCh].resize(nCh);
				for (size_t jCh = 0; jCh < nCh; ++jCh)
					m_scatteringMatrix[iCh][jCh] = (iCh == jCh ? (double)nCh - 2. : -2.) / (double)nCh;
			}
		}

		bool bHoaDecoderConfig = m_hoaDecoder.Configure(hoaOrder, true, nSamples, ambiLayout);
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

		for (size_t iSpk = 0; iSpk < m_outputLayout.channels.size(); ++iSpk)
			if (!m_outputLayout.channels[iSpk].isLFE)
				m_mapNoLfeToLfe.push_back(iSpk);

		// Set up the buffers holding the direct and diffuse speaker signals
		size_t nAmbiCh = m_outputLayout.channels.size();
		m_speakerOut.resize(m_nOutputChannels);
		m_speakerOutDirect.resize(m_nOutputChannels);
		m_speakerOutDiffuse.resize(m_nOutputChannels);
		m_hoaObjectDirect.resize(nAmbiCh);
		m_hoaObjectDiffuse.resize(nAmbiCh);
		for (unsigned int i = 0; i < m_nOutputChannels; ++i)
		{
			m_speakerOut[i].resize(nSamples, 0.f);

			m_speakerOutDirect[i].resize(nSamples, 0.f);
			m_speakerOutDiffuse[i].resize(nSamples, 0.f);
		}
		for (size_t i = 0; i < nAmbiCh; ++i)
		{
			m_hoaObjectDirect[i].resize(nSamples, 0.f);
			m_hoaObjectDiffuse[i].resize(nSamples, 0.f);
		}

		// A buffer of zeros to use to clear the HOA buffer
		m_pZeros = std::make_unique<float[]>(nSamples);
		memset(m_pZeros.get(), 0, m_nSamples * sizeof(float));

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

	void CAdmRenderer::AddObject(float* pIn, unsigned int nSamples, ObjectMetadata metadata, unsigned int nOffset)
	{
		// convert from cartesian to polar metadata (if required)
		toPolar(metadata);

		// Map from the track index to the corresponding panner index
		int nObjectInd = GetMatchingIndex(m_pannerTrackInd, metadata.trackInd, TypeDefinition::Objects);

		if (nObjectInd == -1) // this track was not declared at construction. Stopping here.
		{
			std::cerr << "AdmRender Warning: Expected a track index that was declared an Object in construction. Input will not be rendered." << std::endl;
			return;
		}

		// Check if the metadata has changed
		int iObj = m_channelToObjMap[nObjectInd];
		bool newMetadata = !(metadata == m_objectMetadata[iObj]);
		if (newMetadata)
		{
			// Store the metadata
			m_objectMetadata[iObj] = metadata;
			// Calculate a new gain vector with this metadata
			std::vector<double> directGainsNoLFE;
			std::vector<double> diffuseGainsNoLFE;
			m_objectGainCalc->CalculateGains(metadata, directGainsNoLFE, diffuseGainsNoLFE);

			// Apply scattering to the diffuse gains when output is binaural
			if (m_RenderLayout == OutputLayout::Binaural)
				diffuseGainsNoLFE = multiplyMatVec(m_scatteringMatrix, diffuseGainsNoLFE);

			std::vector<double> directGains;
			std::vector<double> diffuseGains;
			// Advance the index of any speakers after the LFE to leave a gap for the LFE
			if (m_outputLayout.hasLFE)
			{
				directGains.resize(m_nOutputChannels, 0.);
				diffuseGains.resize(m_nOutputChannels, 0.);
				for (unsigned int i = 0; i < m_nOutputChannels - 1; ++i)
				{
					directGains[m_mapNoLfeToLfe[i]] = directGainsNoLFE[i];
					diffuseGains[m_mapNoLfeToLfe[i]] = diffuseGainsNoLFE[i];
				}
			}
			else
			{
				directGains = directGainsNoLFE;
				diffuseGains = diffuseGainsNoLFE;
			}

			// Get the interpolation time
			unsigned int interpLength = 0;
			if (metadata.jumpPosition.flag)
				interpLength = metadata.jumpPosition.interpolationLength;
			else
				interpLength = metadata.blockLength;

			// Set the gains in the interpolators
			m_gainInterpDirect[iObj].SetGainVector(directGains, interpLength);
			m_gainInterpDiffuse[iObj].SetGainVector(diffuseGains, interpLength);
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

	void CAdmRenderer::AddHoa(float** pHoaIn, unsigned int nSamples, HoaMetadata metadata, unsigned int nOffset)
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

	void CAdmRenderer::AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, DirectSpeakerMetadata metadata, unsigned int nOffset)
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
			std::vector<double> gains = m_directSpeakerGainCalc->calculateGains(metadata);
			for (int iSpk = 0; iSpk < (int)m_nOutputChannels; ++iSpk)
				if (gains[iSpk] != 0.)
					for (int iSample = 0; iSample < (int)nSamples; ++iSample)
						m_speakerOut[iSpk][iSample + nOffset] += pDirSpkIn[iSample] * (float)gains[iSpk];
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
			m_hoaDecoder.Process(&m_hoaAudioOut, nSamples, pRender);
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
			std::fill(m_speakerOut[iCh].begin(), m_speakerOut[iCh].end(), 0.f);
		for (unsigned int iAmbiCh = 0; iAmbiCh < m_hoaObjectDirect.size(); ++iAmbiCh)
			std::fill(m_hoaObjectDiffuse[iAmbiCh].begin(), m_hoaObjectDiffuse[iAmbiCh].end(), 0.f);
	}

	void CAdmRenderer::ClearObjectDirectBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(m_speakerOutDirect[iCh].begin(), m_speakerOutDirect[iCh].end(), 0.f);
		for (unsigned int iAmbiCh = 0; iAmbiCh < m_hoaObjectDirect.size(); ++iAmbiCh)
			std::fill(m_hoaObjectDirect[iAmbiCh].begin(), m_hoaObjectDirect[iAmbiCh].end(), 0.f);
	}

	void CAdmRenderer::ClearObjectDiffuseBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(m_speakerOutDiffuse[iCh].begin(), m_speakerOutDiffuse[iCh].end(), 0.f);
	}

	int CAdmRenderer::GetMatchingIndex(std::vector<std::pair<unsigned int, TypeDefinition>> vector, unsigned int nElement, TypeDefinition trackType)
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

}
