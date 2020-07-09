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

//#include <assert.h>
//#include <set>
//#include <algorithm>
//#include <memory>
//#include <limits>
//#include <regex>
//#include <map>
#include <sstream>
#include <iostream>

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
			If the the speaker label is in the format "urn:itu:bs:2051:[0-9]:speaker:X+YYY then
			return the X+YYY portion. Otherwise, returns the original input
		*/
		std::string GetNominalSpeakerLabel(const std::string& label);

		/**
			Determine if a given mapping rule applies for input layout, speaker label and output layout.
			See Rec. ITU-R BS.2127-0 sec 8.4
		*/
		bool MappingRuleApplies(const MappingRule& rule, const std::string& input_layout, const std::string& speakerLabel, admrender::Layout& output_layout);
	};
}
