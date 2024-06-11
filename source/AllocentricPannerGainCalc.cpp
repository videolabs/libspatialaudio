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


#include "AllocentricPannerGainCalc.h"

CAllocentricPannerGainCalc::CAllocentricPannerGainCalc(const Layout& layout)
    : m_cartesianPositions(admrender::positionsForLayout(layout))
    , m_positionsX(m_cartesianPositions.size(), 0.)
    , m_positionsY(m_cartesianPositions.size(), 0.)
    , m_positionsZ(m_cartesianPositions.size(), 0.)
    , m_planes(m_cartesianPositions.size(), -1)
    , m_rows(m_cartesianPositions.size(), -1)
{
    std::vector<bool> isProcessed(m_cartesianPositions.size(), false);
    int indPlane = 0;
    int indRow = 0;
    double epsilon = 0.001;

    // Identify which plane each loudspeaker belongs to
    for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
        if (!isProcessed[i])
        {
            for (size_t j = 0; j < m_cartesianPositions.size(); ++j)
                if (std::abs(m_cartesianPositions[j].z - m_cartesianPositions[i].z) < epsilon)
                {
                    m_planes[j] = indPlane;
                    isProcessed[j] = true;
                }

            indPlane++;
        }
    // Identify which row each loudspeaker belongs to
    for (size_t i = 0; i < isProcessed.size(); ++i)
        isProcessed[i] = false;
    for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
        if (!isProcessed[i])
        {
            for (size_t j = 0; j < m_cartesianPositions.size(); ++j)
                if (std::abs(m_cartesianPositions[j].z - m_cartesianPositions[i].z) < epsilon
                    && std::abs(m_cartesianPositions[j].y - m_cartesianPositions[i].y) < epsilon)
                {
                    m_rows[j] = indRow;
                    isProcessed[j] = true;
                }

            indRow++;
        }

    // Store the y-coordinates of the loudspeakers grouped by plane
    m_p_oy_plane.resize(indPlane);
    m_p_sy_plane.resize(indPlane);
    for (int iPlane = 0; iPlane < indPlane; ++iPlane)
    {
        for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
            if (m_planes[i] == iPlane)
                m_p_oy_plane[iPlane].push_back(m_cartesianPositions[i].y);
        m_p_sy_plane[iPlane].resize(m_p_oy_plane[iPlane].size());
    }

    // Store the y-coordinates of the loudspeakers grouped by row
    m_p_ox_row.resize(indRow);
    m_p_sx_row.resize(indRow);
    for (int iRow = 0; iRow < indRow; ++iRow)
    {
        for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
            if (m_rows[i] == iRow)
                m_p_ox_row[iRow].push_back(m_cartesianPositions[i].x);
        m_p_sx_row[iRow].resize(m_p_ox_row[iRow].size());
    }
}

CAllocentricPannerGainCalc::~CAllocentricPannerGainCalc()
{
}

void CAllocentricPannerGainCalc::CalculateGains(CartesianPosition position, const std::vector<bool>& excluded, std::vector<double>& gains)
{
    auto nLdspk = m_cartesianPositions.size();
    assert(gains.capacity() >= nLdspk);
    gains.resize(nLdspk);

    for (size_t i = 0; i < nLdspk; ++i)
    {
        m_positionsX[i] = m_cartesianPositions[i].x - position.x;
        m_positionsY[i] = m_cartesianPositions[i].y - position.y;
        m_positionsZ[i] = m_cartesianPositions[i].z - position.z;
    }
    for (size_t iPlane = 0; iPlane < m_p_sy_plane.size(); ++iPlane)
        for (size_t iLdspk = 0; iLdspk < m_p_sy_plane[iPlane].size(); ++iLdspk)
            m_p_sy_plane[iPlane][iLdspk] = m_p_oy_plane[iPlane][iLdspk] - position.y;
    for (size_t iRow = 0; iRow < m_p_sx_row.size(); ++iRow)
        for (size_t iLdspk = 0; iLdspk < m_p_sx_row[iRow].size(); ++iLdspk)
            m_p_sx_row[iRow][iLdspk] = m_p_ox_row[iRow][iLdspk] - position.x;

    for (size_t i = 0; i < nLdspk; ++i)
    {
        if (!excluded[i])
        {
            double gx, gy, gz;
            // Calculate the gain for the z-axis
            auto z_this = m_positionsZ[i];
            gz = calculateGainForAxis(z_this, m_positionsZ, excluded);

            // Calculate the gain for the y-axis
            auto y_this = m_positionsY[i];
            gy = calculateGainForAxis(y_this, m_p_sy_plane[m_planes[i]], excluded);

            // Calculate the gain for the z-axis
            auto x_this = m_positionsX[i];
            gx = calculateGainForAxis(x_this, m_p_sx_row[m_rows[i]], excluded);

            gains[i] = gx * gy * gz;
        }
        else
        {
            gains[i] = 0.;
        }
    }
}

void CAllocentricPannerGainCalc::CalculateIndividualGains(CartesianPosition position, const std::vector<bool>& excluded,
    std::vector<double>& gx, std::vector<double>& gy, std::vector<double>& gz)
{
    auto nLdspk = m_cartesianPositions.size();
    assert(gx.capacity() >= nLdspk && gy.capacity() >= nLdspk && gz.capacity() >= nLdspk);
    gx.resize(nLdspk);
    gy.resize(nLdspk);
    gz.resize(nLdspk);

    for (size_t i = 0; i < nLdspk; ++i)
    {
        m_positionsX[i] = m_cartesianPositions[i].x - position.x;
        m_positionsY[i] = m_cartesianPositions[i].y - position.y;
        m_positionsZ[i] = m_cartesianPositions[i].z - position.z;
    }
    for (size_t iPlane = 0; iPlane < m_p_sy_plane.size(); ++iPlane)
        for (size_t iLdspk = 0; iLdspk < m_p_sy_plane[iPlane].size(); ++iLdspk)
            m_p_sy_plane[iPlane][iLdspk] = m_p_oy_plane[iPlane][iLdspk] - position.y;
    for (size_t iRow = 0; iRow < m_p_sx_row.size(); ++iRow)
        for (size_t iLdspk = 0; iLdspk < m_p_sx_row[iRow].size(); ++iLdspk)
            m_p_sx_row[iRow][iLdspk] = m_p_ox_row[iRow][iLdspk] - position.x;

    for (size_t i = 0; i < nLdspk; ++i)
    {
        if (!excluded[i])
        {
            double gxTmp, gyTmp, gzTmp;
            // Calculate the gain for the z-axis
            auto z_this = m_positionsZ[i];
            gzTmp = calculateGainForAxis(z_this, m_positionsZ, excluded);

            // Calculate the gain for the y-axis
            auto y_this = m_positionsY[i];
            gyTmp = calculateGainForAxis(y_this, m_p_sy_plane[m_planes[i]], excluded);

            // Calculate the gain for the z-axis
            auto x_this = m_positionsX[i];
            gxTmp = calculateGainForAxis(x_this, m_p_sx_row[m_rows[i]], excluded);

            gx[i] = gxTmp;
            gy[i] = gyTmp;
            gz[i] = gzTmp;
        }
        else
        {
            gx[i] = 0.;
            gy[i] = 0.;
            gz[i] = 0.;
        }
    }
}

unsigned int CAllocentricPannerGainCalc::getNumChannels()
{
    return (unsigned int)m_cartesianPositions.size();
}

double CAllocentricPannerGainCalc::calculateGainForAxis (double valThis, const std::vector<double>& positionsAxis, const std::vector<bool>& excluded)
{
    double valOther;
    bool hasValue = false;
    if (valThis >= 0.)
    {
        valOther = -4.; // Start with a minimum value beyond the possible range
        for (size_t i = 0; i< positionsAxis.size(); ++i)
            if (positionsAxis[i] < valThis && !excluded[i])
            {
                valOther = std::max(positionsAxis[i], valOther);
                hasValue = true;
            }
    }
    else
    {
        valOther = 4.; // Start with a maximum value beyond the possible range
        for (size_t i = 0; i < positionsAxis.size(); ++i)
            if (positionsAxis[i] > valThis && !excluded[i])
            {
                valOther = std::min(positionsAxis[i], valOther);
                hasValue = true;
            }
    }

    if (!hasValue)
        return 1.;
    else if (Sgn(valOther) == Sgn(valThis))
        return 0.;

    return std::cos(valThis / (valOther - valThis) * M_PI_2);
}
