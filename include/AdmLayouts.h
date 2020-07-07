/*############################################################################*/
/*#                                                                          #*/
/*#  Loudspeaker layouts.		                                             #*/
/*#								                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmLayout.h	                                             #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _ADM_LAYOUTS_H
#define    _ADM_LAYOUTS_H

#include "AdmMetadata.h"

namespace admrender {

	// Information about a speaker channel (as opposed to an audio channel, which could be Object, HOA, etc.)
	struct Channel
	{
		std::string name;
		// Real loudspeaker position
		PolarPosition polarPosition;
		// Nominal loudspeaker position from ITU-R BS.2051-2
		PolarPosition polarPositionNominal;
		bool isLFE = false;
	};

	// Class used to store the layout information
	class Layout
	{
	public:
		std::string name;
		std::vector<Channel> channels;
		bool hasLFE = false;

		/* 
			If the channel name matches one of the channels in the Layout then return
			its index. If not, return -1.
		*/
		int getMatchingChannelIndex(std::string channelName)
		{
			unsigned int nChannels = (unsigned int)channels.size();
			for (unsigned int iCh = 0; iCh < nChannels; ++iCh)
			{
				if (channelName.compare(channels[iCh].name)==0)
					return iCh;
			}
			return -1; // if no matching channel names are found
		}

		/* 
			Returns a list of the channel names in order.
		*/
		std::vector<std::string> channelNames()
		{
			std::vector<std::string> channelNames;
			for (unsigned int iCh = 0; iCh < channels.size(); ++iCh)
				channelNames.push_back(channels[iCh].name);

			return channelNames;
		}

	private:
	};

	static Layout getLayoutWithoutLFE(Layout layout)
	{
		Layout layoutNoLFE = layout;
		unsigned int nCh = (unsigned int)layout.channels.size();
		layoutNoLFE.channels.resize(0);
		for (unsigned int iCh = 0; iCh < nCh; ++iCh)
			if (!layout.channels[iCh].isLFE)
				layoutNoLFE.channels.push_back(layout.channels[iCh]);
		layoutNoLFE.hasLFE = false;

		return layoutNoLFE;
	}

	const std::vector<Layout> speakerLayouts = {
		// Stereo
		Layout{
		"0+2+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
			Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false}}, false} ,
		// Quad - note: Not defined in ITU-R BS.2051-2
		Layout{
		"0+4+0",std::vector<Channel>{ Channel{"M+045",PolarPosition{45.,0.,1.},PolarPosition{45.,0.,1.},false},
			Channel{"M-045",PolarPosition{-45.,0.,1.},PolarPosition{-45.,0.,1.},false},
			Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
			Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false}}, false},
		// 5.1
		Layout{
		"0+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
			Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
			Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
			Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
			Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
			Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true}}, true},
		// 5.1.2
		Layout{
		"2+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
			Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
			Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
			Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
			Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
			Channel{"U+030",PolarPosition{30.,30.,1.},PolarPosition{30.,30.,1.},false},
			Channel{"U-030",PolarPosition{-30.,30.,1.},PolarPosition{-30.,30.,1.},false},
			Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true}}, true},
		// 7.1
		Layout{
		"0+7+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
			Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
			Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
			Channel{"M+90",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
			Channel{"M-90",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
			Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
			Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
			Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true}}, true},
			};

	/**
		Presets of supported output layouts - taken from https://github.com/ebu/libear/blob/master/src/common/facets.cpp
		Apache 2.0 license
	*/
	const std::vector<std::vector<unsigned int>> HULL_0_5_0 = {
	  {16, 13, 14},   {16, 11, 14}, {16, 10, 13},   {8, 5, 15},
	  {9, 6, 15},     {8, 9, 15},   {16, 11, 12},   {16, 10, 12},
	  {15, 6, 7},     {15, 5, 7},   {3, 4, 13, 14}, {8, 9, 3, 4},
	  {1, 11, 4, 14}, {1, 4, 9, 6}, {1, 2, 11, 12}, {1, 2, 6, 7},
	  {0, 10, 3, 13}, {0, 8, 3, 5}, {0, 2, 10, 12}, {0, 2, 5, 7},
	};

	// 0 = FL, 1 = FR, 2 = BL, 3 = BR
	const std::vector<std::vector<unsigned int>> HULL_0_4_0 = {
		{0,1,4,5},{0,2,4,6},{2,3,6,7},{1,3,5,7},
		{0,1,8,9},{0,2,8,10},{2,3,10,11},{1,3,9,11},
		{4,5,12},{4,6,12},{6,7,12},{5,7,12},
		{8,9,13},{8,10,13},{10,11,13},{9,11,13}
	};

	const std::vector<std::vector<unsigned int>> HULL_2_5_0 = {
	{12, 13, 15},   {13, 6, 15},    {12, 5, 15},   {2, 5, 6},
	{5, 6, 15},     {10, 14, 7},    {8, 11, 14},   {10, 11, 14},
	{1, 2, 6},      {8, 9, 14},     {9, 14, 7},    {0, 2, 5},
	{4, 3, 12, 13}, {3, 10, 11, 4}, {1, 4, 13, 6}, {8, 1, 11, 4},
	{8, 9, 2, 1},   {0, 3, 12, 5},  {0, 10, 3, 7}, {0, 9, 2, 7},
	};

	const std::vector<std::vector<unsigned int>> HULL_0_7_0 = {
	{17, 14, 22},   {18, 22, 15},   {16, 14, 22},   {16, 22, 15},
	{10, 21, 7},    {8, 11, 21},    {9, 21, 7},     {8, 9, 21},
	{17, 19, 22},   {21, 11, 13},   {21, 12, 13},   {10, 12, 21},
	{18, 20, 22},   {19, 20, 22},   {0, 17, 3, 14}, {0, 10, 3, 7},
	{0, 16, 2, 14}, {1, 18, 4, 15}, {8, 1, 11, 4},  {16, 1, 2, 15},
	{0, 9, 2, 7},   {8, 9, 2, 1},   {3, 17, 19, 5}, {10, 3, 12, 5},
	{13, 12, 5, 6}, {11, 4, 13, 6}, {18, 20, 4, 6}, {19, 20, 5, 6},
	};
}
#endif //_ADM_LAYOUTS_H