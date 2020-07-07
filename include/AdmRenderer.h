/*############################################################################*/
/*#                                                                          #*/
/*#  A renderer for ADM streams.                                             #*/
/*#  CAdmRenderer - ADM Renderer                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmRenderer.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include <vector>
#include <string>
#include "Ambisonics.h"
#include "AdmMetadata.h"
#include "AdmLayouts.h"
#include "AdmUtils.h"
#include "AdmPointSourcePanner.h"
#include "AdmDecorrelate.h"

/*

	CURRENT FUNCTIONALITY:
		- Spatialise Objects with HOA panner when using binaural output
		- Spatialise Objects with a VBAP-type panner when output to speakers
		- Add HOA signal to the render to be decoded
		- Add DirectSpeaker tracks to be rendered
		- Set the output format to stereo, binaural, quad, 5.x and 7.x
		- Apply decorrelation to Objects and apply compensation delay to the direct signal
		- Handles exclusion zones, divergence, channel lock

	TODO FOR MORE ADVANCED FUNCTIONALITY
		- Handle Matrix types
		- Handle spread panning (extent)
		- Handle screenLock
		- Add Cartesian processing path. Currently convert positions of objects to polar and uses polar processing.

	TODO FOR BETTER ITU COMPLIANCE:
		- Allow more output speaker formats

*/

namespace admrender {

	/** This is a renderer for ADM streams. It aims to provide a simple way to
		add spatial data to the stream audio using the methods available in
		libspatialaudio.
		Note that it currently supports the basic spatialisation features and is
		not compliant with Rec. ITU-R BS.2127-0 */

	class CAdmRenderer
	{
	public:
		CAdmRenderer(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo);
		~CAdmRenderer();

		/**
			Add an audio Object to be rendered

			Inputs: pIn - Pointer to audio 
					channelInd - channel index in the ADM
		*/
		void AddObject(float* pIn, unsigned int nSamples, ObjectMetadata metadata);

		/**
			Adds an HOA stream to be rendered

			Inputs: pHoaIn - nHoaChannels x nSamples array containing the HOA signal to be rendereds
					nSamples - number of samples in the block
					metadata - the HOA metadata
		*/
		void AddHoa(float** pHoaIn, unsigned int nSamples, HoaMetadata metadata);

		/**
			Adds an HOA stream to be rendered

			Inputs: pHoaIn - nHoaChannels x nSamples array containing the HOA signal to be rendereds
					nSamples - number of samples in the block
					metadata - the HOA metadata
		*/
		void AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, DirectSpeakerMetadata metadata);

		/**
			Get the rendered audio
		*/
		void GetRenderedAudio(float** pRender, unsigned int nSamples);

		/**
			Reset the processor
		*/
		void Reset();

	private:
		OutputLayout m_RenderLayout;
		// Number of output channels in the array (use virtual speakers for binaural rendering)
		unsigned int m_nOutputChannels = 2;
		// Ambisonic order to be used for playback
		unsigned int m_HoaOrder = 3;
		// Maximum number of samples expected in a frame
		unsigned int m_nSamples;

		// Flag if it is the first processing frame
		bool m_bFirstFrame = true;

		StreamInformation channelInformation;

		Layout m_outputLayout;

		// The channel indices of the tracks that can use a point source panner
		std::vector<std::pair<unsigned int,TypeDefinition>> pannerTrackInd;
		std::vector<CAdmPointSourcePanner> pointSourcePanners;
		// HOA encoders to use instead of the pointSourcePanner when output is binaural
		std::vector<CAmbisonicEncoder> hoaEncoders;

		std::vector<ObjectMetadata> objectMetadata;

		// Ambisonic Decoder
		CAmbisonicDecoder hoaDecoder;
		// Ambisonic binaural decoder
		CAmbisonicBinauralizer hoaBinaural;
		// Buffers to hold the HOA audio
		CBFormat hoaAudioOut;
		// Buffers to hold the DirectSpeaker audio
		std::vector<std::vector<float>> speakerOut;
		// Buffers to hold the diffuse audio
		std::vector<std::vector<float>> speakerOutDiffuse;
		void ClearDirectSpeakerBuffer();
		void ClearDiffuseSpeakerBuffer();

		std::unique_ptr<CAdmDirectSpeakersGainCalc> directSpeakerGainCalc;

		// Decorrelator filter processor
		CAdmDecorrelate decorrelate;

		// A buffer containing all zeros to use to clear the HOA data
		std::unique_ptr<float[]> m_pZeros;
		void ClearHoaBuffer();

		/**
			Find the element of a vector matching the input. If the track types do not match or no matching
			elements then returns -1
		*/
		int GetMatchingIndex(std::vector<std::pair<unsigned int, TypeDefinition>>, unsigned int nElement, TypeDefinition trackType);
	};

}
