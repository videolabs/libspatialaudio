/*############################################################################*/
/*#                                                                          #*/
/*#  A polar extent panner													 #*/
/*#                                                                          #*/
/*#  Filename:      PolarExtentPanner.h	                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          21/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "AdmMetadata.h"
#include "AdmLayouts.h"
#include "AdmUtils.h"
#include "AdmPointSourcePannerGainCalc.h"

/*
	All of the class defined here are used for the polar extent panning defined
	in Rec. ITU-R BS.2127-0 section 7.3.8 (pg 46).

	It is made up of a PolarExtentHandler, which uses a PolarExtentPanner, which
	in turn uses a SpreadPanner and a PointSourcePanner
*/

namespace admrender {

	/*
		The spread panner defines a set of positions of virtual sources on the sphere
		and calculates the corresponding point source panning gain vector for each of them.
		Then, based on the position, width and height it calculates a set of weights corresponding
		to each of the virtual source directions. Finally, the panning gain vectors are weighted
		by the weighting function and summed together for each loudspeaker.
	*/
	class CSpreadPanner
	{
	public:
		CSpreadPanner(CAdmPointSourcePannerGainCalc& psp);
		~CSpreadPanner();

		/**
			Calculate the gains for a source in the defined direction and
			with the specified width and height
		*/
		std::vector<double> CalculateGains(CartesianPosition position, double width, double height);

	private:
		CAdmPointSourcePannerGainCalc& m_pointSourcePannerGainCalc;
		std::vector<CartesianPosition> m_virtualSourcePositions;
		std::vector<std::vector<double>> m_virtualSourcePanningVectors;

		// The number of virtual source positions
		const int nVirtualSources = 1500;
		// The weights to be applied to the virtual sources
		std::vector<double> weights;

		// The last set width and heights
		double m_width = 0.;
		double m_height = 0.;
		// The fade out marge around the "stadium" in degrees
		double m_fadeOut = 10.;
		// The direction of the source used for the weighting function
		CartesianPosition m_weightPosition;
		// The rotation matrix to convert the virtual source position basis
		std::vector<std::vector<double>> m_rotMat;
		// The coordinates of the circular caps used to define the stadium
		CartesianPosition m_circularCapPosition;
		double m_circularCapAzimuth = 0.;

		/**
			Calculate the weight to be applied to the virtual source gain vector for a virtual
			source in with the specified position
			Rec. ITU-R BS.2127-0 section 7.3.8.2.3
		*/
		double CalculateWeights(CartesianPosition position);

		/**
			Calculate the rotation matrix and "stadium" for the weighting function for a source
			a direction specified by position and the defined width and height.
		*/
		void ConfigureWeightingFunction(CartesianPosition position, double width, double height);
	};

	/**
		Class that handles the extent parameters to calculate a gain vector
	*/
	class CPolarExtentHandler
	{
	public:
		CPolarExtentHandler(CAdmPointSourcePannerGainCalc& psp);
		~CPolarExtentHandler();

		/**
			Return a vector of gains for the loudspeakers in the output layout that correspond
			to the position, width, height and depth.

			See Rec. ITU-R BS.2127-0 section 7.3.8.2 (pg. 48) for more details on the algorithm
		*/
		std::vector<double> handle(CartesianPosition position, double width, double height, double depth);

	private:
		CAdmPointSourcePannerGainCalc m_pointSourcePannerGainGalc;
		CSpreadPanner m_spreadPanner;
	
		// The minimum extent, defined by the standard at 5 deg
		const double m_minExtent = 5.;

		/*
			Calculate the gains of the given position, width and height
		*/
		std::vector<double> CalculateGains(CartesianPosition position, double width, double height);

		/**
			Modifies the extent based on the distance as described in ITU-R BS.2127-0 section 7.3.8.2.1 pg 48
		*/
		double PolarExtentModification(double distance, double extent);

		/**
			Calculate the polar extent gain vector as described in ITU-R BS.2127-0 section 7.3.8.2.2 pg 49
		*/
		std::vector<double> CalculatePolarExtentGains(CartesianPosition position, double width, double height);
	};

}
