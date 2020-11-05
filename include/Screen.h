/*############################################################################*/
/*#                                                                          #*/
/*#  Screen scaling and screen edge lock handling                            #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      Screen.h												 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "AdmConversions.h"
#include "ScreenCommon.h"
#include "LoudspeakerLayouts.h"
#include "Coordinates.h"
#include "Tools.h"

/**
	In some output layouts when cartesian==true, vertical panning in front of the listener may be
	warped. This compensates for that.

	See Rec. ITU-R BS.2127-0 sec. 7.3.2 pg 41 for more details
*/
static inline std::pair<double, double> CompensatePosition(double az, double el, Layout layout)
{
	auto speakerNames = layout.channelNames();
	if (std::find(speakerNames.begin(), speakerNames.end(), std::string("U+045")) != speakerNames.end())
	{
		double az_r = interp(el, { -90., 0., 30., 90. }, { 30.,30.,30. * 30. / 45.,30. });
		double azDash = interp(el, { -180., -30., 30., 180. }, { -180., -az_r, az_r, 180. });

		return { azDash, el };
	}
	else
		return { az,el };

}

/**
	The the position of the source from a position relative to the reference screen to a position
	relative to the reproduction screen.
*/
class CScreenScaleHandler {
public:
	CScreenScaleHandler(std::vector<Screen> reproductionScreen, Layout layout);
	~CScreenScaleHandler();

	/**
		Scales a position depending on the reproduction screen and the reference screen
		See Rec. ITU-R BS.2127-0 sec. 7.3.3 pg 40 for more details
	*/
	CartesianPosition handle(CartesianPosition position, bool screenRef, std::vector<Screen> referenceScreen, bool cartesian);

private:
	Layout m_layout;
	// The reproduction screen
	Screen m_repScreen, m_refScreen;
	// The internal representation of the screens
	PolarEdges m_repPolarEdges, m_refPolarEdges;

	bool m_repScreenSet = false;

	/**
		Scale the position
	*/
	CartesianPosition ScalePosition(CartesianPosition);

	/**
		Scale the azimuth and elevation
	*/
	std::pair<double, double> ScaleAzEl(double az, double el);
};

/**
	Apply screen edge locking to supplied position based on the reproduction screen and (if cartesian == true) the layout.
*/
class CScreenEdgeLock
{
public:
	CScreenEdgeLock(std::vector<Screen> reproductionScreen, Layout layout);
	~CScreenEdgeLock();

	/*
		Apply screen edge locking to a cartesian position
	*/
	CartesianPosition HandleVector(CartesianPosition position, admrender::ScreenEdgeLock screenEdgeLock, bool cartesian = false);

	/*
		Apply screen edge locking to an azimuth and elevation
	*/
	std::pair<double, double> HandleAzEl(double azimuth, double elevation, admrender::ScreenEdgeLock screenEdgeLock);


private:
	bool m_repScreenSet = false;
	Layout m_layout;
	Screen m_reproductionScreen;
	PolarEdges m_repPolarEdges;
};
