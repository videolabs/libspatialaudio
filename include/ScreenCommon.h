/*############################################################################*/
/*#                                                                          #*/
/*#  Some common elements related to the screen                              #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      ScreenCommon.h											 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "Coordinates.h"
#include "Tools.h"

/**
	Holds a screen with either cartesian or polar coordinates. If isCartesianScreen is set true then
	the centre should be set in centreCartesianPosition.
*/
struct Screen {
	// Flag if the screen is cartesian. Only one set of properties is used depending on this flag so make sure they match
	bool isCartesianScreen = false;

	double aspectRatio = 1.78;

	// Polar screen properties
	PolarPosition centrePolarPosition = PolarPosition{ 0.,0.,1. };
	double widthAzimuth = 58.;

	// Cartesian screen properties
	CartesianPosition centreCartesianPosition;
	double widthX;
};

/**
	PolarEdges structure that holds the representation of the screen for
	use in the screen edge lock and screen scaling prcocessing classes.
*/
struct PolarEdges {
	double leftAzimuth;
	double rightAzimuth;
	double bottomElevation;
	double topElevation;

	/**
		Convert from Screen to Polar Edges.
		See Rec. ITU-R BS.2127-0 Sec. 7.3.3.1 pg. 40
	*/
	void fromScreen(Screen screen)
	{
		CartesianPosition centre;
		CartesianPosition v_x, v_z;
		if (screen.isCartesianScreen)
		{
			double w = screen.widthAzimuth;
			double a = screen.aspectRatio;

			centre = screen.centreCartesianPosition;
			double width = w / 2.;
			double height = width / a;

			v_x = CartesianPosition{ width,0.,0. };
			v_z = CartesianPosition{ 0., 0., height };
		}
		else
		{
			double az = screen.centrePolarPosition.azimuth;
			double el = screen.centrePolarPosition.elevation;
			double d = screen.centrePolarPosition.distance;
			double w = screen.widthAzimuth;
			double a = screen.aspectRatio;

			centre = PolarToCartesian(screen.centrePolarPosition);
			double width = d * std::tan(DEG2RAD * w / 2.);
			double height = width / a;

			auto l_xyz = LocalCoordinateSystem(az, el);
			v_x = CartesianPosition{ l_xyz[0][0] * width,l_xyz[0][1] * width,l_xyz[0][2] * width };
			v_z = CartesianPosition{ l_xyz[2][0] * height,l_xyz[2][1] * height,l_xyz[2][2] * height };
		}

		leftAzimuth = CartesianToPolar(centre - v_x).azimuth;
		rightAzimuth = CartesianToPolar(centre + v_x).azimuth;
		bottomElevation = CartesianToPolar(centre - v_z).elevation;
		topElevation = CartesianToPolar(centre + v_z).elevation;
	}
};
