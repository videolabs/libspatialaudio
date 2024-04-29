/*############################################################################*/
/*#                                                                          #*/
/*#  ADM metadata conversions                                                #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      AdmConversions.h                                         #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "Tools.h"
#include "AdmMetadata.h"

namespace admrender {

	/*
		Map a source positioned between two azimuths to linear coordinates.
		Azimuth angles are expected in degrees.
	*/
	static inline double MapAzToLinear(double azL, double azR, double az)
	{
		double azMid = 0.5 * (azL + azR);
		double azRange = azR - azMid;
		double azRel = az - azMid;
		double g_r = 0.5 * (1. + std::tan(DEG2RAD * azRel) / std::tan(DEG2RAD * azRange));

		return 2. / M_PI * std::atan2(g_r, 1. - g_r);
	}

	/*
		Map a linear source coordinate to a polar angle between two azimuths.
		Azimuth angles are expected in degrees.
	*/
	static inline double MapLinearToAz(double azL, double azR, double x)
	{
		double azMid = 0.5 * (azL + azR);
		double azRange = azR - azMid;
		double gDash_l = std::cos(x * M_PI / 2.);
		double gDash_r = std::sin(x * M_PI / 2.);
		double g_r = gDash_r / (gDash_l + gDash_r);
		double azRel = RAD2DEG * std::atan(2. * (g_r - 0.5) * std::tan(DEG2RAD * azRange));

		return azMid + azRel;
	}

	/*
		Find the sector to which a given azimuth angle belongs.
		See Rec. ITU-R BS.2127-0 sec. 10.1 pg 70
	*/

	/**	Find the sector to which a given azimuth angle belongs.
	 *	See Rec. ITU-R BS.2127-0 sec. 10.1 pg 70
	 *  @param az		The azimuth to check
	 * @param sectorOut	A 2D array of size 3x2 holding the sector information
	 */
	static inline void FindSector(double az, double (&sectorOut)[3][2])
	{
		double tol = 1e-10;
		if (insideAngleRange(az, 0., 30., tol))
		{
			sectorOut[0][0] = 30.;
			sectorOut[0][1] = 0.;
			sectorOut[1][0] = -1;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 0.;
			sectorOut[2][1] = 1.;
		}
		else if (insideAngleRange(az, -30., 0., tol))
		{
			sectorOut[0][0] = 0.;
			sectorOut[0][1] = -30.;
			sectorOut[1][0] = 0.;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 1.;
			sectorOut[2][1] = 1.;
		}
		else if(insideAngleRange(az, -110., -30., tol))
		{
			sectorOut[0][0] = -30.;
			sectorOut[0][1] = -110.;
			sectorOut[1][0] = 1;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 1.;
			sectorOut[2][1] = -1.;
		}
		else if (insideAngleRange(az, 110., -110., tol))
		{
			sectorOut[0][0] = -110.;
			sectorOut[0][1] = 110.;
			sectorOut[1][0] = 1.;
			sectorOut[1][1] = -1.;
			sectorOut[2][0] = -1.;
			sectorOut[2][1] = -1.;
		}
		else if (insideAngleRange(az, 30., 110., tol))
		{
			sectorOut[0][0] = 110.;
			sectorOut[0][1] = 30.;
			sectorOut[1][0] = -1;
			sectorOut[1][1] = -1.;
			sectorOut[2][0] = -1.;
			sectorOut[2][1] = 1.;
		}
	}
	/*
		Find the sector to which a given azimuth angle belongs.
		See Rec. ITU-R BS.2127-0 sec. 10.1 pg 70
	*/
	static inline void FindCartSector(double az, double (&sectorOut)[3][2])
	{
		double tol = 1e-10;
		if (insideAngleRange(az, 0., 45., tol))
		{
			sectorOut[0][0] = 30.;
			sectorOut[0][1] = 0.;
			sectorOut[1][0] = -1;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 0.;
			sectorOut[2][1] = 1.;
		}
		else if (insideAngleRange(az, -45., 0., tol))
		{
			sectorOut[0][0] = 0.;
			sectorOut[0][1] = -30.;
			sectorOut[1][0] = 0.;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 1.;
			sectorOut[2][1] = 1.;
		}
		else if (insideAngleRange(az, -135., -45., tol))
		{
			sectorOut[0][0] = -30.;
			sectorOut[0][1] = -110.;
			sectorOut[1][0] = 1;
			sectorOut[1][1] = 1.;
			sectorOut[2][0] = 1.;
			sectorOut[2][1] = -1.;
		}
		else if (insideAngleRange(az, 135., -135., tol))
		{
			sectorOut[0][0] = -110.;
			sectorOut[0][1] = 110.;
			sectorOut[1][0] = 1.;
			sectorOut[1][1] = -1.;
			sectorOut[2][0] = -1.;
			sectorOut[2][1] = -1.;
		}
		else if (insideAngleRange(az, 45., 135., tol))
		{
			sectorOut[0][0] = 110.;
			sectorOut[0][1] = 30.;
			sectorOut[1][0] = -1;
			sectorOut[1][1] = -1.;
			sectorOut[2][0] = -1.;
			sectorOut[2][1] = 1.;
		}
	}

	/**
		Convert a polar position to cartesian.
		Note that this is not a traditional polar-cartesian conversion. In this case cartesian is
		related to the ADM metadata parameter.
		It should therefore generally not be used for coordinate system conversions. Use it for
		metadata conversions.
		See Rec. ITU-R BS.2127-0 sec. 10 for more details on this conversion
	*/
	static inline CartesianPosition PointPolarToCart(PolarPosition polar)
	{
		double az = polar.azimuth;
		double el = polar.elevation;
		double d = polar.distance;

		double elTop = 30.;
		double elDashTop = 45.;

		double z = 0.;
		double r_xy = 0.;

		if (std::abs(el) > elTop)
		{
			double elDash = elDashTop + (90. - elDashTop) * (std::abs(el) - elTop) / (90. - elTop);
			z = d * Sgn(el);
			r_xy = d * std::tan(DEG2RAD * (90. - elDash));
		}
		else
		{
			double elDash = elDashTop * el / elTop;
			z = d * std::tan(DEG2RAD * elDash);
			r_xy = d;
		}

		double sector[3][2];
		FindSector(az, sector);
		double az_l = sector[0][0];
		double az_r = sector[0][1];
		double x_l = sector[1][0];
		double y_l = sector[1][1];
		double x_r = sector[2][0];
		double y_r = sector[2][1];

		double azDash = relativeAngle(az_r, az);
		double azDash_l = relativeAngle(az_r, az_l);
		double p = MapAzToLinear(azDash_l, az_r, azDash);
		double x = r_xy * (x_l + p * (x_r - x_l));
		double y = r_xy * (y_l + p * (y_r - y_l));

		return CartesianPosition{ x,y,z };
	}

	/**
		Convert a cartesian position to polar.
		Note that this is not a traditional polar-cartesian conversion. In this case cartesian is
		related to the ADM metadata parameter.
		It should therefore generally not be used for coordinate system conversions. Use it for
		metadata conversions.
		See Rec. ITU-R BS.2127-0 sec. 10 for more details on this conversion
	*/
	static inline PolarPosition PointCartToPolar(CartesianPosition cart)
	{
		double x = cart.x;
		double y = cart.y;
		double z = cart.z;

		double elTop = 30.;
		double elDashTop = 45.;

		double tol = 1e-10;

		if (std::abs(x) < tol && std::abs(y) < tol)
		{
			if (std::abs(z) < tol)
				return PolarPosition{ 0.,0.,0. };
			else
				return PolarPosition{ 0., 90. * Sgn(z), std::abs(z) };
		}

		double azDash = -RAD2DEG * atan2(x, y);
		double sector[3][2];
		FindCartSector(azDash, sector);
		double az_l = sector[0][0];
		double az_r = sector[0][1];
		double x_l = sector[1][0];
		double y_l = sector[1][1];
		double x_r = sector[2][0];
		double y_r = sector[2][1];

		double det = x_l * y_r - y_l * x_r;
		double invMat[2][2] = { {y_r / det, -y_l / det},{-x_r / det, x_l / det} };
		double g[2] = {x * invMat[0][0] + y * invMat[1][0], x * invMat[0][1] + y * invMat[1][1]};
		double r_xy = g[0] + g[1];
		double azDash_l = relativeAngle(az_r, az_l);
		double azRel = MapLinearToAz(azDash_l, az_r, g[1] / r_xy);
		double az = relativeAngle(-180., azRel);
		double elDash = RAD2DEG * std::atan(z / r_xy);

		double el = 0.;
		double d = 0.;
		if (std::abs(elDash) > elDashTop)
		{
			el = std::abs(elTop + (90. - elTop)*(std::abs(elDash) - elDashTop)/(90. - elDashTop)) * Sgn(elDash);
			d = std::abs(z);
		}
		else
		{
			el = elDash * elTop / elDashTop;
			d = r_xy;
		}

		return PolarPosition{ az, el, d };
	}

	/**
		Convert polar extent to cartesian extent
	*/
	static inline void whd2xyz(double w, double h, double d, double& x, double& y, double& z)
	{
		double s_xw = w < 180. ? std::sin(DEG2RAD * w * 0.5) : 1.;
		double s_yw = 0.5 * (1. - std::cos(DEG2RAD * w * 0.5));
		double s_zh = h < 180. ? std::sin(DEG2RAD * h * 0.5) : 1.;
		double s_yh = 0.5 * (1 - std::cos(DEG2RAD * h * 0.5));
		double s_yd = d;

		x = s_xw;
		y = std::max(std::max(s_yw, s_yh), s_yd);
		z = s_zh;
	}

	/**
		Convert cartesian extent to polar extent
	*/
	static inline void xyz2whd(double s_x, double s_y, double s_z, double& w, double& h, double& d)
	{
		double w_sx = 2. * RAD2DEG * std::asin(s_x);
		double w_sy = 2. * RAD2DEG * std::acos(1. - 2.*s_y);
		w = w_sx + s_x * std::max(w_sy - w_sx, 0.);

		double h_sz = 2. * RAD2DEG * std::asin(s_z);
		double h_sy = 2. * RAD2DEG * std::acos(1. - 2. * s_y);
		h = h_sz + s_z * std::max(h_sy - h_sz, 0.);
		double s_eq[3];
		whd2xyz(w, h, 0., s_eq[0], s_eq[1], s_eq[2]);

		d = std::max(0., s_y - s_eq[1]);
	}

	/**
		Convert a cartesian source position and extent to polar position and polar extent
		See Rec. ITU-R BS.2127-0 sec. 10.2.2 pg 72
	*/
	static inline void ExtentCartToPolar(double x, double y, double z, double s_x, double  s_y, double  s_z,
		PolarPosition& polarPosition, double (&whd)[3])
	{
		polarPosition = PointCartToPolar(CartesianPosition{ x,y,z });
		double diagS[3] = { s_x, s_y, s_z };
		double localCoordSystem[3][3];
		LocalCoordinateSystem(polarPosition.azimuth, polarPosition.elevation, localCoordSystem);
		// Multiply diag([s_x,s_y,s_x])*localCoordSystem
		double M[3][3];
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				M[i][j] = diagS[i] * localCoordSystem[i][j];

		double s_xf = std::sqrt(M[0][0] * M[0][0] + M[1][0] * M[1][0] + M[2][0] * M[2][0]);
		double s_yf = std::sqrt(M[0][1] * M[0][1] + M[1][1] * M[1][1] + M[2][1] * M[2][1]);
		double s_zf = std::sqrt(M[0][2] * M[0][2] + M[1][2] * M[1][2] + M[2][2] * M[2][2]);

		xyz2whd(s_xf, s_yf, s_zf, whd[0], whd[1], whd[2]);
	}

	/*
		Convert a metadata block from Cartesian to polar.

		See Rec. ITU-R BS.2127-0 sec. 10 pg 68.
	*/
	static inline void toPolar(const ObjectMetadata& inMetadataBlock, ObjectMetadata& outMetadataBlock)
	{
		outMetadataBlock = inMetadataBlock;
		if (inMetadataBlock.cartesian)
		{
			// Update the position and the extent
			double whd[3];
			ExtentCartToPolar(inMetadataBlock.cartesianPosition.x, inMetadataBlock.cartesianPosition.y, inMetadataBlock.cartesianPosition.z,
				inMetadataBlock.width, inMetadataBlock.height, inMetadataBlock.depth, outMetadataBlock.polarPosition, whd);
			outMetadataBlock.width = whd[0];
			outMetadataBlock.height = whd[1];
			outMetadataBlock.depth = whd[2];

			// Convert the divergence according to Rec. ITU-R BS.2127-0 sec. 10.3 pg.73
			// TODO: The equation given in this section gives strange results. Need to double check it
			// does not have a mistake.

			// Unflag as cartesian
			outMetadataBlock.cartesian = false;
		}
	}
}
