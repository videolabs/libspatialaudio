/*############################################################################*/
/*#                                                                          #*/
/*#  Helper functions for point source panning and ADM renderer.             #*/
/*#								                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      Tools.h	                                                 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#define _USE_MATH_DEFINES
#include<math.h>
#include<cmath>
#include<numeric>
#include<algorithm>
#include <iostream>
#include <vector>

#include "Coordinates.h"

#define DEG2RAD M_PI/180.
#define RAD2DEG 180./M_PI

// Some Helper Functions

/**
	Convert from cartesian to polar coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
	+ve X = right
	+ve Y = front
	+ve Z = up
*/
static inline std::vector<double> CartesianToPolar(std::vector<double> v)
{
	double x = v[0];
	double y = v[1];
	double z = v[2];
	double azimuth = -RAD2DEG * atan2(x, y);
	double elevation = RAD2DEG * atan2(z, sqrt(x * x + y * y));
	double distance = sqrt(x * x + y * y + z * z);

	return{ azimuth, elevation,distance };
}
static inline PolarPosition CartesianToPolar(CartesianPosition cartesian)
{
	double x = cartesian.x;
	double y = cartesian.y;
	double z = cartesian.z;
	std::vector<double> polarPosition = CartesianToPolar(std::vector<double>{ x, y, z });

	return { polarPosition[0],polarPosition[1],polarPosition[2] };
};
/*
static inline PolarPosition CartesianToPolar(DirectSpeakerCartesianPosition cartesian)
{
	double x = cartesian.x;
	double y = cartesian.y;
	double z = cartesian.z;
	std::vector<double> polarPosition = CartesianToPolar(std::vector<double>{ x,y,z });

	return { polarPosition[0],polarPosition[1],polarPosition[2] };
};
*/
/**
	Convert from polar to cartesian coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
	0 az = front, +ve az = anti-clockwise
	0 el = front, +ve el = up
	Angles are expected in degrees.
*/
static inline std::vector<double> PolarToCartesian(std::vector<double> polar)
{
	double az = DEG2RAD * polar[0];
	double el = DEG2RAD * polar[1];
	double d = polar[2];
	double x = sin(-az) * cos(el) * d;
	double y = cos(-az) * cos(el) * d;
	double z = sin(el) * d;

	return { x,y,z };
};
static inline CartesianPosition PolarToCartesian(PolarPosition polar)
{
	double az = polar.azimuth;
	double el = polar.elevation;
	double d = polar.distance;
	std::vector<double> cart = PolarToCartesian(std::vector<double> {az, el, d});

	return {cart[0],cart[1],cart[2]};
};
/*
static inline CartesianPosition PolarToCartesian(DirectSpeakerPolarPosition polar)
{
	double az = polar.azimuth;
	double el = polar.elevation;
	double d = polar.distance;
	std::vector<double> cart = PolarToCartesian(std::vector<double> {az, el, d});

	return { cart[0],cart[1],cart[2] };
};
*/
/**
	Returns the norm of a vector
*/
static inline double norm(std::vector<double> vec)
{
	double vecNorm = 0.;
	for (size_t i = 0; i < vec.size(); ++i)
		vecNorm += vec[i] * vec[i];
	vecNorm = sqrt(vecNorm);
	return vecNorm;
}
static inline double norm(CartesianPosition vec)
{
	return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}
/**
	Returns a rotation matrix for given yaw, pitch, roll (in degrees)
*/
static inline void getRotationMatrix(double yaw, double pitch, double roll, double* rotMat)
{
	// Convert angles to radians
	yaw *= DEG2RAD;
	pitch *= DEG2RAD;
	roll *= DEG2RAD;
	// Pre-calculation
	double sinYaw = sin(yaw);
	double cosYaw = cos(yaw);
	double sinPitch = sin(pitch);
	double cosPitch = cos(pitch);
	double sinRoll = sin(roll);
	double cosRoll = cos(roll);
	// Calculate the rotation matrix
	// Top row
	rotMat[0] = cosRoll * cosPitch * cosYaw - sinRoll * sinYaw;
	rotMat[1] = -cosRoll * cosPitch * sinYaw - sinRoll * cosYaw;
	rotMat[2] = cosRoll * sinPitch;
	// Middle row
	rotMat[3] = sinRoll * cosPitch * cosYaw + cosRoll * sinYaw;
	rotMat[4] = -sinRoll * cosPitch * sinYaw + cosRoll * cosYaw;
	rotMat[5] = sinRoll * sinPitch;
	// Bottom row
	rotMat[6] = -sinPitch * cosYaw;
	rotMat[7] = sinPitch * sinYaw;
	rotMat[8] = cosPitch;
}
/**
	Converts an angle in degrees to the range 0 to 360
*/
static inline double convertToRange360(double input)
{
	double out = input;
	int i = 0;
	while (out < -1e-10 || out >= 360. + 1e-10)
		out += i++ * 360.;
	return out;
}
/**
	Converts an angle in degrees to the range -180 to 180
*/
static inline double convertToRangeMinus180To180(double input)
{
	double out = input;
	int i = 0;
	while (out < -180. || out > 180.)
	{
		if (out < -180.)
			out += i++ * 360.;
		else if (out > 180.)
			out -= i++ * 360.;
	}
	return out;
}
/**
	 Find an equivalent angle to y which is greater than or equal to x.
	 See Rec. ITU-R BS.2127-0 sec. 6.7 pg. 33
*/
static inline double relativeAngle(double x, double y)
{
	while (y - 360. >= x)
		y -= 360.;
	
	while (y < x)
		y += 360.;
			
	return y;
}
/**
	Returns the element-wise sum of two vectors of the same length
*/
static inline std::vector<double> vecSum(std::vector<double> a, std::vector<double> b)
{
	std::vector<double> c(a.size(), 0.);
	std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::plus<double>());
	return c;
}
/**
	Subtracts vector b from vector a in an element-wise manner. Returns a-b
*/
static inline std::vector<double> vecSubtract(std::vector<double> a, std::vector<double> b)
{
	std::vector<double> c(a.size(), 0.);
	std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::minus<double>());
	return c;
}
/**
	Returns the dot-product of two 3D vector
*/
static inline double dotProduct(std::vector<double> a, std::vector<double> b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
/**
	Returns the cross-product of two 3D vector
*/
static inline std::vector<double> crossProduct(std::vector<double> a, std::vector<double> b)
{
	std::vector<double> c(3, 0.);

	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];

	return c;
}	
/**
	Get the sign of a number. Returns -1 for negative numbers, +1 for positive numbers and 0 otherwise
*/
static inline int Sgn(double x)
{
	double tol = 1e-5;
	if (x > tol)
		return 1;
	else if (x < -tol)
		return -1;
	return 0;
}
/**
	Returns true if x is in side the specified range

	See Rec. ITU-R BS.2127-0 sec. 6.2
*/
static inline bool insideAngleRange(double x, double startAngle, double endAngle, double tol = 0.)
{
	x = convertToRangeMinus180To180(x);
	startAngle = convertToRangeMinus180To180(startAngle);
	endAngle = convertToRangeMinus180To180(endAngle);
		
	if (startAngle <= endAngle)
		return x >= startAngle - tol && x <= endAngle + tol;
	else if (startAngle > endAngle)
		return  x >= startAngle - tol || x <= endAngle + tol;

	return false;
}
/**
	Multiply two matrices
*/
static inline std::vector<std::vector<double>> multiplyMat(std::vector<std::vector<double>> A, std::vector<std::vector<double>> B)
{
	size_t rowsA = A.size();
	size_t colsA = A[0].size();
	size_t colsB = B[0].size();
	std::vector<std::vector<double>> ret(rowsA, std::vector<double>(colsB, 0.));

	for (size_t i = 0; i < rowsA; ++i)
		for (size_t j = 0; j < colsB; ++j)
			for (size_t k = 0; k < colsA; ++k)
				ret[i][j] += A[i][k] * B[k][j];

	return ret;
}
/**
	Multiply a matrix to a vector y = Ax;
*/
static inline std::vector<double> multiplyMatVec(std::vector<std::vector<double>> A, std::vector<double> x)
{
	size_t rowsA = A.size();
	size_t colsA = A[0].size();
	std::vector<double> ret(rowsA, 0.);

	for (size_t i = 0; i < rowsA; ++i)
			for (size_t k = 0; k < colsA; ++k)
				ret[i] += A[i][k] * x[k];

	return ret;
}
/**
	Calculate the inverse of a square matrix of size 2x2
*/
static inline std::vector<std::vector<double>> inverseMatrix2x2(std::vector<std::vector<double>> mat)
{
	double a = mat[0][0];
	double b = mat[0][1];
	double c = mat[1][0];
	double d = mat[1][1];
	double det = a * d - b * c;

	return { {d / det, -b / det},{-c / det, a / det} };
}
/**
	Calculate the inverse of a square matrix
*/
static inline std::vector<std::vector<double>> inverseMatrix(std::vector<std::vector<double>> mat)
{
	size_t matSize = mat.size();

	if (matSize == 2)
		return inverseMatrix2x2(mat);

	std::vector<std::vector<double>> inverseMat(matSize, std::vector<double>(matSize, 0.));

	// Calculate the inverse of the matrix holding the unit vectors
	double det = 0.; // determinant
	for (size_t i = 0; i < matSize; i++)
		det += (mat[0][i] * (mat[1][(i + 1) % 3] * mat[2][(i + 2) % 3] - mat[1][(i + 2) % 3] * mat[2][(i + 1) % 3]));
	double invDet = 1. / det;

	for (size_t i = 0; i < matSize; i++)
		for (size_t j = 0; j < matSize; j++)
		{
			inverseMat[i][j] = ((mat[(j + 1) % 3][(i + 1) % 3] * mat[(j + 2) % 3][(i + 2) % 3])
				- (mat[(j + 1) % 3][(i + 2) % 3] * mat[(j + 2) % 3][(i + 1) % 3])) * invDet;
		}

	return inverseMat;
}

/**
	Get the diverged source positions and directions
*/
static inline std::pair<std::vector<CartesianPosition>, std::vector<double>> divergedPositionsAndGains(double divergenceValue, double divergenceAzRange, CartesianPosition position)
{
	std::vector<CartesianPosition> diverged_positions;
	std::vector<double> diverged_gains;

	PolarPosition polarDirection = CartesianToPolar(position);

	double x = divergenceValue;
	double d = polarDirection.distance;
	// if the divergence value is zero then return the original direction and a gain of 1
	if (x == 0.)
		return { std::vector<CartesianPosition>{position},std::vector<double>{1.} };

	// If there is any divergence then calculate the gains and directions
	// Calculate gains using Rec. ITU-R BS.2127-0 sec. 7.3.7.1
	diverged_gains.resize(3, 0.);
	diverged_gains[0] = (1. - x) / (x + 1.);
	double glr = x / (x + 1.);
	diverged_gains[1] = glr;
	diverged_gains[2] = glr;

	std::vector<std::vector<double>> cartPositions(3);
	cartPositions[0] = { d,0.,0. };
	auto cartesianTmp = PolarToCartesian(PolarPosition{ x * divergenceAzRange,0.,d });
	cartPositions[1] = { cartesianTmp.y,-cartesianTmp.x,cartesianTmp.z };
	cartesianTmp = PolarToCartesian(PolarPosition{ -x * divergenceAzRange,0.,d });
	cartPositions[2] = { cartesianTmp.y,-cartesianTmp.x,cartesianTmp.z };

	// Rotate them so that the centre position is in specified input direction
	double rotMat[9] = { 0. };
	getRotationMatrix(polarDirection.azimuth, -polarDirection.elevation, 0., &rotMat[0]);
	diverged_positions.resize(3);
	for (int iDiverge = 0; iDiverge < 3; ++iDiverge)
	{
		std::vector<double> directionRotated(3, 0.);
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				directionRotated[i] += rotMat[3 * i + j] * cartPositions[iDiverge][j];
		diverged_positions[iDiverge] = CartesianPosition{ -directionRotated[1],directionRotated[0],directionRotated[2] };
	}

	return { diverged_positions, diverged_gains };
}
/**
	Get the rotation matrix required to convert a point from one coordinate system to another.

	See Rec. ITU-R BS.2127-0 sec. 6.8
*/
static inline std::vector<std::vector<double>> LocalCoordinateSystem(double azInDegrees, double elInDegrees)
{
	std::vector<std::vector<double>> rotMat;
	rotMat.resize(3);

	rotMat[0] = PolarToCartesian(std::vector<double>{azInDegrees - 90., 0., 1.});
	rotMat[1] = PolarToCartesian(std::vector<double>{azInDegrees, elInDegrees, 1.});
	rotMat[2] = PolarToCartesian(std::vector<double>{azInDegrees, elInDegrees + 90., 1.});

	return rotMat;
}
/**
	Clamp a value between a minimum and a maximum
*/
static inline double clamp(double val, double minVal, double maxVal)
{
	return std::min(maxVal, std::max(val, minVal));
}
/**
	Apply piecewise linear interpolation to a value
	The size of fromVals and toVals must match.
	The elements of fromVals should be ascending order.
	If the input val is outside the range defined in fromVals then it is returned unchanged
*/
static inline double interp(double val, std::vector<double> fromVals, std::vector<double> toVals)
{
	int nPieces = (int)fromVals.size();
	for(int iPiece = 0; iPiece < nPieces-1; ++iPiece)
		if (val >= fromVals[iPiece] && val < fromVals[iPiece + 1])
		{
			double rangeFrom = fromVals[iPiece + 1] - fromVals[iPiece];
			double rangeTo = toVals[iPiece + 1] - toVals[iPiece];
			return (val - fromVals[iPiece]) / rangeFrom * rangeTo + toVals[iPiece];
		}

	return val;
}
