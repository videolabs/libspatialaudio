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
		m_l2norm.reserve(m_nCh);
		m_closestSpeakersInd.reserve(m_nCh);
		m_equalDistanceSpeakers.reserve(m_nCh);
		m_tuple.resize(m_nCh, std::vector<double>(4, 0.));
		m_tupleSorted.resize(m_nCh, std::vector<double>(4, 0.));
	}

	ChannelLockHandler::~ChannelLockHandler()
	{
	}

	CartesianPosition ChannelLockHandler::handle(const Optional<ChannelLock>& channelLock, CartesianPosition position)
	{
		// If channelLock has not been set then just return the original value
		if (!channelLock.hasValue())
			return position;

		double maxDistance = channelLock->maxDistance.hasValue() ? channelLock->maxDistance.value() : std::numeric_limits<double>::max();
		double tol = 1e-10;

		// Calculate the l-2 norm between the normalised real speaker directions and the source position.
		m_l2norm.resize(0);
		m_closestSpeakersInd.resize(0);
		for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
		{
			PolarPosition speakerPosition = m_layout.channels[iCh].polarPosition;
			// Rec. ITU-R BS.2127-0 pg. 44 - "loudspeaker positions considered are the normalised real loudspeaker
			// positions in layout" so normalise the distance
			speakerPosition.distance = 1.;
			CartesianPosition speakerCartPosition = PolarToCartesian(speakerPosition);
			double differenceVector[3] = {speakerCartPosition.x - position.x, speakerCartPosition.y - position.y, speakerCartPosition.z - position.z};
			double differenceNorm = norm(differenceVector, 3);
			// If the speaker is within the range 
			if (differenceNorm < maxDistance)
			{
				m_closestSpeakersInd.push_back(iCh);
				m_l2norm.push_back(differenceNorm);
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
			// Find the minimum l-2 norm from the speakers within range
			double minL2Norm = *std::min_element(m_l2norm.begin(), m_l2norm.end());
			// Find all the speakers that are within tolerance of this minimum value
			m_equalDistanceSpeakers.resize(0);
			for (unsigned int iNorm = 0; iNorm < m_l2norm.size(); ++iNorm)
				if (m_l2norm[iNorm] > minL2Norm - tol && m_l2norm[iNorm] < minL2Norm + tol)
				{
					m_equalDistanceSpeakers.push_back(m_closestSpeakersInd[iNorm]);
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
		int maxTupleSize = 0;
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
	}

	ZoneExclusionHandler::~ZoneExclusionHandler()
	{
	}

	int ZoneExclusionHandler::GetLayerPriority(const std::string& inputChannelName, const std::string& outputChannelName)
	{
		std::map<char, int> layerIndex = { {'B',0},{'M',1},{'U',2},{'T',3} };
		int inIndex = layerIndex[inputChannelName[0]];
		int outIndex = layerIndex[outputChannelName[0]];

		int layerPriority[4][4] = { {0,1,2,3},{3,0,1,2},{3,2,0,1},{3,2,1,0} };

		return layerPriority[inIndex][outIndex];
	}

	void ZoneExclusionHandler::handle(const std::vector<PolarExclusionZone>& exclusionZones, std::vector<double>& gainInOut)
	{
		double tol = 1e-6;

		assert(gainInOut.size() == m_nCh);

		// Find the set of excluded speakers
		for (unsigned int i = 0; i < m_nCh; ++i)
			m_isExcluded[i] = false;
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
					m_isExcluded[iSpk] = true;
					nCountExcluded++;
				}
			}
		}

		// Clear the downmix matrix
		for (int i = 0; i < (int)m_nCh; ++i)
			for (int j = 0; j < (int)m_nCh; ++j)
				m_D[i][j] = 0.;

		if (nCountExcluded == (int)m_nCh || nCountExcluded == 0)
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
		, m_gains(m_nCh, 0.)
	{
		// There can be up to 3 diverged positions/gains
		m_divergedPos.reserve(3);
		m_divergedGains.reserve(3);
		m_gains_for_each_pos.resize(3);
		for (int i = 0; i < 3; ++i)
			m_gains_for_each_pos[i].resize(m_nCh);
	}

	CGainCalculator::~CGainCalculator()
	{
	}

	void CGainCalculator::CalculateGains(const ObjectMetadata& metadata, std::vector<double>& directGains, std::vector<double>& diffuseGains)
	{
		assert(directGains.size() == m_nCh && diffuseGains.size() == m_nCh); // Gain vectors must already be of the expected size

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
		divergedPositionsAndGains(metadata.objectDivergence, position, m_divergedPos, m_divergedGains);
		auto& diverged_positions = m_divergedPos;
		auto& diverged_gains = m_divergedGains;
		unsigned int nDivergedGains = (unsigned int)diverged_gains.size();

		if (m_outputLayout.isHoa)
		{
			// Calculate the new gains to be applied
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				m_ambiExtentPanner.handle(diverged_positions[iGain], metadata.width, metadata.height, metadata.depth, m_gains_for_each_pos[iGain]);

			for (unsigned int i = 0; i < m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * m_gains_for_each_pos[j][i];
				m_gains[i] = g_tmp;
			}
		}
		else
		{	
			// Calculate the new gains to be applied
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				m_extentPanner.handle(diverged_positions[iGain], metadata.width, metadata.height, metadata.depth, m_gains_for_each_pos[iGain]);

			// Power summation of the gains when playback is to loudspeakers,
			for (unsigned int i = 0; i < m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * m_gains_for_each_pos[j][i] * m_gains_for_each_pos[j][i];
				m_gains[i] = sqrt(g_tmp);
			}

			// Zone exclusion downmix
			// See Rec. ITU-R BS.2127-0 sec. 7.3.12, pg 60
			m_zoneExclusionHandler.handle(metadata.zoneExclusionPolar, m_gains);
		}

		// Apply the overall gain to the spatialisation gains
		for (auto& g : m_gains)
			g *= metadata.gain;

		// Calculate the direct and diffuse gains
		// See Rec. ITU-R BS.2127-0 sec.7.3.1 page 39
		double directCoefficient = std::sqrt(1. - metadata.diffuse);
		double diffuseCoefficient = std::sqrt(metadata.diffuse);

		directGains = m_gains;
		diffuseGains = m_gains;
		for (auto& g : directGains)
			g *= directCoefficient;
		for (auto& g : diffuseGains)
			g *= diffuseCoefficient;
	}

	void CGainCalculator::divergedPositionsAndGains(const admrender::Optional<admrender::ObjectDivergence>& objectDivergence, CartesianPosition position, std::vector<CartesianPosition>& divergedPos, std::vector<double>& divergedGains)
	{
		assert(divergedPos.capacity() == 3 && divergedGains.capacity() == 3); // Must be able to hold up to 3 positions/gains

		PolarPosition polarDirection = CartesianToPolar(position);

		double x = 0.;
		if (objectDivergence.hasValue())
			x = objectDivergence->value;
		double d = polarDirection.distance;
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

		double cartPositions[3][3];
		cartPositions[0][0] = d;
		cartPositions[0][1] = 0.;
		cartPositions[0][2] = 0.;
		auto cartesianTmp = PolarToCartesian(PolarPosition{ x * objectDivergence->azimuthRange,0.,d });
		cartPositions[1][0] = cartesianTmp.y;
		cartPositions[1][1] = -cartesianTmp.x;
		cartPositions[1][2] = cartesianTmp.z;
		cartesianTmp = PolarToCartesian(PolarPosition{ -x * objectDivergence->azimuthRange,0.,d });
		cartPositions[2][0] = cartesianTmp.y;
		cartPositions[2][1] = -cartesianTmp.x;
		cartPositions[2][2] = cartesianTmp.z;

		// Rotate them so that the centre position is in specified input direction
		double rotMat[9] = { 0. };
		getRotationMatrix(polarDirection.azimuth, -polarDirection.elevation, 0., &rotMat[0]);
		divergedPos.resize(3);
		for (int iDiverge = 0; iDiverge < 3; ++iDiverge)
		{
			double directionRotated[3] = { 0. };
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					directionRotated[i] += rotMat[3 * i + j] * cartPositions[iDiverge][j];
			divergedPos[iDiverge] = CartesianPosition{ -directionRotated[1],directionRotated[0],directionRotated[2] };
		}
	}

}//namespace admrenderer
