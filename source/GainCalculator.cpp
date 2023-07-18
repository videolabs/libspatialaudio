/*############################################################################*/
/*#                                                                          #*/
/*#  A gain calculator with ADM metadata with speaker or HOA output			 #*/
/*#                                                                          #*/
/*#  Filename:      GainCalculator.cpp	                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          28/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "GainCalculator.h"

namespace admrender {

	ChannelLockHandler::ChannelLockHandler(Layout layout)
	{
		m_layout = layout;
		m_nCh = (unsigned int)layout.channels.size();
	}

	ChannelLockHandler::~ChannelLockHandler()
	{
	}

	CartesianPosition ChannelLockHandler::handle(ChannelLock channelLock, CartesianPosition position)
	{
		// If distance is <0 then no channel locking is set so return the original direction
		if (channelLock.maxDistance < 0.)
			return position;

		double maxDistance = channelLock.maxDistance;
		double tol = 1e-10;

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
			std::vector<double> differenceVector = { speakerCartPosition.x - position.x, speakerCartPosition.y - position.y, speakerCartPosition.z - position.z };
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
			return position;
		else if (nSpeakersInRange == 1) // if there is a unique speaker in range then return that direction
			return PolarToCartesian(m_layout.channels[closestSpeakersInd[0]].polarPosition);
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
				return PolarToCartesian(m_layout.channels[equalDistanceSpeakers[0]].polarPosition);
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
						return PolarToCartesian(m_layout.channels[equalDistanceSpeakers[iTuple]].polarPosition);
			}
		}

		// Final fallback, return original position
		return position;
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
				for (size_t iSet = 0; iSet < m_downmixMapping[iSpk].size(); ++iSet)
				{
					std::vector<unsigned int> notExcludedElements;
					std::vector<unsigned int> setElements(m_downmixMapping[iSpk][iSet].begin(), m_downmixMapping[iSpk][iSet].end());
					for (size_t i = 0; i < setElements.size(); ++i)
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
	CGainCalculator::CGainCalculator(Layout outputLayoutNoLFE)
		: m_outputLayout(getLayoutWithoutLFE(outputLayoutNoLFE))
		, m_nCh((unsigned int)m_outputLayout.channels.size())
		, m_pspGainCalculator(getLayoutWithoutLFE(outputLayoutNoLFE))
		, m_extentPanner(m_pspGainCalculator)
		, m_ambiExtentPanner(outputLayoutNoLFE.hoaOrder)
		, m_screenScale(outputLayoutNoLFE.reproductionScreen, getLayoutWithoutLFE(outputLayoutNoLFE))
		, m_screenEdgeLock(outputLayoutNoLFE.reproductionScreen, getLayoutWithoutLFE(outputLayoutNoLFE))
		, m_channelLockHandler(getLayoutWithoutLFE(outputLayoutNoLFE))
		, m_zoneExclusionHandler(getLayoutWithoutLFE(outputLayoutNoLFE))
	{
	}

	CGainCalculator::~CGainCalculator()
	{
	}

	void CGainCalculator::CalculateGains(ObjectMetadata metadata, std::vector<double>& directGains, std::vector<double>& diffuseGains)
	{
		std::vector<double> gains(m_nCh, 0.);

		if (metadata.cartesian)
			std::cerr << "Cartesian panning path is not implemented." << std::endl;

		CartesianPosition position = PolarToCartesian(metadata.polarPosition);

		// Apply screen scaling
		position = m_screenScale.handle(position, metadata.screenRef, metadata.referenceScreen, metadata.cartesian);
		// Apply screen edge lock
		position = m_screenEdgeLock.HandleVector(position, metadata.screenEdgeLock, metadata.cartesian);

		// Apply channelLock to modify the position of the source, if required
		position = m_channelLockHandler.handle(metadata.channelLock, position);

		// Apply divergence
		auto divergedData = divergedPositionsAndGains(metadata.objectDivergence.value, metadata.objectDivergence.azimuthRange, position);
		auto diverged_positions = divergedData.first;
		auto diverged_gains = divergedData.second;
		unsigned int nDivergedGains = (unsigned int)diverged_gains.size();

		if (m_outputLayout.isHoa)
		{
			// Calculate the new gains to be applied
			std::vector<std::vector<double>> gains_for_each_pos(nDivergedGains);
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				gains_for_each_pos[iGain] = m_ambiExtentPanner.handle(diverged_positions[iGain], metadata.width, metadata.height, metadata.depth);

			for (unsigned int i = 0; i < m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * gains_for_each_pos[j][i];
				gains[i] = g_tmp;
			}
		}
		else
		{	
			// Calculate the new gains to be applied
			std::vector<std::vector<double>> gains_for_each_pos(nDivergedGains);
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				gains_for_each_pos[iGain] = m_extentPanner.handle(diverged_positions[iGain], metadata.width, metadata.height, metadata.depth);

			// Power summation of the gains when playback is to loudspeakers,
			for (unsigned int i = 0; i < m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * gains_for_each_pos[j][i] * gains_for_each_pos[j][i];
				gains[i] = sqrt(g_tmp);
			}

			// Zone exclusion downmix
			// See Rec. ITU-R BS.2127-0 sec. 7.3.12, pg 60
			gains = m_zoneExclusionHandler.handle(metadata.zoneExclusionPolar, gains);
		}

		// Apply the overall gain to the spatialisation gains
		for (auto& g : gains)
			g *= metadata.gain;

		// Calculate the direct and diffuse gains
		// See Rec. ITU-R BS.2127-0 sec.7.3.1 page 39
		double directCoefficient = std::sqrt(1. - metadata.diffuse);
		double diffuseCoefficient = std::sqrt(metadata.diffuse);

		directGains = gains;
		diffuseGains = gains;
		for (auto& g : directGains)
			g *= directCoefficient;
		for (auto& g : diffuseGains)
			g *= diffuseCoefficient;
	}

}//namespace admrenderer
