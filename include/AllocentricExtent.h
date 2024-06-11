/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gains required for allocentric extent panning.            #*/
/*#  AllocentricExtent - ADM Allocentric Extent Panner                       #*/
/*#                                                                          #*/
/*#  Filename:      AllocentricExtent.h                                      #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/06/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "LoudspeakerLayouts.h"
#include "AllocentricPannerGainCalc.h"

/** Class to calculate the extent panning gains using the ADM cartesian/allocentric method on the specified loudspeaker layout. */
class CAllocentricExtent
{
public:
    CAllocentricExtent(const Layout& layout);
    ~CAllocentricExtent();

    /** Calculate the gains to be applied to a mono signal in order to place it in the target
     *  speaker layout based on Rec. ITU-R BS.2127-1 Sec. 7.3.11.
     * @param sourcePosition Source direction.
     * @param sizeX Extent in the x-axis.
     * @param sizeY Extent in the y-axis.
     * @param sizeZ Extent in the z-axis.
     * @excluded Vector of excluded loudspeakers.
     * @param gainsOut Output vector of the panning gains.
     */
    void handle(CartesianPosition sourcePosition, double sizeX, double sizeY, double sizeZ, const std::vector<bool>& excluded, std::vector<double>& gainsOut);

    /** Get the number of loudspeakers set in the targetLayout. */
    unsigned int getNumChannels();

private:
    Layout m_layout;
    std::vector<CartesianPosition> m_cartesianPositions;
    CAllocentricPannerGainCalc m_alloPanner;

    unsigned int m_nMaxGridPoints = 40;

    // Coordinates of grid
    std::vector<double> m_xs, m_ys, m_zs;

    // Store the last set excluded loudspeakers
    std::vector<bool> m_excluded;

    // Flag if the specified layout has a bottom row
    bool m_hasBottomRow = false;

    std::vector<double> m_extentScaleInput = { 0., 0.2, 0.5, 0.75, 1. };
    std::vector<double> m_extentScaleOutput = { 0., 0.3, 1.0, 1.8, 2.8 };

    // Individual axis gains for all loudspeakers
    std::vector<double> m_gx, m_gy, m_gz;
    // Weighted sum of gains in each axis for all loudspeakers
    std::vector<double> m_fx, m_fy, m_fz;
    // Inside gains
    std::vector<double> m_gInside;
    // Boundary gains
    std::vector<double> m_gBound;
    // Extent gains
    std::vector<double> m_gExtent;
    // Point gains
    std::vector<double> m_gPoint;

    // Temp vector holding the elevation of non-excluded loudspeaker when determining the dimension
    std::vector<double> m_elLdspk;

    // Intermediate boundary gains
    std::vector<double> m_bFloor, m_bCeil, m_bLeft, m_bRight, m_bFront, m_bBack;

    double calculateSEff(const std::vector<CartesianPosition>& cartesianPositions, const std::vector<bool>& excluded, double sx, double sy, double sz);

    std::tuple<double, double, double> calculateWeights(double xs, double ys, double zs, double xo, double yo, double zo, double sx, double sy, double sz);

    /** Normalisation of the gains with a check for small norms
     * @param gains Gains to be normalised.
     */
    void normaliseGains(std::vector<double>& gains);

    /** Count the number of dimensions in the array based on Rec. ITU-R BS.2127-1 Sec. 7.3.11.4
     * @param layout Loudspeaker layout with nominal loudspeaker polar coordinates.
     * @param excluded Which loudspeakers are excluded.
     * @return Number of dimensions as defined in Rec. ITU-R BS.2127-1 Sec. 7.3.11.4.
     */
    int countDimensions(const Layout& cartesianPositions, const std::vector<bool>& excluded);

    /** Calculate mu gain factor to apply to inside-gains. See Rec. ITU-R BS.2127-1 Sec. 7.3.11.5.
     * @param dim The dimension of the loudspeaker array from 1 to 4.
     * @param xo Object position x-coordinate.
     * @param yo Object position y-coordinate.
     * @param zo Object position z-coordinate.
     * @param sx Object extent x-axis.
     * @param sy Object extent y-axis.
     * @param sz Object extent z-axis.
     * @return Gain factor mu
     */
    double calculateMu(int dim, double xo, double yo, double zo, double sx, double sy, double sz);

    /** Calculate h term used when calculating mu. See Rec. ITU-R BS.2127-1 Sec. 7.3.11.5.
     * @param c Coordinate value.
     * @param s Extent value.
     * @param dBound Clipped boundary value.
     * @return h-factor used in calculation of mu.
     */
    double h(double c, double s, double dBound);
};
