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
#include "AdmLayouts.h"
#include "AdmUtils.h"
#include "AdmMappingRules.h"
#include "AdmPointSourcePannerGainCalc.h"

namespace admrender {

	/**
		A class to calculate the gains to be applied to a set of loudspeakers for DirectSpeaker processing
	*/
	class CAdmDirectSpeakersGainCalc
	{
	public:
		CAdmDirectSpeakersGainCalc(Layout layoutWithLFE);
		~CAdmDirectSpeakersGainCalc();

		/**
			Calculate the gain vector corresponding to the metadata input
		*/
		std::vector<double> calculateGains(DirectSpeakerMetadata metadata);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		CAdmPointSourcePannerGainCalc m_pointSourcePannerGainCalc;

		bool isLFE(DirectSpeakerMetadata metadata);

		/**
			Find the closest speaker in the layout within the tolerance bounds
			set
		*/
		int findClosestWithinBounds(DirectSpeakerPolarPosition direction, double tol);

		/**
			Determine if a given mapping rule applies for input layout, speaker label and output layout.
			See Rec. ITU-R BS.2127-0 sec 8.4
		*/
		bool MappingRuleApplies(const MappingRule& rule, const std::string& input_layout, const std::string& speakerLabel, admrender::Layout& output_layout);
	};
}
