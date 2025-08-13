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

#include <limits>

namespace admrender {

	ChannelLockHandler::ChannelLockHandler(const Layout& layout)
	{
		m_layout = layout;
		m_nCh = (unsigned int)layout.channels.size();
		m_distance.reserve(m_nCh);
		m_closestSpeakersInd.reserve(m_nCh);
		m_equalDistanceSpeakers.reserve(m_nCh);
		m_tuple.resize(m_nCh, std::vector<double>(4, 0.));
		m_tupleSorted.resize(m_nCh, std::vector<double>(4, 0.));
	}

	ChannelLockHandler::~ChannelLockHandler()
	{
	}

	CartesianPosition ChannelLockHandler::handle(const Optional<ChannelLock>& channelLock, CartesianPosition position, const std::vector<bool>& exlcuded)
	{
		// If channelLock has not been set then just return the original value
		if (!channelLock.hasValue())
			return position;

		double maxDistance = channelLock->maxDistance.hasValue() ? channelLock->maxDistance.value() : std::numeric_limits<double>::max();
		double tol = 1e-10;

		// Calculate the distance between the normalised real speaker directions and the source position.
		m_distance.resize(0);
		m_closestSpeakersInd.resize(0);
		for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
		{
			bool isExcluded = exlcuded.size() == 0 ? false : exlcuded[iCh];
			if (!isExcluded)
			{
				auto dist = calculateDistance(position, m_spkPos[iCh]);
				// If the speaker is within the range
				if (dist < maxDistance)
				{
					m_closestSpeakersInd.push_back(iCh);
					m_distance.push_back(dist);
				}
			}
		}
		unsigned int nSpeakersInRange = (unsigned int)m_closestSpeakersInd.size();
		// If no speakers are in range then return the original direction
		if (nSpeakersInRange == 0)
			return position;
		else if (nSpeakersInRange == 1) // if there is a unique speaker in range then return that direction
			return PolarToCartesian(m_layout.channels[m_closestSpeakersInd[0]].polarPosition);
		else if (nSpeakersInRange > 1)
		{
			// Find the minimum distance from the speakers within range
			double minDist = *std::min_element(m_distance.begin(), m_distance.end());
			// Find all the speakers that are within tolerance of this minimum value
			m_equalDistanceSpeakers.resize(0);
			for (unsigned int iDist = 0; iDist < m_distance.size(); ++iDist)
				if (m_distance[iDist] > minDist - tol && m_distance[iDist] < minDist + tol)
				{
					m_equalDistanceSpeakers.push_back(m_closestSpeakersInd[iDist]);
				}
			// If only one of the speakers in range is within tol of the minimum then return that direction
			if (m_equalDistanceSpeakers.size() == 1)
				return PolarToCartesian(m_layout.channels[m_equalDistanceSpeakers[0]].polarPosition);
			else // if not, find the closest by lexicographic comparison of the tuple {|az|,az,|el|,el}
			{
				m_activeTuples = 0;
				for (auto& t : m_equalDistanceSpeakers)
				{
					double az = m_layout.channels[t].polarPosition.azimuth;
					double el = m_layout.channels[t].polarPosition.elevation;
					m_tuple[m_activeTuples][0] = std::abs(az);
					m_tuple[m_activeTuples][1] = az;
					m_tuple[m_activeTuples][2] = std::abs(el);
					m_tuple[m_activeTuples][3] = el;
					m_tupleSorted[m_activeTuples] = m_tuple[m_activeTuples];
					m_activeTuples += 1;
				}
				sort(m_tupleSorted.begin(), m_tupleSorted.begin() + m_activeTuples);
				for (int iTuple = 0; iTuple < m_activeTuples; ++iTuple)
					if (m_tuple[iTuple] == m_tupleSorted[0])
						return PolarToCartesian(m_layout.channels[m_equalDistanceSpeakers[iTuple]].polarPosition);
			}
		}

		// Final fallback, return original position
		return position;
	}

	//===================================================================================================================================
	PolarChannelLockHandler::PolarChannelLockHandler(const Layout& layout) : ChannelLockHandler(layout)
	{
		for (const auto& ch : layout.channels)
		{
			auto polPos = ch.polarPosition;
			// Rec. ITU-R BS.2127-0 pg. 44 - "loudspeaker positions considered are the normalised real loudspeaker
			// positions in layout" so normalise the distance
			polPos.distance = 1.;
			m_spkPos.push_back(PolarToCartesian(polPos));
		}
	}

	PolarChannelLockHandler::~PolarChannelLockHandler()
	{
	}

	double PolarChannelLockHandler::calculateDistance(const CartesianPosition& srcPos, const CartesianPosition& spkPos)
	{
		auto deltaPos = spkPos - srcPos;
		return norm(deltaPos);
	}

	//===================================================================================================================================
	AlloChannelLockHandler::AlloChannelLockHandler(const Layout& layout) : ChannelLockHandler(layout)
	{
		m_spkPos = positionsForLayout(layout);
	}

	AlloChannelLockHandler::~AlloChannelLockHandler()
	{
	}

	double AlloChannelLockHandler::calculateDistance(const CartesianPosition& srcPos, const CartesianPosition& spkPos)
	{
		auto deltaPos = spkPos - srcPos;
		double wx = 1. / 16.;
		double wy = 4.;
		double wz = 32.;
		return std::sqrt(wx * deltaPos.x * deltaPos.x + wy * deltaPos.y * deltaPos.y + wz * deltaPos.z * deltaPos.z);
	}

	//===================================================================================================================================
	ZoneExclusionHandler::ZoneExclusionHandler(const Layout& layout)
	{
		m_layout = getLayoutWithoutLFE(layout);
		m_nCh = (unsigned int)m_layout.channels.size();

		// Get the cartesian coordinates of all nominal positions
		for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
		{
			m_cartesianPositions.push_back(PolarToCartesian(m_layout.channels[iSpk].polarPositionNominal));
		}

		// Determine the speaker groups. See Rec. ITU-R BS.2127-0 sec. 7.3.12.2.1 pg. 62
		int maxTupleSize = 0;
		for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
		{
			std::vector<std::vector<double>> tuples;
			CartesianPosition cartIn = m_cartesianPositions[iSpk];
			std::vector<std::string> channelNames = m_layout.channelNames();
			for (unsigned int iOutSpk = 0; iOutSpk < m_nCh; ++iOutSpk)
			{
				CartesianPosition cartOut = m_cartesianPositions[iOutSpk];
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
					{
						tupleOrder[i].insert(iTuple);
						maxTupleSize = std::max(maxTupleSize, (int)tupleOrder[i].size());
					}

			std::vector<std::set<unsigned int>>::iterator ip;

			ip = std::unique(tupleOrder.begin(), tupleOrder.end());
			// Resizing the vector so as to remove the undefined terms
			tupleOrder.resize(std::distance(tupleOrder.begin(), ip));

			m_downmixMapping.push_back(tupleOrder);
		}

		m_D.resize(m_nCh);
		for (unsigned int i = 0; i < m_nCh; ++i)
			m_D[i].resize(m_nCh, 0.);
		m_isExcluded.resize(m_nCh, false);
		m_gainsTmp.resize(m_nCh);
		m_setElements.resize(maxTupleSize);

		// Group the speakers in rows for the cartesian exclusion zones
		auto cartPositions = positionsForLayout(m_layout);

		//  Mark the speaker as already processed
		std::vector<bool> processed(cartPositions.size(), false);

		for (unsigned i = 0; i < (unsigned)cartPositions.size(); ++i) {
			if (processed[i]) continue;

			std::vector<unsigned int> curRow;
			auto curPoint = cartPositions[i];
			curRow.push_back(i);
			processed[i] = true;

			// Find all points with the same y and z that are in the same row
			for (unsigned j = i + 1; j < (unsigned)cartPositions.size(); ++j) {
				if (!processed[j] && cartPositions[j].y == curPoint.y && cartPositions[j].z == curPoint.z)
				{
					curRow.push_back(j);
					processed[j] = true;
				}
			}

			// Add the current row to rows
			m_rowInds.push_back(curRow);
		}

	}

	ZoneExclusionHandler::~ZoneExclusionHandler()
	{
	}

	void ZoneExclusionHandler::getCartesianExcluded(const std::vector<ExclusionZone>& exclusionZones, std::vector<bool>& excluded)
	{
		getExcluded(exclusionZones, excluded);
		auto nExcluded = getNumExcluded(excluded);

		// Remove rows reduced to a single speaker.
		for (size_t iRow = 0; iRow < m_rowInds.size(); ++iRow)
		{
			int exclCount = 0;
			for (auto& i : m_rowInds[iRow])
				if (excluded[i])
					exclCount++;
			if (exclCount == m_rowInds[iRow].size() - 1)
				for (auto& i : m_rowInds[iRow])
					excluded[i] = true;
		}

		if (nExcluded == excluded.size())
		{
			// "If the process of applying zone exclusion would result in all loudspeakers being removed, then no
			// loudspeakers are removed."
			for (size_t i = 0; i < excluded.size(); ++i)
				excluded[i] = false;
		}
	}

	void ZoneExclusionHandler::getExcluded(const std::vector<ExclusionZone>& exclusionZones, std::vector<bool>& excluded)
	{
		double tol = 1e-6;

		assert(excluded.size() == m_nCh);

		// Find the set of excluded speakers
		for (unsigned int i = 0; i < m_nCh; ++i)
			m_isExcluded[i] = false;
		for (auto& zone : exclusionZones)
		{
			assert(exclusionZones[0].isPolarZone() == zone.isPolarZone()); // All zones should be of the same type!
			if (zone.isPolarZone())
			{
				auto& polarZone = zone.polarZone();
				for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
				{
					double az = m_layout.channels[iSpk].polarPositionNominal.azimuth;
					double el = m_layout.channels[iSpk].polarPositionNominal.elevation;
					if ((polarZone.minElevation - tol < el && el < polarZone.maxElevation + tol) && (el > 90 - tol || insideAngleRange(az, polarZone.minAzimuth, polarZone.maxAzimuth)))
					{
						m_isExcluded[iSpk] = true;
					}
				}
			}
			else
			{
				auto& cartesianZone = zone.cartesianZone();
				for (unsigned int iSpk = 0; iSpk < m_nCh; ++iSpk)
				{
					auto x = m_cartesianPositions[iSpk].x;
					auto y = m_cartesianPositions[iSpk].y;
					auto z = m_cartesianPositions[iSpk].z;
					if (x > cartesianZone.minX - tol && x < cartesianZone.maxX + tol
						&& y > cartesianZone.minY - tol && y < cartesianZone.maxY + tol
						&& z > cartesianZone.minZ - tol && z < cartesianZone.maxZ + tol)
					{
						m_isExcluded[iSpk] = true;
					}
				}
			}
		}
	}

	int ZoneExclusionHandler::getNumExcluded(const std::vector<bool>& exlcuded)
	{
		int nExcluded = 0;
		for (size_t i = 0; i < exlcuded.size(); ++i)
			if (exlcuded[i])
				nExcluded++;
		return nExcluded;
	}

	int ZoneExclusionHandler::GetLayerPriority(const std::string& inputChannelName, const std::string& outputChannelName)
	{
		std::map<char, int> layerIndex = { {'B',0},{'M',1},{'U',2},{'T',3} };
		int inIndex = layerIndex[inputChannelName[0]];
		int outIndex = layerIndex[outputChannelName[0]];

		int layerPriority[4][4] = { {0,1,2,3},{3,0,1,2},{3,2,0,1},{3,2,1,0} };

		return layerPriority[inIndex][outIndex];
	}

	void ZoneExclusionHandler::handle(const std::vector<ExclusionZone>& exclusionZones, std::vector<double>& gainInOut)
	{
		assert(gainInOut.size() == m_nCh);

		getExcluded(exclusionZones, m_isExcluded);
		auto nExcluded = getNumExcluded(m_isExcluded);

		// Clear the downmix matrix
		for (int i = 0; i < (int)m_nCh; ++i)
			for (int j = 0; j < (int)m_nCh; ++j)
				m_D[i][j] = 0.;

		if (nExcluded == (int)m_nCh || nExcluded == 0)
		{
			return; // No change to the gain vector
		}
		else
		{
			// Go through all the speakers and find the first set that contains non-exlcuded speakers
			for (int iSpk = 0; iSpk < (int)m_nCh; ++iSpk)
			{
				// Find the first set with non-excluded speakers
				for (size_t iSet = 0; iSet < m_downmixMapping[iSpk].size(); ++iSet)
				{
					m_notExcludedElements.resize(0);
					m_setElements.resize(m_downmixMapping[iSet][iSpk].size());
					int i = 0;
					for (auto it = m_downmixMapping[iSet][iSpk].begin(); it != m_downmixMapping[iSet][iSpk].end(); ++it)
						m_setElements[i++] = *it;
					for (size_t iEl = 0; iEl < m_setElements.size(); ++iEl)
						if (!m_isExcluded[m_setElements[iEl]])
							m_notExcludedElements.push_back(m_setElements[iEl]);
					int numNotExcluded = (int)m_notExcludedElements.size();
					if (numNotExcluded > 0)
					{
						// Fill the downmix matrix D
						for (int iEl = 0; iEl < numNotExcluded; ++iEl)
							m_D[m_notExcludedElements[iEl]][iSpk] = 1. / (double)numNotExcluded;
						break;
					}
				}
			}
			// Calculate the downmixed output gain vector
			for (int i = 0; i < (int)m_nCh; ++i)
				m_gainsTmp[i] = gainInOut[i];
			for (int i = 0; i < (int)m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < m_nCh; ++j)
					g_tmp += m_D[i][j] * m_gainsTmp[j] * m_gainsTmp[j];
				gainInOut[i] = sqrt(g_tmp);
			}
		}
	}

	//===================================================================================================================================
	CGainCalculator::CGainCalculator(Layout outputLayout)
		: m_outputLayout(outputLayout)
		, m_nCh((unsigned int)m_outputLayout.channels.size())
		, m_nChNoLFE((unsigned int)getLayoutWithoutLFE(outputLayout).channels.size())
		, m_cartPositions(positionsForLayout(outputLayout))
		, m_pspGainCalculator(getLayoutWithoutLFE(outputLayout))
		, m_extentPanner(m_pspGainCalculator)
		, m_ambiExtentPanner(outputLayout.hoaOrder)
		, m_alloGainCalculator(getLayoutWithoutLFE(outputLayout))
		, m_alloExtentPanner(getLayoutWithoutLFE(outputLayout))
		, m_screenScale(outputLayout.reproductionScreen, getLayoutWithoutLFE(outputLayout))
		, m_screenEdgeLock(outputLayout.reproductionScreen, getLayoutWithoutLFE(outputLayout))
		, m_polarChannelLockHandler(getLayoutWithoutLFE(outputLayout))
		, m_alloChannelLockHandler(getLayoutWithoutLFE(outputLayout))
		, m_zoneExclusionHandler(getLayoutWithoutLFE(outputLayout))
		, m_gains(m_nChNoLFE, 0.)
	{
		// There can be up to 3 diverged positions/gains
		m_divergedPos.reserve(3);
		m_divergedGains.reserve(3);
		m_gainsForEachPos.resize(3);
		for (int i = 0; i < 3; ++i)
			m_gainsForEachPos[i].resize(m_nChNoLFE);
		m_excluded.resize(m_nChNoLFE);

		m_cartesianLayout = m_cartPositions.size() > 0;
	}

	CGainCalculator::~CGainCalculator()
	{
	}

	void CGainCalculator::CalculateGains(const ObjectMetadata& metadata, std::vector<double>& directGains, std::vector<double>& diffuseGains)
	{
		assert(directGains.size() == m_nCh && diffuseGains.size() == m_nCh); // Gain vectors must already be of the expected size

		if ((metadata.cartesian && !m_cartesianLayout) || m_outputLayout.isHoa)
			toPolar(metadata, m_objMetadata);
		else
			m_objMetadata = metadata;

		CartesianPosition position;
		bool cartesian = metadata.cartesian;

		if (m_objMetadata.cartesian && !m_objMetadata.position.isPolar())
			position = { clamp(m_objMetadata.position.cartesianPosition().x,-1.,1.),clamp(m_objMetadata.position.cartesianPosition().y,-1.,1.),
			clamp(m_objMetadata.position.cartesianPosition().z,-1.,1.)};
		else
			position = PolarToCartesian(m_objMetadata.position.polarPosition());

		// Apply screen scaling
		position = m_screenScale.handle(position, m_objMetadata.screenRef, m_objMetadata.referenceScreen, m_objMetadata.cartesian);
		// Apply screen edge lock
		position = m_screenEdgeLock.HandleVector(position, m_objMetadata.screenEdgeLock, m_objMetadata.cartesian);

		if (cartesian)
		{
			m_zoneExclusionHandler.getCartesianExcluded(m_objMetadata.zoneExclusion, m_excluded);
			// Apply channelLock to modify the position of the source, if required
			position = m_alloChannelLockHandler.handle(m_objMetadata.channelLock, position, m_excluded);
		}
		else
		{
			m_excluded.resize(0);
			// Apply channelLock to modify the position of the source, if required
			position = m_polarChannelLockHandler.handle(m_objMetadata.channelLock, position, m_excluded);
		}

		// Apply divergence
		divergedPositionsAndGains(m_objMetadata.objectDivergence, position, m_objMetadata.cartesian, m_divergedPos, m_divergedGains);
		auto& diverged_positions = m_divergedPos;
		auto& diverged_gains = m_divergedGains;
		unsigned int nDivergedGains = (unsigned int)diverged_gains.size();

		if (m_outputLayout.isHoa)
		{
			// Calculate the new gains to be applied
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				m_ambiExtentPanner.handle(diverged_positions[iGain], m_objMetadata.width, m_objMetadata.height, m_objMetadata.depth, m_gainsForEachPos[iGain]);

			for (unsigned int i = 0; i < m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * m_gainsForEachPos[j][i];
				m_gains[i] = g_tmp;
			}
		}
		else
		{
			if (cartesian)
			{
				// Calculate the new gains to be m_alloExtentPanner
				for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
					if (m_objMetadata.width == 0. && m_objMetadata.height == 0. && m_objMetadata.depth == 0)
						m_alloGainCalculator.CalculateGains(diverged_positions[iGain], m_excluded, m_gainsForEachPos[iGain]);
					else
						m_alloExtentPanner.handle(diverged_positions[iGain], m_objMetadata.width, m_objMetadata.height, m_objMetadata.depth, m_excluded, m_gainsForEachPos[iGain]);
			}
			else
			{
				// Calculate the new gains to be applied
				for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
					m_extentPanner.handle(diverged_positions[iGain], m_objMetadata.width, m_objMetadata.height, m_objMetadata.depth, m_gainsForEachPos[iGain]);
			}

			// Power summation of the gains when playback is to loudspeakers,
			for (unsigned int i = 0; i < m_nChNoLFE; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * m_gainsForEachPos[j][i] * m_gainsForEachPos[j][i];
				m_gains[i] = sqrt(g_tmp);
			}

			// Zone exclusion downmix
			// See Rec. ITU-R BS.2127-0 sec. 7.3.12, pg 60
			if (!cartesian)
				m_zoneExclusionHandler.handle(m_objMetadata.zoneExclusion, m_gains);
		}

		// Apply the overall gain to the spatialisation gains
		for (auto& g : m_gains)
			g *= m_objMetadata.gain;

		// "gains is extended by adding LFE channel gains with value 0 to produce gains_full"
		insertLFE(m_outputLayout, m_gains, directGains);

		// Calculate the direct and diffuse gains
		// See Rec. ITU-R BS.2127-0 sec.7.3.1 page 39
		double directCoefficient = std::sqrt(1. - m_objMetadata.diffuse);
		double diffuseCoefficient = std::sqrt(m_objMetadata.diffuse);

		diffuseGains = directGains;
		for (auto& g : directGains)
			g *= directCoefficient;
		for (auto& g : diffuseGains)
			g *= diffuseCoefficient;
	}

	void CGainCalculator::divergedPositionsAndGains(const admrender::Optional<admrender::ObjectDivergence>& objectDivergence, CartesianPosition position, bool cartesian, std::vector<CartesianPosition>& divergedPos, std::vector<double>& divergedGains)
	{
		assert(divergedPos.capacity() == 3 && divergedGains.capacity() == 3); // Must be able to hold up to 3 positions/gains

		double x = 0.;
		if (objectDivergence.hasValue())
			x = objectDivergence->value;
		// if the divergence value is zero or isn't set then return the original direction and a gain of 1
		if (x == 0. || !objectDivergence.hasValue())
		{
			divergedPos.resize(1);
			divergedGains.resize(1);
			divergedPos[0] = position;
			divergedGains[0] = 1.;
			return;
		}

		// If there is any divergence then calculate the gains and directions
		// Calculate gains using Rec. ITU-R BS.2127-0 sec. 7.3.7.1
		assert(divergedGains.capacity() >= 3);
		divergedGains.resize(3, 0.);
		divergedGains[0] = (1. - x) / (x + 1.);
		double glr = x / (x + 1.);
		divergedGains[1] = glr;
		divergedGains[2] = glr;

		divergedPos.resize(3);

		if (cartesian)
		{
			assert(!objectDivergence->azimuthRange.hasValue()); // Azimuth range is set for cartesian processing!

			double positionRange = objectDivergence->positionRange.hasValue() ? objectDivergence->positionRange.value() : 0.;

			divergedPos[0] = { clamp(position.x,-1.,1.), clamp(position.y,-1.,1.), clamp(position.z,-1.,1.) };
			divergedPos[1] = { clamp(position.x + positionRange,-1.,1.), clamp(position.y,-1.,1.), clamp(position.z,-1.,1.) };
			divergedPos[2] = { clamp(position.x - positionRange,-1.,1.), clamp(position.y,-1.,1.), clamp(position.z,-1.,1.) };
		}
		else
		{
			assert(!objectDivergence->positionRange.hasValue()); // Position range is set for polar processing!

			PolarPosition polarDirection = CartesianToPolar(position);
			double d = polarDirection.distance;

			auto azimuthRange = objectDivergence->azimuthRange.hasValue() ? objectDivergence->azimuthRange.value() : 0.;

			double cartPositions[3][3];
			cartPositions[0][0] = d;
			cartPositions[0][1] = 0.;
			cartPositions[0][2] = 0.;
			auto cartesianTmp = PolarToCartesian(PolarPosition{ x * azimuthRange,0.,d});
			cartPositions[1][0] = cartesianTmp.y;
			cartPositions[1][1] = -cartesianTmp.x;
			cartPositions[1][2] = cartesianTmp.z;
			cartesianTmp = PolarToCartesian(PolarPosition{ -x * azimuthRange,0.,d});
			cartPositions[2][0] = cartesianTmp.y;
			cartPositions[2][1] = -cartesianTmp.x;
			cartPositions[2][2] = cartesianTmp.z;

			// Rotate them so that the centre position is in specified input direction
			double rotMat[9] = { 0. };
			getRotationMatrix(polarDirection.azimuth, -polarDirection.elevation, 0., &rotMat[0]);
			for (int iDiverge = 0; iDiverge < 3; ++iDiverge)
			{
				double directionRotated[3] = { 0. };
				for (int i = 0; i < 3; ++i)
					for (int j = 0; j < 3; ++j)
						directionRotated[i] += rotMat[3 * i + j] * cartPositions[iDiverge][j];
				divergedPos[iDiverge] = CartesianPosition{ -directionRotated[1],directionRotated[0],directionRotated[2] };
			}
		}
	}

	void CGainCalculator::insertLFE(const Layout& layout, const std::vector<double>& gainsNoLFE, std::vector<double>& gainsWithLFE)
	{
		assert(gainsWithLFE.capacity() >= layout.channels.size());
		gainsWithLFE.resize(layout.channels.size());

		if (!layout.hasLFE) // No LFE to insert so just copy the gain vector
		{
			gainsWithLFE = gainsNoLFE;
			return;
		}

		int iCount = 0;
		for (int i = 0; i < layout.channels.size(); ++i)
			if (!layout.channels[i].isLFE)
				gainsWithLFE[i] = gainsNoLFE[iCount++];
			else
				gainsWithLFE[i] = 0.;
	}

}//namespace admrenderer
