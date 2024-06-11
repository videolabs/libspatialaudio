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
#include "AdmMetadata.h"
#include "Coordinates.h"
#include "ScreenCommon.h"
#include "AdmConversions.h"

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

	admrender::Optional<Screen> reproductionScreen;

	/** If the channel name matches one of the channels in the Layout then return
	 *  its index. If not, return -1.
	 * @param channelName	String containing the name of the channel.
	 * @return				The index of the channel in the layout. If it is not present, returns -1.
	 */
	int getMatchingChannelIndex(const std::string& channelName)
	{
		unsigned int nChannels = (unsigned int)channels.size();
		for (unsigned int iCh = 0; iCh < nChannels; ++iCh)
		{
			if (channelName.compare(channels[iCh].name) == 0)
				return iCh;
		}
		return -1; // if no matching channel names are found
	}

	/** Returns a list of the channel names in order. */
	std::vector<std::string> channelNames() const
	{
		std::vector<std::string> channelNames;
		for (unsigned int iCh = 0; iCh < channels.size(); ++iCh)
			channelNames.push_back(channels[iCh].name);

		return channelNames;
	}

	/** Returns true if the layout contains the specified channel.
	 * @param channelName Name of the channel to check for.
	 * @return Returns true if channelName is present in the layout
	 */
	inline bool containsChannel(const std::string& channelName) const
	{
		for (unsigned int iCh = 0; iCh < channels.size(); ++iCh)
			if (channels[iCh].name == channelName)
				return true;
		return false;
	}

private:
};

/** List of labels for audio channels from Rec. ITU-R BS.2094-1 Table 1. */
static const std::vector<std::string> channelLabels = { "M+030",
	"M-030",
	"M+000",
	"M+110",
	"M-110",
	"M+022",
	"M-022",
	"M+180",
	"M+090",
	"M-090",
	"T+000",
	"U+030",
	"U+000",
	"U-030",
	"U+110",
	"U+180",
	"U-110",
	"U+090",
	"U-090",
	"B+000",
	"B+045",
	"B-045",
	"B+060",
	"B-060",
	"M+135_Diff",
	"M-135_Diff",
	"M+135",
	"M-135",
	"U+135",
	"U-135",
	"LFE1",
	"LFE2",
	"U+045",
	"U-045",
	"M+SC",
	"M-SC",
	"M+045",
	"M-045",
	"UH+180",
	"" /* empty to indicate no appropriate channel name */
};

/** If the the speaker label is in the format "urn:itu:bs:2051:[0-9]:speaker:X+YYY then
 *  return the X+YYY portion. Otherwise, returns the original input.
 * @param label		String containing the label.
 * @return			String of the nominal speaker label (X+YYY portion of input).
 */
static inline const std::string& GetNominalSpeakerLabel(const std::string& label)
{
	for (size_t i = 0; i < channelLabels.size(); ++i)
		if (stringContains(label, channelLabels[i]))
			return channelLabels[i];

	// Rename the LFE channels, if requried.
	// See Rec. ITU-R BS.2127-0 sec 8.3
	if (stringContains(label, "LFE") || stringContains(label,"LFEL"))
		return channelLabels[30];
	else if (stringContains(label, "LFER"))
		return channelLabels[31];

	return channelLabels.back();
}

/** Returns a version of the input layout without any LFE channels.
 * @param layout	Input layout.
 * @return			Copy of the input layout with any LFE channels removed.
 */
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

/** Check if the input DirectSpeakerMetadata is for an LFE channel.
* @param metadata	Metadata to check.
* @return			Returns true if the input metadata refers to an LFE channel.
*/
static inline bool isLFE(const admrender::DirectSpeakerMetadata& metadata)
{
	// See Rec. ITU-R BS.2127-1 sec. 6.3
	if (metadata.channelFrequency.lowPass.hasValue())
		if (metadata.channelFrequency.lowPass.value() <= 120.)
			return true;

	const std::string& nominalLabel = GetNominalSpeakerLabel(metadata.speakerLabel);
	if (stringContains(nominalLabel, "LFE1") || stringContains(nominalLabel, "LFE2"))
	{
		return true;
	}
	return false;
}

/** Directions of audio channels from Rec. ITU-R BS.2094-1 Table 1. */
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

/** Predefined speaker layouts. */
const std::vector<Layout> speakerLayouts = {
// Stereo - BS.2051-3 System A 0+2+0
Layout{
"0+2+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false}}, false} ,
// Quad - note: Not defined in ITU-R BS.2051-3
Layout{
"0+4+0",std::vector<Channel>{ Channel{"M+045",PolarPosition{45.,0.,1.},PolarPosition{45.,0.,1.},false},
Channel{"M-045",PolarPosition{-45.,0.,1.},PolarPosition{-45.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false}}, false},
// 5.1 - BS.2051-3 System B 0+5+0
Layout{
"0+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false}}, true},
// 5.1.2 - BS.2051-3 System C 2+5+0
Layout{
"2+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
Channel{"U+030",PolarPosition{30.,30.,1.},PolarPosition{30.,30.,1.},false},
Channel{"U-030",PolarPosition{-30.,30.,1.},PolarPosition{-30.,30.,1.},false}}, true},
// 5.1.4 - BS.2051-3 System D 4+5+0
Layout{
"4+5+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
Channel{"U+030",PolarPosition{30.,30.,1.},PolarPosition{30.,30.,1.},false},
Channel{"U-030",PolarPosition{-30.,30.,1.},PolarPosition{-30.,30.,1.},false},
Channel{"U+110",PolarPosition{110.,30.,1.},PolarPosition{110.,30.,1.},false},
Channel{"U-110",PolarPosition{-110.,30.,1.},PolarPosition{-110.,30.,1.},false}}, true},
// BS.2051-3 System E 4+5+1
Layout{
"4+5+1",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+110",PolarPosition{110.,0.,1.},PolarPosition{110.,0.,1.},false},
Channel{"M-110",PolarPosition{-110.,0.,1.},PolarPosition{-110.,0.,1.},false},
Channel{"U+030",PolarPosition{30.,30.,1.},PolarPosition{30.,30.,1.},false},
Channel{"U-030",PolarPosition{-30.,30.,1.},PolarPosition{-30.,30.,1.},false},
Channel{"U+030",PolarPosition{110.,30.,1.},PolarPosition{110.,30.,1.},false},
Channel{"U-030",PolarPosition{-110.,30.,1.},PolarPosition{-110.,30.,1.},false},
Channel{"B+000",PolarPosition{0.,-30.,1.},PolarPosition{0.,-30.,1.},false}}, true},
// BS.2051-3 System F 3+7+0
Layout{
"3+7+0",std::vector<Channel>{ Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"UH+180",PolarPosition{180.,45,1.},PolarPosition{180.,45.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"LFE2",PolarPosition{-45.,-30.,1.},PolarPosition{-45.,-30.,1.},true}}, true},
// BS.2051-3 System G 4+9+0
Layout{
"4+9+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false},
Channel{"U+135",PolarPosition{135.,30.,1.},PolarPosition{135.,30.,1.},false},
Channel{"U-135",PolarPosition{-135.,30.,1.},PolarPosition{-135.,30.,1.},false},
Channel{"M+SC",PolarPosition{15.,0.,1.},PolarPosition{15.,0.,1.},false}, // BS.2051-3 does not define the azimuth for M+SC but Rec. ITU-R BS.2127-1 defines it as 15 degrees.
Channel{"M-SC",PolarPosition{-15.,0.,1.},PolarPosition{-15.,0.,1.},false}} // BS.2051-3 does not define the azimuth for M-SC but Rec. ITU-R BS.2127-1 defines it as 15 degrees.
, true},
// BS.2051-3 System H 9+10+3
Layout{
"9+10+3",std::vector<Channel>{ Channel{"M+060",PolarPosition{60.,0.,1.},PolarPosition{60.,0.,1.},false},
Channel{"M-060",PolarPosition{-60.,0.,1.},PolarPosition{-60.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+180",PolarPosition{180.,0.,1.},PolarPosition{180.,0.,1.},false},
Channel{"LFE2",PolarPosition{-45.,-30.,1.},PolarPosition{-45.,-30.,1.},true},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false},
Channel{"U+000",PolarPosition{0.,30.,1.},PolarPosition{0.,30.,1.},false},
Channel{"T+000",PolarPosition{0.,90.,1.},PolarPosition{0.,90.,1.},false},
Channel{"U+135",PolarPosition{135.,30.,1.},PolarPosition{135.,30.,1.},false},
Channel{"U-135",PolarPosition{-135.,30.,1.},PolarPosition{-135.,30.,1.},false},
Channel{"U+090",PolarPosition{90.,30.,1.},PolarPosition{90.,30.,1.},false},
Channel{"U-090",PolarPosition{-90.,30.,1.},PolarPosition{-90.,30.,1.},false},
Channel{"U+180",PolarPosition{180.,30.,1.},PolarPosition{180.,30.,1.},false},
Channel{"B+000",PolarPosition{0.,-30.,1.},PolarPosition{0.,-30.,1.},false},
Channel{"B+045",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},false},
Channel{"B-045",PolarPosition{-45.,-30.,1.},PolarPosition{-45.,-30.,1.},false}}, true},
// 7.1 - BS.2051-3 System I 0+7+0
Layout{
"0+7+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false}}, true},
// 7.1.4 - BS.2051-3 System J 4+7+0
Layout{
"4+7+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false},
Channel{"U+135",PolarPosition{135.,30.,1.},PolarPosition{135.,30.,1.},false},
Channel{"U-135",PolarPosition{-135.,30.,1.},PolarPosition{-135.,30.,1.},false}}, true},
// 7.1.2 - IAMF v1.0.0-errata
Layout{
"2+7+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false}}, true },
// 3.1.2 - IAMF v1.0.0-errata
Layout{
"2+3+0",std::vector<Channel>{ Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"LFE1",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},true},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false}}, true },
// EBU Tech 3369 (BEAR) 9+10+5 - 9+10+3 with LFE1 & LFE2 removed and B+135 & B-135 added
Layout{
"9+10+5",std::vector<Channel>{ Channel{"M+060",PolarPosition{60.,0.,1.},PolarPosition{60.,0.,1.},false},
Channel{"M-060",PolarPosition{-60.,0.,1.},PolarPosition{-60.,0.,1.},false},
Channel{"M+000",PolarPosition{0.,0.,1.},PolarPosition{0.,0.,1.},false},
Channel{"M+135",PolarPosition{135.,0.,1.},PolarPosition{135.,0.,1.},false},
Channel{"M-135",PolarPosition{-135.,0.,1.},PolarPosition{-135.,0.,1.},false},
Channel{"M+030",PolarPosition{30.,0.,1.},PolarPosition{30.,0.,1.},false},
Channel{"M-030",PolarPosition{-30.,0.,1.},PolarPosition{-30.,0.,1.},false},
Channel{"M+180",PolarPosition{180.,0.,1.},PolarPosition{180.,0.,1.},false},
Channel{"M+090",PolarPosition{90.,0.,1.},PolarPosition{90.,0.,1.},false},
Channel{"M-090",PolarPosition{-90.,0.,1.},PolarPosition{-90.,0.,1.},false},
Channel{"U+045",PolarPosition{45.,30.,1.},PolarPosition{45.,30.,1.},false},
Channel{"U-045",PolarPosition{-45.,30.,1.},PolarPosition{-45.,30.,1.},false},
Channel{"U+000",PolarPosition{0.,30.,1.},PolarPosition{0.,30.,1.},false},
Channel{"T+000",PolarPosition{0.,90.,1.},PolarPosition{0.,90.,1.},false},
Channel{"U+135",PolarPosition{135.,30.,1.},PolarPosition{135.,30.,1.},false},
Channel{"U-135",PolarPosition{-135.,30.,1.},PolarPosition{-135.,30.,1.},false},
Channel{"U+090",PolarPosition{90.,30.,1.},PolarPosition{90.,30.,1.},false},
Channel{"U-090",PolarPosition{-90.,30.,1.},PolarPosition{-90.,30.,1.},false},
Channel{"U+180",PolarPosition{180.,30.,1.},PolarPosition{180.,30.,1.},false},
Channel{"B+000",PolarPosition{0.,-30.,1.},PolarPosition{0.,-30.,1.},false},
Channel{"B+045",PolarPosition{45.,-30.,1.},PolarPosition{45.,-30.,1.},false},
Channel{"B-045",PolarPosition{-45.,-30.,1.},PolarPosition{-45.,-30.,1.},false},
Channel{"B+135",PolarPosition{135.,-30.,1.},PolarPosition{135.,-30.,1.},false},
Channel{"B-135",PolarPosition{-135.,-30.,1.},PolarPosition{-135.,-30.,1.},false}}, true },
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

/** Get the speakerLayout that matches the given name. If no match then returns empty.
 * @param layoutName	String containing the name of the desired speaker layout.
 * @return				The speaker layout matching the name. Returns an empty layout if none is found.
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

/*
	Presets of supported output layouts used in the point source panner
*/

// 0 = FL, 1 = FR, 2 = BL, 3 = BR
const std::vector<std::vector<unsigned int>> HULL_0_4_0 = {
{4, 12, 6},{6, 12, 7},{8, 10, 13},{10, 11, 13},
{4, 5, 12},{5, 7, 12},{8, 13, 9},{9, 13, 11},
{0, 4, 6, 2},{2, 6, 7, 3},{0, 2, 10, 8},{2, 3, 11, 10},
{0, 1, 5, 4},{1, 3, 7, 5},{0, 8, 9, 1},{1, 9, 11, 3} };

const std::vector<std::vector<unsigned int>> HULL_0_5_0 = {
{5, 15, 8},{8, 15, 9},{5, 7, 15},{10, 13, 16},
{11, 16, 14},{13, 14, 16},{10, 16, 12},{11, 12, 16},
{6, 9, 15},{6, 15, 7},{3, 8, 9, 4},{0, 2, 7, 5},
{0, 5, 8, 3},{0, 3, 13, 10},{3, 4, 14, 13},{0, 10, 12, 2},
{1, 11, 14, 4},{1, 4, 9, 6},{1, 2, 12, 11},{1, 6, 7, 2} };

const std::vector<std::vector<unsigned int>> HULL_2_5_0 = {
{2, 5, 6},{5, 15, 6},{7, 14, 10},{10, 14, 11},
{7, 9, 14},{5, 12, 15},{0, 5, 2},{6, 15, 13},
{12, 13, 15},{8, 11, 14},{8, 14, 9},{1, 2, 6},
{3, 10, 11, 4},{0, 2, 9, 7},{0, 7, 10, 3},{0, 3, 12, 5},
{3, 4, 13, 12},{1, 6, 13, 4},{1, 4, 11, 8},{1, 8, 9, 2} };

const std::vector<std::vector<unsigned int>> HULL_4_5_0 = {
{2, 5, 6},{5, 15, 6},{9, 14, 12},{12, 14, 13},
{9, 11, 14},{5, 7, 15},{0, 5, 2},{6, 15, 8},
{7, 8, 15},{10, 13, 14},{10, 14, 11},{1, 2, 6},
{3, 12, 13, 4},{0, 2, 11, 9},{0, 9, 12, 3},{0, 3, 7, 5},
{3, 4, 8, 7},{1, 6, 8, 4},{1, 4, 13, 10},{1, 10, 11, 2} };

const std::vector<std::vector<unsigned int>> HULL_4_5_1 = {
{0, 10, 3},{10, 12, 11},{0, 2, 9},{9, 11, 12},
{9, 12, 10},{0, 9, 10},{6, 13, 8},{7, 8, 13},
{5, 13, 6},{5, 7, 13},{0, 5, 2},{2, 5, 6},
{1, 11, 9},{1, 4, 11},{1, 2, 6},{1, 9, 2},
{3, 10, 11, 4},{3, 4, 8, 7},{0, 3, 7, 5},{1, 6, 8, 4} };

const std::vector<std::vector<unsigned int>> HULL_3_7_0 = {
{4, 9, 6},{0, 3, 4},{3, 5, 9},{3, 9, 4},
{6, 9, 8},{15, 17, 16},{2, 4, 6},{0, 4, 2},
{14, 16, 17},{12, 14, 17},{10, 12, 17},{10, 17, 11},
{1, 5, 3},{0, 1, 3},{11, 17, 13},{13, 17, 15},
{5, 7, 9},{7, 8, 9},{6, 8, 16, 14},{2, 6, 14, 12},
{0, 2, 12, 10},{0, 10, 11, 1},{1, 11, 13, 5},{5, 13, 15, 7},
{7, 15, 16, 8} };

const std::vector<std::vector<unsigned int>> HULL_4_9_0 = {
{16, 22, 18},{18, 22, 19},{2, 7, 8},{7, 23, 8},
{7, 9, 23},{4, 8, 10},{8, 23, 10},{9, 10, 23},
{14, 17, 22},{17, 19, 22},{13, 22, 16},{1, 8, 4},
{3, 9, 7},{3, 5, 9},{4, 10, 6},{0, 3, 7},
{2, 8, 12},{1, 12, 8},{14, 22, 21},{15, 21, 22},
{2, 11, 7},{0, 7, 11},{13, 20, 22},{15, 22, 20},
{1, 4, 17, 14},{3, 16, 18, 5},{4, 6, 19, 17},{5, 18, 19, 6},
{5, 6, 10, 9},{0, 13, 16, 3},{1, 14, 21, 12},{2, 12, 21, 15},
{0, 11, 20, 13},{2, 15, 20, 11} };

const std::vector<std::vector<unsigned int>> HULL_4_9_0_wide = {
{16, 22, 18},{18, 22, 19},{4, 8, 10},{8, 23, 10},
{9, 10, 23},{17, 19, 22},{17, 22, 21},{4, 12, 8},
{16, 20, 22},{13, 22, 20},{13, 15, 22},{4, 10, 6},
{3, 5, 9},{1, 2, 8},{1, 8, 12},{2, 7, 8},
{7, 23, 8},{3, 7, 11},{7, 9, 23},{3, 9, 7},
{14, 21, 22},{14, 22, 15},{0, 7, 2},{0, 11, 7},
{4, 17, 21, 12},{4, 6, 19, 17},{5, 18, 19, 6},{5, 6, 10, 9},
{3, 16, 18, 5},{3, 11, 20, 16},{1, 12, 21, 14},{1, 14, 15, 2},
{0, 2, 15, 13},{0, 13, 20, 11} };

const std::vector<std::vector<unsigned int>> HULL_4_9_0_wideL = {
{16, 22, 18},{18, 22, 19},{2, 7, 8},{7, 23, 8},
{7, 9, 23},{4, 8, 10},{8, 23, 10},{9, 10, 23},
{14, 17, 22},{17, 19, 22},{1, 8, 4},{3, 9, 7},
{3, 5, 9},{4, 10, 6},{13, 15, 22},{0, 7, 2},
{2, 8, 12},{1, 12, 8},{14, 22, 21},{15, 21, 22},
{3, 7, 11},{0, 11, 7},{16, 20, 22},{13, 22, 20},
{1, 4, 17, 14},{3, 16, 18, 5},{4, 6, 19, 17},{5, 18, 19, 6},
{5, 6, 10, 9},{0, 2, 15, 13},{1, 14, 21, 12},{2, 12, 21, 15},
{3, 11, 20, 16},{0, 13, 20, 11} };

const std::vector<std::vector<unsigned int>> HULL_4_9_0_wideR = {
{16, 22, 18},{18, 22, 19},{13, 22, 16},{4, 8, 10},
{8, 23, 10},{9, 10, 23},{17, 19, 22},{17, 22, 21},
{4, 12, 8},{2, 7, 8},{7, 23, 8},{7, 9, 23},
{4, 10, 6},{13, 20, 22},{15, 22, 20},{14, 21, 22},
{14, 22, 15},{3, 9, 7},{3, 5, 9},{0, 3, 7},
{1, 2, 8},{1, 8, 12},{2, 11, 7},{0, 7, 11},
{4, 17, 21, 12},{4, 6, 19, 17},{5, 18, 19, 6},{5, 6, 10, 9},
{3, 16, 18, 5},{0, 13, 16, 3},{1, 14, 15, 2},{1, 12, 21, 14},
{2, 15, 20, 11},{0, 11, 20, 13} };

const std::vector<std::vector<unsigned int>> HULL_9_10_3 = {
{23, 24, 27},{19, 21, 27},{23, 27, 26},{21, 26, 27},
{13, 18, 15},{11, 12, 13},{2, 6, 19},{6, 21, 19},
{2, 12, 6},{6, 12, 11},{13, 15, 17},{11, 13, 17},
{22, 27, 24},{19, 27, 20},{13, 14, 18},{10, 13, 12},
{1, 21, 6},{1, 9, 26},{1, 26, 21},{1, 6, 11},
{1, 17, 9},{1, 11, 17},{22, 25, 27},{20, 27, 25},
{13, 16, 14},{10, 16, 13},{0, 25, 8},{0, 20, 25},
{0, 8, 16},{0, 16, 10},{0, 5, 20},{2, 19, 5},
{5, 19, 20},{0, 10, 5},{2, 5, 12},{5, 10, 12},
{4, 7, 24, 23},{4, 15, 18, 7},{4, 23, 26, 9},{4, 9, 17, 15},
{3, 8, 25, 22},{3, 22, 24, 7},{3, 7, 18, 14},{3, 14, 16, 8} };

const std::vector<std::vector<unsigned int>> HULL_0_7_0 = {
{10, 21, 12},{12, 21, 13},{17, 19, 22},{19, 20, 22},
{8, 11, 21},{11, 13, 21},{7, 21, 10},{15, 16, 22},
{15, 22, 18},{18, 22, 20},{8, 21, 9},{7, 9, 21},
{14, 17, 22},{14, 22, 16},{1, 4, 11, 8},{1, 2, 16, 15},
{4, 6, 13, 11},{5, 12, 13, 6},{5, 6, 20, 19},{1, 15, 18, 4},
{4, 18, 20, 6},{1, 8, 9, 2},{3, 10, 12, 5},{3, 5, 19, 17},
{0, 7, 10, 3},{0, 2, 9, 7},{0, 3, 17, 14},{0, 14, 16, 2} };

const std::vector<std::vector<unsigned int>> HULL_4_7_0 = {
{14, 18, 16},{16, 18, 17},{2, 7, 8},{7, 19, 8},
{7, 9, 19},{4, 8, 10},{8, 19, 10},{9, 10, 19},
{12, 15, 18},{15, 17, 18},{11, 18, 14},{1, 2, 8},
{1, 8, 4},{3, 9, 7},{3, 5, 9},{4, 10, 6},
{12, 18, 13},{11, 13, 18},{0, 7, 2},{0, 3, 7},
{1, 4, 15, 12},{3, 14, 16, 5},{4, 6, 17, 15},{5, 16, 17, 6},
{5, 6, 10, 9},{1, 12, 13, 2},{0, 2, 13, 11},{0, 11, 14, 3} };

const std::vector<std::vector<unsigned int>> HULL_2_7_0 = {
{12, 20, 14},{14, 20, 15},{2, 7, 8},{7, 21, 8},
{18, 19, 21},{10, 13, 20},{13, 15, 20},{9, 20, 12},
{1, 2, 8},{1, 8, 4},{8, 21, 17},{17, 21, 19},
{4, 8, 17},{10, 20, 11},{9, 11, 20},{0, 7, 2},
{0, 3, 7},{3, 16, 7},{16, 18, 21},{7, 16, 21},
{1, 4, 13, 10},{3, 12, 14, 5},{4, 6, 15, 13},{5, 14, 15, 6},
{5, 6, 19, 18},{4, 17, 19, 6},{1, 10, 11, 2},{0, 2, 11, 9},
{0, 9, 12, 3},{3, 5, 18, 16} };

const std::vector<std::vector<unsigned int>> HULL_9_10_5 = {
{9, 23, 21},{21, 23, 25},{23, 24, 25},{19, 21, 25},
{13, 18, 15},{4, 23, 9},{11, 12, 13},{2, 6, 19},
{6, 21, 19},{2, 12, 6},{6, 12, 11},{13, 15, 17},
{11, 13, 17},{22, 25, 24},{19, 25, 20},{20, 25, 22},
{8, 20, 22},{13, 14, 18},{10, 13, 12},{1, 9, 21},
{1, 21, 6},{1, 6, 11},{1, 17, 9},{1, 11, 17},
{3, 8, 22},{13, 16, 14},{10, 16, 13},{0, 20, 8},
{0, 8, 16},{0, 16, 10},{0, 5, 20},{2, 19, 5},
{5, 19, 20},{0, 10, 5},{2, 5, 12},{5, 10, 12},
{4, 7, 24, 23},{4, 15, 18, 7},{4, 9, 17, 15},{3, 22, 24, 7},
{3, 7, 18, 14},{3, 14, 16, 8} };

/**
	Cartesian speaker coordinates defined in Rec. ITU-R BS.2127-1 Sec. 11.2
*/

namespace admrender {

	const std::map<std::string, std::map<std::string, CartesianPosition>> alloPositions = {
		// Stereo - BS.2051-3 System A 0+2+0
		{"0+2+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}} }},
		// 5.1 - BS.2051-3 System B 0+5+0
		{"0+5+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+110",{-1.,-1.,0.}},
		{"M-110",{1.,-1.,0.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// 5.1.2 - BS.2051-3 System C 2+5+0
		{"2+5+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+110",{-1.,-1.,0.}},
		{"M-110",{1.,-1.,0.}},
		{"U+030",{-1.,1.,1.}},
		{"U-030",{1.,1.,1.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// 5.1.4 - BS.2051-3 System D 4+5+0
		{"4+5+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+110",{-1.,-1.,0.}},
		{"M-110",{1.,-1.,0.}},
		{"U+030",{-1.,1.,1.}},
		{"U-030",{1.,1.,1.}},
		{"U+110",{-1.,-1.,1.}},
		{"U-110",{1.,-1.,1.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// BS.2051-3 System E 4+5+1
		{"4+5+1",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+110",{-1.,-1.,0.}},
		{"M-110",{1.,-1.,0.}},
		{"U+030",{-1.,1.,1.}},
		{"U-030",{1.,1.,1.}},
		{"U+110",{-1.,-1.,1.}},
		{"U-110",{1.,-1.,1.}},
		{"B+000",{0.,1.,-1.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// BS.2051-3 System F 3+7+0
		{"3+7+0",{ {"M+000",{0.,1.,0.}},
		{"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"U+045",{-1.,1.,1.}},
		{"U-045",{1.,1.,1.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"UH+180",{0.,-1.,1.}},
		{"LFE1",{-1.,1.,-1.}},
		{"LFE2",{1.,1.,-1.}}
		}},
		// BS.2051-3 System G 4+9+0 - Note: doesn't include the screen speakers because these are defined as in Rec. ITU-R BS.2127-1 Sec. 7.3.9
		{"4+9+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"U+045",{-1.,1.,1.}},
		{"U-045",{1.,1.,1.}},
		{"U+135",{-1.,-1.,1.}},
		{"U-135",{1.,-1.,1.}},
		{"LFE1",{-1.,1.,-1.}},
		{"LFE2",{1.,1.,-1.}}
		}},
		// BS.2051-3 System H 9+10+3
		{"9+10+3",{ {"M+060",{-1.,0.414214,0.}},
		{"M-060",{1.,0.414214,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+180",{0.,-1.,0.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"U+045",{-1.,1.,1.}},
		{"U-045",{1.,1.,1.}},
		{"U+000",{0.,1.,1.}},
		{"T+000",{0.,0.,1.}},
		{"U+135",{-1.,-1.,1.}},
		{"U-135",{1.,-1.,1.}},
		{"U+090",{-1.,0.,1.}},
		{"U-090",{1.,0.,1.}},
		{"U+180",{0.,-1.,1.}},
		{"B+000",{0.,1.,-1.}},
		{"B+045",{-1.,1.,-1.}},
		{"B-045",{1.,1.,-1.}},
		{"LFE1",{-1.,1.,-1.}},
		{"LFE2",{1.,1.,-1.}}
		}},
		// 7.1 - BS.2051-3 System I 0+7+0
		{ "0+7+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// 7.1.4 - BS.2051-3 System J 4+7+0
		{ "4+7+0",{ {"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"U+045",{-1.,1.,1.}},
		{"U-045",{1.,1.,1.}},
		{"U+135",{-1.,-1.,1.}},
		{"U-135",{1.,-1.,1.}},
		{"LFE1",{-1.,1.,-1.}}
		}},
		// BEAR 9+10+5
		{"9+10+5", { {"M+060",{-1.,0.414214,0.}},
		{"M-060",{1.,0.414214,0.}},
		{"M+000",{0.,1.,0.}},
		{"M+135",{-1.,-1.,0.}},
		{"M-135",{1.,-1.,0.}},
		{"M+030",{-1.,1.,0.}},
		{"M-030",{1.,1.,0.}},
		{"M+180",{0.,-1.,0.}},
		{"M+090",{-1.,0.,0.}},
		{"M-090",{1.,0.,0.}},
		{"U+045",{-1.,1.,1.}},
		{"U-045",{1.,1.,1.}},
		{"U+000",{0.,1.,1.}},
		{"T+000",{0.,0.,1.}},
		{"U+135",{-1.,-1.,1.}},
		{"U-135",{1.,-1.,1.}},
		{"U+090",{-1.,0.,1.}},
		{"U-090",{1.,0.,1.}},
		{"U+180",{0.,-1.,1.}},
		{"B+000",{0.,1.,-1.}},
		{"B+045",{-1.,1.,-1.}},
		{"B-045",{1.,1.,-1.}},
		{"B+135",{-1.,-1.,-1.}},
		{"B-135",{1.,-1.,-1.}},
		{"LFE1",{-1.,1.,-1.}},
		{"LFE2",{1.,1.,-1.}} }}
	};

	/** Returns the cartesian/allocentric positions of specified layout as specified in Rec. ITU-R BS.2127-1 Sec. 7.3.9
	 * @param layout Layout of the speaker array
	 * @return Positions of the speakers in cartesian/allocentric format. If the layout is not supported by (i.e. not defined
	 * in the tables in section 11.2 then an empty vector is returned.
	 */
	static std::vector<CartesianPosition> positionsForLayout(const Layout& layout)
	{
		std::vector<CartesianPosition> layoutPositions;

		auto it = alloPositions.find(layout.name);
		if (it != alloPositions.end())
		{
			const auto& positions = it->second;

			for (auto& channel : layout.channels)
				if (channel.name == "M+SC" || channel.name == "M-SC")
					layoutPositions.push_back(PointPolarToCart(channel.polarPosition));
				else
					layoutPositions.push_back(positions.at(channel.name));
		}
		return layoutPositions;
	}
}//namespace admrender
