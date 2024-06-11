/*############################################################################*/
/*#                                                                          #*/
/*#  Calculate the gains required for allocentric panning.                   #*/
/*#  AllocentricPannerGainCalc - ADM Allocentric Panner                      #*/
/*#                                                                          #*/
/*#  Filename:      AllocentricPannerGainCalc.h                              #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/06/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "LoudspeakerLayouts.h"

/** Class to calculate the panning gains for a point source using the ADM cartesian/allocentric method on the specified loudspeaker layout. */
class CAllocentricPannerGainCalc
{
public:
    CAllocentricPannerGainCalc(const Layout& layout);
    ~CAllocentricPannerGainCalc();

    /** Calculate the gains to be applied to a mono signal in order to place it in the target
     *  speaker layout based on Rec. ITU-R BS.2127-1 Sec. 7.3.10.
     * @param position Source direction.
     * @excluded Vector of excluded loudspeakers.
     * @param gainsOut Output vector of the panning gains.
     */
    void CalculateGains(CartesianPosition position, const std::vector<bool>& excluded, std::vector<double>& gainsOut);

    /** Calculate the individual x-, y- and z-axis gains
     * @param position Source direction.
     * @excluded Vector of excluded loudspeakers.
     * @param gx Gains for each loudspeaker for the x-axis.
     * @param gy Gains for each loudspeaker for the y-axis.
     * @param gz Gains for each loudspeaker for the z-axis.
     */
    void CalculateIndividualGains(CartesianPosition position, const std::vector<bool>& excluded
        , std::vector<double>& gx, std::vector<double>& gy, std::vector<double>& gz);

    /** Get the number of loudspeakers set in the targetLayout. */
    unsigned int getNumChannels();

private:
    // The cartesian positions of the loudspeaker layout
    std::vector<CartesianPosition> m_cartesianPositions;
    // Vectors holding the coordinates of the loudspeakers relative to the source position
    std::vector<double> m_positionsX, m_positionsY, m_positionsZ;

    // Which planes each of the loudspeakers belong to
    std::vector<int> m_planes;
    // Which rows each of the loudspeakers belong to
    std::vector<int> m_rows;

    // Pre-grouping of the loudspeaker coordinates into different planes
    std::vector<std::vector<double>> m_p_oy_plane;
    // Matrix holding the y-coordinates of the loudspeaker relative to the source position, grouped by plane
    std::vector<std::vector<double>> m_p_sy_plane;
    // Pre-grouping of the loudspeaker coordinates into different rows
    std::vector<std::vector<double>> m_p_ox_row;
    // Matrix holding the x-coordinates of the loudspeaker relative to the source position, grouped by row
    std::vector<std::vector<double>> m_p_sx_row;

    /** Calculate the 1D gain in a particular axis.
     * @param valThis Panning value.
     * @param positionsAxis The coordinates of the current layer/plane/row.
     * @param excluded Vector of loudspeakers that have been excluded.
     * @return Gain for the currrent axis.
     */
    double calculateGainForAxis(double valThis, const std::vector<double>& positionsAxis, const std::vector<bool>& excluded);
};
