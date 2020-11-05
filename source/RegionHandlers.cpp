/*############################################################################*/
/*#                                                                          #*/
/*#  Region handlers for the point source panner                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmRegionHandlers.cpp                                    #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "RegionHandlers.h"
#include<string>
#include <map>

Triplet::Triplet(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
	: RegionHandler(chanInds, polPos)
{
	// calculate the unit vectors in each of the loudspeaker directions
	std::vector<std::vector<double>> unitVectors(3, std::vector<double>(3, 0.));
	for (int i = 0; i < 3; ++i)
	{
		polPos[i].distance = 1.;
		CartesianPosition cartPos = PolarToCartesian(polPos[i]);
		unitVectors[i][0] = cartPos.x;
		unitVectors[i][1] = cartPos.y;
		unitVectors[i][2] = cartPos.z;
	}
	// Calculate the inverse of the matrix holding the unit vectors
	m_inverseDirections = inverseMatrix(unitVectors);
}

std::vector<double> Triplet::CalculateGains(std::vector<double> directionUnitVec)
{
	std::vector<double> gains(3, 0.);

	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			gains[i] += directionUnitVec[j] * m_inverseDirections[j][i];

	// if any of the gains is negative then return zero
	for (int i = 0; i < 3; ++i)
		if (gains[i] < -m_tol)
			return { 0.,0.,0. };

	// Normalise
	double vecNorm = norm(gains);

	for (int i = 0; i < 3; ++i)
		gains[i] /= vecNorm;

	return gains;
}

//=======================================================================================
VirtualNgon::VirtualNgon(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos, PolarPosition centrePosition)
	: RegionHandler(chanInds, polPos)
{
	m_nCh = (unsigned int)chanInds.size();
	// See Rec. ITU-R BS.2127-0 sec. 6.1.3.1 at pg.27
	m_downmixCoefficient = 1. / sqrt(double(m_nCh));

	// Order the speakers so that they go anti-clockwise from the point of view of the origin to the centre speaker
	std::vector<unsigned int> vertOrder = getNgonVectexOrder(polPos, centrePosition);

	// Make a triplet from each adjacent pair of speakers and the virtual centre speaker
	for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
	{
		unsigned int spk1 = vertOrder[iCh];
		unsigned int spk2 = vertOrder[(iCh + 1) % m_nCh];
		std::vector<unsigned int>channelIndSubset = { spk1,spk2,m_nCh };// { orderCh[spk1], orderCh[spk2], centreInd };
		std::vector<PolarPosition> tripletPositions(3);
		tripletPositions[0] = polPos[spk1];
		tripletPositions[1] = polPos[spk2];
		tripletPositions[2] = centrePosition;
		m_triplets.push_back(Triplet(channelIndSubset, tripletPositions));
	}
}

std::vector<double> VirtualNgon::CalculateGains(std::vector<double> directionUnitVec)
{
	std::vector<double> gains(m_nCh, 0.);

	unsigned int nTriplets = (unsigned int)m_triplets.size();

	unsigned int iTriplet = 0;
	std::vector<double> tripletGains;
	// All gains must be above this value for the triplet to be valid
	// Select a very small negative number to account for rounding errors
	for (iTriplet = 0; iTriplet < nTriplets; ++iTriplet)
	{
		tripletGains = m_triplets[iTriplet].CalculateGains(directionUnitVec);
		double gainSum = std::accumulate(tripletGains.begin(), tripletGains.end(), 0.);
		// All gains must be positive (within tolerance) and the sum should be greater than 0
		if (tripletGains[0] > -m_tol && tripletGains[1] > -m_tol && tripletGains[2] > -m_tol
			&& gainSum > m_tol)
		{
			break;
		}
	}
	// If no triplet is found then return the vector of zero gains
	if (iTriplet == nTriplets)
		return gains;

	std::vector<unsigned int> tripletInds = m_triplets[iTriplet].m_channelInds;
	for (int i = 0; i < 2; ++i)
		gains[tripletInds[i]] += tripletGains[i];
	for (unsigned int i = 0; i < m_nCh; ++i)
		gains[i] += m_downmixCoefficient * tripletGains[2];

	double gainNorm = 1. / norm(gains);
	for (unsigned int i = 0; i < m_nCh; ++i)
		gains[i] *= gainNorm;

	return gains;
}

//=======================================================================================
QuadRegion::QuadRegion(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
	: RegionHandler(chanInds, polPos)
{
	// Get the centre position of the four points
	CartesianPosition centrePosition;
	centrePosition.x = 0.;
	centrePosition.y = 0.;
	centrePosition.z = 0.;
	std::vector<CartesianPosition> cartesianPositions;
	for (int i = 0; i < 4; ++i)
	{
		cartesianPositions.push_back(PolarToCartesian(polPos[i]));
		centrePosition.x += cartesianPositions[i].x / 4.;
		centrePosition.y += cartesianPositions[i].y / 4.;
		centrePosition.z += cartesianPositions[i].z / 4.;
	}
	// Get the order of the loudspeakers
	PolarPosition centrePolarPosition = CartesianToPolar(centrePosition);
	m_vertOrder = getNgonVectexOrder(polPos, centrePolarPosition);
	for (int i = 0; i < 4; ++i)
	{
		m_quadVertices.push_back(cartesianPositions[m_vertOrder[i]]);
	}

	// Calculate the polynomial coefficients
	m_polynomialXProdX = CalculatePolyXProdTerms(m_quadVertices);
	// For the Y terms rotate the order in which the vertices are sent
	m_polynomialXProdY = CalculatePolyXProdTerms({ m_quadVertices[1],m_quadVertices[2],m_quadVertices[3],m_quadVertices[0] });
}

double QuadRegion::GetPanningValue(std::vector<double> directionUnitVec, std::vector<std::vector<double>> xprodTerms)
{
	// Take the dot product with the direction vector to get the polynomial terms
	double a = dotProduct(xprodTerms[0], directionUnitVec);
	double b = dotProduct(xprodTerms[1], directionUnitVec);
	double c = dotProduct(xprodTerms[2], directionUnitVec);

	std::vector<double> roots(2, -1.);

	// No quadratic term so solve linear bx + c = 0
	if (abs(a) < m_tol)
	{
		return -c / b;
	}

	// Find the roots of the quadratic equation
	// TODO: Add some checks here for cases when values are close to zero
	double d = b * b - 4 * a * c;
	if (d >= 0.)
	{
		double sqrtTerm = sqrt(d);
		roots[0] = (-b + sqrtTerm) / (2 * a);
		roots[1] = (-b - sqrtTerm) / (2 * a);
		for (int i = 0; i < 2; ++i)
		{
			if (roots[i] >= 0. && roots[i] <= 1.0)
				return roots[i];
		}
	}

	return -1.; // if no gain was found between 0 and 1 then return -1
}

std::vector<double> QuadRegion::CalculateGains(std::vector<double> directionUnitVec)
{
	std::vector<double> gains_tmp(4, 0.);
	std::vector<double> gains(4, 0.);
	// Calculate the gains in anti-clockwise order
	double x = GetPanningValue(directionUnitVec, m_polynomialXProdX);
	double y = GetPanningValue(directionUnitVec, m_polynomialXProdY);

	// Check that both of the panning values are between zero and one and that gP.d > 0
	if (x > 1. + m_tol || x < -m_tol || y > 1. + m_tol || y < -m_tol)
		return gains; // return zero gains
	gains_tmp = { (1. - x) * (1. - y),x * (1. - y),x * y,(1. - x) * y };

	std::vector<double> gP(3, 0.);
	for (int i = 0; i < 4; ++i)
	{
		gP[0] += gains_tmp[i] * m_quadVertices[i].x;
		gP[1] += gains_tmp[i] * m_quadVertices[i].y;
		gP[2] += gains_tmp[i] * m_quadVertices[i].z;
	}
	double dirCheck = dotProduct(gP, directionUnitVec);
	if (dirCheck < 0.)
		return std::vector<double>(4, 0.);

	double gainNorm = 1. / norm(gains_tmp);
	for (int i = 0; i < 4; ++i)
		gains_tmp[i] *= gainNorm;

	// Map the gains to the order the channels were input
	for (int i = 0; i < 4; ++i)
		gains[m_vertOrder[i]] = gains_tmp[i];

	return gains;
}

std::vector<std::vector<double>> QuadRegion::CalculatePolyXProdTerms(std::vector<CartesianPosition> quadVertices)
{
	// See ITU Rec. ITU-R BS.2127-0 pg 24 last equation
	std::vector<double> p1 = { quadVertices[0].x,quadVertices[0].y,quadVertices[0].z };
	std::vector<double> p2 = { quadVertices[1].x,quadVertices[1].y,quadVertices[1].z };
	std::vector<double> p3 = { quadVertices[2].x,quadVertices[2].y,quadVertices[2].z };
	std::vector<double> p4 = { quadVertices[3].x,quadVertices[3].y,quadVertices[3].z };

	std::vector<std::vector<double>> polyXProdTerms;
	// Quadratic term
	polyXProdTerms.push_back(crossProduct(vecSubtract(p2, p1), vecSubtract(p3, p4)));

	// Linear term
	polyXProdTerms.push_back(vecSum(crossProduct(p1, vecSubtract(p3, p4)), crossProduct(vecSubtract(p2, p1), p4)));

	// Constant term
	polyXProdTerms.push_back(crossProduct(p1, p4));

	return polyXProdTerms;
}