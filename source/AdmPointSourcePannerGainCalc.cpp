/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gains required for point source panning.                  #*/
/*#  CAdmPointSourcePannerGainCalc - ADM Point Source Panner                 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePannerGainCalc.cpp                         #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmPointSourcePannerGainCalc.h"
#include<cmath>
#include<string>
#include <map>

namespace admrender {

	CAdmPointSourcePannerGainCalc::CAdmPointSourcePannerGainCalc(Layout layout)
	{
		// Store the output layout
		m_outputLayout = layout;
		std::string layoutName = m_outputLayout.name;
		// Check that the loudspeaker layout is supported
		assert(layoutName.compare("0+2+0") == 0 || layoutName.compare("0+4+0") == 0
			|| layoutName.compare("0+5+0") == 0 || layoutName.compare("2+5+0") == 0
			|| layoutName.compare("0+7+0") == 0);

		std::vector<std::vector<unsigned int>> hull;
		if (layoutName == "0+2+0")
		{
			hull = HULL_0_5_0;
			m_isStereo = true;
			m_outputLayout = getLayoutWithoutLFE(GetMatchingLayout("0+5+0"));
		}
		else if (layoutName == "0+5+0")
			hull = HULL_0_5_0;
		else if (layoutName == "2+5+0")
			hull = HULL_2_5_0;
		else if (layoutName == "0+4+0")
			hull = HULL_0_4_0;
		else if (layoutName == "0+7+0")
			hull = HULL_0_7_0;
		unsigned int nOutputCh = (unsigned int)m_outputLayout.channels.size();

		// Get the positions of all of the loudspeakers
		std::vector<PolarPosition> positions;
		for (int i = 0; i < m_outputLayout.channels.size(); ++i)
		{
			m_downmixMapping.push_back(i); // one-to-one downmix mapping
			positions.push_back(m_outputLayout.channels[i].polarPosition);
		}

		// get the extra speakers
		m_extraSpeakersLayout = CalculateExtraSpeakersLayout(m_outputLayout);
		// Get the indices of the virtual speakers at the top and bottom
		// Just check the last 2. Some layouts may not have a virtual speaker at
		// the top so do not assume that the last 2 entries are definitely virtual
		// speakers
		std::vector<unsigned int> virtualSpkInd;
		unsigned int nExtraSpk = (unsigned int)m_extraSpeakersLayout.channels.size();
		if (m_extraSpeakersLayout.channels[nExtraSpk - 2].name == "TOP" ||
			m_extraSpeakersLayout.channels[nExtraSpk - 2].name == "BOTTOM")
			virtualSpkInd.push_back(nOutputCh + nExtraSpk - 2);
		if (m_extraSpeakersLayout.channels[nExtraSpk - 1].name == "TOP" ||
			m_extraSpeakersLayout.channels[nExtraSpk - 1].name == "BOTTOM")
			virtualSpkInd.push_back(nOutputCh + nExtraSpk - 1);

		// Add the extra speakers to the list of positions
		for (int i = 0; i < m_extraSpeakersLayout.channels.size(); ++i)
			positions.push_back(m_extraSpeakersLayout.channels[i].polarPosition);

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
					m_regions.quadRegions.push_back(QuadRegion(hull[iFacet], facetPositions));
				}
				else if (nVertices == 3)
				{
					std::vector<PolarPosition> facetPositions;
					for (int i = 0; i < 3; ++i)
						facetPositions.push_back(positions[hull[iFacet][i]]);
					m_regions.triplets.push_back(Triplet(hull[iFacet], facetPositions));
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
			m_regions.virtualNgons.push_back(VirtualNgon(ngonInds, ngonPositions, positions[virtualSpkInd[iVirt]]));
		}

	}

	CAdmPointSourcePannerGainCalc::~CAdmPointSourcePannerGainCalc()
	{
	}

	std::vector<double> CAdmPointSourcePannerGainCalc::CalculateGains(PolarPosition direction)
	{
		std::vector<double> gains = _CalculateGains(direction);
		if (m_isStereo) // then downmix from 0+5+0 to 0+2+0
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

	std::vector<double> CAdmPointSourcePannerGainCalc::_CalculateGains(PolarPosition direction)
	{
		double tol = 1e-6;
		std::vector<double> gains(m_outputLayout.channels.size(), 0.);

		// get the unit vector in the target direction
		std::vector<double> directionUnitVec(3, 0.);
		direction.distance = 1.;
		CartesianPosition cartesianDirection = PolarToCartesian(direction);
		directionUnitVec = { cartesianDirection.x, cartesianDirection.y,cartesianDirection.z };

		// Loop through all of the regions until one is found that is not zero gain
		for (int iNgon = 0; iNgon < m_regions.virtualNgons.size(); ++iNgon)
		{
			std::vector<double> nGonGains = m_regions.virtualNgons[iNgon].CalculateGains(directionUnitVec);
			if (norm(nGonGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> nGonInds = m_regions.virtualNgons[iNgon].m_channelInds;
				for (int iGain = 0; iGain < nGonGains.size(); ++iGain)
					gains[m_downmixMapping[nGonInds[iGain]]] += nGonGains[iGain];

				return gains;
			}
		}
		// Loop through the triplets Ngons
		for (int iTriplet = 0; iTriplet < m_regions.triplets.size(); ++iTriplet)
		{
			std::vector<double> tripletGains = m_regions.triplets[iTriplet].CalculateGains(directionUnitVec);
			if (norm(tripletGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> tripletInds = m_regions.triplets[iTriplet].m_channelInds;
				for (int iGain = 0; iGain < tripletGains.size(); ++iGain)
					gains[m_downmixMapping[tripletInds[iGain]]] += tripletGains[iGain];

				return gains;
			}
		}
		// Loop through the triplets Ngons
		for (int iQuad = 0; iQuad < m_regions.quadRegions.size(); ++iQuad)
		{
			std::vector<double> quadGains = m_regions.quadRegions[iQuad].CalculateGains(directionUnitVec);
			if (norm(quadGains) > tol) // if the gains are not zero then map them to the output gains
			{
				std::vector<unsigned int> quadInds = m_regions.quadRegions[iQuad].m_channelInds;
				for (int iGain = 0; iGain < quadGains.size(); ++iGain)
					gains[m_downmixMapping[quadInds[iGain]]] += quadGains[iGain];

				return gains;
			}
		}

		return gains;
	}

	Layout CAdmPointSourcePannerGainCalc::CalculateExtraSpeakersLayout(Layout layout)
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
				m_downmixMapping.push_back(iMid);
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
				m_downmixMapping.push_back(iMid);
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
				int layerPriority = GetLayerPriority(channelNames[iSpk], channelNames[iOutSpk]);
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

	int ZoneExclusionHandler::GetLayerPriority(std::string inputChannelName, std::string outputChannelName)
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
}
