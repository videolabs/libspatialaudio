/*############################################################################*/
/*#                                                                          #*/
/*#  Region handlers for the point source panner                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmRegionHandlers.h                                      #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

//#include "AdmMetadata.h"
#include "Tools.h"

#include <assert.h>
#include <set>

/**
	Returns the order of a set of points in an anti-clockwise direction.
	The points should be form a Quad or Ngon and be roughly co-planar
*/
static inline std::vector<unsigned int> getNgonVectexOrder(std::vector<PolarPosition> polarPositions, PolarPosition centrePosition)
{
	unsigned int nVertices = (unsigned int)polarPositions.size();
	double rotMat[9] = { 0. };
	// Get the rotation matrix that makes the centre the front
	getRotationMatrix(-centrePosition.azimuth, centrePosition.elevation, 0., &rotMat[0]);

	std::vector<double> angle(nVertices, 0.);
	std::vector<unsigned int> vertInds;
	for (unsigned int iVert = 0; iVert < nVertices; ++iVert)
	{
		vertInds.push_back(iVert);
		// Rotate the centre position to check it is to the front (0,1,0)
		CartesianPosition cartesianPositions = PolarToCartesian(polarPositions[iVert]);
		// Unit vector in coordinate system with x-axis to the front and y-axis to the left
		std::vector<double> vertexVector = { cartesianPositions.y,-cartesianPositions.x,cartesianPositions.z };
		std::vector<double> vertexRotatedVector(3, 0.);
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				vertexRotatedVector[i] += rotMat[3 * i + j] * vertexVector[j];
		angle[iVert] = convertToRange360(RAD2DEG * atan2(-vertexRotatedVector[2], vertexRotatedVector[1]));
	}
	// Sort the angles
	int x = 0;
	std::iota(vertInds.begin(), vertInds.end(), x++); //Initializing
	std::sort(vertInds.begin(), vertInds.end(), [&](int i, int j) {return angle[i] < angle[j]; });

	return vertInds;
}

// Holds the output channel indices and their polar coordinates
class RegionHandler
{
public:
	RegionHandler(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
		: m_channelInds(chanInds), m_polarPositions(polPos)
	{

	}

	std::vector<unsigned int> m_channelInds;
	std::vector<PolarPosition> m_polarPositions;
	// Tolerance value used to check near zero values
	double m_tol = 1e-6;
};

/**
	Holds a triplet of speaker indices and calculates the gain for a given source direction
*/
class Triplet : public RegionHandler
{
public:
	Triplet(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos);

	std::vector<double> CalculateGains(std::vector<double> directionUnitVec);

private:
	// Inverse of the matrix holding the triplet unit vectors
	std::vector<std::vector<double>> m_inverseDirections;
};


/**
	Holds a VirtualNgon of speaker indices and calculates the gain for a given source direction.
	A VirtualNgon has a virtual loudspeaker placed at its centre whose gain is mixed to the real loudspeakers
*/
class VirtualNgon : public RegionHandler
{
public:
	VirtualNgon(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos, PolarPosition centrePosition);

	std::vector<double> CalculateGains(std::vector<double> directionUnitVec);

private:
	std::vector<Triplet> m_triplets;
	double m_downmixCoefficient;
	// The number of channels in the Ngon
	unsigned int m_nCh = 0;
};

/**
	Holds a VirtualNgon of speaker indices and calculates the gain for a given source direction.
	See Rec. ITU-R BS.2127-0 sec. 6.1.2.3 for full details of the gain calculation
*/
class QuadRegion : public RegionHandler
{
public:
	QuadRegion(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos);

	double GetPanningValue(std::vector<double> directionUnitVec, std::vector<std::vector<double>> xprodTerms);

	std::vector<double> CalculateGains(std::vector<double> directionUnitVec);

private:
	std::vector<std::vector<double>> CalculatePolyXProdTerms(std::vector<CartesianPosition> quadVertices);

	// The coordinates of the vertices in anti-clockwise order start from the bottom left.
	std::vector<CartesianPosition> m_quadVertices;
	// The ordering of the input speaker coordinates needed to put them in the right order
	std::vector<unsigned int> m_vertOrder;
	// The cross product terms from the final equation in section 6.1.2.3.2 (pg 24)
	std::vector<std::vector<double>> m_polynomialXProdX;
	std::vector<std::vector<double>> m_polynomialXProdY;
};

struct LayoutRegions
{
	std::vector<Triplet> triplets;
	std::vector<QuadRegion> quadRegions;
	std::vector<VirtualNgon> virtualNgons;
};
