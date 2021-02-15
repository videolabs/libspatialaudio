/*############################################################################*/
/*#                                                                          #*/
/*#  Some tests for the ADM classes											 #*/
/*#                                                                          #*/
/*#  Filename:      Tests.h		                                             #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          06/11/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include <assert.h>

/**
	Get the gain vector for a signal routed directed to a specified speaker in the layout
*/
static inline std::vector<double> getDirectGain(std::string speakerName, Layout layout)
{
	std::vector<double> g(layout.channels.size(), 0.);
	if (layout.getMatchingChannelIndex(speakerName) >= 0)
		g[layout.getMatchingChannelIndex(speakerName)] = 1.;

	return g;
}

/**
	Insert LFE channels in a vector of gains from the point source panner
*/
static inline std::vector<double> insertLFE(std::vector<double> a, Layout layout)
{
	std::vector<double> g(layout.channels.size());
	int iCount = 0;
	for (int i = 0; i < layout.channels.size(); ++i)
		if (!layout.channels[i].isLFE)
			g[i] = a[iCount++];

	return g;
}

static inline bool compareGainVectors(std::vector<double> a, std::vector<double> b)
{
	return std::abs(norm(vecSubtract(a, b))) < 1e-5;
}

/**
	Test the functioning of the DirectSpeaker gain calculator with different conditions.
	Returns true if all return the expected results
*/
static inline bool testDirectSpeakers()
{
	admrender::DirectSpeakerMetadata metadata;
	metadata.audioPackFormatID.push_back("AP_00010004");

	Layout layout = GetMatchingLayout(admrender::ituPackNames.find("AP_00010004")->second);
	// Need to set the reproduction screen in the layout for screen edge locking to work
	layout.reproductionScreen.push_back(Screen());
	double screenWidth = 20.;
	layout.reproductionScreen[0].widthAzimuth = screenWidth;
	admrender::CAdmDirectSpeakersGainCalc gainCalc(layout);
	CPointSourcePannerGainCalc psp(layout);

	std::vector<std::string> speakerNamePrefixes = { "", "urn:itu:bs:2051:0:speaker:", "urn:itu:bs:2051:1:speaker:" };

	for (auto& p : speakerNamePrefixes)
	{
		// Test that the signal is routed to the correct loudspeaker based on the name of a speaker in the array
		std::string speakerLabel = "M+030";
		metadata.speakerLabel = p + speakerLabel;
		auto gName = gainCalc.calculateGains(metadata);
		auto gNameTest = getDirectGain(speakerLabel, layout);
		bool bName = gName == gNameTest;
		assert(bName);

		// Test that the signal is routed to the correct loudspeakers for a speaker not in the array
		// This should use the mapping rules
		speakerLabel = "U+180";
		metadata.speakerLabel = p + speakerLabel;
		auto gMapping = gainCalc.calculateGains(metadata);
		auto gMappingTest = std::vector<double>{ 0.,0.,0.,0.,1. / std::sqrt(2.),1. / std::sqrt(2.),0.,0. };
		bool bMapping = gMapping == gMappingTest;
		assert(bMapping);
	}

	// Test that the signal is routed to the LFE if the low-pass frequency has been set < 200 Hz
	metadata.channelFrequency.lowPass.push_back(100.);
	metadata.speakerLabel = "";
	auto gFreq = gainCalc.calculateGains(metadata);
	auto gFreqTest = getDirectGain("LFE1", layout);
	bool bFreq = gFreq == gFreqTest;
	assert(bFreq);

	// Test that the LFE labels route to the LFE channel
	metadata.channelFrequency.lowPass.clear();// Remove low frequency data and test routing based on LFE names
	std::vector<std::string> lfeNames = { "LFE","LFEL","LFER","LFE1","LFE2" };
	for (auto& lfe : lfeNames)
	{
		metadata.speakerLabel = lfe;
		auto gLFE = gainCalc.calculateGains(metadata);
		auto gLFETest = getDirectGain("LFE1", layout);
		bool bLFE = gFreq == gLFETest;
		assert(bLFE);
	}

	// Test no pack format set but with a speaker label matching one in the layout
	metadata.audioPackFormatID.clear();
	std::string speakerLabel = "M-030";
	metadata.speakerLabel = speakerLabel;
	auto gNoPackMatch = gainCalc.calculateGains(metadata);
	auto gNoPackMatchTest = getDirectGain(speakerLabel, layout);
	bool bNoPackMatch = gNoPackMatch == gNoPackMatchTest;
	assert(bNoPackMatch);

	// Test screen edge locking
	speakerLabel = "";
	metadata.speakerLabel = speakerLabel;
	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::LEFT;
	auto gEdgeLock = gainCalc.calculateGains(metadata);
	auto gEdgeLockTest = insertLFE(psp.CalculateGains(PolarPosition{ screenWidth/2.,0.,1. }), layout);
	bool bEdgeLock = compareGainVectors(gEdgeLock, gEdgeLockTest);
	assert(bEdgeLock);

	// Test with bounds
	// Should find the M+030 speaker within the bounds set
	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::NO_HOR;
	metadata.polarPosition.azimuth = 28.;
	metadata.polarPosition.elevation = 5.;
	metadata.polarPosition.distance = 1.;
	metadata.polarPosition.bounds.push_back({ 25.,35.,-10.,10.,0.9,1.1 });
	auto gBounds = gainCalc.calculateGains(metadata);
	auto gBoundsTest = getDirectGain("M+030", layout);
	bool bBounds = compareGainVectors(gBounds, gBoundsTest);
	assert(bBounds);

	// Should not find any speaker within bounds so fall back to PSP
	metadata.polarPosition.azimuth = 28.;
	metadata.polarPosition.elevation = 5.;
	metadata.polarPosition.distance = 1.;
	metadata.polarPosition.bounds.clear();
	metadata.polarPosition.bounds.push_back({ 27.,29.,-10.,10.,0.9,1.1 });
	auto gNoBounds = gainCalc.calculateGains(metadata);
	auto gNoBoundsTest = insertLFE(psp.CalculateGains(PolarPosition{ metadata.polarPosition.azimuth,metadata.polarPosition.elevation,metadata.polarPosition.distance }), layout);
	bool bNoBounds = compareGainVectors(gNoBounds, gNoBoundsTest);
	assert(bNoBounds);

	// Test the bounds if 2 speakers are within the bounds. Should go to the closer of the two
	metadata.polarPosition.azimuth = 16.;
	metadata.polarPosition.elevation = 0.;
	metadata.polarPosition.distance = 1.;
	metadata.polarPosition.bounds.clear();
	metadata.polarPosition.bounds.push_back({ 0.,30.,0.,0.,1.,1. });
	auto gTwoBounds = gainCalc.calculateGains(metadata);
	auto gTwoBoundsTest = getDirectGain("M+030", layout);
	bool bTwoBounds = compareGainVectors(gTwoBounds, gTwoBoundsTest);
	assert(bTwoBounds);

	// Test fallback panning with the PSP
	metadata.polarPosition = { 10., 5., 1. };
	auto gFallback = gainCalc.calculateGains(metadata);
	auto gFallbackTest = insertLFE(psp.CalculateGains(PolarPosition{ metadata.polarPosition.azimuth,metadata.polarPosition.elevation,metadata.polarPosition.distance }), layout);
	bool bFallback = compareGainVectors(gFallback, gFallbackTest);
	assert(bFallback);

	return true;
}

/**
	Test the point source panner around the horizontal across all layouts that have been defined, except HOA and stereo.
*/
static inline bool testPointSourcePanner()
{
	// Loop through all layouts
	for (auto& layout : speakerLayouts)
	{
		auto layoutNoLFE = getLayoutWithoutLFE(layout);
		CPointSourcePannerGainCalc psp(layout);

		for (double az = 0.; az <= 360.; az += 5.)
		{
			auto position = PolarToCartesian(PolarPosition{ az, 0., 1. });
			auto gains = psp.CalculateGains(position);
			auto gainSum = std::accumulate(gains.begin(), gains.end(), 0.);
			auto numCh = psp.getNumChannels();

			// Calculate the velocity vector based on the loudspeaker gains and directions
			CartesianPosition velVec;
			velVec.x = 0.;
			velVec.y = 0.;
			velVec.z = 0.;
			for (unsigned int i = 0; i < numCh; ++i)
			{
				CartesianPosition speakerDirections = PolarToCartesian(layoutNoLFE.channels[i].polarPosition);
				velVec = velVec + CartesianPosition{ gains[i] * speakerDirections.x, gains[i] * speakerDirections.y,gains[i] * speakerDirections.z, };
			}
			velVec.x /= gainSum;
			velVec.y /= gainSum;
			velVec.z /= gainSum;

			// Convert to a unit vector since for VBAP-type panning the velocity vector usually has a magnitude less than 1 except when on a single speakers
			auto velVecNorm = norm(velVec);
			velVec.x /= velVecNorm;
			velVec.y /= velVecNorm;
			velVec.z /= velVecNorm;

			// Check the direction matches with the intput position
			if (layout.name != "0+2+0" && layout.name != "1OA"
				&& layout.name != "2OA" && layout.name != "3OA")
			{
				assert(norm(position - velVec) < 1e-5);
			}
		}
	}

	return true;
}

/**
	Test the gain calculator with different metadata conditions
*/
static inline bool testGainCalculator()
{
	admrender::ObjectMetadata metadata;
	metadata.referenceScreen.push_back(Screen());

	Layout layout = GetMatchingLayout(admrender::ituPackNames.find("AP_00010004")->second);
	// Need to set the reproduction screen in the layout for screen scaling and screen edge locking to work
	layout.reproductionScreen.push_back(Screen());
	double screenWidth = 20.;
	layout.reproductionScreen[0].widthAzimuth = screenWidth;
	admrender::CGainCalculator gainCalc(layout);
	CPointSourcePannerGainCalc psp(layout);

	std::vector<double> directGains, diffuseGains;

	// Test the panning without any other metadata parameters set
	metadata.polarPosition.azimuth = 28.;
	metadata.polarPosition.elevation = 5.;
	metadata.polarPosition.distance = 1.;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(directGains, psp.CalculateGains(metadata.polarPosition)));

	// Test the screen scaling
	// First test a source on the edge of the screen moves
	metadata.screenRef = true;
	metadata.polarPosition.azimuth = Screen().widthAzimuth / 2.;
	metadata.polarPosition.elevation = 0.;
	metadata.polarPosition.distance = 1.;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(directGains, psp.CalculateGains(PolarPosition{ screenWidth / 2.,0.,1. })));

	// Test the case where the reference screen was not in the centre but the reproduction one is
	metadata.referenceScreen.clear();
	metadata.referenceScreen.push_back(Screen());
	metadata.referenceScreen[0].centrePolarPosition = { 30.,0.,1. };
	metadata.referenceScreen[0].widthAzimuth = 60.;
	metadata.polarPosition = { 30.,0.,1. };
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(directGains, getDirectGain("M+000", layout)));

	// Test the case where the reference screen was in the centre but the reproduction screen is not
	metadata.referenceScreen.clear();
	metadata.referenceScreen.push_back(Screen()); // default reproduction screens
	metadata.polarPosition = { 0.,0.,1. };
	Layout layoutScreenScaling = layout;
	layoutScreenScaling.reproductionScreen.push_back(Screen());
	layoutScreenScaling.reproductionScreen[0].widthAzimuth = 60.;
	layoutScreenScaling.reproductionScreen[0].centrePolarPosition = { 30.,0.,1. };
	admrender::CGainCalculator gainCalcScrnScale(layoutScreenScaling);
	gainCalcScrnScale.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(directGains, getDirectGain("M+030", layout)));

	// Switch off screen scaling
	metadata.screenRef = false;

	// Test screen edge lock - left
	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::LEFT;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	auto gainsEdge = psp.CalculateGains(PolarPosition{ screenWidth / 2.,0.,1. });
	assert(compareGainVectors(directGains, gainsEdge));

	// Test screen edge lock - right
	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::RIGHT;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	gainsEdge = psp.CalculateGains(PolarPosition{ -screenWidth / 2.,0.,1. });
	assert(compareGainVectors(directGains, gainsEdge));

	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::NO_HOR;

	// Test screen edge lock - top
	metadata.screenEdgeLock.vertical = admrender::ScreenEdgeLock::TOP;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	double screenTopEdge = RAD2DEG * std::atan(std::tan(DEG2RAD * 0.5 * screenWidth) / layout.reproductionScreen[0].aspectRatio);
	gainsEdge = psp.CalculateGains(PolarPosition{ 0.,screenTopEdge,1. });
	assert(compareGainVectors(directGains, gainsEdge));

	// Test screen edge lock - bottom
	metadata.screenEdgeLock.vertical = admrender::ScreenEdgeLock::BOTTOM;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	gainsEdge = psp.CalculateGains(PolarPosition{ 0.,-screenTopEdge,1. });
	assert(compareGainVectors(directGains, gainsEdge));


	// Switch off screen edge lock
	metadata.screenEdgeLock.horizontal = admrender::ScreenEdgeLock::NO_HOR;
	metadata.screenEdgeLock.vertical = admrender::ScreenEdgeLock::NO_VERT;


	// Test channel lock - source on speakers
	metadata.polarPosition = { 30.,0.,1. };
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);

	// Test channel lock - source closer to one speaker than another (left)
	metadata.polarPosition = { 14,0.,1. };
	metadata.channelLock.maxDistance = 0.5;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("M+000", layout)));
	metadata.polarPosition = { 16,0.,1. };
	metadata.channelLock.maxDistance = 0.5;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains,layout), getDirectGain("M+030", layout)));

	// Test channel lock - source closer to one speaker than another (right)
	metadata.polarPosition = { -14,0.,1. };
	metadata.channelLock.maxDistance = 0.5;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("M+000", layout)));
	metadata.polarPosition = { -16,0.,1. };
	metadata.channelLock.maxDistance = 0.5;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("M-030", layout)));

	// Test channel lock - source equidistant between two speakers and in range
	metadata.polarPosition = { 15.,0.,1. };
	metadata.channelLock.maxDistance = 1.;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("M+000", layout)));

	// Test channel lock - source equidistant between two speakers with the same abs(azimuth) and in range
	metadata.polarPosition = { 180.,0.,1. };
	metadata.channelLock.maxDistance = 2.;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("M-110", layout)));

	// Test channel lock - source equidistant between two speakers but not in range
	metadata.polarPosition = { 180.,0.,1. };
	metadata.channelLock.maxDistance = 0.05;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), { 0.,0.,0.,0.,std::sqrt(0.5),std::sqrt(0.5),0.,0. }));

	// Test channel lock - closer to upper layer speaker
	metadata.polarPosition = { 30.,16.,1. };
	metadata.channelLock.maxDistance = 1.;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	assert(compareGainVectors(insertLFE(directGains, layout), getDirectGain("U+030", layout)));


	// Remove channel locking
	metadata.channelLock.maxDistance = 0.;

	// Test the diffuseness parameter
	metadata.diffuse = 0.25;
	gainCalc.CalculateGains(metadata, directGains, diffuseGains);
	auto pspGains = psp.CalculateGains(metadata.polarPosition);
	auto pspDirect = pspGains;
	auto pspDiffuse = pspGains;
	for (int i = 0; i < (int)pspGains.size(); ++i)
	{
		pspDirect[i] *= std::sqrt(1. - metadata.diffuse);
		pspDiffuse[i] *= std::sqrt(metadata.diffuse);
	}
	assert(compareGainVectors(directGains, pspDirect));
	assert(compareGainVectors(diffuseGains, pspDiffuse));

	return true;
}
