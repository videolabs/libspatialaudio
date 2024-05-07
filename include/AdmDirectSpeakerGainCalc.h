/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gain vector to spatialise a DirectSpeaker channel.        #*/
/*#  CAdmDirectSpeakersGainCalc                                              #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmDirectSpeakersGainCalc.h                              #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "AdmMetadata.h"
#include "LoudspeakerLayouts.h"
#include "Tools.h"
#include "AdmMappingRules.h"
#include "PointSourcePannerGainCalc.h"
#include "Screen.h"

namespace admrender {

	/** A class to calculate the gains to be applied to a set of loudspeakers for DirectSpeaker processing. */
	class CAdmDirectSpeakersGainCalc
	{
	public:
		CAdmDirectSpeakersGainCalc(Layout layoutWithLFE);
		~CAdmDirectSpeakersGainCalc();

		/** Calculate the gain vector corresponding to the metadata input.
		 *
		 * @param metadata	DirectSpeaker metadata.
		 * @param gainsOut	Return of the gain vector corresponding to the input metadata.
		 */
		void calculateGains(const DirectSpeakerMetadata& metadata, std::vector<double>& gainsOut);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		CPointSourcePannerGainCalc m_pointSourcePannerGainCalc;

		// Temp vector holding the point-source panner gains
		std::vector<double> m_gainsPSP;

		CScreenEdgeLock m_screenEdgeLock;

		// Labels for LFE channels to use in isLFE and avoid allocations to the heap by using temp strings
		std::vector<std::string> m_lfeLabels = { "LFE1", "LFE2" };

		/** Check if the input metadata is for an LFE channel
		 * @param metadata	Metadata to check
		 * @return			Returns true if the input metadata refers to an LFE channel
		 */
		bool isLFE(const DirectSpeakerMetadata& metadata);

		/** Find the closest speaker in the layout within the tolerance bounds set.
		 *
		 * @param direction	Polar position of the DirectSpeaker
		 * @param tol		Tolerance value when checking proximity
		 * @return			Returns the index of the closest speaker in the layout. If none are within bounds returns -1.
		 */
		int findClosestWithinBounds(const DirectSpeakerPolarPosition& direction, double tol);

		/** Determine if a given mapping rule applies for input layout, speaker label and output layout.
		 *  See Rec. ITU-R BS.2127-0 sec 8.4s
		 *
		 * @param rule			Mapping rule to test.
		 * @param input_layout	Input speaker layout to test.
		 * @param speakerLabel	Speaker label to test.
		 * @param output_layout	Output speaker layout to test.
		 * @return				Returns true if a mapping rule applies.
		 */
		bool MappingRuleApplies(const MappingRule& rule, const std::string& input_layout, const std::string& speakerLabel, Layout& output_layout);
	};
}
