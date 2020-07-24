/*############################################################################*/
/*#                                                                          #*/
/*#  Helper functions for the ADM renderer.                                  #*/
/*#								                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmUtils.h	                                             #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "AdmMetadata.h"
#include "AdmLayouts.h"

#define _USE_MATH_DEFINES
#include<math.h>
#include<numeric>
#include<algorithm>
#include <sstream>
#include <iostream>

#define DEG2RAD M_PI/180.
#define RAD2DEG 180./M_PI

namespace admrender {
	// Some Helper Functions
	/**
		Convert from cartesian to polar coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
		+ve X = right
		+ve Y = front
		+ve Z = up
	*/
	static std::vector<double> CartesianToPolar(std::vector<double> v)
	{
		double x = v[0];
		double y = v[1];
		double z = v[2];
		double azimuth = -RAD2DEG * atan2(x, y);
		double elevation = RAD2DEG * atan2(z, sqrt(x * x + y * y));
		double distance = sqrt(x * x + y * y + z * z);

		return{ azimuth, elevation,distance };
	}
	static PolarPosition CartesianToPolar(CartesianPosition cartesian)
	{
		double x = cartesian.x;
		double y = cartesian.y;
		double z = cartesian.z;
		std::vector<double> polarPosition = CartesianToPolar(std::vector<double>{ x, y, z });

		return { polarPosition[0],polarPosition[1],polarPosition[2] };
	};
	static PolarPosition CartesianToPolar(DirectSpeakerCartesianPosition cartesian)
	{
		double x = cartesian.x;
		double y = cartesian.y;
		double z = cartesian.z;
		std::vector<double> polarPosition = CartesianToPolar(std::vector<double>{ x,y,z });

		return { polarPosition[0],polarPosition[1],polarPosition[2] };
	};
	/**
		Convert from polar to cartesian coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
		0 az = front, +ve az = anti-clockwise
		0 el = front, +ve el = up
	*/
	static std::vector<double> PolarToCartesian(std::vector<double> polar)
	{
		double az = DEG2RAD * polar[0];
		double el = DEG2RAD * polar[1];
		double d = polar[2];
		double x = sin(-az) * cos(el) * d;
		double y = cos(-az) * cos(el) * d;
		double z = sin(el) * d;

		return { x,y,z };
	};
	static CartesianPosition PolarToCartesian(PolarPosition polar)
	{
		double az = polar.azimuth;
		double el = polar.elevation;
		double d = polar.distance;
		std::vector<double> cart = PolarToCartesian(std::vector<double> {az, el, d});

		return {cart[0],cart[1],cart[2]};
	};
	static CartesianPosition PolarToCartesian(DirectSpeakerPolarPosition polar)
	{
		double az = polar.azimuth;
		double el = polar.elevation;
		double d = polar.distance;
		std::vector<double> cart = PolarToCartesian(std::vector<double> {az, el, d});

		return { cart[0],cart[1],cart[2] };
	};
	/**
		Returns the norm of a vector
	*/
	static double norm(std::vector<double> vec)
	{
		double vecNorm = 0.;
		for (int i = 0; i < vec.size(); ++i)
			vecNorm += vec[i] * vec[i];
		vecNorm = sqrt(vecNorm);
		return vecNorm;
	}
	/**
		Returns a rotation matrix for given yaw, pitch, roll (in degrees)
	*/
	static void getRotationMatrix(double yaw, double pitch, double roll, double* rotMat)
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
	static double convertToRange360(double input)
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
	static double convertToRangeMinus180To180(double input)
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
		Returns the order of a set of points in an anti-clockwise direction.
		The points should be form a Quad or Ngon and be roughly co-planar
	*/
	static std::vector<unsigned int> getNgonVectexOrder(std::vector<PolarPosition> polarPositions, PolarPosition centrePosition)
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
			angle[iVert] = convertToRange360(RAD2DEG*atan2(-vertexRotatedVector[2], vertexRotatedVector[1]));
		}
		// Sort the angles
		int x = 0;
		std::iota(vertInds.begin(), vertInds.end(), x++); //Initializing
		std::sort(vertInds.begin(), vertInds.end(), [&](int i, int j) {return angle[i] < angle[j]; });

		return vertInds;
	}
	/**
		Returns the element-wise sum of two vectors of the same length
	*/
	static std::vector<double> vecSum(std::vector<double> a, std::vector<double> b)
	{
		std::vector<double> c(a.size(), 0.);
		std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::plus<double>());
		return c;
	}
	/**
		Subtracts vector b from vector a in an element-wise manner. Returns a-b
	*/
	static std::vector<double> vecSubtract(std::vector<double> a, std::vector<double> b)
	{
		std::vector<double> c(a.size(), 0.);
		std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::minus<double>());
		return c;
	}
	/**
		Returns the dot-product of two 3D vector
	*/
	static double dotProduct(std::vector<double> a, std::vector<double> b)
	{
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}
	/**
		Returns the cross-product of two 3D vector
	*/
	static std::vector<double> crossProduct(std::vector<double> a, std::vector<double> b)
	{
		std::vector<double> c(3, 0.);

		c[0] = a[1] * b[2] - a[2] * b[1];
		c[1] = a[2] * b[0] - a[0] * b[2];
		c[2] = a[0] * b[1] - a[1] * b[0];

		return c;
	}	
	/**
		Get the speakerLayout that matches the given name. If no match then returns empty.
	*/
	static Layout GetMatchingLayout(std::string layoutName)
	{
		unsigned int nLayouts = (unsigned int)speakerLayouts.size();
		for (unsigned int iLayout = 0; iLayout < nLayouts; ++iLayout)
		{
			if (layoutName.compare(speakerLayouts[iLayout].name) == 0)
				return speakerLayouts[iLayout];
		}
		return {}; // if no matching channel names are found
	}
	/**
		Get the sign of a number. Returns -1 for negative numbers, +1 for positive numbers and 0 otherwise
	*/
	static int Sgn(double x)
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
	static bool insideAngleRange(double x, double startAngle, double endAngle, double tol = 0.)
	{
		x = convertToRangeMinus180To180(x);
		startAngle = convertToRangeMinus180To180(startAngle);
		endAngle = convertToRangeMinus180To180(endAngle);
		
		if (startAngle < endAngle)
			return x >= startAngle - tol && x <= endAngle + tol;
		else if (startAngle > endAngle)
			return  x >= startAngle - tol || x <= endAngle + tol;

		return x;
	}
	/**
		Multiply two matrices
	*/
	static std::vector<std::vector<double>> multiplyMat(std::vector<std::vector<double>> A, std::vector<std::vector<double>> B)
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
		Calculate the inverse of a square matrix
	*/
	static std::vector<std::vector<double>> inverseMatrix(std::vector<std::vector<double>> mat)
	{
		size_t matSize = mat.size();
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
		If the the speaker label is in the format "urn:itu:bs:2051:[0-9]:speaker:X+YYY then
		return the X+YYY portion. Otherwise, returns the original input
	*/
	static std::string GetNominalSpeakerLabel(const std::string& label)
	{
		std::string speakerLabel = label;

		std::stringstream ss(speakerLabel);
		std::string token;
		std::vector<std::string> tokens;
		while (std::getline(ss, token, ':'))
		{
			tokens.push_back(token);
		}

		if (tokens.size() == 7)
			if (tokens[0] == "urn" && tokens[1] == "itu" && tokens[2] == "bs" && tokens[3] == "2051" &&
				(std::stoi(tokens[4]) >= 0 || std::stoi(tokens[4]) < 10) && tokens[5] == "speaker")
				speakerLabel = tokens[6];

		// Rename the LFE channels, if requried.
		// See Rec. ITU-R BS.2127-0 sec 8.3
		if (speakerLabel == "LFE" || speakerLabel == "LFEL")
			speakerLabel = "LFE1";
		else if (speakerLabel == "LFER")
			speakerLabel = "LFE2";

		return speakerLabel;
	}
}
