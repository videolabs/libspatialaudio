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
#include "GainCalculator.h"

namespace admrender {

	/** The different output layouts supported by CAdmRenderer */
	enum class OutputLayout
	{
		Stereo = 1,
		Quad,
		FivePointOne, // BS.2051-3 System B 0+5+0
		FivePointZero, // BS.2051-3 System B 0+5+0 without sub channel
		SevenPointOne, // BS.2051-3 System I 0+5+0
		SevenPointZero, // BS.2051-3 System I 0+5+0 without sub channel
		// BS.2051-3 list. Note that 0_2_0 = Stereo above, 0_5_0 = FivePointOne, and 0_7_0 = SevenPointOne
		ITU_0_2_0, ITU_0_5_0, ITU_2_5_0, ITU_4_5_0, ITU_4_5_1, ITU_3_7_0, ITU_4_9_0, ITU_9_10_3, ITU_0_7_0, ITU_4_7_0,
		BEAR_9_10_5, // BEAR layout. 9+10+3 with 2 extra lower speakers
		_2_7_0, // 7.1.2 layout specified in IAMF v1.0.0
		_3p1p2, // 3.1.2 layout specified in IAMF v1.0.0
		Binaural
	};

	/** This is a renderer for ADM streams. It aims to provide a simple way to
	 *	add spatial data to the stream audio using the methods available in
	 *	libspatialaudio.
	 *
	 *	CURRENT FUNCTIONALITY:
	 *	- Spatialise Objects with HOA panner when using binaural output
	 *	- Spatialise Objects with a VBAP-type panner when output to speakers
	 *	- Add HOA signal to the render to be decoded
	 *	- Add DirectSpeaker tracks to be rendered
	 *	- Set the output format to stereo, binaural, quad, 5.x and 7.x
	 *	- Apply decorrelation to Objects and apply compensation delay to the direct signal
	 *	- Handles exclusion zones, divergence, channel lock
	 *	- Handles extent panning for both loudspeaker and binaural output
	 *	- Handle screen scaling and screen edge lock
	 *
	 *	Required to meet full compliance with Rec. ITU-R BS.2127-0:
	 *	- Handle Matrix types (need samples to be able to test)
	 *	- Add Cartesian processing path. Currently convert positions of objects to polar and uses polar processing.
	 */
	class CAdmRenderer
	{
	public:
		CAdmRenderer();
		~CAdmRenderer();

		/** Configure the ADM Renderer.
		 *
		 * @param outputTarget			The target output layout.
		 * @param hoaOrder				The ambisonic order of the signal to be rendered.
		 * @param nSampleRate			Sample rate of the streams to be processed.
		 * @param nSamples				The maximum number of samples in an audio frame to be added to the stream.
		 * @param channelInfo			Information about the expected stream formats.
		 * @param HRTFPath				Path to an HRTF to be used when the output layout is binaural.
		 * @param reproductionScreen	Screen details used for screen scaling/locking.
		 * @return						Returns true if the class is correctly configured and ready to use.
		 */
		bool Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, const StreamInformation& channelInfo, std::string HRTFPath = "", Optional<Screen> reproductionScreen = Optional<Screen>());

		/** Add an audio Object to be rendered.
		 *
		 * @param pIn		Pointer to the object buffer to be rendered.
		 * @param nSamples	Number of samples in the stream.
		 * @param metadata	Metadata for the object stream.
		 * @param nOffset	Number of samples of delay to applied to the signal.
		 */
		void AddObject(float* pIn, unsigned int nSamples, const ObjectMetadata& metadata, unsigned int nOffset = 0);

		/** Adds an HOA stream to be rendered. Currently only supports SN3D normalisation.
		 *
		 * @param pHoaIn	The HOA audio channels to be rendered of size nAmbiCh x nSamples
		 * @param nSamples	Number of samples in the stream.
		 * @param metadata	Metadata for the HOA stream
		 * @param nOffset	Number of samples of delay to applied to the signal.
		 */
		void AddHoa(float** pHoaIn, unsigned int nSamples, const HoaMetadata& metadata, unsigned int nOffset = 0);

		/** Adds an DirectSpeaker stream to be rendered.
		 *
		 * @param pDirSpkIn	Pointer to the direct speaker stream.
		 * @param nSamples	Number of samples in the stream.
		 * @param metadata	Metadata for the DirectSpeaker stream
		 * @param nOffset	Number of samples of delay to applied to the signal.
		 */
		void AddDirectSpeaker(float* pDirSpkIn, unsigned int nSamples, const DirectSpeakerMetadata& metadata, unsigned int nOffset = 0);

		/** Adds a binaural signal to the output. If the output type was not set to Binaural at startup
		 *	then the input is discarded.
		 * @param pBinIn	The binaural stream of size 2 x nSamples
		 * @param nSamples	Number of samples in the stream.
		 * @param nOffset	Number of samples of delay to applied to the signal.
		 */
		void AddBinaural(float** pBinIn, unsigned int nSamples, unsigned int nOffset = 0);

		/** Get the rendered audio. This should only be called after all streams have been added.
		 * @param pRender	Rendered audio output.
		 * @param nSamples	The number of samples to get
		 */
		void GetRenderedAudio(float** pRender, unsigned int nSamples);

		/** Reset the processor. */
		void Reset();

		/** Get the number of speakers in the layout specified to Configure.
		 * @return Number of output channels
		 */
		unsigned int GetSpeakerCount();

		/** Set head orientation to apply head tracking when rendering to binaural.
		 *  Rotations are applied in the order yaw-pitch-roll
		 * @param orientation	Head orientation
		 */
		void SetHeadOrientation(const RotationOrientation& orientation);

	private:
		OutputLayout m_RenderLayout;
		// Number of channels in the array (use virtual speakers for binaural rendering)
		unsigned int m_nChannelsToRender = 2;
		// Ambisonic order to be used for playback
		unsigned int m_HoaOrder = 3;
		// Number of ambisonic channels corresponding to the HOA order
		unsigned int m_nAmbiChannels = 16;
		// Maximum number of samples expected in a frame
		unsigned int m_nSamples;

		StreamInformation m_channelInformation;

		Layout m_outputLayout;

		// Vector holding the last unique set metadata for each object in the stream
		std::vector<ObjectMetadata> m_objectMetadata;
		// A map from the channel index to the object index in the order the objects were listed
		// in the stream at configuration
		std::map<int, int> m_channelToObjMap;

		// Object metadata for internal use when converting to polar coordinates
		ObjectMetadata m_objMetaDataTmp;

		// Temp DirectSpeaker metadata when in binaural mode to ensure only the desired gain calculation elements are used
		DirectSpeakerMetadata m_dirSpkBinMetaDataTmp;

		// The channel indices of the tracks that can use a point source panner
		std::vector<std::pair<unsigned int, TypeDefinition>> m_pannerTrackInd;
		// Gain interpolators
		std::vector<CGainInterp<double>> m_gainInterpDirect;
		std::vector<CGainInterp<double>> m_gainInterpDiffuse;
		// The gain calculator for Object type channels
		std::unique_ptr<CGainCalculator> m_objectGainCalc;
		// The gain calculator for the DirectSpeaker channels
		std::unique_ptr<CAdmDirectSpeakersGainCalc> m_directSpeakerGainCalc;

		// Ambisonic Decoder
		CAmbisonicAllRAD m_hoaDecoder;
		// Ambisonic encoders to use convert from speaker feeds to HOA for binaural decoding
		std::vector<CAmbisonicEncoder> m_hoaEncoders;
		// Ambisonic rotation for binaural with head-tracking
		CAmbisonicRotator m_hoaRotate;
		// Ambisonic binaural decoder
		CAmbisonicBinauralizer m_hoaBinaural;
		// Buffers to hold the HOA audio
		CBFormat m_hoaAudioOut;
		// Buffers holding the output signal
		float** m_speakerOut;
		// Buffers to hold the direct object audio
		float** m_speakerOutDirect;
		// Buffers to hold the diffuse object audio
		float** m_speakerOutDiffuse;
		// Buffers to the hold the virtual speaker layout signals when rendering to binaural
		float** m_virtualSpeakerOut;
		// Buffers to hold binaural signals added via AddBinaural() when rendering to binaural
		float** m_binauralOut;
		void ClearOutputBuffer();
		void ClearObjectDirectBuffer();
		void ClearObjectDiffuseBuffer();
		void ClearBinauralBuffer();
		void ClearVirtualSpeakerBuffer();

		// Decorrelator filter processor
		CDecorrelate m_decorrelate;

		// A buffer containing all zeros to use to clear the HOA data
		std::unique_ptr<float[]> m_pZeros;
		void ClearHoaBuffer();

		// Temp vectors
		std::vector<double> m_directGains;
		std::vector<double> m_diffuseGains;
		std::vector<double> m_directSpeakerGains;

		/** Find the element of a vector matching the input. If the track types do not match or no matching elements then returns -1 */
		int GetMatchingIndex(const std::vector<std::pair<unsigned int, TypeDefinition>>& vector, unsigned int nElement, TypeDefinition trackType);

		/** Allocate internal 2D buffers of the specified size (nCh x nSamples).
		 * @param buffers Array of pointers to the buffers.
		 * @param nCh Number of channels to allocate.
		 * @param nSamples Number of samples to allocate.
		 */
		void AllocateBuffers(float**& buffers, unsigned nCh, unsigned nSamples);

		/** Deallocate internal 2D buffers.
		 * @param buffers Buffers to deallocate.
		 * @param nCh Number of channels in the buffer.
		 */
		void DeallocateBuffers(float**& buffers, unsigned nCh);
	};

}
