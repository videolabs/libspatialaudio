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


#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "Coordinates.h"
#include "ScreenCommon.h"

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

	bool isHoa = false;
	unsigned int hoaOrder = 0;

	std::vector<Screen> reproductionScreen = {};

	/*
		If the channel name matches one of the channels in the Layout then return
		its index. If not, return -1.
	*/
	int getMatchingChannelIndex(std::string channelName)
	{
		unsigned int nChannels = (unsigned int)channels.size();
		for (unsigned int iCh = 0; iCh < nChannels; ++iCh)
		{
			if (channelName.compare(channels[iCh].name) == 0)
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

/**
	If the the speaker label is in the format "urn:itu:bs:2051:[0-9]:speaker:X+YYY then
	return the X+YYY portion. Otherwise, returns the original input
*/
static inline std::string GetNominalSpeakerLabel(const std::string& label)
{
	std::string speakerLabel = label;

	std::stringstream ss(speakerLabel);
	std::string token;
	std::vector<std::string> tokens;
	while (std::getline(ss, token, ':'))
	{
		tokens.push_back(token);
	}

	if (tokens.size() == 7)
		if (tokens[0] == "urn" && tokens[1] == "itu" && tokens[2] == "bs" && tokens[3] == "2051" &&
			(std::stoi(tokens[4]) >= 0 || std::stoi(tokens[4]) < 10) && tokens[5] == "speaker")
			speakerLabel = tokens[6];

	// Rename the LFE channels, if requried.
	// See Rec. ITU-R BS.2127-0 sec 8.3
	if (speakerLabel == "LFE" || speakerLabel == "LFEL")
		speakerLabel = "LFE1";
	else if (speakerLabel == "LFER")
		speakerLabel = "LFE2";

	return speakerLabel;
}

static inline Layout getLayoutWithoutLFE(Layout layout)
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

/**
	Directions of audio channels from Rec. ITU-R BS.2094-1 Table 1
*/
const std::map<std::string, PolarPosition> bs2094Positions = {
	{"M+030", PolarPosition{30.,0.,1.}},
	{"M-030", PolarPosition{-30.,0.,1.}},
	{"M+000", PolarPosition{0.,0.,1.}},
	{"LFE", PolarPosition{0.,-30.,1.}},
	{"M+110", PolarPosition{110.,0.,1.}},
	{"M-110", PolarPosition{-110.,0.,1.}},
	{"M+022", PolarPosition{22.5,0.,1.}},
	{"M-022", PolarPosition{-22.5,0.,1.}},
	{"M+180", PolarPosition{180.,0.,1.}},
	{"M+090", PolarPosition{90.,0.,1.}},
	{"M-090", PolarPosition{-90.,0.,1.}},
	{"T+000", PolarPosition{0.,90.,1.}},
	{"U+030", PolarPosition{30.,30.,1.}},
	{"U+000", PolarPosition{0.,30.,1.}},
	{"U-030", PolarPosition{-30.,30.,1.}},
	{"U+110", PolarPosition{110.,30.,1.}},
	{"U+180", PolarPosition{180.,30.,1.}},
	{"U-110", PolarPosition{-110.,30.,1.}},
	{"U+090", PolarPosition{90.,30.,1.}},
	{"U-090", PolarPosition{-90.,30.,1.}},
	{"B+000", PolarPosition{0.,-30.,1.}},
	{"B+045", PolarPosition{45.,-30.,1.}},
	{"B-045", PolarPosition{-45.,-30.,1.}},
	{"B+060", PolarPosition{60.,-30.,1.}},
	{"B-060", PolarPosition{-60.,-30.,1.}},
	{"M+135_Diff", PolarPosition{135.,0.,1.}},
	{"M-135_Diff", PolarPosition{-135.,0.,1.}},
	{"M+135", PolarPosition{135.,0.,1.}},
	{"M-135", PolarPosition{-135.,0.,1.}},
	{"U+135", PolarPosition{135.,30.,1.}},
	{"U-135", PolarPosition{-135.,30.,1.}},
	{"LFE1", PolarPosition{45.,-30.,1.}},
	{"LFE2", PolarPosition{-45.,-30.,1.}},
	{"U+045", PolarPosition{45.,0.,1.}},
	{"U-045", PolarPosition{-45.,0.,1.}},
	{"M+SC", PolarPosition{25.,0.,1.}},
	{"M-SC", PolarPosition{-25.,0.,1.}},
	{"M+045", PolarPosition{45.,0.,1.}},
	{"M-045", PolarPosition{-45.,0.,1.}},
	{"UH+180", PolarPosition{180.,45.,1.}}
};

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
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false}}, true},
// 5.1.2
Layout{
"2+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
Channel{"U+030",PolarPosition{30.,30.,1.},PolarPosition{30.,30.,1.},false},
Channel{"U-030",PolarPosition{-30.,30.,1.},PolarPosition{-30.,30.,1.},false}}, true},
// 7.1
Layout{
"0+7+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+90",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-90",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false}}, true},
// First order Ambisonics (AmbiX). Directions are meaningless so all set to front
Layout{
"1OA",std::vector<Channel>{ Channel{"ACN0",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN1",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN2",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN3",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false}}, false, true, 1},
// Second order Ambisonics (AmbiX). Directions are meaningless so all set to front
Layout{
"2OA",std::vector<Channel>{ Channel{"ACN0",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN1",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN2",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN3",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN4",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN5",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN6",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN7",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN8",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false}}, false, true, 2},
// Third order Ambisonics (AmbiX). Directions are meaningless so all set to front
Layout{
"3OA",std::vector<Channel>{ Channel{"ACN0",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN1",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN2",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN3",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN4",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN5",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN6",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN7",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN8",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN9",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN10",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN11",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN12",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN13",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN14",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"ACN15",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false}}, false, true, 3},
};

/**
	Get the speakerLayout that matches the given name. If no match then returns empty.
*/
static inline Layout GetMatchingLayout(std::string layoutName)
{
	unsigned int nLayouts = (unsigned int)speakerLayouts.size();
	for (unsigned int iLayout = 0; iLayout < nLayouts; ++iLayout)
	{
		if (layoutName.compare(speakerLayouts[iLayout].name) == 0)
			return speakerLayouts[iLayout];
	}
	return {}; // if no matching channel names are found
}

/**
	Presets of supported output layouts used in the point source panner
*/
const std::vector<std::vector<unsigned int>> HULL_0_5_0 = { {3, 4, 13, 14}, {8, 9, 3, 4},
	{1, 11, 4, 14}, {1, 4, 9, 6}, {1, 2, 11, 12}, {1, 2, 6, 7},
	{0, 10, 3, 13}, {0, 8, 3, 5}, {0, 2, 10, 12}, {0, 2, 5, 7},
	{16, 13, 14}, {16, 11, 14}, {16, 10, 13}, {8, 5, 15},
	{9, 6, 15}, {8, 9, 15}, {16, 11, 12}, {16, 10, 12},
	{15, 6, 7}, {15, 5, 7}
};

// 0 = FL, 1 = FR, 2 = BL, 3 = BR
const std::vector<std::vector<unsigned int>> HULL_0_4_0 = {
	{0,1,4,5}, {0,2,4,6}, {2,3,6,7}, {1,3,5,7},
	{0,1,8,9}, {0,2,8,10}, {2,3,10,11}, {1,3,9,11},
	{4,5,12}, {4,6,12}, {6,7,12}, {5,7,12},
	{8,9,13}, {8,10,13}, {10,11,13}, {9,11,13}
};

const std::vector<std::vector<unsigned int>> HULL_2_5_0 = { {4, 3, 12, 13}, {3, 10, 11, 4},
{1, 4, 13, 6}, {8, 1, 11, 4}, {8, 9, 2, 1}, {0, 3, 12, 5},
{0, 10, 3, 7}, {0, 9, 2, 7}, {12, 13, 15}, {13, 6, 15},
{12, 5, 15}, {2, 5, 6}, {5, 6, 15}, {10, 14, 7},
{8, 11, 14}, {10, 11, 14}, {1, 2, 6}, {8, 9, 14},
{9, 14, 7}, {0, 2, 5}
};

const std::vector<std::vector<unsigned int>> HULL_0_7_0 = { {0, 17, 3, 14}, {0, 10, 3, 7},
{0, 16, 2, 14}, {1, 18, 4, 15}, {8, 1, 11, 4},  {16, 1, 2, 15},
{0, 9, 2, 7}, {8, 9, 2, 1}, {3, 17, 19, 5}, {10, 3, 12, 5},
{13, 12, 5, 6}, {11, 4, 13, 6}, {18, 20, 4, 6}, {19, 20, 5, 6},
{17, 14, 22}, {18, 22, 15}, {16, 14, 22}, {16, 22, 15},
{10, 21, 7}, {8, 11, 21}, {9, 21, 7}, {8, 9, 21},
{17, 19, 22}, {21, 11, 13}, {21, 12, 13}, {10, 12, 21},
{18, 20, 22}, {19, 20, 22}
};
