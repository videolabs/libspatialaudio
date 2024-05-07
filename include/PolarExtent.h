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

#include "Coordinates.h"
#include "Tools.h"
#include "PointSourcePannerGainCalc.h"
#include "AmbisonicSource.h"

/*
	All of the class defined here are used for the polar extent panning defined
	in Rec. ITU-R BS.2127-0 section 7.3.8 (pg 46).

	It is made up of a PolarExtentHandler, which uses a PolarExtentPanner, which
	in turn uses a SpreadPanner and a PointSourcePanner
*/

/** The spread panner defines a set of positions of virtual sources on the sphere
 *  and calculates the corresponding point source panning gain vector for each of them.
 *  Then, based on the position, width and height it calculates a set of weights corresponding
 *  to each of the virtual source directions. Finally, the panning gain vectors are weighted
 *  by the weighting function and summed together for each loudspeaker.
 */
class CSpreadPannerBase
{
public:
	CSpreadPannerBase();
	~CSpreadPannerBase();

protected:
	std::vector<CartesianPosition> m_virtualSourcePositions;
	std::vector<std::vector<double>> m_virtualSourcePanningVectors;

	// The number of virtual source positions
	int m_nVirtualSources;
	// The weights to be applied to the virtual sources
	std::vector<double> m_weights;

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

	// The polar coordinates of the position when converted to the coordinate system of the weighting function
	std::vector<double> m_positionBasisPol;

	// A temp vector holding the position of the signal
	std::vector<double> m_posVec;
	// A temp vector holding the position basis
	std::vector<double> m_positionBasis;
	// A temp vector holding the position of the closest circle
	std::vector<double> m_closestCircle;

	/** Calculate the weight to be applied to the virtual source gain vector for a virtual
	 *  source in with the specified position.
	 *  Rec. ITU-R BS.2127-0 section 7.3.8.2.3
	 * @param position	The unit vector of the grid point.
	 * @return			The weight corresponding to the direction.
	 */
	double CalculateWeights(CartesianPosition position);

	/** Calculate the rotation matrix and "stadium" for the weighting function for a source
	 *  a direction specified by position and the defined width and height.
	 * @param position	The centre position of the "stadium" of the weighting function.
	 * @param width		The angular width of the "stadium".
	 * @param height	The angular height of the "stadium".
	 */
	void ConfigureWeightingFunction(CartesianPosition position, double width, double height);
};

/** Spread panner that calculates the gains for the layout supplied in the point source panner
 *  supplied in the constructor.
 */
class CSpreadPanner : private CSpreadPannerBase
{
public:
	CSpreadPanner(CPointSourcePannerGainCalc& psp);
	~CSpreadPanner();

	/** Calculate the gains for a source in the defined direction and with the specified width and height.
	 * @param position	The source position.
	 * @param width		The source width in degrees.
	 * @param height	The source height in degrees.
	 * @param gainsOut	Output vector of speaker gains.
	 */
	void CalculateGains(CartesianPosition position, double width, double height, std::vector<double>& gainsOut);

private:
	CPointSourcePannerGainCalc& m_pointSourcePannerGainCalc;
	unsigned int m_nCh = 0;
};

/** Spread panner that calculates the gains encoding an ambisonic signal with the specified spread.
 *  Note that this is not part of the ADM specification. It is used for the libspatialaudio binaural ADM rendering.
 */
class CAmbisonicSpreadPanner : private CSpreadPannerBase
{
public:
	CAmbisonicSpreadPanner(unsigned int ambiOrder);
	~CAmbisonicSpreadPanner();

	/** Calculate the ambisonic encoding gains for a source in the defined direction and with the specified width and height.
	 * @param position	The source position.
	 * @param width		The source width in degrees.
	 * @param height	The source height in degrees.
	 * @param gainsOut	Output vector of speaker gains.
	 */
	void CalculateGains(CartesianPosition position, double width, double height, std::vector<double>& gainsOut);

	/** Get the order of the HOA encoding for the sources. */
	unsigned int GetAmbisonicOrder();

private:
	 CAmbisonicSource m_ambiSource;
	 unsigned int m_nCh = 0;
};

/** Class that handles the extent parameters to calculate a gain vector. */
class CPolarExtentHandlerBase
{
public:
	CPolarExtentHandlerBase();
	~CPolarExtentHandlerBase();

	/** Pure virtual function for the main gain calculation function. */
	virtual void handle(CartesianPosition position, double width, double height, double depth, std::vector<double>& gainsOut) = 0;

protected:
	unsigned int m_nCh = 0;
	// The minimum extent, defined by the standard at 5 deg
	const double m_minExtent = 5.;

	/** Modifies the width and height extent based on the distance as described in ITU-R BS.2127-0 section 7.3.8.2.1 pg 48.
	 * @param distance	The distance of the source.
	 * @param extent	Width/height extent.
	 * @return			Modified width/height extent.
	 */
	double PolarExtentModification(double distance, double extent);

	/** Calculate the polar extent gain vector as described in ITU-R BS.2127-0 section 7.3.8.2.2 pg 49.
	 * @param position	Position of the source.
	 * @param width		Width in degrees.
	 * @param height	Height in degrees.
	 * @param outGains	Output vector of panning gains.
	 */
	virtual void CalculatePolarExtentGains(CartesianPosition position, double width, double height, std::vector<double>& outGains) = 0;

	// Temp vectors
	std::vector<double> m_g_p;
	std::vector<double> m_g_s;
	std::vector<double> m_g1;
	std::vector<double> m_g2;
};


/** Class that handles the extent parameters to calculate a gain vector */
class CPolarExtentHandler : public CPolarExtentHandlerBase
{
public:
	CPolarExtentHandler(CPointSourcePannerGainCalc& psp);
	~CPolarExtentHandler();

	/** Return a vector of gains for the loudspeakers in the output layout that correspond
	 *  to the position, width, height and depth.
	 *  See Rec. ITU-R BS.2127-0 section 7.3.8.2 (pg. 48) for more details on the algorithm.
	 * @param position	Source position.
	 * @param width		Source width in degrees.
	 * @param height	Source height in degrees.
	 * @param depth		Source depth.
	 * @param gainsOut	Output vector of panning gains.
	 */
	void handle(CartesianPosition position, double width, double height, double depth, std::vector<double>& gainsOut) override;

private:
	CPointSourcePannerGainCalc m_pointSourcePannerGainGalc;
	CSpreadPanner m_spreadPanner;

	/** Calculate the polar extent gain vector as described in ITU-R BS.2127-0 section 7.3.8.2.2 pg 49.
	 * @param position	Source position.
	 * @param width		Source width in degrees.
	 * @param height	Source height in degrees.
	 * @param gainsOut	Output vector of panning gains.
	 */
	void CalculatePolarExtentGains(CartesianPosition position, double width, double height, std::vector<double>& gainsOut) override;
};

/** Class that handles the extent parameters to calculate a gain vector for Ambisonic panning. */
class CAmbisonicPolarExtentHandler : public CPolarExtentHandlerBase
{
public:
	CAmbisonicPolarExtentHandler(unsigned int ambiOrder);
	~CAmbisonicPolarExtentHandler();

	/** Return a vector of ambisonic coefficients that correspond to the position, width, height and depth.
	 *
	 *  See Rec. ITU-R BS.2127-0 section 7.3.8.2 (pg. 48) for more details on basis the algorithm.
	 *  Note that some changes have been made to account for the fact that ambisonic coefficients should
	 *  be summed by amplitude in order to preserve their polarity.
	 * @param position	Source position.
	 * @param width		Source width in degrees.
	 * @param height	Source height in degrees.
	 * @param depth		Source depth.
	 * @param gainsOut	Output vector of panning gains.
	 */
	void handle(CartesianPosition position, double width, double height, double depth, std::vector<double>& gainsOut) override;

private:
	CAmbisonicSource m_ambiSource;
	CAmbisonicSpreadPanner m_ambiSpreadPanner;

	/** Calculate the HOA extent gain vector. Essentially works as described in ITU-R BS.2127-0 section 7.3.8.2.2 pg 49
	 *  except instead of using loudspeaker gains it uses HOA encoding coefficients
	 * @param position	Source position.
	 * @param width		Source width in degrees.
	 * @param height	Source height in degrees.
	 * @param gainsOut	Output vector of panning gains.
	 */
	void CalculatePolarExtentGains(CartesianPosition position, double width, double height, std::vector<double>& outGains) override;
};