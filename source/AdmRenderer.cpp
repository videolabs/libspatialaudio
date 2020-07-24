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

	CAdmRenderer::CAdmRenderer()
	{

	}

	CAdmRenderer::~CAdmRenderer()
	{
	}

	bool CAdmRenderer::Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo, std::string HRTFPath)
	{
		// Set the output layout
		m_RenderLayout = outputTarget;
		// Set the order to be used for the HOA rendering
		m_HoaOrder = hoaOrder;
		if (m_HoaOrder > 3 || m_HoaOrder < 0)
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
		bool bDecorConfig = m_decorrelate.Configure(m_outputLayout, nSamples);
		if (!bDecorConfig)
			return false;

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
				m_objectMetadata.push_back(ObjectMetadata());
				m_objectMetadataProc.push_back(ObjectMetadata());
				m_channelToObjMap.insert(std::pair<int, int>(iCh, iObj++));
				m_lastObjPos.push_back(PolarPosition());
				break;
			case TypeDefinition::HOA:
				break;
			case TypeDefinition::Binaural:
				break;
			default:
				break;
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


	void CAdmRenderer::Reset()
	{
		m_bFirstFrame = true;
		m_decorrelate.Reset();
		ClearOutputBuffer();
		ClearObjectDirectBuffer();
		ClearObjectDiffuseBuffer();
		ClearHoaBuffer();
	}

	void CAdmRenderer::AddObject(float* pIn, unsigned int nSamples, ObjectMetadata metadata, unsigned int nOffset)
	{
		if (metadata.cartesian)
		{
			std::cerr << "AdmRender Warning: Cartesian flag not implemented. Position will be converted to polar." << std::endl;
			// convert from cartesian to polar coordinates
			metadata.polarPosition = CartesianToPolar(metadata.cartesianPosition);
			metadata.cartesian = false;
		}
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
			m_objectMetadata[iObj] = metadata;
			m_objectMetadataProc[iObj] = metadata;
		}

		// Get a block of metadata corresponding to the nSamples of the input
		ObjectMetadata metadataBlock = m_objectMetadataProc[iObj];

		if (m_objectMetadataProc[iObj].jumpPosition.interpolationLength > 0) // if the last set metadata still requires interpolation
		{
			// The interpolation length for the block is either the full block length or the number of samples interpolation left, if less than nSamples
			metadataBlock.jumpPosition.interpolationLength = std::min(nSamples, (unsigned int)metadataBlock.jumpPosition.interpolationLength);
			// Get the polar position (cartesian processing is not handled) required at the end of the interpolation
			PolarPosition targetPosition = m_objectMetadataProc[iObj].polarPosition;
			if (m_objectMetadataProc[iObj].jumpPosition.interpolationLength <= nSamples)
			{
				metadataBlock.polarPosition = targetPosition;
			}
			else // get the polar position expected after nSamples samples
			{
				double azimuthDifference = convertToRangeMinus180To180(targetPosition.azimuth - m_lastObjPos[iObj].azimuth);
				auto deltaAz = nSamples * azimuthDifference / m_objectMetadataProc[iObj].jumpPosition.interpolationLength;
				auto deltaEl = nSamples * (targetPosition.elevation - m_lastObjPos[iObj].elevation) / m_objectMetadataProc[iObj].jumpPosition.interpolationLength;
				auto deltaDist = nSamples * (targetPosition.distance - m_lastObjPos[iObj].distance) / m_objectMetadataProc[iObj].jumpPosition.interpolationLength;
				metadataBlock.polarPosition.azimuth = m_lastObjPos[iObj].azimuth +  deltaAz;
				metadataBlock.polarPosition.elevation = m_lastObjPos[iObj].elevation + deltaEl;
				metadataBlock.polarPosition.distance = m_lastObjPos[iObj].distance + deltaDist;
			}
			m_lastObjPos[iObj] = metadataBlock.polarPosition;

			// Subtract the number of input samples from the metadata interpolationLength
			m_objectMetadataProc[iObj].jumpPosition.interpolationLength -= nSamples;
			if (m_objectMetadataProc[iObj].jumpPosition.interpolationLength <= 0)
			{
				m_objectMetadataProc[iObj].jumpPosition.interpolationLength = 0;
				// interpolation is finished so flag jumpPosition as false
				m_objectMetadataProc[iObj].jumpPosition.flag = false;
			}
		}

		if (m_RenderLayout == OutputLayout::Binaural)
		{
			// If the output is set to binaural then convert the object to HOA so it is rendered with the HOA-to-binaural decoder
			// Set the position of the source from the metadata
			PolarPoint newPos;
			newPos.fAzimuth = DegreesToRadians((float)metadataBlock.polarPosition.azimuth);
			newPos.fElevation = DegreesToRadians((float)metadataBlock.polarPosition.elevation);
			newPos.fDistance = (float)metadataBlock.polarPosition.distance;

			// Set the interpolation duration based on the conditions on page 35 of Rec. ITU-R BS.2127-0
			// If the flag is 
			double interpDur = 0.;
			if (metadataBlock.jumpPosition.flag && !m_bFirstFrame)
				interpDur = metadataBlock.jumpPosition.interpolationLength/nSamples;

			m_hoaEncoders[nObjectInd].SetPosition(newPos, (float)interpDur);
			// Encode the audio and add it to the buffer for output
			m_hoaEncoders[nObjectInd].ProcessAccumul(pIn, nSamples, &m_hoaAudioOut, nOffset);
		}
		else
			m_pointSourcePanners[nObjectInd].ProcessAccumul(metadataBlock, pIn, m_speakerOutDirect, m_speakerOutDiffuse, nSamples, nOffset);
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
			m_hoaEncoders[nDirectSpeakerInd].SetPosition(newPos);
			// Encode the audio and add it to the buffer for output
			m_hoaEncoders[nDirectSpeakerInd].ProcessAccumul(pDirSpkIn, nSamples, &m_hoaAudioOut, nOffset);
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