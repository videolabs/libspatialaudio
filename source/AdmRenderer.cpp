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
#include<iostream>

namespace admrender {

	CAdmRenderer::CAdmRenderer(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo, std::string HRTFPath)
	{
		// Set the output layout
		m_RenderLayout = outputTarget;
		// Set the order to be used for the HOA rendering
		m_HoaOrder = hoaOrder;
		// Set the maximum number of samples expected in a frame
		m_nSamples = nSamples;
		// Store the channel information
		m_channelInformation = channelInfo;
		// Configure the B-format buffers
		m_hoaAudioOut.Configure(hoaOrder, true, nSamples);

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
			m_outputLayout.hasLFE;
			break;
		case OutputLayout::Binaural:
			ambiLayout = kAmblib_Dodecahedron;
			// Load stereo here because the layout is not used for binaural decoding
			m_outputLayout = GetMatchingLayout("0+2+0");
			break;
		default:
			break;
		}

		//Set up the direct gain calculator
		m_directSpeakerGainCalc = std::make_unique<CAdmDirectSpeakersGainCalc>(m_outputLayout);
		// Set up the decorrelator
		m_decorrelate.Configure(m_outputLayout, nSamples);

		// Set up required processors based on channelInfo
		unsigned int nObject = 0;
		for (unsigned int iCh = 0; iCh < channelInfo.nChannels; ++iCh)
		{
			switch (channelInfo.typeDefinition[iCh])
			{
			case TypeDefinition::DirectSpeakers:
				m_pannerTrackInd.push_back({ iCh,TypeDefinition::DirectSpeakers });
				if (m_RenderLayout == OutputLayout::Binaural)
				{
					m_hoaEncoders.push_back(CAmbisonicEncoder());
					m_hoaEncoders[nObject++].Configure(hoaOrder, true, 0);
				}
				else
				{
					m_pointSourcePanners.push_back(CAdmPointSourcePanner(m_outputLayout));
				}
				break;
			case TypeDefinition::Matrix:
				break;
			case TypeDefinition::Objects:
				m_pannerTrackInd.push_back({ iCh,TypeDefinition::Objects });
				if (m_RenderLayout == OutputLayout::Binaural)
				{
					m_hoaEncoders.push_back(CAmbisonicEncoder());
					m_hoaEncoders[nObject++].Configure(hoaOrder, true, 0);
				}
				else
				{
					m_pointSourcePanners.push_back(CAdmPointSourcePanner(m_outputLayout));
				}
				break;
			case TypeDefinition::HOA:
				break;
			case TypeDefinition::Binaural:
				break;
			default:
				break;
			}
		}

		m_hoaDecoder.Configure(hoaOrder, true, nSamples, ambiLayout);
		m_nOutputChannels = m_hoaDecoder.GetSpeakerCount();
		if (m_RenderLayout == OutputLayout::Binaural)
			m_nOutputChannels = 2;

		unsigned int tailLength = 0;
		bool binConfigured = m_hoaBinaural.Configure(hoaOrder, true, nSampleRate, nSamples, tailLength, HRTFPath);
		// TODO: if (!binConfigured) throw or configure outside the ctor
				

		// Set up the buffers holding the direct and diffuse speaker signals
		m_speakerOut.resize(m_nOutputChannels);
		m_speakerOutDirect.resize(m_nOutputChannels);
		m_speakerOutDiffuse.resize(m_nOutputChannels);
		for (unsigned int i = 0; i < m_nOutputChannels; ++i)
		{
			m_speakerOut[i].resize(nSamples, 0.f);
			m_speakerOutDirect[i].resize(nSamples, 0.f);
			m_speakerOutDiffuse[i].resize(nSamples, 0.f);
		}

		// A buffer of zeros to use to clear the HOA buffer
		m_pZeros = std::make_unique<float[]>(nSamples);
		memset(m_pZeros.get(), 0, m_nSamples * sizeof(float));
	}

	CAdmRenderer::~CAdmRenderer()
	{
	}

	void CAdmRenderer::Reset()
	{
		m_bFirstFrame = true;
		m_decorrelate.Reset();
		ClearOutputBuffer();
		ClearObjectDirectBuffer();
		ClearObjectDiffuseBuffer();
		ClearHoaBuffer();
	}

	void CAdmRenderer::AddObject(float* pIn, unsigned int nSamples, ObjectMetadata metadata)
	{
		if (metadata.cartesian)
		{
			std::cerr << "AdmRender Warning: Cartesian flag not implemented. Position will be converted to polar." << std::endl;
			// convert from cartesian to polar coordinates
			metadata.polarPosition = CartesianToPolar(metadata.cartesianPosition);
		}
		// Map from the track index to the corresponding panner index
		int nObjectInd = GetMatchingIndex(m_pannerTrackInd, metadata.trackInd, TypeDefinition::Objects);

		if (nObjectInd == -1) // this track was not declared at construction. Stopping here.
		{
			std::cerr << "AdmRender Warning: Expected a track index that was declared an Object in construction. Input will not be rendered." << std::endl;
			return;
		}

		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// If the output is set to binaural then convert the object to HOA so it is rendered with the HOA-to-binaural decoder
			// Set the position of the source from the metadata
			PolarPoint newPos;
			newPos.fAzimuth = DegreesToRadians((float)metadata.polarPosition.azimuth);
			newPos.fElevation = DegreesToRadians((float)metadata.polarPosition.elevation);
			newPos.fDistance = (float)metadata.polarPosition.distance;

			// Set the interpolation duration based on the conditions on page 35 of Rec. ITU-R BS.2127-0
			// If the flag is 
			double interpDur = 0.;
			if (metadata.jumpPosition.flag && !m_bFirstFrame)
				interpDur = metadata.jumpPosition.interpolationLength;

			m_hoaEncoders[nObjectInd].SetPosition(newPos, (float)interpDur);
			// Encode the audio and add it to the buffer for output
			m_hoaEncoders[nObjectInd].ProcessAccumul(pIn, nSamples, &m_hoaAudioOut);
		}
		else
			m_pointSourcePanners[nObjectInd].ProcessAccumul(metadata, pIn, m_speakerOutDirect, m_speakerOutDiffuse, nSamples);
	}

	void CAdmRenderer::AddHoa(float** pHoaIn, unsigned int nSamples, HoaMetadata metadata)
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
			m_hoaAudioOut.AddStream(pHoaIn[iHoaCh], iHoaChWrite, nSamples);
		}
	}

	void CAdmRenderer::AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, DirectSpeakerMetadata metadata)
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
			m_hoaEncoders[nDirectSpeakerInd].SetPosition(newPos);
			// Encode the audio and add it to the buffer for output
			m_hoaEncoders[nDirectSpeakerInd].ProcessAccumul(pDirSpkIn, nSamples, &m_hoaAudioOut);
		}
		else
		{
			// Get the gain vector to be applied to the DirectSpeaker channel
			std::vector<double> gains = m_directSpeakerGainCalc->calculateGains(metadata);
			for (int iSpk = 0; iSpk < (int)m_nOutputChannels; ++iSpk)
				if (gains[iSpk] != 0.)
					for (int iSample = 0; iSample < (int)nSamples; ++iSample)
						m_speakerOut[iSpk][iSample] += pDirSpkIn[iSample] * (float)gains[iSpk];
		}
	}

	void CAdmRenderer::AddBinaural(float** pBinIn, unsigned int nSamples)
	{
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// Add the binaural signals directly to the output buffer with no processing
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					m_speakerOut[iSpk][iSample] += pBinIn[iSpk][iSample];
		}
	}

	void CAdmRenderer::GetRenderedAudio(float** pRender, unsigned int nSamples)
	{
		// Clear the input buffers
		for (int iCh = 0; iCh < (int)m_nOutputChannels; ++iCh)
			memset(pRender[iCh], 0, nSamples * sizeof(float));
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// For binaural, everything has been rendered to HOA since we don't have any real loudspeaker system to target
			m_hoaBinaural.Process(&m_hoaAudioOut, pRender);
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

		m_bFirstFrame = false;
	}

	void CAdmRenderer::ClearHoaBuffer()
	{
		unsigned int nHoaSamples = m_hoaAudioOut.GetSampleCount();
		for (unsigned int iHoaCh = 0; iHoaCh < m_hoaAudioOut.GetChannelCount(); ++iHoaCh)
			m_hoaAudioOut.InsertStream(m_pZeros.get(), iHoaCh, nHoaSamples);
	}

	void CAdmRenderer::ClearOutputBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(m_speakerOut[iCh].begin(), m_speakerOut[iCh].end(), 0.);
	}

	void CAdmRenderer::ClearObjectDirectBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(m_speakerOutDirect[iCh].begin(), m_speakerOutDirect[iCh].end(),0.);
	}

	void CAdmRenderer::ClearObjectDiffuseBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(m_speakerOutDiffuse[iCh].begin(), m_speakerOutDiffuse[iCh].end(), 0.);
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