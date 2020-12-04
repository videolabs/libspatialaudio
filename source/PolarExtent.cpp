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
}

CSpreadPannerBase::~CSpreadPannerBase()
{
}

double CSpreadPannerBase::CalculateWeights(CartesianPosition position)
{
	// Convert position to coordinate system of the weighting function
	auto positionBasis = multiplyMatVec(m_rotMat, std::vector<double>({ position.x,position.y,position.z }));
	// Get the azimuth and elevation of the converted position
	auto positionBasisPol = CartesianToPolar(positionBasis);

	// Calculate the distance of the input position from the "stadium"
	double distance = 0.;
	if (std::abs(positionBasisPol[0]) < m_circularCapAzimuth)
		distance = std::abs(positionBasisPol[1]) - 0.5 * m_height;
	else
	{
		// if the direction is to the right (x > 0) then reflect to the left to be in the same hemisphere are the circular cap
		positionBasis[0] = positionBasis[0] > 0. ? -positionBasis[0] : positionBasis[0];
		std::vector<double> closestCircle({ m_circularCapPosition.x, m_circularCapPosition.y, m_circularCapPosition.z });
		// Sometimes the dot product of the 2 unit vectors can be greater than one, which leads to a nan from acos()
		// so it is capped to 1
		double dotProd = std::min(dotProduct(positionBasis, closestCircle), 1.);
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
	m_rotMat = LocalCoordinateSystem(polarPosition.azimuth, polarPosition.elevation);

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
	// Calculate the panning gain vector for this grid point
	for (int i = 0; i < m_nVirtualSources; ++i)
		m_virtualSourcePanningVectors.push_back(m_pointSourcePannerGainCalc.CalculateGains(m_virtualSourcePositions[i]));
}

CSpreadPanner::~CSpreadPanner()
{
}

std::vector<double> CSpreadPanner::CalculateGains(CartesianPosition position, double width, double height)
{
	ConfigureWeightingFunction(position, width, height);

	std::vector<double> gains(m_nCh, 0.);
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

	return gains;
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

std::vector<double> CAmbisonicSpreadPanner::CalculateGains(CartesianPosition position, double width, double height)
{
	ConfigureWeightingFunction(position, width, height);

	std::vector<double> gains(m_nCh, 0.);
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

	return gains;
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
}

CPolarExtentHandler::~CPolarExtentHandler()
{
}

std::vector<double> CPolarExtentHandler::handle(CartesianPosition position, double width, double height, double depth)
{
	// Get the distance of the source
	double sourceDistance = norm(position);

	std::vector<double> gains(m_nCh, 0.);

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

		std::vector<double> g1 = CalculatePolarExtentGains(position, modWidth1, modHeight1);
		std::vector<double> g2 = CalculatePolarExtentGains(position, modWidth2, modHeight2);

		for (size_t i = 0; i < gains.size(); ++i)
			gains[i] = std::sqrt(0.5 * (g1[i] * g1[i] + g2[i] * g2[i]));
	}
	else
	{
		double modWidth = PolarExtentModification(sourceDistance, width);
		double modHeight = PolarExtentModification(sourceDistance, height);

		gains = CalculatePolarExtentGains(position, modWidth, modHeight);
	}
	return gains;
}

std::vector<double> CPolarExtentHandler::CalculatePolarExtentGains(CartesianPosition position, double width, double height)
{
	double p = clamp(std::max(width, height) / m_minExtent, 0., 1.);
	std::vector<double> g_p(m_nCh);
	std::vector<double> g_s(m_nCh);
	std::vector<double> gains(m_nCh);
	// If width is low or zero then calculate the point source panning gains
	if (p < 1.)
	{
		g_p = m_pointSourcePannerGainGalc.CalculateGains(position);
	}
	if (p > 0.)
	{
		g_s = m_spreadPanner.CalculateGains(position, width, height);
	}

	// Weight and add the point source gains and the spread gains
	for (size_t i = 0; i < m_nCh; ++i)
		gains[i] = std::sqrt(p * g_s[i] * g_s[i] + (1. - p) * g_p[i] * g_p[i]);

	return gains;
}

// AmbisonicPolarExtentHandler ==========================================================================
CAmbisonicPolarExtentHandler::CAmbisonicPolarExtentHandler(unsigned int ambiOrder) : m_ambiSpreadPanner(ambiOrder)
{
	m_ambiSource.Configure(ambiOrder, true, 0);
	m_nCh = m_ambiSource.GetChannelCount();
}

CAmbisonicPolarExtentHandler::~CAmbisonicPolarExtentHandler()
{
}

std::vector<double> CAmbisonicPolarExtentHandler::handle(CartesianPosition position, double width, double height, double depth)
{
	// Get the distance of the source
	double sourceDistance = norm(position);

	std::vector<double> gains(m_nCh, 0.);

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

		std::vector<double> g1 = CalculatePolarExtentGains(position, modWidth1, modHeight1);
		std::vector<double> g2 = CalculatePolarExtentGains(position, modWidth2, modHeight2);

		for (size_t i = 0; i < gains.size(); ++i)
			gains[i] = 0.5 * (g1[i] + g2[i]);
	}
	else
	{
		double modWidth = PolarExtentModification(sourceDistance, width);
		double modHeight = PolarExtentModification(sourceDistance, height);

		gains = CalculatePolarExtentGains(position, modWidth, modHeight);
	}
	return gains;
}

std::vector<double> CAmbisonicPolarExtentHandler::CalculatePolarExtentGains(CartesianPosition position, double width, double height)
{
	double p = clamp(std::max(width, height) / m_minExtent, 0., 1.);
	std::vector<double> g_p(m_nCh);
	std::vector<double> g_s(m_nCh);
	std::vector<double> gains(m_nCh);
	// If width is low or zero then calculate the point source panning gains
	if (p < 1.)
	{
		auto polarPos = CartesianToPolar(position);
		m_ambiSource.SetPosition(PolarPoint{ DegreesToRadians((float)polarPos.azimuth), DegreesToRadians((float)polarPos.elevation), (float)polarPos.distance });
		m_ambiSource.Refresh();
		auto hoaGains = m_ambiSource.GetCoefficients();
		g_p = std::vector<double>(hoaGains.begin(), hoaGains.end());
	}
	if (p > 0.)
	{
		g_s = m_ambiSpreadPanner.CalculateGains(position, width, height);
	}

	// Weight and add the point source gains and the spread gains
	for (size_t i = 0; i < m_nCh; ++i)
		gains[i] = p * g_s[i] + (1. - p) * g_p[i];

	return gains;
}