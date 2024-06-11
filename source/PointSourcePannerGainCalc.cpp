/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gains required for point source panning.                  #*/
/*#  CPointSourcePannerGainCalc - ADM Point Source Panner                    #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      PointSourcePannerGainCalc.cpp                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "PointSourcePannerGainCalc.h"
#include<cmath>
#include<string>
#include <map>

CPointSourcePannerGainCalc::CPointSourcePannerGainCalc(const Layout& layout)
{
	// Store the output layout
	m_outputLayout = getLayoutWithoutLFE(layout);
	// Internal layout that is used for processing then (if requried) downmixing to the output
	m_internalLayout = m_outputLayout;
	std::string layoutName = m_internalLayout.name;

	// Check that the loudspeaker layout is supported
	assert(layoutName.compare("0+2+0") == 0 || layoutName.compare("0+4+0") == 0
		|| layoutName.compare("0+5+0") == 0 || layoutName.compare("2+5+0") == 0
		|| layoutName.compare("4+5+0") == 0 || layoutName.compare("4+5+1") == 0
		|| layoutName.compare("3+7+0") == 0 || layoutName.compare("4+9+0") == 0
		|| layoutName.compare("9+10+3") == 0 || layoutName.compare("0+7+0") == 0
		|| layoutName.compare("4+7+0") == 0 || layoutName.compare("2+7+0") == 0
		|| layoutName.compare("2+3+0") == 0 || layoutName.compare("9+10+5") == 0);

	std::vector<std::vector<unsigned int>> hull;
	if (layoutName == "0+2+0")
	{
		hull = HULL_0_5_0;
		m_downmixOutput = DownmixOutput::Downmix_0_2_0;
		m_internalLayout = getLayoutWithoutLFE(GetMatchingLayout("0+5+0"));
	}
	else if (layoutName == "0+4+0")
		hull = HULL_0_4_0;
	else if (layoutName == "0+5+0")
		hull = HULL_0_5_0;
	else if (layoutName == "2+5+0")
		hull = HULL_2_5_0;
	else if (layoutName == "4+5+0")
		hull = HULL_4_5_0;
	else if (layoutName == "4+5+1")
		hull = HULL_4_5_1;
	else if (layoutName == "3+7+0")
		hull = HULL_3_7_0;
	else if (layoutName == "4+9+0")
	{
		bool wideRight, wideLeft;
		bool validScreenSpk = CheckScreenSpeakerWidths(layout, wideLeft, wideRight);
		assert(validScreenSpk); // M+SC and/or M-SC are not correctly configured!
		// Select the correct convex hull based on the azimuth of the screen speakers.
		if (!wideLeft && !wideRight)
			hull = HULL_4_9_0;
		else if (wideLeft && !wideRight)
			hull = HULL_4_9_0_wideL;
		else if (!wideLeft && wideRight)
			hull = HULL_4_9_0_wideR;
		else if (wideLeft && wideRight)
			hull = HULL_4_9_0_wide;

		// Set nominal azimuth values depending on what azimuth range they fall in
		m_internalLayout.channels[11].polarPositionNominal.azimuth = wideLeft ? 15 : 45;
		m_internalLayout.channels[12].polarPositionNominal.azimuth = wideLeft ? 15 : 45;
	}
	else if (layoutName == "9+10+3")
		hull = HULL_9_10_3;
	else if (layoutName == "0+7+0")
		hull = HULL_0_7_0;
	else if (layoutName == "4+7+0")
		hull = HULL_4_7_0;
	else if (layoutName == "2+7+0")
		hull = HULL_2_7_0;
	else if (layoutName == "9+10+5")
		hull = HULL_9_10_5;
	else if (layoutName == "2+3+0")
	{
		hull = HULL_4_7_0;
		m_downmixOutput = DownmixOutput::Downmix_2_3_0;
		m_internalLayout = getLayoutWithoutLFE(GetMatchingLayout("4+7+0"));
	}
	else
		assert(false);
	unsigned int nOutputCh = (unsigned int)m_internalLayout.channels.size();

	// Allocate temp gains based on the size of the internal layout used (0+5+0 when outputting stereo)
	m_gainsTmp.resize(m_internalLayout.channels.size(), 0.);
	m_directionUnitVec.resize(3, 0.);

	// if the layout is HOA then don't go any further because this panner is intended for loudspeaker arrays
	if (layout.isHoa)
		return;

	// Get the positions of all of the loudspeakers
	std::vector<PolarPosition> positions;
	for (size_t i = 0; i < m_internalLayout.channels.size(); ++i)
	{
		m_downmixMapping.push_back(i); // one-to-one downmix mapping
		positions.push_back(m_internalLayout.channels[i].polarPosition);
	}

	// get the extra speakers
	m_extraSpeakersLayout = CalculateExtraSpeakersLayout(m_internalLayout);
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
	for (size_t i = 0; i < m_extraSpeakersLayout.channels.size(); ++i)
		positions.push_back(m_extraSpeakersLayout.channels[i].polarPosition);

	// Go through all the facets of the hull to create the required RegionHandlers
	unsigned int nFacets = (unsigned int)hull.size();
	for (size_t iFacet = 0; iFacet < nFacets; ++iFacet)
	{
		unsigned int nVertices = (unsigned int)hull[iFacet].size();
		// Check if the facet contains one of the virtual speakers
		bool hasVirtualSpeaker = false;
		for (unsigned int i = 0; i < nVertices; ++i)
			for (size_t iVirt = 0; iVirt < virtualSpkInd.size(); ++iVirt)
				if (hull[iFacet][i] == virtualSpkInd[iVirt])
					hasVirtualSpeaker = true;
		if (!hasVirtualSpeaker)
		{
			if (nVertices == 4)
			{
				std::vector<PolarPosition> facetPositions;
				for (size_t i = 0; i < 4; ++i)
					facetPositions.push_back(positions[hull[iFacet][i]]);
				m_regions.quadRegions.push_back(QuadRegion(hull[iFacet], facetPositions));
			}
			else if (nVertices == 3)
			{
				std::vector<PolarPosition> facetPositions;
				for (size_t i = 0; i < 3; ++i)
					facetPositions.push_back(positions[hull[iFacet][i]]);
				m_regions.triplets.push_back(Triplet(hull[iFacet], facetPositions));
			}
		}
	}
	// Loop through all facets to find those that contain a virtual speaker. If they do, add their
	// indices to a list and then create a virtualNgon for the corresponding set
	for (size_t iVirt = 0; iVirt < virtualSpkInd.size(); ++iVirt)
	{
		std::set<unsigned int> virtualNgonVertInds;
		for (size_t iFacet = 0; iFacet < nFacets; ++iFacet)
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
		for (size_t i = 0; i < ngonInds.size(); ++i)
		{
			ngonPositions.push_back(positions[ngonInds[i]]);
		}
		m_regions.virtualNgons.push_back(VirtualNgon(ngonInds, ngonPositions, positions[virtualSpkInd[iVirt]]));
	}

	for (size_t iNgon = 0; iNgon < m_regions.virtualNgons.size(); ++iNgon)
	{
		auto nVerts = m_regions.virtualNgons[iNgon].m_polarPositions.size();
		if (nVerts > m_nGonGains.size())
			m_nGonGains.resize(nVerts);
	}
	m_tripletGains.resize(3, 0.);
	m_quadGains.resize(4, 0.);
}

CPointSourcePannerGainCalc::~CPointSourcePannerGainCalc()
{
}

void CPointSourcePannerGainCalc::CalculateGains(PolarPosition direction, std::vector<double>& gains)
{
	return CalculateGains(PolarToCartesian(direction), gains);
}

void CPointSourcePannerGainCalc::CalculateGains(CartesianPosition position, std::vector<double>& gains)
{
	if (m_downmixOutput == DownmixOutput::Downmix_0_2_0) // then downmix from 0+5+0 to 0+2+0
	{
		assert(gains.size() == 2);
		CalculateGainsFromRegions(position, m_gainsTmp);

		gains[0] = 0.;
		gains[1] = 0.;
		// See Rec. ITU-R BS.2127-0 6.1.2.4 (page 2.5) for downmix method 
		double stereoDownmix[2][5] = { {1.,0.,1. / sqrt(3.),1. / sqrt(2.),0.}, {0.,1.,1. / sqrt(3.),0.,1. / sqrt(2.)} };
		for (int i = 0; i < 2; ++i)
			for (int j = 0; j < 5; ++j)
				gains[i] += stereoDownmix[i][j] * m_gainsTmp[j];
		double a_front;
		int i = 0;
		for (i = 0; i < 3; ++i)
			a_front = std::max(a_front, m_gainsTmp[i]);
		double a_rear;
		for (i = 3; i < 5; ++i)
			a_rear = std::max(a_rear, m_gainsTmp[i]);
		double r = a_rear / (a_front + a_rear);
		double gainNormalisation = std::pow(0.5, r / 2.) / norm(gains);

		gains[0] *= gainNormalisation;
		gains[1] *= gainNormalisation;
	}
	else if (m_downmixOutput == DownmixOutput::Downmix_2_3_0)
	{
		assert(gains.size() == 5);
		CalculateGainsFromRegions(position, m_gainsTmp);

		for (auto& g : gains)
			g = 0.;

		// See IAMF v1.0.0 sec. 7.6.2 for downmix matrix
		double p = std::sqrt(0.5);
		double gainNormalisation = 2. / (1. + 2. * p);
		double downmixMatrix[5][11] = { 0. };
		downmixMatrix[0][0] = 1.;
		downmixMatrix[0][3] = p;
		downmixMatrix[0][5] = p;
		downmixMatrix[1][1] = 1.;
		downmixMatrix[2][2] = 1.;
		downmixMatrix[2][4] = p;
		downmixMatrix[2][6] = p;
		downmixMatrix[3][7] = 1.;
		downmixMatrix[3][9] = p;
		downmixMatrix[4][8] = 1.;
		downmixMatrix[4][10] = p;

		for (int i = 0; i < 5; ++i)
			for (int j = 0; j < 11; ++j)
				if (downmixMatrix[i][j] != 0.)
					gains[i] += downmixMatrix[i][j] * m_gainsTmp[j];

		for (auto& g : gains)
			g *= gainNormalisation;
	}
	else
	{
		CalculateGainsFromRegions(position, gains);
	}
}

unsigned int CPointSourcePannerGainCalc::getNumChannels()
{
	return (unsigned int)m_outputLayout.channels.size();
}

void CPointSourcePannerGainCalc::CalculateGainsFromRegions(CartesianPosition position, std::vector<double>& gains)
{
	double tol = 1e-6;

	assert(gains.capacity() >= m_internalLayout.channels.size()); // Gains vector length must match the number of channels
	gains.resize(m_internalLayout.channels.size());
	for (auto& g : gains)
		g = 0.;

	// get the unit vector in the target direction
	double vecNorm = norm(position);
	m_directionUnitVec[0] = position.x / vecNorm;
	m_directionUnitVec[1] = position.y / vecNorm;
	m_directionUnitVec[2] = position.z / vecNorm;

	for (double& g : m_nGonGains)
		g = 0.;
	for (double& g : m_tripletGains)
		g = 0.;
	for (double& g : m_quadGains)
		g = 0.;

	// Loop through all of the regions until one is found that is not zero gain
	for (size_t iNgon = 0; iNgon < m_regions.virtualNgons.size(); ++iNgon)
	{
		m_regions.virtualNgons[iNgon].CalculateGains(m_directionUnitVec, m_nGonGains);
		if (norm(m_nGonGains) > tol) // if the gains are not zero then map them to the output gains
		{
			std::vector<unsigned int>& nGonInds = m_regions.virtualNgons[iNgon].m_channelInds;
			for (size_t iGain = 0; iGain < m_nGonGains.size(); ++iGain)
				gains[m_downmixMapping[nGonInds[iGain]]] += m_nGonGains[iGain];

			return;
		}
	}
	// Loop through the triplets Ngons
	for (size_t iTriplet = 0; iTriplet < m_regions.triplets.size(); ++iTriplet)
	{
		m_regions.triplets[iTriplet].CalculateGains(m_directionUnitVec, m_tripletGains);
		if (norm(m_tripletGains) > tol) // if the gains are not zero then map them to the output gains
		{
			std::vector<unsigned int>& tripletInds = m_regions.triplets[iTriplet].m_channelInds;
			for (size_t iGain = 0; iGain < m_tripletGains.size(); ++iGain)
				gains[m_downmixMapping[tripletInds[iGain]]] += m_tripletGains[iGain];

			return;
		}
	}
	// Loop through the triplets Ngons
	for (size_t iQuad = 0; iQuad < m_regions.quadRegions.size(); ++iQuad)
	{
		m_regions.quadRegions[iQuad].CalculateGains(m_directionUnitVec, m_quadGains);
		if (norm(m_quadGains) > tol) // if the gains are not zero then map them to the output gains
		{
			std::vector<unsigned int>& quadInds = m_regions.quadRegions[iQuad].m_channelInds;
			for (unsigned int iGain = 0; iGain < m_quadGains.size(); ++iGain)
				gains[m_downmixMapping[quadInds[iGain]]] += m_quadGains[iGain];

			return;
		}
	}
}

Layout CPointSourcePannerGainCalc::CalculateExtraSpeakersLayout(const Layout& layout)
{
	Layout extraSpeakers;
	unsigned int nSpeakers = (unsigned int)layout.channels.size();

	// Find if speakers are present in each layer
	std::vector<unsigned int> upperLayerSet;
	std::vector<unsigned int> midLayerSet;
	std::vector<unsigned int> lowerLayerSet;
	double maxUpperAz = 0.f;
	double maxLowerAz = 0.f;
	double meanUpperEl = 0.;
	double meanMidEl = 0.;
	double meanLowerEl = 0.;
	for (unsigned int iSpk = 0; iSpk < nSpeakers; ++iSpk)
	{
		double el = layout.channels[iSpk].polarPositionNominal.elevation;
		if (el >= 30 && el <= 70)
		{
			upperLayerSet.push_back(iSpk);
			// TODO: consider remapping azimuth to range -180 to 180
			maxUpperAz = std::max(maxUpperAz, abs(layout.channels[iSpk].polarPositionNominal.azimuth));
			meanUpperEl += layout.channels[iSpk].polarPosition.elevation;
		}
		else if (el >= -10 && el <= 10)
		{
			midLayerSet.push_back(iSpk);
			meanMidEl += layout.channels[iSpk].polarPosition.elevation;
		}
		else if (el >= -70 && el <= -30)
		{
			lowerLayerSet.push_back(iSpk);
			// TODO: consider remapping azimuth to range -180 to 180
			maxLowerAz = std::max(maxLowerAz, abs(layout.channels[iSpk].polarPositionNominal.azimuth));
			meanLowerEl += layout.channels[iSpk].polarPosition.elevation;
		}
	}
	meanUpperEl = upperLayerSet.size() > 0 ? meanUpperEl / (double)upperLayerSet.size() : 30.;
	meanMidEl = meanMidEl / (double)midLayerSet.size();
	meanLowerEl = lowerLayerSet.size() > 0 ? meanLowerEl / (double)lowerLayerSet.size() : -30.;

	PolarPosition position, positionNominal;
	for (size_t iMid = 0; iMid < midLayerSet.size(); ++iMid)
	{
		auto name = layout.channels[midLayerSet[iMid]].name;
		double azimuth = layout.channels[midLayerSet[iMid]].polarPosition.azimuth;
		// Lower layer
		if ((lowerLayerSet.size() > 0 && abs(azimuth) > maxLowerAz + 40.) || lowerLayerSet.size() == 0)
		{
			m_downmixMapping.push_back(iMid);
			name.at(0) = 'B';
			position.azimuth = azimuth;
			position.elevation = meanLowerEl;
			positionNominal.azimuth = layout.channels[midLayerSet[iMid]].polarPositionNominal.azimuth;
			positionNominal.elevation = -30.;
			extraSpeakers.channels.push_back({ name,position,positionNominal,false });
		}
	}
	for (size_t iMid = 0; iMid < midLayerSet.size(); ++iMid)
	{
		auto name = layout.channels[midLayerSet[iMid]].name;
		double azimuth = layout.channels[midLayerSet[iMid]].polarPosition.azimuth;
		// Upper layer
		if ((upperLayerSet.size() > 0 && abs(azimuth) > maxUpperAz + 40.) || upperLayerSet.size() == 0)
		{
			m_downmixMapping.push_back(iMid);
			name.at(0) = 'U';
			position.azimuth = azimuth;
			position.elevation = meanUpperEl;
			positionNominal.azimuth = layout.channels[midLayerSet[iMid]].polarPositionNominal.azimuth;
			positionNominal.elevation = 30.;
			extraSpeakers.channels.push_back({ name,position,positionNominal,false });
		}
	}

	// Add top and bottom virtual speakers
	position.azimuth = 0.;
	position.elevation = -90.;
	extraSpeakers.channels.push_back({ "BOTTOM",position,position,false });
	if (!layout.containsChannel("T+000") && !layout.containsChannel("UH+180"))
	{
		position.elevation = 90.;
		extraSpeakers.channels.push_back({ "TOP",position,position,false });
	}

	return extraSpeakers;
}

bool CPointSourcePannerGainCalc::CheckScreenSpeakerWidths(const Layout& layout, bool& wideLeft, bool& wideRight)
{
	int chCount = 0;
	for (auto& channel : layout.channels)
		if (channel.name == "M+SC")
		{
			if (channel.polarPosition.azimuth >= 5. && channel.polarPosition.azimuth <= 25.)
				wideLeft = false;
			else if (channel.polarPosition.azimuth >= 35. && channel.polarPosition.azimuth <= 60.)
				wideLeft = false;
			else
				return false; // M+SC is not in the valid range.
			chCount++;
		}
		else if (channel.name == "M-SC")
		{
			if (channel.polarPosition.azimuth <= -5. && channel.polarPosition.azimuth >= -25.)
				wideRight = false;
			else if (channel.polarPosition.azimuth <= -35. && channel.polarPosition.azimuth >= -60.)
				wideRight = false;
			else
				return false; // M-SC is not in the valid range.
			chCount++;
		}

	return chCount == 2;
}
