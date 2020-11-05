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
	static inline std::vector<std::pair<double, double>> FindSector(double az)
	{
		double tol = 1e-10;
		if (insideAngleRange(az, 0., 30., tol))
			return std::vector<std::pair<double, double>>{ { 30., 0. }, { -1.,1. }, { 0.,1. }};
		else if (insideAngleRange(az, -30., 0., tol))
			return std::vector<std::pair<double, double>>{ { 0., -30. }, { 0.,1. }, { 1.,1. }};
		else if(insideAngleRange(az, -110., -30., tol))
			return std::vector<std::pair<double, double>>{ { -30., -110. }, { 1.,1. }, { 1.,-1. }};
		else if (insideAngleRange(az, 110., -110., tol))
			return std::vector<std::pair<double, double>>{ { -110., 110. }, { 1.,-1. }, { -1.,-1. }};
		else if (insideAngleRange(az, 30., 110., tol))
			return std::vector<std::pair<double, double>>{ { 110., 30. }, { -1.,-1. }, { -1.,1. }};

		return std::vector<std::pair<double, double>>{};
	}
	/*
		Find the sector to which a given azimuth angle belongs.
		See Rec. ITU-R BS.2127-0 sec. 10.1 pg 70
	*/
	static inline std::vector<std::pair<double, double>> FindCartSector(double az)
	{
		double tol = 1e-10;
		if (insideAngleRange(az, 0., 45., tol))
			return std::vector<std::pair<double, double>>{ { 30., 0. }, { -1.,1. }, { 0.,1. }};
		else if (insideAngleRange(az, -45., 0., tol))
			return std::vector<std::pair<double, double>>{ { 0., -30. }, { 0.,1. }, { 1.,1. }};
		else if (insideAngleRange(az, -135., -45., tol))
			return std::vector<std::pair<double, double>>{ { -30., -110. }, { 1.,1. }, { 1.,-1. }};
		else if (insideAngleRange(az, 135., -135., tol))
			return std::vector<std::pair<double, double>>{ { -110., 110. }, { 1.,-1. }, { -1.,-1. }};
		else if (insideAngleRange(az, 45., 135., tol))
			return std::vector<std::pair<double, double>>{ { 110., 30. }, { -1.,-1. }, { -1.,1. }};

		return std::vector<std::pair<double, double>>{};
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

		auto sector = FindSector(az);
		double az_l = sector[0].first;
		double az_r = sector[0].second;
		double x_l = sector[1].first;
		double y_l = sector[1].second;
		double x_r = sector[2].first;
		double y_r = sector[2].second;

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
		auto sector = FindCartSector(azDash);
		double az_l = sector[0].first;
		double az_r = sector[0].second;
		double x_l = sector[1].first;
		double y_l = sector[1].second;
		double x_r = sector[2].first;
		double y_r = sector[2].second;

		std::vector<std::vector<double>> mat = { {x_l, y_l},{x_r, y_r} };
		auto invMat = inverseMatrix(mat);
		auto g = std::vector<double>({ x * invMat[0][0] + y * invMat[1][0], x * invMat[0][1] + y * invMat[1][1] });
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
	static inline std::vector<double> whd2xyz(double w, double h, double d)
	{
		double s_xw = w < 180. ? std::sin(DEG2RAD * w * 0.5) : 1.;
		double s_yw = 0.5 * (1. - std::cos(DEG2RAD * w * 0.5));
		double s_zh = h < 180. ? std::sin(DEG2RAD * h * 0.5) : 1.;
		double s_yh = 0.5 * (1 - std::cos(DEG2RAD * h * 0.5));
		double s_yd = d;

		return std::vector<double>{s_xw, std::max(std::max(s_yw, s_yh), s_yd), s_zh};
	}

	/**
		Convert cartesian extent to polar extent
	*/
	static inline std::vector<double> xyz2whd(double s_x, double s_y, double s_z)
	{
		double w_sx = 2. * RAD2DEG * std::asin(s_x);
		double w_sy = 2. * RAD2DEG * std::acos(1. - 2.*s_y);
		double w = w_sx + s_x * std::max(w_sy - w_sx, 0.);

		double h_sz = 2. * RAD2DEG * std::asin(s_z);
		double h_sy = 2. * RAD2DEG * std::acos(1. - 2. * s_y);
		double h = h_sz + s_z * std::max(h_sy - h_sz, 0.);
		auto s_eq = whd2xyz(w, h, 0.);

		double d = std::max(0., s_y - s_eq[1]);

		return std::vector<double>{w, h, d};
	}

	/**
		Convert a cartesian source position and extent to polar position and polar extent
		See Rec. ITU-R BS.2127-0 sec. 10.2.2 pg 72
	*/
	static inline std::pair<PolarPosition, std::vector<double>> ExtentCartToPolar(double x, double y, double z, double s_x, double  s_y, double  s_z)
	{
		auto polarPosition = PointCartToPolar(CartesianPosition{ x,y,z });
		std::vector<std::vector<double>> diagS = { {s_x,0.,0.}, {0.,s_y,0.}, {0.,0.,s_z} };
		auto localCoordSystem = LocalCoordinateSystem(polarPosition.azimuth, polarPosition.elevation);
		auto M = multiplyMat(diagS, localCoordSystem);

		double s_xf = norm(std::vector<double>{M[0][0], M[1][0], M[2][0]});
		double s_yf = norm(std::vector<double>{M[0][1], M[1][1], M[2][1]});
		double s_zf = norm(std::vector<double>{M[0][2], M[1][2], M[2][2]});

		auto whd = xyz2whd(s_xf, s_yf, s_zf);

		return { polarPosition, whd };
	}

	/*
		Convert a metadata block from Cartesian to polar.

		See Rec. ITU-R BS.2127-0 sec. 10 pg 68.
	*/
	static inline void toPolar(ObjectMetadata& metadataBlock)
	{
		if (metadataBlock.cartesian)
		{
			// Update the position and the extent
			auto posWhd = ExtentCartToPolar(metadataBlock.cartesianPosition.x, metadataBlock.cartesianPosition.y, metadataBlock.cartesianPosition.z,
				metadataBlock.width, metadataBlock.height, metadataBlock.depth);
			metadataBlock.polarPosition = posWhd.first;
			metadataBlock.width = posWhd.second[0];
			metadataBlock.height = posWhd.second[1];
			metadataBlock.depth = posWhd.second[2];

			// Convert the divergence according to Rec. ITU-R BS.2127-0 sec. 10.3 pg.73
			// TODO: The equation given in this section gives strange results. Need to double check it
			// does not have a mistake.

			// Unflag as cartesian
			metadataBlock.cartesian = false;
		}
	}
}
