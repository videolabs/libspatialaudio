/*############################################################################*/
/*#                                                                          #*/
/*#  Screen scaling and screen edge lock handling                            #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      Screen.cpp												 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "Screen.h"

CScreenScaleHandler::CScreenScaleHandler(std::vector<Screen> reproductionScreen, Layout layout) : m_layout(layout)
{
	if (reproductionScreen.size() > 0)
	{
		m_repScreen = reproductionScreen[0];
		m_repPolarEdges.fromScreen(m_repScreen);
		m_repScreenSet = true;
	}
}

CScreenScaleHandler::~CScreenScaleHandler()
{

}

CartesianPosition CScreenScaleHandler::handle(CartesianPosition position, bool screenRef, std::vector<Screen> referenceScreen, bool cartesian)
{
	if (screenRef && m_repScreenSet)
	{
		if (referenceScreen.size() > 0)
			m_refScreen = referenceScreen[0];
		else
			m_refScreen = Screen(); // default screen
		m_refPolarEdges.fromScreen(m_refScreen);

		if (cartesian)
		{
			auto polarPosition = admrender::PointCartToPolar(position);
			auto AzEl_s = ScaleAzEl(polarPosition.azimuth, polarPosition.elevation);
			auto AzEl_sc = CompensatePosition(AzEl_s.first, AzEl_s.second, m_layout);
			return admrender::PointPolarToCart({ AzEl_sc.first,AzEl_sc.second,polarPosition.distance });
		}
		else
			return ScalePosition(position);
	}
	else
		return position; // return unmodified
}

CartesianPosition CScreenScaleHandler::ScalePosition(CartesianPosition position)
{
	PolarPosition polarPosition = CartesianToPolar(position);
	auto AzEl_s = ScaleAzEl(polarPosition.azimuth, polarPosition.elevation);
	return PolarToCartesian(PolarPosition{ AzEl_s.first,AzEl_s.second,polarPosition.distance });
}

std::pair<double, double> CScreenScaleHandler::ScaleAzEl(double az, double el)
{
	double azScaled = interp(az, { -180., m_refPolarEdges.rightAzimuth,m_refPolarEdges.leftAzimuth, 180. },
		{ -180., m_repPolarEdges.rightAzimuth,m_repPolarEdges.leftAzimuth, 180. });
	double elScaled = interp(el, { -90., m_refPolarEdges.bottomElevation,m_refPolarEdges.topElevation, 90. },
		{ -90., m_repPolarEdges.bottomElevation,m_repPolarEdges.topElevation, 90. });

	return { azScaled,elScaled };
}


// CScreenEdgeLock ==========================================================================================================
CScreenEdgeLock::CScreenEdgeLock(std::vector<Screen> reproductionScreen, Layout layout) : m_layout(layout)
{
	if (reproductionScreen.size() > 0)
	{
		m_reproductionScreen = reproductionScreen[0];
		m_repPolarEdges.fromScreen(m_reproductionScreen);
		m_repScreenSet = true;
	}
}

CScreenEdgeLock::~CScreenEdgeLock()
{

}

CartesianPosition CScreenEdgeLock::HandleVector(CartesianPosition position, admrender::ScreenEdgeLock screenEdgeLock, bool cartesian)
{
	if (m_repScreenSet)
	{
		if (cartesian)
		{
			auto polarPosition = admrender::PointCartToPolar(position);
			auto AzEl_s = HandleAzEl(polarPosition.azimuth, polarPosition.elevation, screenEdgeLock);
			auto AzEl_sc = CompensatePosition(AzEl_s.first, AzEl_s.second, m_layout);
			return admrender::PointPolarToCart({ AzEl_sc.first,AzEl_sc.second,polarPosition.distance });
		}
		else
		{
			PolarPosition polarPosition = CartesianToPolar(position);
			auto AzEl_s = HandleAzEl(polarPosition.azimuth, polarPosition.elevation, screenEdgeLock);
			return PolarToCartesian(PolarPosition{ AzEl_s.first,AzEl_s.second,polarPosition.distance });
		}
	}
	else
		return position;
}

std::pair<double, double> CScreenEdgeLock::HandleAzEl(double az, double el, admrender::ScreenEdgeLock screenEdgeLock)
{
	if (m_repScreenSet)
	{
		if (screenEdgeLock.horizontal == admrender::ScreenEdgeLock::LEFT)
			az = m_repPolarEdges.leftAzimuth;
		if (screenEdgeLock.horizontal == admrender::ScreenEdgeLock::RIGHT)
			az = m_repPolarEdges.rightAzimuth;

		if (screenEdgeLock.vertical == admrender::ScreenEdgeLock::TOP)
			el = m_repPolarEdges.topElevation;
		if (screenEdgeLock.vertical == admrender::ScreenEdgeLock::BOTTOM)
			el = m_repPolarEdges.bottomElevation;
	}

	return { az, el };
}
