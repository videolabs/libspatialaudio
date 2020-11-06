/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gain vector to spatialise a DirectSpeaker channel.        #*/
/*#  CAdmDirectSpeakersGainCalc                                              #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmDirectSpeakersGainCalc.cpp                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmDirectSpeakerGainCalc.h"
#include<string>
#include <map>

namespace admrender {

	//===================================================================================================================================
	CAdmDirectSpeakersGainCalc::CAdmDirectSpeakersGainCalc(Layout layoutWithLFE)
		: m_pointSourcePannerGainCalc(getLayoutWithoutLFE(layoutWithLFE)), m_screenEdgeLock(layoutWithLFE.reproductionScreen, layoutWithLFE)
	{
		m_layout = layoutWithLFE;
		m_nCh = (unsigned int)m_layout.channels.size();
	}

	CAdmDirectSpeakersGainCalc::~CAdmDirectSpeakersGainCalc()
	{
	}

	bool CAdmDirectSpeakersGainCalc::isLFE(DirectSpeakerMetadata metadata)
	{
		// See Rec. ITU-R BS.2127-0 sec. 8.2
		if (metadata.channelFrequency.lowPass.size() > 0)
			if (metadata.channelFrequency.lowPass[0] <= 200.)
				return true;

		std::string nominalLabel = GetNominalSpeakerLabel(metadata.speakerLabel);
		if (nominalLabel == std::string("LFE1") ||
			nominalLabel == std::string("LFE2"))
		{
			return true;
		}
		return false;
	}

	int CAdmDirectSpeakersGainCalc::findClosestWithinBounds(DirectSpeakerPolarPosition direction, double tol)
	{
		// See Rec. ITU-R BS.2127-0 sec 8.5
		std::vector<unsigned int> withinBounds;

		// speaker coordinates
		double az = direction.azimuth;
		double el = direction.elevation;
		double d = direction.distance;

		double minAz = az;
		double minEl = el;
		double maxAz = az;
		double maxEl = el;
		double minDist = d;
		double maxDist = d;
		if (direction.bounds.size() > 0)
		{
			minAz = direction.bounds[0].minAzimuth;
			minEl = direction.bounds[0].minElevation;
			maxAz = direction.bounds[0].maxAzimuth;
			maxEl = direction.bounds[0].maxElevation;
			minDist = direction.bounds[0].minDistance;
			maxDist = direction.bounds[0].maxDistance;
		}

		for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
		{
			PolarPosition spkDir = m_layout.channels[iSpk].polarPositionNominal;
			if ((insideAngleRange(spkDir.azimuth, minAz, maxAz, tol) || spkDir.elevation > 90. - tol) &&
				(spkDir.elevation <= maxEl + tol && spkDir.elevation >= minEl - tol) &&
				(spkDir.distance <= maxDist + tol && spkDir.distance >= minDist - tol))
			{
				withinBounds.push_back(iSpk);
			}
		}
		if (withinBounds.size() == 0.)
			return -1; // No speakers found
		else if (withinBounds.size() == 1)
			return withinBounds[0];
		else if (withinBounds.size() > 1)
		{
			std::vector<double> distanceWithinBounds;
			CartesianPosition cartDirection = PolarToCartesian(PolarPosition{ direction.azimuth,direction.elevation,direction.distance });
			for (auto& t : withinBounds)
			{
				CartesianPosition spkCart = PolarToCartesian(m_layout.channels[t].polarPositionNominal);
				double distance = norm(vecSubtract({ spkCart.x,spkCart.y,spkCart.z }, { cartDirection.x,cartDirection.y,cartDirection.z }));
				distanceWithinBounds.push_back(distance);
			}
			// Find the closest
			auto distanceWithinBoundsSorted = distanceWithinBounds;
			std::sort(distanceWithinBoundsSorted.begin(), distanceWithinBoundsSorted.end());
			double smallestDistance = distanceWithinBoundsSorted[0];
			// Check all possible speakers within bounds to determine their distance
			std::vector<unsigned int> closestSpeakers;
			for (unsigned int iDist = 0; iDist < distanceWithinBounds.size(); ++iDist)
			{
				if (distanceWithinBounds[iDist] == smallestDistance)
					closestSpeakers.push_back(withinBounds[iDist]);
			}
			// If only one matches then return that index
			if (closestSpeakers.size() == 1)
				return closestSpeakers[0];
			else if (closestSpeakers.size() == 0) // something probably went wrong...
				return -1;
			else if (closestSpeakers.size() > 1) // no unique answer
				return -1;
		}

		// If nothing found by this point return -1
		return -1;
	}

	std::vector<double> CAdmDirectSpeakersGainCalc::calculateGains(DirectSpeakerMetadata metadata)
	{
		std::vector<double> gains(m_nCh, 0.);

		// is the current channel an LFE
		bool isLfeChannel = isLFE(metadata);

		std::string nominalSpeakerLabel = GetNominalSpeakerLabel(metadata.speakerLabel);

		if (metadata.audioPackFormatID.size() > 0)
		{
			auto ituPack = ituPackNames.find(metadata.audioPackFormatID[0]);
			if (ituPack != ituPackNames.end()) // if the audioPackFormat is in the list of ITU packs
			{
				std::string layoutName = ituPack->second;

				for (const MappingRule& rule : mappingRules)
				{
					if (MappingRuleApplies(rule, layoutName, nominalSpeakerLabel, m_layout))
					{
						for (auto& gain : rule.gains) {
							int idx = m_layout.getMatchingChannelIndex(gain.first);
							if (idx >= 0)
								gains[idx] = gain.second;
						}
						return gains;
					}
				}
			}
		}

		// Check if there are any speakers with the same label and LFE type
		std::string speakerLabel = GetNominalSpeakerLabel(metadata.speakerLabel);
		int idx = m_layout.getMatchingChannelIndex(speakerLabel);
		if (idx >= 0 && (m_layout.channels[idx].isLFE == isLfeChannel))
		{
			gains[idx] = 1.;

			return gains;
		}

		DirectSpeakerPolarPosition direction = metadata.polarPosition;

		// Screen edge locking
		CartesianPosition position = PolarToCartesian(PolarPosition{ direction.azimuth,direction.elevation,direction.distance });
		position = m_screenEdgeLock.HandleVector(position, metadata.screenEdgeLock);
		PolarPosition polarPosition = CartesianToPolar(position);
		direction.azimuth = polarPosition.azimuth;
		direction.elevation = polarPosition.elevation;
		direction.distance = polarPosition.distance;

		// If the channel is LFE then send to the appropriate LFE (if any exist)
		if (isLfeChannel)
		{
			idx = m_layout.getMatchingChannelIndex("LFE1");
			if (idx >= 0)
				gains[idx] = 1.;
		}
		else
		{
			// Check for speakers within bounds
			double tol = 1e-5;
			int idxWithinBounds = findClosestWithinBounds(direction, tol);
			if (idxWithinBounds >= 0)
			{
				gains[idxWithinBounds] = 1.;

				return gains;
			}

			std::vector<double> gainsPSP = m_pointSourcePannerGainCalc.CalculateGains(PolarPosition{ direction.azimuth,direction.elevation,direction.distance });
			// fill in the gains on the non-LFE channels
			int indNonLfe = 0;
			for (unsigned int i = 0; i < gains.size(); ++i)
				if (!m_layout.channels[i].isLFE)
					gains[i] = gainsPSP[indNonLfe++];
		}

		return gains;
	}

	bool CAdmDirectSpeakersGainCalc::MappingRuleApplies(const MappingRule& rule, const std::string& inputLayout, const std::string& speakerLabel, Layout& outputLayout)
	{
		// All conditions must be met for the rule to apply
		// "rule.speakerLabel is equal to the first (and only) speakerLabel"
		if (speakerLabel != rule.speakerLabel)
			return false;

		// "input_layout [...] is listed in rule.input_layouts, if this is listed."
		bool containsLayout = false;
		if (rule.inputLayouts.size() > 0)
		{
			for (auto& layout : rule.inputLayouts)
				if (layout == inputLayout)
					containsLayout = true;
			if (!containsLayout)
				return false;
		}

		// "The name of the output loudspeaker layout, layout.name, is listed in
		// rule.output_layouts, if this is listed."
		containsLayout = false;
		if (rule.outputLayouts.size() > 0)
		{
			for (auto& layout : rule.outputLayouts)
				if (layout == outputLayout.name)
					containsLayout = true;
			if (!containsLayout)
				return false;
		}

		// All channel names listed in rule.gains exist in layout.channel_names
		for (auto& gain : rule.gains)
		{
			bool containsChannelName = false;
			for (auto& channelName : outputLayout.channelNames())
				if (channelName == gain.first)
					containsChannelName = true;
			if (!containsChannelName)
				return false;
		}

		return true;
	}
}