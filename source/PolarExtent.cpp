/*############################################################################*/
/*#                                                                          #*/
/*#  A polar extent panner													 #*/
/*#                                                                          #*/
/*#  Filename:      PolarExtentPanner.h	                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          21/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "PolarExtent.h"

// CSpreadPannerBase ================================================================================
CSpreadPannerBase::CSpreadPannerBase()
{
	// Set up the grid on the sphere
	// The algorithm can be found here:
	// http://web.archive.org/web/20150108040043/http://www.math.niu.edu/~rusin/known-math/95/equispace.elect
	double perimiter_centre = 2. * M_PI;
	int nRows = 37; // 5deg elevation from -90 to +90
	double deltaEl = 180. / (double)(nRows - 1);
	for (int iEl = 0; iEl < nRows; ++iEl)
	{
		double el = iEl * deltaEl - 90.;
		double radius = std::cos(el * DEG2RAD);
		double perimiter = perimiter_centre * radius;
		int nAz = (int)std::round((perimiter / perimiter_centre) * 2. * (double)(nRows - 1));
		// There must be at least one point at the poles
		if (nAz == 0)
			nAz = 1;
		double deltaAz = 360. / (double)(nAz);
		for (int iAz = 0; iAz < nAz; ++iAz)
		{
			double az = iAz * deltaAz;
			m_virtualSourcePositions.push_back(PolarToCartesian(PolarPosition{ az,el,1. }));
		}
	}
	m_nVirtualSources = (int)m_virtualSourcePositions.size();

	m_weights.resize(m_nVirtualSources);
	m_rotMat.resize(3, std::vector<double>(3, 0.));
	m_positionBasisPol.resize(3, 0.);
	m_posVec.resize(3, 0.);
	m_positionBasis.resize(3, 0.);
	m_closestCircle.resize(3, 0.);
}

CSpreadPannerBase::~CSpreadPannerBase()
{
}

double CSpreadPannerBase::CalculateWeights(CartesianPosition position)
{
	// Convert position to coordinate system of the weighting function
	m_posVec[0] = position.x;
	m_posVec[1] = position.y;
	m_posVec[2] = position.z;
	multiplyMatVec(m_rotMat, m_posVec, m_positionBasis);
	// Get the azimuth and elevation of the converted position
	CartesianToPolar(m_positionBasis, m_positionBasisPol);

	// Calculate the distance of the input position from the "stadium"
	double distance = 0.;
	if (std::abs(m_positionBasisPol[0]) < m_circularCapAzimuth)
		distance = std::abs(m_positionBasisPol[1]) - 0.5 * m_height;
	else
	{
		// if the direction is to the right (x > 0) then reflect to the left to be in the same hemisphere are the circular cap
		m_positionBasis[0] = m_positionBasis[0] > 0. ? -m_positionBasis[0] : m_positionBasis[0];
		m_closestCircle[0] = m_circularCapPosition.x;
		m_closestCircle[1] = m_circularCapPosition.y;
		m_closestCircle[2] = m_circularCapPosition.z;
		// Sometimes the dot product of the 2 unit vectors can be greater than one, which leads to a nan from acos()
		// so it is capped to 1
		double dotProd = std::min(dotProduct(m_positionBasis, m_closestCircle), 1.);
		distance = std::acos(dotProd) * RAD2DEG - 0.5 * m_height;
	}

	// Calculate the weight based on the distance from the "stadium"
	distance = clamp(distance, 0., m_fadeOut);
	double w = 1. - distance / m_fadeOut;

	return w;
}

void CSpreadPannerBase::ConfigureWeightingFunction(CartesianPosition position, double width, double height)
{
	m_width = width;
	m_height = height;
	// Calculate the rotation matrix to convert the virtual source positions to the coordinate system defined by position
	auto polarPosition = CartesianToPolar(position);
	LocalCoordinateSystem(polarPosition.azimuth, polarPosition.elevation, m_rotMat);

	if (m_height > m_width)
	{
		std::swap(m_width, m_height);
		std::swap(m_rotMat[0], m_rotMat[2]);
	}

	// Handle the case where width > 180 so that they meet at the back
	if (m_width > 180.)
		m_width = 180. + (m_width - 180.) / 180. * (180. + m_height);

	// Get the coordinates of the centre of the cap circles such that the edge of the circle is at width / 2 when width < 180deg
	m_circularCapAzimuth = m_width / 2. - m_height / 2.;
	m_circularCapPosition = PolarToCartesian(PolarPosition{ m_circularCapAzimuth, 0, 1 });
}

// CSpreadPanner ================================================================================
CSpreadPanner::CSpreadPanner(CPointSourcePannerGainCalc& psp) : m_pointSourcePannerGainCalc(psp)
{
	m_nCh = m_pointSourcePannerGainCalc.getNumChannels();
	std::vector<double> gainsTmp(m_nCh, 0.);
	// Calculate the panning gain vector for this grid point
	for (int i = 0; i < m_nVirtualSources; ++i)
	{
		m_pointSourcePannerGainCalc.CalculateGains(m_virtualSourcePositions[i], gainsTmp);
		m_virtualSourcePanningVectors.push_back(gainsTmp);
	}
}

CSpreadPanner::~CSpreadPanner()
{
}

void CSpreadPanner::CalculateGains(CartesianPosition position, double width, double height, std::vector<double>& gains)
{
	ConfigureWeightingFunction(position, width, height);

	assert(gains.size() == m_nCh); // output gain vector length must match the number of channels

	// Calculate the weights to be applied to each of the virtual source gain vectors
	for (int i = 0; i < m_nVirtualSources; ++i)
	{
		m_weights[i] = CalculateWeights(m_virtualSourcePositions[i]);
		if (m_weights[i] > 1e-4) // Weight and sum the virtual source gain vectors
			for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
				gains[iCh] += m_weights[i] * m_virtualSourcePanningVectors[i][iCh];
	}

	// Normalise
	double normGains = norm(gains);
	if (normGains > 1e-3)
		for (auto& g : gains)
			g /= normGains;
	else
		for (auto& g : gains)
			g = 0.;
}

// CAmbisonicSpreadPanner ================================================================================
CAmbisonicSpreadPanner::CAmbisonicSpreadPanner(unsigned int ambiOrder)
{
	m_ambiSource.Configure(ambiOrder, true, 0);
	// Calculate the panning gain vector for this grid point
	for (int i = 0; i < m_nVirtualSources; ++i)
	{
		auto polarPos = CartesianToPolar(m_virtualSourcePositions[i]);
		m_ambiSource.SetPosition(PolarPoint{ DegreesToRadians((float)polarPos.azimuth), DegreesToRadians((float)polarPos.elevation), (float)polarPos.distance });
		m_ambiSource.Refresh();
		auto hoaGains = m_ambiSource.GetCoefficients();
		m_virtualSourcePanningVectors.push_back(std::vector<double>(hoaGains.begin(), hoaGains.end()));
	}
	m_nCh = m_ambiSource.GetChannelCount();
}

CAmbisonicSpreadPanner::~CAmbisonicSpreadPanner()
{
}

void CAmbisonicSpreadPanner::CalculateGains(CartesianPosition position, double width, double height, std::vector<double>& gains)
{
	ConfigureWeightingFunction(position, width, height);

	assert(gains.size() == m_nCh);
	// Clear the gains
	for (size_t i = 0; i < gains.size(); ++i)
		gains[i] = 0.;
	// Calculate the weights to be applied to each of the virtual source gain vectors
	double weightSum = 0.;
	for (int i = 0; i < m_nVirtualSources; ++i)
	{
		m_weights[i] = CalculateWeights(m_virtualSourcePositions[i]);
		weightSum += m_weights[i];
	}

	double invWeightSum = 1.;
	if (weightSum > 1e-6)
		invWeightSum = 1. / weightSum;

	// Normalise the weights so they sum to 1
	for (auto& w : m_weights)
		w *= invWeightSum;

	double tol = 1. / (double)m_nVirtualSources * 1e-6;
	for (int i = 0; i < m_nVirtualSources; ++i)
		if (m_weights[i] > tol) // Weight and sum the virtual source gain vectors
			for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
				gains[iCh] += m_weights[i] * m_virtualSourcePanningVectors[i][iCh];
}

unsigned int CAmbisonicSpreadPanner::GetAmbisonicOrder()
{
	return m_ambiSource.GetOrder();
}

// PolarExtentHandlerBase ==========================================================================
CPolarExtentHandlerBase::CPolarExtentHandlerBase()
{
}

CPolarExtentHandlerBase::~CPolarExtentHandlerBase()
{
}

double CPolarExtentHandlerBase::PolarExtentModification(double distance, double extent)
{
	const double minSize = 0.2;
	double size = minSize + (1 - minSize) * extent / 360.;
	double e_1 = 4. * RAD2DEG * std::atan2(size, 1.);
	double e_d = 4. * RAD2DEG * std::atan2(size, distance);
	if (e_d < e_1)
		return extent * e_d / e_1;
	else if (e_d >= e_1)
		return extent + (360. - extent) * (e_d - e_1) / (360. - e_1);

	return distance;
}

// PolarExtentHandler ==========================================================================
CPolarExtentHandler::CPolarExtentHandler(CPointSourcePannerGainCalc& psp) : m_pointSourcePannerGainGalc(psp),
	m_spreadPanner(m_pointSourcePannerGainGalc)
{
	m_nCh = m_pointSourcePannerGainGalc.getNumChannels();
	m_g_p.resize(m_nCh);
	m_g_s.resize(m_nCh);
	m_g1.resize(m_nCh);
	m_g2.resize(m_nCh);
}

CPolarExtentHandler::~CPolarExtentHandler()
{
}

void CPolarExtentHandler::handle(CartesianPosition position, double width, double height, double depth, std::vector<double>& gains)
{
	// Get the distance of the source
	double sourceDistance = norm(position);

	assert(gains.size() == m_nCh); // Length must match the number of channels

	// See Rec. ITU-R BS.2127-0 7.3.8.2 pg 48
	if (depth != 0.)
	{
		double d1 = std::max(0., sourceDistance + depth / 2.);
		double d2 = std::max(0., sourceDistance - depth / 2.);

		// Calculate the modified width and height for both distances
		double modWidth1 = PolarExtentModification(d1, width);
		double modHeight1 = PolarExtentModification(d1, height);
		double modWidth2 = PolarExtentModification(d2, width);
		double modHeight2 = PolarExtentModification(d2, height);

		CalculatePolarExtentGains(position, modWidth1, modHeight1, m_g1);
		CalculatePolarExtentGains(position, modWidth2, modHeight2, m_g2);

		for (size_t i = 0; i < gains.size(); ++i)
			gains[i] = std::sqrt(0.5 * (m_g1[i] * m_g1[i] + m_g2[i] * m_g2[i]));
	}
	else
	{
		double modWidth = PolarExtentModification(sourceDistance, width);
		double modHeight = PolarExtentModification(sourceDistance, height);

		CalculatePolarExtentGains(position, modWidth, modHeight, gains);
	}
}

void CPolarExtentHandler::CalculatePolarExtentGains(CartesianPosition position, double width, double height, std::vector<double>& gains)
{
	double p = clamp(std::max(width, height) / m_minExtent, 0., 1.);

	assert(gains.size() == m_nCh); // gain output vector must match the number of channels

	// If width is low or zero then calculate the point source panning gains
	if (p < 1.)
	{
		m_pointSourcePannerGainGalc.CalculateGains(position, m_g_p);
	}
	else
	{
		// Otherwise set to zero
		for (auto& g : m_g_p)
			g = 0.;
	}
	if (p > 0.)
	{
		m_spreadPanner.CalculateGains(position, width, height, m_g_s);
	}
	else
	{
		// Otherwise set to zero
		for (auto& g : m_g_s)
			g = 0.;
	}

	// Weight and add the point source gains and the spread gains
	for (size_t i = 0; i < m_nCh; ++i)
		gains[i] = std::sqrt(p * m_g_s[i] * m_g_s[i] + (1. - p) * m_g_p[i] * m_g_p[i]);
}

// AmbisonicPolarExtentHandler ==========================================================================
CAmbisonicPolarExtentHandler::CAmbisonicPolarExtentHandler(unsigned int ambiOrder) : m_ambiSpreadPanner(ambiOrder)
{
	m_ambiSource.Configure(ambiOrder, true, 0);
	m_nCh = m_ambiSource.GetChannelCount();
	m_g_p.resize(m_nCh);
	m_g_s.resize(m_nCh);
	m_g1.resize(m_nCh);
	m_g2.resize(m_nCh);
}

CAmbisonicPolarExtentHandler::~CAmbisonicPolarExtentHandler()
{
}

void CAmbisonicPolarExtentHandler::handle(CartesianPosition position, double width, double height, double depth, std::vector<double>& gains)
{
	// Get the distance of the source
	double sourceDistance = norm(position);

	assert(gains.size() == m_nCh); // Length must match the number of channels

	// See Rec. ITU-R BS.2127-0 7.3.8.2 pg 48
	if (depth != 0.)
	{
		double d1 = std::max(0., sourceDistance + depth / 2.);
		double d2 = std::max(0., sourceDistance - depth / 2.);

		// Calculate the modified width and height for both distances
		double modWidth1 = PolarExtentModification(d1, width);
		double modHeight1 = PolarExtentModification(d1, height);
		double modWidth2 = PolarExtentModification(d2, width);
		double modHeight2 = PolarExtentModification(d2, height);

		CalculatePolarExtentGains(position, modWidth1, modHeight1, m_g1);
		CalculatePolarExtentGains(position, modWidth2, modHeight2, m_g2);

		for (size_t i = 0; i < gains.size(); ++i)
			gains[i] = 0.5 * (m_g1[i] + m_g2[i]);
	}
	else
	{
		double modWidth = PolarExtentModification(sourceDistance, width);
		double modHeight = PolarExtentModification(sourceDistance, height);

		CalculatePolarExtentGains(position, modWidth, modHeight, gains);
	}
}

void CAmbisonicPolarExtentHandler::CalculatePolarExtentGains(CartesianPosition position, double width, double height, std::vector<double>& gains)
{
	double p = clamp(std::max(width, height) / m_minExtent, 0., 1.);

	assert(gains.size() == m_nCh); // gain output vector must match the number of channels

	// If width is low or zero then calculate the point source panning gains
	if (p < 1.)
	{
		auto polarPos = CartesianToPolar(position);
		m_ambiSource.SetPosition(PolarPoint{ DegreesToRadians((float)polarPos.azimuth), DegreesToRadians((float)polarPos.elevation), (float)polarPos.distance });
		m_ambiSource.Refresh();
		for (unsigned iAmbiCh = 0; iAmbiCh < m_ambiSource.GetChannelCount(); ++iAmbiCh)
			m_g_p[iAmbiCh] = m_ambiSource.GetCoefficient(iAmbiCh);
	}
	else
	{
		// Otherwise set to zero
		for (auto& g : m_g_p)
			g = 0.;
	}
	if (p > 0.)
	{
		m_ambiSpreadPanner.CalculateGains(position, width, height, m_g_s);
	}
	else
	{
		// Otherwise set to zero
		for (auto& g : m_g_s)
			g = 0.;
	}

	// Weight and add the point source gains and the spread gains
	for (size_t i = 0; i < m_nCh; ++i)
		gains[i] = p * m_g_s[i] + (1. - p) * m_g_p[i];
}