/*############################################################################*/
/*#                                                                          #*/
/*#  A point source panner for ADM renderer.                                 #*/
/*#  CAdmPointSourcePanner - ADM Point Source Panner						 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePanner.cpp							     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmPointSourcePanner.h"
#include<string>
#include <map>

namespace admrender {

	CAdmPointSourcePannerGainCalc::CAdmPointSourcePannerGainCalc(Layout layout)
	{
		// Store the output layout
		outputLayout = layout;
		std::string layoutName = outputLayout.name;
		// Check that the loudspeaker layout is supported
		assert(layoutName.compare("0+2+0") == 0 || layoutName.compare("0+4+0") == 0
			|| layoutName.compare("0+5+0") == 0 || layoutName.compare("2+5+0") == 0
			|| layoutName.compare("0+7+0") == 0);

		// TODO: Handle 0+2+0 as a special case

		std::vector<std::vector<unsigned int>> hull;
		if (layoutName == "0+2+0")
		{
			hull = HULL_0_5_0;
			isStereo = true;
			outputLayout = getLayoutWithoutLFE(GetMatchingLayout("0+5+0"));
		}
		else if (layoutName == "0+5+0")
			hull = HULL_0_5_0;
		else if (layoutName == "2+5+0")
			hull = HULL_2_5_0;
		else if (layoutName == "0+4+0")
			hull = HULL_0_4_0;
		else if (layoutName == "0+7+0")
			hull = HULL_0_7_0;
		unsigned int nOutputCh = (unsigned int)outputLayout.channels.size();

		// Get the positions of all of the loudspeakers
		std::vector<PolarPosition> positions;
		for (int i = 0; i < outputLayout.channels.size(); ++i)
		{
			downmixMapping.push_back(i); // one-to-one downmix mapping
			positions.push_back(outputLayout.channels[i].polarPosition);
		}

		// get the extra speakers
		extraSpeakersLayout = calculateExtraSpeakersLayout(outputLayout);
		// Get the indices of the virtual speakers at the top and bottom
		// Just check the last 2. Some layouts may not have a virtual speaker at
		// the top so do not assume that the last 2 entries are definitely virtual
		// speakers
		std::vector<unsigned int> virtualSpkInd;
		unsigned int nExtraSpk = (unsigned int)extraSpeakersLayout.channels.size();
		if (extraSpeakersLayout.channels[nExtraSpk - 2].name == "TOP" ||
			extraSpeakersLayout.channels[nExtraSpk - 2].name == "BOTTOM")
			virtualSpkInd.push_back(nOutputCh + nExtraSpk - 2);
		if (extraSpeakersLayout.channels[nExtraSpk - 1].name == "TOP" ||
			extraSpeakersLayout.channels[nExtraSpk - 1].name == "BOTTOM")
			virtualSpkInd.push_back(nOutputCh + nExtraSpk - 1);

		// Add the extra speakers to the list of positions
		for (int i = 0; i < extraSpeakersLayout.channels.size(); ++i)
			positions.push_back(extraSpeakersLayout.channels[i].polarPosition);

		// Go through all the facets of the hull to create the required RegionHandlers
		unsigned int nFacets = (unsigned int)hull.size();
		for (unsigned int iFacet = 0; iFacet < nFacets; ++iFacet)
		{
			unsigned int nVertices = (unsigned int)hull[iFacet].size();
			// Check if the facet contains one of the virtual speakers
			bool hasVirtualSpeaker = false;
			for (unsigned int i = 0; i < nVertices; ++i)
				for (int iVirt = 0; iVirt < virtualSpkInd.size(); ++iVirt)
					if (hull[iFacet][i] == virtualSpkInd[iVirt])
						hasVirtualSpeaker = true;
			if (!hasVirtualSpeaker)
			{
				if (nVertices == 4)
				{
					std::vector<PolarPosition> facetPositions;
					for (int i = 0; i < 4; ++i)
						facetPositions.push_back(positions[hull[iFacet][i]]);
					regions.quadRegions.push_back(QuadRegion(hull[iFacet], facetPositions));
				}
				else if (nVertices == 3)
				{
					std::vector<PolarPosition> facetPositions;
					for (int i = 0; i < 3; ++i)
						facetPositions.push_back(positions[hull[iFacet][i]]);
					regions.triplets.push_back(Triplet(hull[iFacet], facetPositions));
				}
			}
		}
		// Loop through all facets to find those that contain a virtual speaker. If they do, add their
		// indices to a list and then create a virtualNgon for the corresponding set
		for (int iVirt = 0; iVirt < virtualSpkInd.size(); ++iVirt)
		{
			std::set<unsigned int> virtualNgonVertInds;
			for (unsigned int iFacet = 0; iFacet < nFacets; ++iFacet)
			{
				unsigned int nVertices = (unsigned int)hull[iFacet].size();
				// Check if the facet contains one of the virtual speakers
				bool hasVirtualSpeaker = false;
				for (unsigned int i = 0; i < nVertices; ++i)
					if (hull[iFacet][i] == virtualSpkInd[iVirt])
						hasVirtualSpeaker = true;
				if (hasVirtualSpeaker)
				{
					for (unsigned int i = 0; i < nVertices; ++i)
						virtualNgonVertInds.insert(hull[iFacet][i]);
				}
			}
			// Remove the virtual speaker from the set
			virtualNgonVertInds.erase(virtualSpkInd[iVirt]);
			std::vector<PolarPosition> ngonPositions;
			std::vector<unsigned int> ngonInds(virtualNgonVertInds.begin(), virtualNgonVertInds.end());
			for (int i = 0; i < ngonInds.size(); ++i)
			{
				ngonPositions.push_back(positions[ngonInds[i]]);
			}
			regions.virtualNgons.push_back(VirtualNgon(ngonInds, ngonPositions, positions[virtualSpkInd[iVirt]]));
		}

	}

	CAdmPointSourcePannerGainCalc::~CAdmPointSourcePannerGainCalc()
	{
	}

	std::vector<double> CAdmPointSourcePannerGainCalc::calculateGains(PolarPosition direction)
	{
		std::vector<double> gains = _calculateGains(direction);
		if (isStereo) // then downmix from 0+5+0 to 0+2+0
		{
			// See Rec. ITU-R BS.2127-0 6.1.2.4 (page 2.5) for downmix method 
			double stereoDownmix[2][5] = { {1.,0.,1. / sqrt(3.),1. / sqrt(2.),0.}, {0.,1.,1. / sqrt(3.),0.,1. / sqrt(2.)} };
			std::vector<double> gainsStereo(2, 0.);
			for (int i = 0; i < 2; ++i)
				for (int j = 0; j < 5; ++j)
					gainsStereo[i] += stereoDownmix[i][j] * gains[j];
			double a_front;
			int i = 0;
			for (i = 0; i < 3; ++i)
				a_front = std::max(a_front, gains[i]);
			double a_rear;
			for (i = 3; i < 5; ++i)
				a_rear = std::max(a_rear, gains[i]);
			double r = a_rear / (a_front + a_rear);
			double gainNormalisation = std::pow(0.5, r / 2.) / norm(gainsStereo);

			gainsStereo[0] *= gainNormalisation;
			gainsStereo[1] *= gainNormalisation;

			return gainsStereo;
		}
		else
			return gains;
	}

	std::vector<double> CAdmPointSourcePannerGainCalc::_calculateGains(PolarPosition direction)
	{
		double tol = 1e-6;
		std::vector<double> gains(outputLayout.channels.size(), 0.);

		// get the unit vector in the target direction
		std::vector<double> directionUnitVec(3, 0.);
		direction.distance = 1.;
		CartesianPosition cartesianDirection = PolarToCartesian(direction);
		directionUnitVec = { cartesianDirection.x, cartesianDirection.y,cartesianDirection.z };

		// Loop through all of the regions until one is found that is not zero gain
		for (int iNgon = 0; iNgon < regions.virtualNgons.size(); ++iNgon)
		{
			std::vector<double> nGonGains = regions.virtualNgons[iNgon].calculateGains(directionUnitVec);
			if (norm(nGonGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> nGonInds = regions.virtualNgons[iNgon].channelInds;
				for (int iGain = 0; iGain < nGonGains.size(); ++iGain)
					gains[downmixMapping[nGonInds[iGain]]] += nGonGains[iGain];

				return gains;
			}
		}
		// Loop through the triplets Ngons
		for (int iTriplet = 0; iTriplet < regions.triplets.size(); ++iTriplet)
		{
			std::vector<double> tripletGains = regions.triplets[iTriplet].calculateGains(directionUnitVec);
			if (norm(tripletGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> tripletInds = regions.triplets[iTriplet].channelInds;
				for (int iGain = 0; iGain < tripletGains.size(); ++iGain)
					gains[downmixMapping[tripletInds[iGain]]] += tripletGains[iGain];

				return gains;
			}
		}
		// Loop through the triplets Ngons
		for (int iQuad = 0; iQuad < regions.quadRegions.size(); ++iQuad)
		{
			std::vector<double> quadGains = regions.quadRegions[iQuad].calculateGains(directionUnitVec);
			if (norm(quadGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> quadInds = regions.quadRegions[iQuad].channelInds;
				for (int iGain = 0; iGain < quadGains.size(); ++iGain)
					gains[downmixMapping[quadInds[iGain]]] += quadGains[iGain];

				return gains;
			}
		}

		return gains;
	}

	Layout CAdmPointSourcePannerGainCalc::calculateExtraSpeakersLayout(Layout layout)
	{
		Layout extraSpeakers;
		unsigned int nSpeakers = (unsigned int)layout.channels.size();

		// Find if speakers are present in each layer
		std::vector<unsigned int> upperLayerSet;
		std::vector<unsigned int> midLayerSet;
		std::vector<unsigned int> lowerLayerSet;
		double maxUpperAz = 0.f;
		double maxLowerAz = 0.f;
		for (unsigned int iSpk = 0; iSpk < nSpeakers; ++iSpk)
		{
			double el = layout.channels[iSpk].polarPositionNominal.elevation;
			if (el >= 30 && el <= 70)
			{
				upperLayerSet.push_back(iSpk);
				// TODO: consider remapping azimuth to range -180 to 180
				maxUpperAz = std::max(maxUpperAz, abs(layout.channels[iSpk].polarPositionNominal.azimuth));
			}
			else if (el >= -10 && el <= 10)
			{
				midLayerSet.push_back(iSpk);
			}
			else if (el >= -70 && el <= -30)
			{
				lowerLayerSet.push_back(iSpk);
				// TODO: consider remapping azimuth to range -180 to 180
				maxLowerAz = std::max(maxLowerAz, abs(layout.channels[iSpk].polarPositionNominal.azimuth));
			}
		}

		PolarPosition position;
		for (int iMid = 0; iMid < midLayerSet.size(); ++iMid)
		{
			auto name = layout.channels[midLayerSet[iMid]].name;
			double azimuth = layout.channels[midLayerSet[iMid]].polarPositionNominal.azimuth;
			// Lower layer
			if ((lowerLayerSet.size() > 0 && abs(azimuth) > maxLowerAz + 40.) || lowerLayerSet.size() == 0)
			{
				downmixMapping.push_back(iMid);
				name.at(0) = 'B';
				position.azimuth = azimuth;
				position.elevation = -30.;
				extraSpeakers.channels.push_back({ name,position,position,false });
			}
		}
		for (int iMid = 0; iMid < midLayerSet.size(); ++iMid)
		{
			auto name = layout.channels[midLayerSet[iMid]].name;
			double azimuth = layout.channels[midLayerSet[iMid]].polarPositionNominal.azimuth;
			// Upper layer
			if ((upperLayerSet.size() > 0 && abs(azimuth) > maxUpperAz + 40.) || upperLayerSet.size() == 0)
			{
				downmixMapping.push_back(iMid);
				name.at(0) = 'U';
				position.azimuth = azimuth;
				position.elevation = 30.;
				extraSpeakers.channels.push_back({ name,position,position,false });
			}
		}

		// Add top and bottom virtual speakers
		position.azimuth = 0.;
		position.elevation = -90.;
		extraSpeakers.channels.push_back({ "BOTTOM",position,position,false });
		position.elevation = 90.;
		extraSpeakers.channels.push_back({ "TOP",position,position,false });

		return extraSpeakers;
	}

	//===================================================================================================================================
	ChannelLockHandler::ChannelLockHandler(Layout layout)
	{
		m_layout = layout;
		m_nCh = (unsigned int)layout.channels.size();
	}

	ChannelLockHandler::~ChannelLockHandler()
	{
	}

	PolarPosition ChannelLockHandler::handle(ChannelLock channelLock, PolarPosition polarDirection)
	{
		// If distance is <0 then no channel locking is set so return the original direction
		if (channelLock.maxDistance < 0.)
			return polarDirection;

		double maxDistance = channelLock.maxDistance;
		double tol = 1e-10;

		// Get the cartesian source position to use when calculating the l-2 norm.
		CartesianPosition cartDirection = PolarToCartesian(polarDirection);

		// Calculate the l-2 norm between the normalised real speaker directions and the source position.
		std::vector<double> l2norm;
		std::vector<unsigned int> closestSpeakersInd;
		for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
		{
			PolarPosition speakerPosition = m_layout.channels[iCh].polarPosition;
			// Rec. ITU-R BS.2127-0 pg. 44 - "loudspeaker positions considered are the normalised real loudspeaker
			// positions in layout" so normalise the distance
			speakerPosition.distance = 1.;
			CartesianPosition speakerCartPosition = PolarToCartesian(speakerPosition);
			std::vector<double> differenceVector = { speakerCartPosition.x - cartDirection.x, speakerCartPosition.y - cartDirection.y, speakerCartPosition.z - cartDirection.z };
			double differenceNorm = norm(differenceVector);
			// If the speaker is within the range 
			if (differenceNorm < maxDistance)
			{
				closestSpeakersInd.push_back(iCh);
				l2norm.push_back(differenceNorm);
			}
		}
		unsigned int nSpeakersInRange = (unsigned int)closestSpeakersInd.size();
		// If no speakers are in range then return the original direction
		if (nSpeakersInRange == 0)
			return polarDirection;
		else if (nSpeakersInRange == 1) // if there is a unique speaker in range then return that direction
			return m_layout.channels[closestSpeakersInd[0]].polarPosition;
		else if (nSpeakersInRange > 1)
		{
			// Find the minimum l-2 norm from the speakers within range
			double minL2Norm = *std::min_element(l2norm.begin(), l2norm.end());
			// Find all the speakers that are within tolerance of this minimum value
			std::vector<unsigned int> equalDistanceSpeakers;
			for (unsigned int iNorm = 0; iNorm < l2norm.size(); ++iNorm)
				if (l2norm[iNorm] > minL2Norm - tol && l2norm[iNorm] < minL2Norm + tol)
				{
					equalDistanceSpeakers.push_back(closestSpeakersInd[iNorm]);
				}
			// If only one of the speakers in range is within tol of the minimum then return that direction
			if (equalDistanceSpeakers.size() == 1)
				return m_layout.channels[equalDistanceSpeakers[0]].polarPosition;
			else // if not, find the closest by lexicographic comparison of the tuple {|az|,az,|el|,el}
			{
				std::vector<std::vector<double>> tuple;
				for (auto& t : equalDistanceSpeakers)
				{
					double az = m_layout.channels[t].polarPosition.azimuth;
					double el = m_layout.channels[t].polarPosition.elevation;
					tuple.push_back({ abs(az), az, abs(el), el });
				}
				auto tupleSorted = tuple;
				sort(tupleSorted.begin(), tupleSorted.end());
				for (unsigned int iTuple = 0; iTuple < tuple.size(); ++iTuple)
					if (tuple[iTuple] == tupleSorted[0])
						return m_layout.channels[equalDistanceSpeakers[iTuple]].polarPosition;
			}
		}

		// Final fallback, return original position
		return polarDirection;
	}

	//===================================================================================================================================
	ZoneExclusionHandler::ZoneExclusionHandler(Layout layout)
	{
		m_layout = getLayoutWithoutLFE(layout);
		m_nCh = (unsigned int)m_layout.channels.size();

		// Get the cartesian coordinates of all nominal positions
		std::vector<CartesianPosition> cartesianPositions;
		for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
		{
			cartesianPositions.push_back(PolarToCartesian(m_layout.channels[iSpk].polarPositionNominal));
		}

		// Determine the speaker groups. See Rec. ITU-R BS.2127-0 sec. 7.3.12.2.1 pg. 62
		for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
		{
			std::vector<std::vector<double>> tuples;
			CartesianPosition cartIn = cartesianPositions[iSpk];
			std::vector<std::string> channelNames = m_layout.channelNames();
			for (unsigned int iOutSpk = 0; iOutSpk < m_nCh; ++iOutSpk)
			{
				CartesianPosition cartOut = cartesianPositions[iOutSpk];
				// key 1 - layer priority
				int layerPriority = getLayerPriority(channelNames[iSpk], channelNames[iOutSpk]);
				// key 2 - front/back priority
				int frontBackPriority = abs(Sgn(cartOut.y) - Sgn(cartIn.y));
				// key 3 - vector distance between nominal positions
				double vectorDistance = norm(vecSubtract({ cartOut.x,cartOut.y,cartOut.z }, { cartIn.x,cartIn.y,cartIn.z }));
				// key 4 - absolute difference in nominal y coordinates
				double absYDiff = abs(cartOut.y - cartIn.y);

				tuples.push_back({ (double)layerPriority,(double)frontBackPriority,vectorDistance,absYDiff });
			}
			// Determine the groupings for the current intput speaker
			auto tupleSorted = tuples;
			std::sort(tupleSorted.begin(), tupleSorted.end());
			std::vector<std::set<unsigned int>> tupleOrder(m_nCh);
			for (unsigned int iTuple = 0; iTuple < tuples.size(); ++iTuple)
				for (unsigned int i = 0; i < tuples.size(); ++i)
					if (tuples[iTuple] == tupleSorted[i])
						tupleOrder[i].insert(iTuple);

			std::vector<std::set<unsigned int>>::iterator ip;

			ip = std::unique(tupleOrder.begin(), tupleOrder.end());
			// Resizing the vector so as to remove the undefined terms 
			tupleOrder.resize(std::distance(tupleOrder.begin(), ip));

			m_downmixMapping.push_back(tupleOrder);
		}
	}

	ZoneExclusionHandler::~ZoneExclusionHandler()
	{
	}

	int ZoneExclusionHandler::getLayerPriority(std::string inputChannelName, std::string outputChannelName)
	{
		std::map<char, int> layerIndex = { {'B',0},{'M',1},{'U',2},{'T',3} };
		int inIndex = layerIndex[inputChannelName[0]];
		int outIndex = layerIndex[outputChannelName[0]];

		int layerPriority[4][4] = { {0,1,2,3},{3,0,1,2},{3,2,0,1},{3,2,1,0} };

		return layerPriority[inIndex][outIndex];
	}

	std::vector<double> ZoneExclusionHandler::handle(std::vector<PolarExclusionZone> exclusionZones, std::vector<double> gains)
	{
		double tol = 1e-6;

		// Downmix matrix
		std::vector<std::vector<double>> D(m_nCh, std::vector<double>(m_nCh, 0.));

		// Find the set of excluded speakers
		std::vector<bool> isExcluded(m_nCh, false);
		int nCountExcluded = 0;
		for (unsigned int iZone = 0; iZone < exclusionZones.size(); ++iZone)
		{
			PolarExclusionZone zone = exclusionZones[iZone];

			for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
			{
				double az = m_layout.channels[iSpk].polarPositionNominal.azimuth;
				double el = m_layout.channels[iSpk].polarPositionNominal.elevation;
				if ((zone.minElevation - tol < el && el < zone.maxElevation + tol) && (el > 90 - tol || insideAngleRange(az, zone.minAzimuth, zone.maxAzimuth)))
				{
					isExcluded[iSpk] = true;
					nCountExcluded++;
				}
			}
		}

		if (nCountExcluded == (int)m_nCh || nCountExcluded == 0)
		{
			// If all or none of the speakers are exlcuded then set the downmix matrix to the identity matrix
			for (int i = 0; i < (int)m_nCh; ++i)
				D[i][i] = 1.;
		}
		else
		{
			// Go through all the speakers and find the first set that contains non-exlcuded speakers
			for (int iSpk = 0; iSpk < (int)m_nCh; ++iSpk)
			{
				// Find the first set with non-excluded speakers
				for (int iSet = 0; iSet < m_downmixMapping[iSpk].size(); ++iSet)
				{
					std::vector<unsigned int> notExcludedElements;
					std::vector<unsigned int> setElements(m_downmixMapping[iSpk][iSet].begin(), m_downmixMapping[iSpk][iSet].end());
					for (int i = 0; i < setElements.size(); ++i)
						if (!isExcluded[setElements[i]])
							notExcludedElements.push_back(setElements[i]);
					int numNotExcluded = (int)notExcludedElements.size();
					if (numNotExcluded > 0)
					{
						// Fill the downmix matrix D
						for (int i = 0; i < numNotExcluded; ++i)
							D[notExcludedElements[i]][iSpk] = 1. / (double)numNotExcluded;
						break;
					}
				}
			}
		}

		// Calculate the downmixed output gain vector
		std::vector<double> outGains(m_nCh, 0.);
		for (int i = 0; i < (int)m_nCh; ++i)
		{
			double g_tmp = 0.;
			for (unsigned int j = 0; j < m_nCh; ++j)
				g_tmp += D[i][j] * gains[j] * gains[j];
			outGains[i] = sqrt(g_tmp);
		}

		return outGains;
	}

	//===================================================================================================================================
	CAdmDirectSpeakersGainCalc::CAdmDirectSpeakersGainCalc(Layout layoutWithLFE)
		: m_pointSourcePannerGainCalc(getLayoutWithoutLFE(layoutWithLFE))
	{
		m_layout = layoutWithLFE;
		m_nCh = (unsigned int) m_layout.channels.size();
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
			CartesianPosition cartDirection = PolarToCartesian(direction);
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

		// TODO: Implement screenLock

		DirectSpeakerPolarPosition direction = metadata.polarPosition;

		// Check for speakers within bounds
		double tol = 1e-5;
		int idxWithinBounds = findClosestWithinBounds(direction, tol);
		if (idxWithinBounds >= 0)
		{
			gains[idxWithinBounds] = 1.;

			return gains;
		}

		// If no speakers within bounds then get the gains from a point source panner or route
		// the output to LFE1
		if (isLfeChannel)
		{
			idx = m_layout.getMatchingChannelIndex("LFE1");
			gains[idx] = 1.;
		}
		else
		{
			std::vector<double> gainsPSP = m_pointSourcePannerGainCalc.calculateGains(PolarPosition{ direction.azimuth,direction.elevation,direction.distance });
			// fill in the gains on the non-LFE channels
			int indNonLfe = 0;
			for (unsigned int i = 0; i < gains.size(); ++i)
				if (!m_layout.channels[i].isLFE)
					gains[i] = gainsPSP[indNonLfe++];
		}

		return gains;
	}

	std::string CAdmDirectSpeakersGainCalc::GetNominalSpeakerLabel(const std::string& label)
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
		if (speakerLabel.compare("LFE") || speakerLabel.compare("LFEL"))
			speakerLabel = "LFE1";
		else if (speakerLabel.compare("LFER"))
			speakerLabel = "LFE2";

		return speakerLabel;
	}

	bool CAdmDirectSpeakersGainCalc::MappingRuleApplies(const MappingRule& rule, const std::string& inputLayout, const std::string& speakerLabel, admrender::Layout& outputLayout)
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

	//===================================================================================================================================
	CAdmPointSourcePanner::CAdmPointSourcePanner(Layout targetLayout) : m_gainCalculator(getLayoutWithoutLFE(targetLayout)),
		channelLockHandler(targetLayout), zoneExclusionHandler(targetLayout)
	{
		m_layout = targetLayout;
		m_nCh = 0;
		for (unsigned int iCh = 0; iCh < m_layout.channels.size(); ++iCh)
			if (!m_layout.channels[iCh].isLFE)
				m_nCh++;
		m_gains.resize(m_nCh, 0.);
	}

	CAdmPointSourcePanner::~CAdmPointSourcePanner()
	{

	}

	void CAdmPointSourcePanner::ProcessAccumul(ObjectMetadata metadata, float* pIn, std::vector<std::vector<float>> &ppDirect, std::vector<std::vector<float>>& ppDiffuse, unsigned int nSamples)
	{
		int nInterpSamples = 0;

		std::vector<double> gains(m_nCh);

		if (!(metadata == m_metadata))
		{
			// Get the panning direction
			PolarPosition direction;
			if (metadata.cartesian)
			{
				// If cartesian = true then convert the position to polar coordinates.
				// Note: Rec. ITU-R BS.2127-0 defines a different set of processing when
				// this flag is set but that is not yet implemented here. Instead, the polar
				// position path is used regardless of this flag.
				direction = CartesianToPolar(metadata.cartesianPosition);
			}
			else
				direction = metadata.polarPosition;

			// TODO: Apply screenEdgeLock and screenScaling

			// Apply channelLock to modify the position of the source, if required
			direction = channelLockHandler.handle(metadata.channelLock, direction);

			// Apply divergence
			auto divergedData = divergedPositionsAndGains(metadata.objectDivergence, direction);
			auto diverged_positions = divergedData.first;
			auto diverged_gains = divergedData.second;
			unsigned int nDivergedGains = (unsigned int)diverged_gains.size();

			// Calculate the new gains to be applied
			std::vector<std::vector<double>> gains_for_each_pos(nDivergedGains);
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				gains_for_each_pos[iGain] = m_gainCalculator.calculateGains(diverged_positions[iGain]);

			// Power summation of the gains
			for (int i = 0; i < (int)m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * gains_for_each_pos[j][i] * gains_for_each_pos[j][i];
				gains[i] = sqrt(g_tmp);
			}

			// Zone exclusion downmix
			// See Rec. ITU-R BS.2127-0 sec. 7.3.12, pg 60
			gains = zoneExclusionHandler.handle(metadata.zoneExclusionPolar, gains);

			// Apply the overall gain to the spatialisation gains
			for (auto& g : gains)
				g *= metadata.gain;

			// Set the interpolation duration based on the conditions on page 35 of Rec. ITU-R BS.2127-0
			if (metadata.jumpPosition.flag && !m_bFirstFrame)
			{
				nInterpSamples = (int)round(metadata.jumpPosition.interpolationLength * nSamples);
			}
		}
		else // if the metadata input is the same as the last set
		{
			gains = m_gains;
		}

		// Calculate the direct and diffuse gains
		// See Rec. ITU-R BS.2127-0 sec.7.3.1 page 39
		float directCoefficient = (float)std::sqrt(1. - metadata.diffuse);
		float diffuseCoefficient = (float)std::sqrt(metadata.diffuse);

		unsigned int iCh = 0;
		// Apply the gains and add them to the non-LFE channels
		for (unsigned int i = 0; i < m_layout.channels.size(); ++i)
		{
			if (!m_layout.channels[i].isLFE) // if not LFE then skip the output channel
			{
				float deltaCoeff = 1.f / ((float)nInterpSamples);
				int iSample = 0;
				for (iSample = 0; iSample < nInterpSamples; ++iSample)
				{
					float fInterp = (float)iSample * deltaCoeff;
					float sampleData = pIn[iSample] * (fInterp * (float)gains[iCh] + (1.f - fInterp) * (float)m_gains[iCh]);
					ppDirect[i][iSample] += sampleData * directCoefficient;
					ppDiffuse[i][iSample] += sampleData * diffuseCoefficient;
				}
				for (iSample = nInterpSamples; iSample < (int)nSamples; ++iSample)
				{
					float sampleData = pIn[iSample] * (float)gains[iCh];
					ppDirect[i][iSample] += sampleData * directCoefficient;
					ppDiffuse[i][iSample] += sampleData * diffuseCoefficient;
				}
				iCh++;
			}
		}

		// Store the last calculated gains
		m_gains = gains;
		// Store the last input metadata
		m_metadata = metadata;
		// Flag that at least one frame has been processed
		m_bFirstFrame = false;
	}

	std::pair<std::vector<PolarPosition>, std::vector<double>> CAdmPointSourcePanner::divergedPositionsAndGains(ObjectDivergence divergence, PolarPosition polarDirection)
	{
		std::vector<PolarPosition> diverged_positions;
		std::vector<double> diverged_gains;
		
		double x = divergence.value;
		double d = polarDirection.distance;
		// if the divergence value is zero then return the original direction and a gain of 1
		if (x == 0.)
			return { std::vector<PolarPosition>{polarDirection},std::vector<double>{1.} };

		// If there is any divergence then calculate the gains and directions
		// Calculate gains using Rec. ITU-R BS.2127-0 sec. 7.3.7.1
		diverged_gains.resize(3, 0.);
		diverged_gains[0] = (1.-x)/(x+1.);
		double glr = x / (x + 1.);
		diverged_gains[1] = glr;
		diverged_gains[2] = glr;

		std::vector<std::vector<double>> cartPositions(3);
		cartPositions[0] = { d,0.,0. };
		auto cartesianTmp = PolarToCartesian(PolarPosition{ x * divergence.azimuthRange,0.,d });
		cartPositions[1] = { cartesianTmp.y,-cartesianTmp.x,cartesianTmp.z };
		cartesianTmp = PolarToCartesian(PolarPosition{ -x * divergence.azimuthRange,0.,d });
		cartPositions[2] = { cartesianTmp.y,-cartesianTmp.x,cartesianTmp.z };

		// Rotate them so that the centre position is in specified input direction
		double rotMat[9] = { 0. };
		getRotationMatrix(polarDirection.azimuth, -polarDirection.elevation, 0., &rotMat[0]);
		diverged_positions.resize(3);
		for (int iDiverge = 0; iDiverge < 3; ++iDiverge)
		{
			std::vector<double> directionRotated(3, 0.);
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					directionRotated[i] += rotMat[3 * i + j] * cartPositions[iDiverge][j];
			diverged_positions[iDiverge] = CartesianToPolar(CartesianPosition{ -directionRotated[1],directionRotated[0],directionRotated[2] });
		}

		return { diverged_positions, diverged_gains };
	}

}