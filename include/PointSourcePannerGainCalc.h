/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gains required for point source panning.                  #*/
/*#  CAdmPointSourcePannerGainCalc - ADM Point Source Panner                 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePannerGainCalc.h                           #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "LoudspeakerLayouts.h"
#include "Tools.h"
#include "RegionHandlers.h"

#include <assert.h>
#include <set>

class CPointSourcePannerGainCalc
{
public:
	CPointSourcePannerGainCalc(Layout layout);
	~CPointSourcePannerGainCalc();

	/**
		Calculate the gains to be applied to a mono signal in order to place it in the target
		speaker layout
	*/
	std::vector<double> CalculateGains(CartesianPosition directionUnitVec);
	std::vector<double> CalculateGains(PolarPosition directionUnitVec);

	/**
		Get the number of loudspeakers set in the targetLayout
	*/
	unsigned int getNumChannels();

private:
	// The loudspeaker layout 
	Layout m_outputLayout;
	// The layout of the extra loudspeakers used to fill in any gaps in the array
	Layout m_extraSpeakersLayout;

	// Flag if the output is stereo (0+2+0) and treat as a special case
	bool m_isStereo = false;

	std::vector<unsigned int> m_downmixMapping;

	// All of the region handlers for the different types
	LayoutRegions m_regions;

	/**
		Return the extra loudspeakers needed to fill in the gaps in the array.
		This currently works for the supported arrays: 0+5+0, 0+4+0, 0+7+0
		See Rec. ITU-R BS.2127-0 pg. 27
	*/
	Layout CalculateExtraSpeakersLayout(Layout layout);

	/**
		Calculate the gains for the panning layout. In most cases this will be the same
		as the output layout but in the case of 0+2+0 the panning layout is 0+5+0
	*/
	std::vector<double> _CalculateGains(CartesianPosition directionUnitVec);
};
