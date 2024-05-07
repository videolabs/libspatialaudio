/*############################################################################*/
/*#                                                                          #*/
/*#  Screen scaling and screen edge lock handling                            #*/
/*#								                                             #*/
/*#                                                                          #*/
/*#  Filename:      Screen.h												 #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "AdmConversions.h"
#include "ScreenCommon.h"
#include "LoudspeakerLayouts.h"
#include "Coordinates.h"
#include "Tools.h"

/** In some output layouts when cartesian==true, vertical panning in front of the listener may be
 *  warped. This compensates for that. See Rec. ITU-R BS.2127-0 sec. 7.3.2 pg 41 for more details.
 * @param az		Azimuth of the source position in degrees.
 * @param el		Elevation fo the source position in degrees.
 * @param layout	The loudspeaker layout.
 * @return			The compensated position (az, el) in degrees.
 */
static inline std::pair<double, double> CompensatePosition(double az, double el, const Layout& layout)
{
	auto speakerNames = layout.channelNames();
	if (std::find(speakerNames.begin(), speakerNames.end(), std::string("U+045")) != speakerNames.end())
	{
		double az_r = interp(el, { -90., 0., 30., 90. }, { 30.,30.,30. * 30. / 45.,30. });
		double azDash = interp(el, { -180., -30., 30., 180. }, { -180., -az_r, az_r, 180. });

		return { azDash, el };
	}
	else
		return { az,el };
}

/** The the position of the source from a position relative to the reference screen to a position
 *  relative to the reproduction screen.
*/
class CScreenScaleHandler {
public:
	CScreenScaleHandler(std::vector<Screen> reproductionScreen, Layout layout);
	~CScreenScaleHandler();

	/** Scales a position depending on the reproduction screen and the reference screen. See Rec. ITU-R BS.2127-0 sec. 7.3.3 pg 40 for more details
	 * @param position			Cartesian position to be scaled.
	 * @param screenRef			If true then applies the scaling, otherwise return the original position unmodified.
	 * @param referenceScreen	Reference screen to use for the scaling.
	 * @param cartesian			Flag if the Cartesian rendering path is to be used.
	 * @return					Returns the modified position.
	 */
	CartesianPosition handle(CartesianPosition position, bool screenRef, const std::vector<Screen>& referenceScreen, bool cartesian);

private:
	Layout m_layout;
	// The reproduction screen
	Screen m_repScreen, m_refScreen;
	// The internal representation of the screens
	PolarEdges m_repPolarEdges, m_refPolarEdges;

	bool m_repScreenSet = false;

	/** Scale the position for the polar/egocentric path.
	 * @param position	The position to scale.
	 * @return			The scaled postion.
	 */
	CartesianPosition ScalePosition(CartesianPosition position);

	/** Scale the azimuth and elevation based on the polar edges of the screen.
	 * @param az	The azimuth to scale in degrees.
	 * @param el	The elevation to scale in degrees.
	 * @return		Pair containing scaled az and el.
	 */
	std::pair<double, double> ScaleAzEl(double az, double el);
};

/** Apply screen edge locking to supplied position based on the reproduction screen and (if cartesian == true) the layout. */
class CScreenEdgeLock
{
public:
	CScreenEdgeLock(std::vector<Screen> reproductionScreen, Layout layout);
	~CScreenEdgeLock();

	/** Apply screen edge locking to a position. See Rec. ITU-R BS.2127-1 sec. 7.3.4 pg. 43.
	 * @param position			The position to process.
	 * @param screenEdgeLock	Screen edge lock metadata.
	 * @param cartesian			If true then uses cartesian/allocentric processing. Otherwise uses polar/egocentric method.
	 * @return					The modified position.
	 */
	CartesianPosition HandleVector(CartesianPosition position, admrender::ScreenEdgeLock screenEdgeLock, bool cartesian = false);

	/** Apply screen edge locking to an azimuth and elevation.
	 * @param azimuth			The azimuth in degrees to process.
	 * @param elevation			The elevation in degrees to process.
	 * @param screenEdgeLock	ScreenEdgeLock metadata.
	 * @return					The modified directions in degrees.
	 */
	std::pair<double, double> HandleAzEl(double azimuth, double elevation, admrender::ScreenEdgeLock screenEdgeLock);

private:
	bool m_repScreenSet = false;
	Layout m_layout;
	Screen m_reproductionScreen;
	PolarEdges m_repPolarEdges;
};
