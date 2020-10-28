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

namespace admrender {


	// SpreadPanner ================================================================================
	CSpreadPanner::CSpreadPanner(CAdmPointSourcePannerGainCalc& psp) : m_pointSourcePannerGainCalc(psp)
	{
		// Set up the grid on the sphere 
	    // The algorithm and some extra reading can be found here :
		// http://extremelearning.com.au/how-to-evenly-distribute-points-on-a-sphere-more-effectively-than-the-canonical-fibonacci-lattice/
		double goldenRatio = (1. + std::pow(5.,0.5)) / 2.;
		for (int i = 0; i < nVirtualSources; ++i)
		{
			double theta = 2. * M_PI * i / goldenRatio;
			double phi = std::acos(1. - 2. * ((double)i + 0.5) / (double)nVirtualSources);
			m_virtualSourcePositions.push_back({cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi)});
			// Calculate the panning gain vector for this grid point
			m_virtualSourcePanningVectors.push_back(m_pointSourcePannerGainCalc.CalculateGains(m_virtualSourcePositions[i]));
		}

		weights.resize(nVirtualSources);
	}

	CSpreadPanner::~CSpreadPanner()
	{
	}

	std::vector<double> CSpreadPanner::CalculateGains(CartesianPosition position, double width, double height)
	{
		ConfigureWeightingFunction(position, width, height);

		int nCh = m_pointSourcePannerGainCalc.getNumChannels();
		std::vector<double> gains(nCh, 0.);
		// Calculate the weights to be applied to each of the virtual source gain vectors
		for (int i = 0; i < nVirtualSources; ++i)
		{
			weights[i] = CalculateWeights(m_virtualSourcePositions[i]);
			if (weights[i] > 1e-4) // Weight and sum the virtual source gain vectors
				for (int iCh = 0; iCh < nCh; ++iCh)
					gains[iCh] += weights[i] * m_virtualSourcePanningVectors[i][iCh];
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

	double CSpreadPanner::CalculateWeights(CartesianPosition position)
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
			distance = std::acos(dotProduct(positionBasis, closestCircle)) * RAD2DEG - 0.5 * m_height;
		}

		// Calculate the weight based on the distance from the "stadium"
		distance = std::clamp(distance, 0., m_fadeOut);
		double w = 1. - distance / m_fadeOut;

		return w;
	}

	void CSpreadPanner::ConfigureWeightingFunction(CartesianPosition position, double width, double height)
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

	// PolarExtentHandler ==========================================================================
	CPolarExtentHandler::CPolarExtentHandler(CAdmPointSourcePannerGainCalc& psp) : m_pointSourcePannerGainGalc(psp),
		m_spreadPanner(m_pointSourcePannerGainGalc)
	{
	}

	CPolarExtentHandler::~CPolarExtentHandler()
	{
	}

	std::vector<double> CPolarExtentHandler::handle(CartesianPosition position, double width, double height, double depth)
	{
		// Get the distance of the source
		double sourceDistance = norm(position);

		std::vector<double> gains(m_pointSourcePannerGainGalc.getNumChannels(), 0.);

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

	std::vector<double> CPolarExtentHandler::CalculateGains(CartesianPosition position, double width, double height)
	{

		return std::vector<double>();
	}

	double CPolarExtentHandler::PolarExtentModification(double distance, double extent)
	{
		const double minSize = 0.2;
		double size = minSize + (1 - minSize) * extent / 360.;
		double e_1 = 4. * RAD2DEG * std::atan2(size, 1.);
		double e_d = 4. * RAD2DEG * std::atan2(size, distance);
		if (e_d < e_1)
			return extent * e_d / e_1;
		else if (e_d >= e_1)
			return extent + (360. - extent) * (e_d - e_1) / (360. - e_1);
	}

	std::vector<double> CPolarExtentHandler::CalculatePolarExtentGains(CartesianPosition position, double width, double height)
	{
		double p = std::clamp(std::max(width, height) / m_minExtent, 0., 1.);
		const int nCh = m_pointSourcePannerGainGalc.getNumChannels();
		std::vector<double> g_p(nCh);
		std::vector<double> g_s(nCh);
		std::vector<double> gains(nCh);
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
		for (size_t i = 0; i < nCh; ++i)
			gains[i] = std::sqrt(p * g_s[i] * g_s[i] + (1. - p) * g_p[i] * g_p[i]);

		return gains;
	}
}