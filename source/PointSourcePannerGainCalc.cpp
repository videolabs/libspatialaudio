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

	// Allocate temp gains based on the size of the internal layout used (0+5+0 when outputting stereo)
	m_gainsTmp.resize(m_outputLayout.channels.size(), 0.);
	m_directionUnitVec.resize(3, 0.);

	// if the layout is HOA then don't go any further because this panner is intended for loudspeaker arrays
	if (layout.isHoa)
		return;

	// Get the positions of all of the loudspeakers
	std::vector<PolarPosition> positions;
	for (size_t i = 0; i < m_outputLayout.channels.size(); ++i)
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
	if (m_isStereo) // then downmix from 0+5+0 to 0+2+0
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
	else
	{
		CalculateGainsFromRegions(position, gains);
	}
}

unsigned int CPointSourcePannerGainCalc::getNumChannels()
{
	return m_isStereo ? 2 : (unsigned int)m_outputLayout.channels.size();
}

void CPointSourcePannerGainCalc::CalculateGainsFromRegions(CartesianPosition position, std::vector<double>& gains)
{
	double tol = 1e-6;

	assert(gains.size() == m_outputLayout.channels.size()); // Gains vector length must match the number of channels
	for (size_t i = 0; i < gains.size(); ++i)
		gains[i] = 0.;

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
	for (size_t iMid = 0; iMid < midLayerSet.size(); ++iMid)
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
	for (size_t iMid = 0; iMid < midLayerSet.size(); ++iMid)
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
