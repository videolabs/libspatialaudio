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
#include "assert.h"

#include "Coordinates.h"

#define DEG2RAD M_PI/180.
#define RAD2DEG 180./M_PI

// Some Helper Functions

/** Convert from cartesian to polar coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
 *	+ve X = right
 *	+ve Y = front
 *	+ve Z = up
 *	Note that this convention differs from the convention used for Ambisonics.
 *
 * @param cartesian		Cartesian position to be converted to ADM polar.
 * @return				The ADM-convention polar coordinates.
 */
static inline PolarPosition CartesianToPolar(CartesianPosition cartesian)
{
	PolarPosition sphCoords;
	double x = cartesian.x;
	double y = cartesian.y;
	double z = cartesian.z;
	sphCoords.azimuth = -RAD2DEG * std::atan2(x, y);
	sphCoords.elevation = RAD2DEG * std::atan2(z, std::sqrt(x * x + y * y));
	sphCoords.distance = std::sqrt(x * x + y * y + z * z);

	return sphCoords;
};
static inline void CartesianToPolar(const std::vector<double>& cartVec, std::vector<double>& polVec)
{
	CartesianPosition cartPos = { cartVec[0],cartVec[1],cartVec[2] };
	auto sphPos = CartesianToPolar(cartPos);
	polVec[0] = sphPos.azimuth;
	polVec[1] = sphPos.elevation;
	polVec[2] = sphPos.distance;
};

/** Convert from polar to cartesian coordinates. See Rec. ITU-R BS.2127-0 pg 33 for conversions used.
 *	0 az = front, +ve az = anti-clockwise
 *	0 el = front, +ve el = up
 *	Angles are expected in degrees.
 * @param polar		The polar angle to be converted to ADM-convention cartesian.
 * @return			ADM-convention cartesian coordinates.
 */
static inline CartesianPosition PolarToCartesian(PolarPosition polar)
{
	CartesianPosition cartCoords;

	double az = DEG2RAD * polar.azimuth;
	double el = DEG2RAD * polar.elevation;
	double d = polar.distance;
	cartCoords.x = sin(-az) * cos(el) * d;
	cartCoords.y = cos(-az) * cos(el) * d;
	cartCoords.z = sin(el) * d;

	return cartCoords;
};
static inline void PolarToCartesian(const std::vector<double>& polar, std::vector<double>& cartesian)
{
	double az = DEG2RAD * polar[0];
	double el = DEG2RAD * polar[1];
	double d = polar[2];
	cartesian[0] = std::sin(-az) * std::cos(el) * d;
	cartesian[1] = std::cos(-az) * std::cos(el) * d;
	cartesian[2] = std::sin(el) * d;
};

/** Returns the norm of a vector.
 * @param vec	Vector to have its norm calculated.
 * @return		Norm of the input vector.
 */
static inline double norm(const std::vector<double>& vec)
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

/** Returns the norm of a vector.
 * @param vec	Pointer to the vector.
 * @param n		Number of entries in the vector.
 * @return		Norm of the input vector.
 */
static inline double norm(const double* vec, int n)
{
	double vecNorm = 0.;
	for (int i = 0; i < n; ++i)
		vecNorm += vec[i] * vec[i];
	vecNorm = std::sqrt(vecNorm);
	return vecNorm;
}

/** Returns a rotation matrix for given yaw, pitch, roll (in degrees).
 *  Rotation is applied in the order yaw, pitch then roll.
 * @param yaw		Yaw rotation angle in degrees.
 * @param pitch		Pitch rotation angle in degrees.
 * @param roll		Roll rotation angle in degrees.
 * @param rotMat	Pointer to the 3x3 rotation-matrix stored in row-major format.
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

/** Converts an angle in degrees to the range 0 to 360.
 * @param input		Any angle in degrees.
 * @return			The input angle convert to the range 0 to 360 degrees.
 */
static inline double convertToRange360(double input)
{
	double out = input;
	int i = 0;
	while (out < -1e-10 || out >= 360. + 1e-10)
		out += i++ * 360.;
	return out;
}

/** Converts an angle in degrees to the range -180 to 180.
 * @param input		Any angle in degrees.
 * @return			Angle converted to the range -180 to 180.
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

/** Find an equivalent angle to y which is greater than or equal to x.
 *  See Rec. ITU-R BS.2127-0 sec. 6.7 pg. 33
 * @param x		Angle that the output must be greater than
 * @param y		Angle to be processed
 * @return		Angle equivalent to y such that it is >= x
 */
static inline double relativeAngle(double x, double y)
{
	while (y - 360. >= x)
		y -= 360.;

	while (y < x)
		y += 360.;

	return y;
}

/** Returns the element-wise sum of two vectors of the same length.
 * @param a		First vector to add
 * @param b		Second vector to add
 * @return		Output = a + b
 */
static inline std::vector<double> vecSum(const std::vector<double>& a, const std::vector<double>& b)
{
	std::vector<double> c(a.size(), 0.);
	std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::plus<double>());
	return c;
}

/** Subtracts vector b from vector a in an element-wise manner.
 * @param a		First vector to add
 * @param b		Second vector to add
 * @return		Output = a - b
 */
static inline std::vector<double> vecSubtract(const std::vector<double>& a, const std::vector<double>& b)
{
	std::vector<double> c(a.size(), 0.);
	std::transform(a.begin(), a.end(), b.begin(), c.begin(), std::minus<double>());
	return c;
}

/** Returns the dot-product of two 3D vectors.
 * @param a		First vector of length 3.
 * @param b		Second vector of length 3.
 * @return		Dot-product of a and b.
 */
static inline double dotProduct(const std::vector<double>& a, const std::vector<double>& b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

/**Returns the cross-product of two 3D vectors.
 * @param a		First vector of length 3.
 * @param b		Second vector of length 3.
 * @return		Cross-product of a and b.
 */
static inline std::vector<double> crossProduct(const std::vector<double>& a, const std::vector<double>& b)
{
	std::vector<double> c(3, 0.);

	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];

	return c;
}	

/** Get the sign of a number.
 * @param x		The number to get the sign of.
 * @return		Returns -1 for negative numbers, +1 for positive numbers and 0 otherwise.
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

/** Check if the input x is inside a specified range. See Rec. ITU-R BS.2127-0 sec. 6.2.
 * @param x				The angle to check.
 * @param startAngle	The start angle of the range.
 * @param endAngle		The end angle of the range.
 * @param tol			Optional tolerance value
 * @return				Returns true if x is inside the specified range, including if equal to the start/end values.
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

/** Matrix multiplication of two matrices C = A*B.
 *  This function allocated memory so should not be used during real-time processing. Instead, see its overload.
 * @param A		First matrix to multiply.
 * @param B		Second matrix to multiply.
 * @return		Matrix product of A and B.
 */
template<typename T>
static inline std::vector<std::vector<T>> multiplyMat(const std::vector<std::vector<T>>& A, const std::vector<std::vector<T>>& B)
{
	size_t rowsA = A.size();
	size_t colsA = A[0].size();
	size_t colsB = B[0].size();
	std::vector<std::vector<T>> ret(rowsA, std::vector<T>(colsB, 0.));

	for (size_t i = 0; i < rowsA; ++i)
		for (size_t j = 0; j < colsB; ++j)
			for (size_t k = 0; k < colsA; ++k)
				ret[i][j] += A[i][k] * B[k][j];

	return ret;
}

/** Matrix multiplication of two matrices C = A*B
 * @tparam T	The data type of the matrices (float, double, int).
 * @param A		First matrix to multiply.
 * @param B		Second matrix to multiply.
 * @param C		Matrix product A*B
 */
template<typename T>
static inline void multiplyMat(const std::vector<std::vector<T>>& A, const std::vector<std::vector<T>>& B, std::vector<std::vector<T>>& C)
{
	size_t rowsA = A.size();
	size_t colsA = A[0].size();
	size_t colsB = B[0].size();
	assert(C.size() == rowsA && C[0].size() == colsB); // Destination matrix C must match the size of the product of A and B

	// Clear the destination matrix
	for (size_t i = 0; i < rowsA; ++i)
		for (size_t j = 0; j < colsB; ++j)
			C[i][j] = 0.f;

	for (size_t i = 0; i < rowsA; ++i)
		for (size_t j = 0; j < colsB; ++j)
			for (size_t k = 0; k < colsA; ++k)
				C[i][j] += A[i][k] * B[k][j];
}

/** Multiply a matrix to a vector y = Ax.
 * @param A		Matrix to multiply
 * @param x		Vector to multiply. Must have the same number of rows as columns in A.
 * @param y		Matrix-vector product y = A*x.
 */
static inline void multiplyMatVec(const std::vector<std::vector<double>>& A, const std::vector<double>& x, std::vector<double>& y)
{
	size_t rowsA = A.size();
	size_t colsA = A[0].size();

	assert(y.size() == rowsA);

	for (size_t i = 0; i < colsA; ++i)
		y[i] = 0.;

	for (size_t i = 0; i < rowsA; ++i)
		for (size_t k = 0; k < colsA; ++k)
			y[i] += A[i][k] * x[k];
}

/** Calculate the inverse of a square matrix of size 2x2.
 * @param mat	The 2x2 matrix to calculate the inverse of.
 * @return		Inverse matrix of the input.
 */
static inline std::vector<std::vector<double>> inverseMatrix2x2(const std::vector<std::vector<double>>& mat)
{
	double a = mat[0][0];
	double b = mat[0][1];
	double c = mat[1][0];
	double d = mat[1][1];
	double det = a * d - b * c;

	return { {d / det, -b / det},{-c / det, a / det} };
}

/** Calculate the inverse of a square matrix.
 * @param mat	Square matrix.
 * @return		Inverse of the input square matrix.
 */
static inline std::vector<std::vector<double>> inverseMatrix(const std::vector<std::vector<double>>& mat)
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

/** Get the rotation matrix required to convert a point from one coordinate system to another. See Rec. ITU-R BS.2127-0 sec. 6.8.
 * @param azInDegrees	The azimuth of the desired coordiante system.
 * @param elInDegrees	The elevation of the desired coordinate system.
 * @param rotMat		The rotation matrix that will rotate a point to the desired coordinate system.
 */
static inline void LocalCoordinateSystem(double azInDegrees, double elInDegrees, double (&rotMat)[3][3])
{
	CartesianPosition cartPos;
	PolarPosition polPos;

	polPos.azimuth = azInDegrees - 90.;
	polPos.elevation = 0.;
	polPos.distance = 1.;
	cartPos = PolarToCartesian(polPos);
	rotMat[0][0] = cartPos.x;
	rotMat[0][1] = cartPos.y;
	rotMat[0][2] = cartPos.z;

	polPos.azimuth = azInDegrees;
	polPos.elevation = elInDegrees;
	cartPos = PolarToCartesian(polPos);
	rotMat[1][0] = cartPos.x;
	rotMat[1][1] = cartPos.y;
	rotMat[1][2] = cartPos.z;

	polPos.azimuth = azInDegrees;
	polPos.elevation = elInDegrees + 90.;
	cartPos = PolarToCartesian(polPos);
	rotMat[2][0] = cartPos.x;
	rotMat[2][1] = cartPos.y;
	rotMat[2][2] = cartPos.z;
}
static inline void LocalCoordinateSystem(double azInDegrees, double elInDegrees, std::vector<std::vector<double>>& rotMat)
{
	double rotMatTmp[3][3] = { {rotMat[0][0],rotMat[0][1],rotMat[0][2]},
		{rotMat[1][0],rotMat[1][1],rotMat[1][2]},
		{rotMat[2][0],rotMat[2][1],rotMat[2][2]} };
	LocalCoordinateSystem(azInDegrees, elInDegrees, rotMatTmp);
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			rotMat[i][j] = rotMatTmp[i][j];
}

/** lamp a value between a minimum and a maximum.
 * @param val		The value to clamp.
 * @param minVal	The minimum value of the clamp.
 * @param maxVal	The maximum value of the clamp.
 * @return			The clamped value.
 */
static inline double clamp(double val, double minVal, double maxVal)
{
	return std::min(maxVal, std::max(val, minVal));
}

/** Apply piecewise linear interpolation to a value.
 *  The size of fromVals and toVals must match.
 *  The elements of fromVals should be ascending order.
 * @param val		The value to be interpolated. If it is outside the range of values in fromValues it is returned unchanged.
 * @param fromVals	The from values for the the piecewise interpolation.
 * @param toVals	The to-values for the piecewise interpolation.
 * @return			The interpolated value from its position in the fromValue to the corresponding toValue.
 */
static inline double interp(double val, const std::vector<double>& fromVals, const std::vector<double>& toVals)
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

/** Returns true if string1 contains string2. This check is case sensitive
 * @param string1	The string to be checked
 * @param string2	The substring to check for
 * @return			Returns true if string1 contains string2
 */
static inline bool stringContains(const std::string& string1, const std::string& string2)
{
	return string1.find(string2) != std::string::npos;
}
