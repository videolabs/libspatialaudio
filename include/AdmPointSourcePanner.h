/*############################################################################*/
/*#                                                                          #*/
/*#  A point source panner for ADM renderer.                                 #*/
/*#  CAdmPointSourcePanner - ADM Point Source Panner						 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePanner.h                                   #*/
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
#include "AdmPointSourcePannerGainCalc.h"

namespace admrender {

	/**
		This class calculates then applies a vector of gains to a mono input signal to generate the Direct and Diffuse
		Object signals.
	*/
	class CAdmPointSourcePanner
	{
	public:
		CAdmPointSourcePanner(Layout targetLayout);
		~CAdmPointSourcePanner();

		/**
			Spatialises the input pIn and splits it between direct ppDirect and diffuse ppDiffuse
			signals based on the parameters set in the metadata.

			** The outputs are *added* to the ppDirect and ppDiffuse buffers so be sure to clear the buffers
			before passing them to this for the first time **
		*/
		void ProcessAccumul(ObjectMetadata metadata, float* pIn, std::vector<std::vector<float>>& ppDirect, std::vector<std::vector<float>>& ppDiffuse,
			unsigned int nSamples, unsigned int nOffset);

	private:
		Layout m_layout;
		// Number of channels in the layout
		unsigned int m_nCh;
		// The vector of gains applied the last time the position data was updated
		std::vector<double> m_gains;
		// Flag if it is the first processing frame
		bool m_bFirstFrame = true;
		// The gain calculator
		CAdmPointSourcePannerGainCalc m_gainCalculator;

		// The previously set metadata
		ObjectMetadata m_metadata;

		ChannelLockHandler channelLockHandler;
		ZoneExclusionHandler zoneExclusionHandler;
		
		/**
			Get the diverged source positions and directions
		*/
		std::pair<std::vector<PolarPosition>, std::vector<double>> divergedPositionsAndGains(ObjectDivergence divergence, PolarPosition polarDirection);
	};
}
