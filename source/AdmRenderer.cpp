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

	CAdmRenderer::CAdmRenderer(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo)
	{
		// Set the output layout
		m_RenderLayout = outputTarget;
		// Set the order to be used for the HOA rendering
		m_HoaOrder = hoaOrder;
		// Set the maximum number of samples expected in a frame
		m_nSamples = nSamples;
		// Store the channel information
		channelInformation = channelInfo;
		// Configure the B-format buffers
		hoaAudioOut.Configure(hoaOrder, true, nSamples);

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
		directSpeakerGainCalc = std::make_unique<CAdmDirectSpeakersGainCalc>(m_outputLayout);
		// Set up the decorrelator
		decorrelate.Configure(m_outputLayout, nSamples);

		// Set up required processors based on channelInfo
		unsigned int nObject = 0;
		for (unsigned int iCh = 0; iCh < channelInfo.nChannels; ++iCh)
		{
			switch (channelInfo.typeDefinition[iCh])
			{
			case TypeDefinition::DirectSpeakers:
				pannerTrackInd.push_back({ iCh,TypeDefinition::DirectSpeakers });
				if (m_RenderLayout == OutputLayout::Binaural)
				{
					hoaEncoders.push_back(CAmbisonicEncoder());
					hoaEncoders[nObject++].Configure(hoaOrder, true, 0);
				}
				else
				{
					pointSourcePanners.push_back(CAdmPointSourcePanner(m_outputLayout));
				}
				break;
			case TypeDefinition::Matrix:
				break;
			case TypeDefinition::Objects:
				pannerTrackInd.push_back({ iCh,TypeDefinition::Objects });
				if (m_RenderLayout == OutputLayout::Binaural)
				{
					hoaEncoders.push_back(CAmbisonicEncoder());
					hoaEncoders[nObject++].Configure(hoaOrder, true, 0);
				}
				else
				{
					pointSourcePanners.push_back(CAdmPointSourcePanner(m_outputLayout));
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

		hoaDecoder.Configure(hoaOrder, true, nSamples, ambiLayout);
		m_nOutputChannels = hoaDecoder.GetSpeakerCount();
		if (m_RenderLayout == OutputLayout::Binaural)
			m_nOutputChannels = 2;

		unsigned int tailLength = 0;
		hoaBinaural.Configure(hoaOrder, true, nSampleRate, nSamples, tailLength);

		// Set up the buffers holding the direct and diffuse speaker signals
		speakerOut.resize(m_nOutputChannels);
		speakerOutDiffuse.resize(m_nOutputChannels);
		for (unsigned int i = 0; i < m_nOutputChannels; ++i)
		{
			speakerOut[i].resize(nSamples, 0.f);
			speakerOutDiffuse[i].resize(nSamples, 0.f);
		}

		// A buffer of zeros to use to clear the HOA buffer
		m_pZeros = std::make_unique<float[]>(nSamples);
		memset(m_pZeros.get(), 0., m_nSamples * sizeof(float));
	}

	CAdmRenderer::~CAdmRenderer()
	{
	}

	void CAdmRenderer::Reset()
	{
		m_bFirstFrame = true;
		decorrelate.Reset();
		ClearDirectSpeakerBuffer();
		ClearDiffuseSpeakerBuffer();
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
		int nObjectInd = GetMatchingIndex(pannerTrackInd, metadata.trackInd, TypeDefinition::Objects);

		if (nObjectInd == -1) // this track was not declared at construction. Stopping here.
			return;

		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// Set the position of the source from the metadata
			PolarPoint newPos;
			newPos.fAzimuth = DegreesToRadians((float)metadata.polarPosition.azimuth);
			newPos.fElevation = DegreesToRadians((float)metadata.polarPosition.elevation);
			newPos.fDistance = (float)metadata.polarPosition.distance;

			// Set the interpolation duration based on the conditions on page 35 of Rec. ITU-R BS.2127-0
			// If the flag is 
			float interpDur = 0.f;
			if (metadata.jumpPosition.flag && !m_bFirstFrame)
				interpDur = metadata.jumpPosition.interpolationLength;

			hoaEncoders[nObjectInd].SetPosition(newPos, interpDur);
			// Encode the audio and add it to the buffer for output
			hoaEncoders[nObjectInd].ProcessAccumul(pIn, nSamples, &hoaAudioOut);
		}
		else
			pointSourcePanners[nObjectInd].ProcessAccumul(metadata, pIn, speakerOut, speakerOutDiffuse, nSamples);

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
			hoaAudioOut.AddStream(pHoaIn[iHoaCh], iHoaChWrite, nSamples);
		}
	}

	void CAdmRenderer::AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, DirectSpeakerMetadata metadata)
	{
		// If no matching output speaker then pan using the point source panner
		// Set the position of the source from the metadata
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			PolarPoint newPos;
			newPos.fAzimuth = DegreesToRadians((float)metadata.polarPosition.azimuth);
			newPos.fElevation = DegreesToRadians((float)metadata.polarPosition.elevation);
			newPos.fDistance = (float)metadata.polarPosition.distance;
			int nDirectSpeakerInd = GetMatchingIndex(pannerTrackInd, metadata.trackInd, TypeDefinition::DirectSpeakers);
			hoaEncoders[nDirectSpeakerInd].SetPosition(newPos);
			// Encode the audio and add it to the buffer for output
			hoaEncoders[nDirectSpeakerInd].ProcessAccumul(pDirSpkIn, nSamples, &hoaAudioOut);
		}
		else
		{
			// Get the gain vector to be applied to the DirectSpeaker channel
			std::vector<double> gains = directSpeakerGainCalc->calculateGains(metadata);
			for (int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				if (gains[iSpk] != 0.)
					for (int iSample = 0; iSample < nSamples; ++iSample)
						speakerOut[iSpk][iSample] += pDirSpkIn[iSample] * gains[iSpk];
		}
	}

	void CAdmRenderer::GetRenderedAudio(float** pRender, unsigned int nSamples)
	{
		// For now, clear the input buffers
		for (int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			memset(pRender[iCh], 0.f, nSamples * sizeof(float));
		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// For binaural, everything has been rendered to HOA since we don't have any real loudspeaker system to target
			hoaBinaural.Process(&hoaAudioOut, pRender);
		}
		else
		{
			// Apply diffuseness filters and compensation delay
			decorrelate.Process(speakerOut, speakerOutDiffuse, nSamples);

			// Decode the HOA stream to the output buffer
			hoaDecoder.Process(&hoaAudioOut, nSamples, pRender);
			// Add the signals that have already been routed to the speaker layout to the output buffer
			for (unsigned int iSpk = 0; iSpk < m_nOutputChannels; ++iSpk)
				for (unsigned int iSample = 0; iSample < nSamples; ++iSample)
					pRender[iSpk][iSample] += speakerOut[iSpk][iSample] + speakerOutDiffuse[iSpk][iSample];
		}

		// Clear the HOA data for the next frame
		ClearHoaBuffer();
		// Clear the DirectSpeaker data for the next frame
		ClearDirectSpeakerBuffer();
		// Clear the data in the diffuse buffers
		ClearDiffuseSpeakerBuffer();

		m_bFirstFrame = false;
	}

	void CAdmRenderer::ClearHoaBuffer()
	{
		unsigned int nHoaSamples = hoaAudioOut.GetSampleCount();
		for (unsigned int iHoaCh = 0; iHoaCh < hoaAudioOut.GetChannelCount(); ++iHoaCh)
			hoaAudioOut.InsertStream(m_pZeros.get(), iHoaCh, nHoaSamples);
	}

	void CAdmRenderer::ClearDirectSpeakerBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(speakerOut[iCh].begin(),speakerOut[iCh].end(),0.);
	}

	void CAdmRenderer::ClearDiffuseSpeakerBuffer()
	{
		for (unsigned int iCh = 0; iCh < m_nOutputChannels; ++iCh)
			std::fill(speakerOutDiffuse[iCh].begin(), speakerOutDiffuse[iCh].end(), 0.);
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