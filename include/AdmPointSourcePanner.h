/*############################################################################*/
/*#                                                                          #*/
/*#  A point source panner for ADM renderer.                                 #*/
/*#  CAdmPointSourcePanner - ADM Point Source Panner						 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePanner.h                                   #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _ADM_POINT_SOURCE_PANNER_H
#define    _ADM_POINT_SOURCE_PANNER_H

#include "AdmMetadata.h"
#include "AdmLayouts.h"
#include "AdmUtils.h"
#include <assert.h>
#include <set>
#include <algorithm>
#include <memory>
#include <limits>
#include <regex>
#include <map>

namespace admrender {

	struct DirectDiffuseGains {
		std::vector<float> direct;
		std::vector<float> diffuse;
	};

	// Holds the output channel indices and their polar coordinates
	class RegionHandler
	{
	public:
		RegionHandler(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
			: channelInds(chanInds), polarPositions(polPos)
		{

		}

		std::vector<unsigned int> channelInds;
		std::vector<PolarPosition> polarPositions;
		// Tolerance value used to check near zero values
		double tol = 1e-6;
	};

	// Holds a triplet of speaker indices and calculates the gain for a given source direction
	class Triplet : public RegionHandler
	{
	public:
		Triplet(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
			: RegionHandler(chanInds, polPos)
		{
			// calculate the unit vectors in each of the loudspeaker directions
			double unitVectors[3][3];
			for (int i = 0; i < 3; ++i)
			{
				polPos[i].distance = 1.;
				CartesianPosition cartPos = PolarToCartesian(polPos[i]);
				unitVectors[i][0] = cartPos.x;
				unitVectors[i][1] = cartPos.y;
				unitVectors[i][2] = cartPos.z;
			}
			// Calculate the inverse of the matrix holding the unit vectors
			double det = 0.; // determinant
			for (int i = 0; i < 3; i++)
				det += (unitVectors[0][i] * (unitVectors[1][(i + 1) % 3] * unitVectors[2][(i + 2) % 3] - unitVectors[1][(i + 2) % 3] * unitVectors[2][(i + 1) % 3]));
			double invDet = 1. / det;

			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
				{
					inverseDirections[i][j] = ((unitVectors[(j + 1) % 3][(i + 1) % 3] * unitVectors[(j + 2) % 3][(i + 2) % 3])
						- (unitVectors[(j + 1) % 3][(i + 2) % 3] * unitVectors[(j + 2) % 3][(i + 1) % 3])) * invDet;
				}

		}

		std::vector<double> calculateGains(std::vector<double> directionUnitVec)
		{
			std::vector<double> gains(3, 0.);

			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 3; ++j)
					gains[i] += directionUnitVec[j] * inverseDirections[j][i];

			// if any of the gains is negative then return zero
			for (int i = 0; i < 3; ++i)
				if (gains[i] < -tol)
					return { 0.,0.,0. };

			// Normalise
			double vecNorm = norm(gains);

			for (int i = 0; i < 3; ++i)
				gains[i] /= vecNorm;

			return gains;
		}

	private:
		// Inverse of the matrix holding the triplet unit vectors
		double inverseDirections[3][3] = { {0.} };
	};

	class VirtualNgon : public RegionHandler
	{
	public:
		VirtualNgon(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos, PolarPosition centrePosition)
			: RegionHandler(chanInds, polPos)
		{
			nCh = (unsigned int)chanInds.size();
			// See Rec. ITU-R BS.2127-0 sec. 6.1.3.1 at pg.27
			downmixCoefficient = 1. / sqrt(double(nCh));

			// Order the speakers so that they go anti-clockwise from the point of view of the origin to the centre speaker
			std::vector<unsigned int> vertOrder = getNgonVectexOrder(polPos, centrePosition);

			// Make a triplet from each adjacent pair of speakers and the virtual centre speaker
			for (unsigned int iCh = 0; iCh < nCh; ++iCh)
			{
				unsigned int spk1 = vertOrder[iCh];
				unsigned int spk2 = vertOrder[(iCh + 1) % nCh];
				std::vector<unsigned int>channelIndSubset = { spk1,spk2,nCh };// { orderCh[spk1], orderCh[spk2], centreInd };
				std::vector<PolarPosition> tripletPositions(3);
				tripletPositions[0] = polPos[spk1];
				tripletPositions[1] = polPos[spk2];
				tripletPositions[2] = centrePosition;
				triplets.push_back(Triplet(channelIndSubset, tripletPositions));
			}
		}

		std::vector<double> calculateGains(std::vector<double> directionUnitVec)
		{
			std::vector<double> gains(nCh, 0.);

			unsigned int nTriplets = (unsigned int)triplets.size();

			unsigned int iTriplet = 0;
			std::vector<double> tripletGains;
			// All gains must be above this value for the triplet to be valid
			// Select a very small negative number to account for rounding errors
			for (iTriplet = 0; iTriplet < nTriplets; ++iTriplet)
			{
				tripletGains = triplets[iTriplet].calculateGains(directionUnitVec);
				if (tripletGains[0] > -tol && tripletGains[1] > -tol && tripletGains[2] > -tol)
				{
					break;
				}
			}
			// If no triplet is found then return the vector of zero gains
			if (iTriplet == nTriplets)
				return gains;

			std::vector<unsigned int> tripletInds = triplets[iTriplet].channelInds;
			for (int i = 0; i < 2; ++i)
				gains[tripletInds[i]] += tripletGains[i];
			for (unsigned int i = 0; i < nCh; ++i)
				gains[i] += downmixCoefficient * tripletGains[2];

			double gainNorm = 1. / norm(gains);
			for (unsigned int i = 0; i < nCh; ++i)
				gains[i] *= gainNorm;

			return gains;
		}

	private:
		std::vector<Triplet> triplets;
		double downmixCoefficient;
		// The number of channels in the Ngon
		unsigned int nCh = 0;
	};

	class QuadRegion : public RegionHandler
	{
	public:
		QuadRegion(std::vector<unsigned int> chanInds, std::vector<PolarPosition> polPos)
			: RegionHandler(chanInds, polPos)
		{
			// Get the centre position of the four points
			CartesianPosition centrePosition;
			centrePosition.x = 0.;
			centrePosition.y = 0.;
			centrePosition.z = 0.;
			std::vector<CartesianPosition> cartesianPositions;
			for (int i = 0; i < 4; ++i)
			{
				cartesianPositions.push_back(PolarToCartesian(polPos[i]));
				centrePosition.x += cartesianPositions[i].x / 4.;
				centrePosition.y += cartesianPositions[i].y / 4.;
				centrePosition.z += cartesianPositions[i].z / 4.;
			}
			// Get the order of the loudspeakers
			PolarPosition centrePolarPosition = CartesianToPolar(centrePosition);
			vertOrder = getNgonVectexOrder(polPos, centrePolarPosition);
			for (int i = 0; i < 4; ++i)
			{
				m_quadVertices.push_back(cartesianPositions[vertOrder[i]]);
			}

			// Calculate the polynomial coefficients
			polynomialXProdX = calculatePolyXProdTerms(m_quadVertices);
			// For the Y terms rotate the order in which the vertices are sent
			polynomialXProdY = calculatePolyXProdTerms({ m_quadVertices[1],m_quadVertices[2],m_quadVertices[3],m_quadVertices[0] });
		}

		double getPanningValue(std::vector<double> directionUnitVec, std::vector<std::vector<double>> xprodTerms)
		{
			// Take the dot product with the direction vector to get the polynomial terms
			double a = dotProduct(xprodTerms[0], directionUnitVec);
			double b = dotProduct(xprodTerms[1], directionUnitVec);
			double c = dotProduct(xprodTerms[2], directionUnitVec);

			std::vector<double> roots(2, -1.);

			// No quadratic term
			if (abs(a) < tol)
			{
				return -c / b;
			}

			// Find the roots of the quadratic equation
			// TODO: Add some checks here for cases when values are close to zero
			double d = b * b - 4 * a * c;
			if (d >= 0.)
			{
				double sqrtTerm = sqrt(d);
				roots[0] = (-b + sqrtTerm) / (2 * a);
				roots[1] = (-b - sqrtTerm) / (2 * a);
				for (int i = 0; i < 2; ++i)
				{
					if (roots[i] >= 0. && roots[i] <= 1.0)
						return roots[i];
				}
			}

			return -1.; // if no gain was found between 0 and 1 then return -1
		}

		std::vector<double> calculateGains(std::vector<double> directionUnitVec)
		{
			std::vector<double> gains_tmp(4, 0.);
			std::vector<double> gains(4, 0.);
			// Calculate the gains in anti-clockwise order
			double x = getPanningValue(directionUnitVec, polynomialXProdX);
			double y = getPanningValue(directionUnitVec, polynomialXProdY);

			// Check that both of the panning values are between zero and one and that gP.d > 0
			if (x > 1. + tol || x < -tol || y > 1. + tol || y < -tol)
				return gains; // return zero gains
			gains_tmp = { (1. - x) * (1. - y),x * (1. - y),x * y,(1. - x) * y };

			std::vector<double> gP(3, 0.);
			for (int i = 0; i < 4; ++i)
			{
				gP[0] += gains_tmp[i] * m_quadVertices[i].x;
				gP[1] += gains_tmp[i] * m_quadVertices[i].y;
				gP[2] += gains_tmp[i] * m_quadVertices[i].z;
			}
			double dirCheck = dotProduct(gP, directionUnitVec);
			if (dirCheck < 0.)
				return std::vector<double>(4, 0.);

			double gainNorm = 1. / norm(gains_tmp);
			for (int i = 0; i < 4; ++i)
				gains_tmp[i] *= gainNorm;

			// Map the gains to the order the channels were input
			for (int i = 0; i < 4; ++i)
				gains[vertOrder[i]] = gains_tmp[i];

			return gains;
		}

	private:
		std::vector<std::vector<double>> calculatePolyXProdTerms(std::vector<CartesianPosition> quadVertices)
		{
			// See ITU Rec. ITU-R BS.2127-0 pg 24 last equation
			std::vector<double> p1 = { quadVertices[0].x,quadVertices[0].y,quadVertices[0].z };
			std::vector<double> p2 = { quadVertices[1].x,quadVertices[1].y,quadVertices[1].z };
			std::vector<double> p3 = { quadVertices[2].x,quadVertices[2].y,quadVertices[2].z };
			std::vector<double> p4 = { quadVertices[3].x,quadVertices[3].y,quadVertices[3].z };

			std::vector<std::vector<double>> polyXProdTerms;
			// Quadratic term
			polyXProdTerms.push_back(crossProduct(vecSubtract(p2, p1), vecSubtract(p3, p4)));

			// Linear term
			polyXProdTerms.push_back(vecSum(crossProduct(p1, vecSubtract(p3, p4)), crossProduct(vecSubtract(p2, p1), p4)));

			// Constant term
			polyXProdTerms.push_back(crossProduct(p1, p4));

			return polyXProdTerms;
		}

		// The coordinates of the vertices in anti-clockwise order start from the bottom left.
		std::vector<CartesianPosition> m_quadVertices;
		// The ordering of the input speaker coordinates needed to put them in the right order
		std::vector<unsigned int> vertOrder;
		// The cross product terms from the final equation in section 6.1.2.3.2 (pg 24)
		std::vector<std::vector<double>> polynomialXProdX;
		std::vector<std::vector<double>> polynomialXProdY;
	};

	struct LayoutRegions
	{
		std::vector<Triplet> triplets;
		std::vector<QuadRegion> quadRegions;
		std::vector<VirtualNgon> virtualNgons;
	};

	class ChannelLockHandler
	{
	public:
		ChannelLockHandler(Layout layout);
		~ChannelLockHandler();

		/**
			If the Object has a valid channelLock distance then determines the new direction of the object
		*/
		PolarPosition handle(ChannelLock channelLock, PolarPosition polarDirection);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
	};

	class ZoneExclusionHandler
	{
	public:
		ZoneExclusionHandler(Layout layout);
		~ZoneExclusionHandler();

		/**
			Calculate the gain vector once the appropriate loudspeakers have been exlcuded
		*/
		std::vector<double> handle(std::vector<PolarExclusionZone> exclusionZones, std::vector<double> gains);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		std::vector<std::vector<std::set<unsigned int>>> m_downmixMapping;
		std::vector<std::vector<unsigned int>> m_downmixMatrix;

		int getLayerPriority(std::string inputChannelName, std::string outputChannelName);
	};

	class CAdmPointSourcePannerGainCalc
	{
	public:
		CAdmPointSourcePannerGainCalc(Layout layout);
		~CAdmPointSourcePannerGainCalc();

		/**
			Calculate the gains to be applied to a mono signal in order to place it in the target
			speaker layout
		*/
		std::vector<double> calculateGains(PolarPosition directionUnitVec);

	private:
		// The loudspeaker layout 
		Layout outputLayout;
		// The layout of the extra loudspeakers used to fill in any gaps in the array
		Layout extraSpeakersLayout;

		// Flag if the output is stereo (0+2+0) and treat as a special case
		bool isStereo = false;

		std::vector<unsigned int> downmixMapping;

		// All of the region handlers for the different types
		LayoutRegions regions;

		/**
			Return the extra loudspeakers needed to fill in the gaps in the array.
			This currently works for the supported arrays: 0+5+0, 0+4+0, 0+7+0
			See Rec. ITU-R BS.2127-0 pg. 27
		*/
		Layout calculateExtraSpeakersLayout(Layout layout);

		/**
			Calculate the gains for the panning layout. In most cases this will be the same
			as the output layout but in the case of 0+2+0 the panning layout is 0+5+0
		*/
		std::vector<double> _calculateGains(PolarPosition directionUnitVec);
	};


	/**
		A class to calculate the gains to be applied to a set of loudspeakers for DirectSpeaker processing
	*/
	class CAdmDirectSpeakersGainCalc
	{
	public:
		CAdmDirectSpeakersGainCalc(Layout layoutWithLFE);
		~CAdmDirectSpeakersGainCalc();

		/**
			Calculate the gain vector corresponding to the metadata input
		*/
		std::vector<double> calculateGains(DirectSpeakerMetadata metadata);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		CAdmPointSourcePannerGainCalc m_pointSourcePannerGainCalc;

		bool isLFE(DirectSpeakerMetadata metadata);

		/**
			Find the closest speaker in the layout within the tolerance bounds
			set
		*/
		int findClosestWithinBounds(DirectSpeakerPolarPosition direction, double tol);

		std::map<std::string, std::string> m_LfeSubstitutions;
		std::string _nominalSpeakerLabel(
			const std::string& label) {
			std::string ret = label;
			std::smatch idMatch;
			if (std::regex_search(label, idMatch, SPEAKER_URN_REGEX)) {
				ret = idMatch[1].str();
			}
			if (m_LfeSubstitutions.count(label)) {
				ret = m_LfeSubstitutions.at(label);
			}
			return ret;
		}

		const std::regex SPEAKER_URN_REGEX =
			std::regex("^urn:itu:bs:2051:[0-9]+:speaker:(.*)$");
	};

	/**
		This class calculates the gains required to spatialise a mono signal.
	*/
	class CAdmPointSourcePanner
	{
	public:
		CAdmPointSourcePanner(Layout targetLayout);
		~CAdmPointSourcePanner();

		/**
			Spatialises the input pIn and splits it between direct ppDirect and diffuse ppDiffuse
			signals based on the parameters set in the metadata.

			** The outputs are *added* to the ppDirect and ppDiffuse buffers so be sure to clear the buffers
			before passing them to this for the first time **
		*/
		void ProcessAccumul(ObjectMetadata metadata, float* pIn, std::vector<std::vector<float>>& ppDirect, std::vector<std::vector<float>>& ppDiffuse, unsigned int nSamples);

	private:
		Layout m_layout;
		// Number of channels in the layout
		unsigned int m_nCh;
		// The vector of gains applied the last time the position data was updated
		std::vector<double> m_gains;
		// Flag if it is the first processing frame
		bool m_bFirstFrame = true;
		// The gain calculator
		CAdmPointSourcePannerGainCalc m_gainCalculator;

		ChannelLockHandler channelLockHandler;
		ZoneExclusionHandler zoneExclusionHandler;
		
		/**
			Get the diverged source positions and directions
		*/
		std::pair<std::vector<PolarPosition>, std::vector<double>> divergedPositionsAndGains(ObjectDivergence divergence, PolarPosition polarDirection);
	};
}

#endif //_ADM_POINT_SOURCE_PANNER_H
