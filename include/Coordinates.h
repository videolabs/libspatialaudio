/*############################################################################*/
/*#                                                                          #*/
/*#  Cartesian and polar coordinates                                         #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      Coordinates.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

struct PolarPosition
{
	double azimuth = 0.0;
	double elevation = 0.0;
	double distance = 1.f;
};
inline bool operator==(const PolarPosition& lhs, const PolarPosition& rhs)
{
	return lhs.azimuth == rhs.azimuth && lhs.elevation == rhs.elevation && lhs.distance == rhs.distance;
}

struct CartesianPosition
{
	double x = 1.0;
	double y = 0.0;
	double z = 0.0;
};
inline bool operator==(const CartesianPosition& lhs, const CartesianPosition& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}
inline CartesianPosition operator+(const CartesianPosition& lhs, const CartesianPosition& rhs)
{
	return CartesianPosition{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}
inline CartesianPosition operator-(const CartesianPosition& lhs, const CartesianPosition& rhs)
{
	return CartesianPosition{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}
