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
#include "LoudspeakerLayouts.h"
#include "Tools.h"
#include "AdmConversions.h"
#include "GainInterp.h"
#include "AdmDirectSpeakerGainCalc.h"
#include "Decorrelate.h"
#include "PolarExtent.h"
#include "GainCalculator.h"

/*

	CURRENT FUNCTIONALITY:
		- Spatialise Objects with HOA panner when using binaural output
		- Spatialise Objects with a VBAP-type panner when output to speakers
		- Add HOA signal to the render to be decoded
		- Add DirectSpeaker tracks to be rendered
		- Set the output format to stereo, binaural, quad, 5.x and 7.x
		- Apply decorrelation to Objects and apply compensation delay to the direct signal
		- Handles exclusion zones, divergence, channel lock
		- Handles extent panning for both loudspeaker and binaural output
		- Handle screen scaling and screen edge lock

	TODO FOR MORE ADVANCED FUNCTIONALITY
		- Handle Matrix types (need samples to be able to test)
		- Add Cartesian processing path. Currently convert positions of objects to polar and uses polar processing.

*/

namespace admrender {

	// The different output layouts supported by this class
	enum class OutputLayout
	{
		Stereo = 1,
		Quad,
		FivePointOne,
		FivePointZero,
		SevenPointOne,
		SevenPointZero,
		Binaural
	};

	/** This is a renderer for ADM streams. It aims to provide a simple way to
		add spatial data to the stream audio using the methods available in
		libspatialaudio.
		Note that it currently supports the basic spatialisation features and is
		not compliant with Rec. ITU-R BS.2127-0 */

	class CAdmRenderer
	{
	public:
		CAdmRenderer();
		~CAdmRenderer();

		/**
			Configure the module.

			Inputs: outputTarget - enum of the output speaker layout target
					hoaOrder - the HOA order used for decoding HOA streams or in the binaural rendering for OutputLayout::Binaural
					nSampleRate - the sample rate of the audio
					nSamples - the maximum number of samples expected in a frame
					StreamInformation - structure containing the types of each track in the stream
					HRTFPath - path to the SOFA file used for binaural rendering
					reproductionScreen - the reproduction screen used on the playback side. If none or unknown then leave blank. Vector should be of size = 1 (i.e. only one screen)

			Returns true if everything is configured correctly. Otherwise, returns false.
		*/
		bool Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, StreamInformation channelInfo, std::string HRTFPath = "", std::vector<Screen> reproductionScreen = std::vector<Screen>{});

		/**
			Add an audio Object to be rendered

			Inputs: pIn - Pointer to audio
					channelInd - channel index in the ADM
		*/
		void AddObject(float* pIn, unsigned int nSamples, ObjectMetadata metadata, unsigned int nOffset = 0);

		/**
			Adds an HOA stream to be rendered

			Inputs: pHoaIn - nHoaChannels x nSamples array containing the HOA signal to be rendereds
					nSamples - number of samples in the block
					metadata - the HOA metadata
		*/
		void AddHoa(float** pHoaIn, unsigned int nSamples, HoaMetadata metadata, unsigned int nOffset = 0);

		/**
			Adds an DirectSpeaker stream to be rendered

			Inputs: pDirSpkIn -  nSamples array containing the mono speaker signal to be rendered
					nSamples - number of samples in the block
					metadata - the DirectSpeaker metadata
		*/
		void AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, DirectSpeakerMetadata metadata, unsigned int nOffset = 0);

		/**
			Adds a binaural signal to the output. If the output type was not set to Binaural at startup
			then the input is discarded.

			Inputs: pBinIn - 2 x nSamples array containing the HOA signal to be rendereds
					nSamples - number of samples in the block
					metadata - the HOA metadata
		*/
		void AddBinaural(float** pBinIn, unsigned int nSamples, unsigned int nOffset = 0);

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

		StreamInformation m_channelInformation;

		Layout m_outputLayout;
		// Index map to go from the output array excluding LFE to the full array with LFE
		std::vector<unsigned int> m_mapNoLfeToLfe;

		// Vector holding the last unique set metadata for each object in the stream
		std::vector<ObjectMetadata> m_objectMetadata;
		// A map from the channel index to the object index in the order the objects were listed
		// in the stream at configuration
		std::map<int, int> m_channelToObjMap;

		// The channel indices of the tracks that can use a point source panner
		std::vector<std::pair<unsigned int, TypeDefinition>> m_pannerTrackInd;
		// Gain interpolators
		std::vector<CGainInterp> m_gainInterpDirect;
		std::vector<CGainInterp> m_gainInterpDiffuse;
		// The gain calculator for Object type channels
		std::unique_ptr<CGainCalculator> m_objectGainCalc;
		// HOA encoders to use instead of the pointSourcePanner when output is binaural
		std::vector<std::vector<CAmbisonicEncoder>> m_hoaEncoders;
		// The gain calculator for the DirectSpeaker channels
		std::unique_ptr<CAdmDirectSpeakersGainCalc> m_directSpeakerGainCalc;

		// Ambisonic Decoder
		CAmbisonicDecoder m_hoaDecoder;
		// Ambisonic binaural decoder
		CAmbisonicBinauralizer m_hoaBinaural;
		// Buffers to hold the HOA audio
		CBFormat m_hoaAudioOut;
		// Vector to pass the direct and diffuse HOA object data to the decorrelator
		std::vector<std::vector<float>> m_hoaObjectDirect, m_hoaObjectDiffuse;
		// Buffers holding the output signal
		std::vector<std::vector<float>> m_speakerOut;
		// Buffers to hold the direct object audio
		std::vector<std::vector<float>> m_speakerOutDirect;
		// Buffers to hold the diffuse object audio
		std::vector<std::vector<float>> m_speakerOutDiffuse;
		void ClearOutputBuffer();
		void ClearObjectDirectBuffer();
		void ClearObjectDiffuseBuffer();

		// Decorrelator filter processor
		CDecorrelate m_decorrelate;
		// Scattering matrix applied to the diffuse gains before decorrelation is applied. Only used when input layout is HOA.
		// This is useful when hoa component W = 1 and all others are near-zero (for objects with high width/height, for example)
		// the the field is actually diffuse rather than just a decorrelation of a near-mono signal, which can collapse to an in-head
		// effect
		std::vector<std::vector<double>> m_scatteringMatrix;

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
