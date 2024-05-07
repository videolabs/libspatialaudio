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

namespace admrender {

	/** The different output layouts supported by CAdmRenderer */
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
		bool Configure(OutputLayout outputTarget, unsigned int hoaOrder, unsigned int nSampleRate, unsigned int nSamples, const StreamInformation& channelInfo, std::string HRTFPath = "", std::vector<Screen> reproductionScreen = std::vector<Screen>{});

		/** Add an audio Object to be rendered.
		 *
		 * @param pIn		Pointer to the object buffer to be rendered.
		 * @param nSamples	Number of samples in the stream.
		 * @param metadata	Metadata for the object stream. If the metadata is in Cartesian format it will be converted to polar.
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

		// Object metadata for internal use when converting to polar coordinates
		ObjectMetadata m_objMetaDataTmp;

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

		// Temp vectors
		std::vector<double> m_directGainsNoLFE;
		std::vector<double> m_diffuseGainsNoLFE;
		std::vector<double> m_directGains;
		std::vector<double> m_diffuseGains;
		std::vector<double> m_directSpeakerGains;

		/** Find the element of a vector matching the input. If the track types do not match or no matching elements then returns -1 */
		int GetMatchingIndex(const std::vector<std::pair<unsigned int, TypeDefinition>>& vector, unsigned int nElement, TypeDefinition trackType);
	};

}
