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


#include "AllocentricExtent.h"
#include <algorithm>

CAllocentricExtent::CAllocentricExtent(const Layout& layout)
    : m_layout(getLayoutWithoutLFE(layout))
    , m_cartesianPositions(admrender::positionsForLayout(layout))
    , m_alloPanner(m_layout)
    , m_xs(m_nMaxGridPoints)
    , m_ys(m_nMaxGridPoints)
    , m_zs(m_nMaxGridPoints)
    , m_excluded(m_cartesianPositions.size(), false)
    , m_gx(m_cartesianPositions.size())
    , m_gy(m_cartesianPositions.size())
    , m_gz(m_cartesianPositions.size())
    , m_fx(m_cartesianPositions.size())
    , m_fy(m_cartesianPositions.size())
    , m_fz(m_cartesianPositions.size())
    , m_gInside(m_cartesianPositions.size())
    , m_gBound(m_cartesianPositions.size())
    , m_gExtent(m_cartesianPositions.size())
    , m_gPoint(m_cartesianPositions.size())
    , m_elLdspk(m_cartesianPositions.size())
    , m_bFloor(m_cartesianPositions.size())
    , m_bCeil(m_cartesianPositions.size())
    , m_bLeft(m_cartesianPositions.size())
    , m_bRight(m_cartesianPositions.size())
    , m_bFront(m_cartesianPositions.size())
    , m_bBack(m_cartesianPositions.size())
{
    // Check if the layout has a bottom row
    m_hasBottomRow = false;
    for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
        if (m_cartesianPositions[i].z < 0.)
            m_hasBottomRow = true;

    int Nx = m_nMaxGridPoints;
    int Ny = m_nMaxGridPoints;
    int Nz = m_hasBottomRow ? m_nMaxGridPoints : m_nMaxGridPoints / 2;
    m_zs.resize(Nz);

    for (unsigned int i = 0; i < m_nMaxGridPoints; ++i)
    {
        m_xs[i] = -1. + 2. * (double)i / ((double)Nx - 1.);
        m_ys[i] = -1. + 2. * (double)i / ((double)Ny - 1.);
        if (m_hasBottomRow)
            m_zs[i] = -1. + 2. * (double)i / ((double)Nz - 1.);
    }
    if (!m_hasBottomRow)
        for (unsigned int i = 0; i < Nz; ++i)
            m_zs[i] = (double)i / ((double)Nz - 1.);
}

CAllocentricExtent::~CAllocentricExtent()
{
}

void CAllocentricExtent::handle(CartesianPosition position, double sizeX, double sizeY, double sizeZ, const std::vector<bool>& excluded, std::vector<double>& gains)
{
    auto nLdspk = m_alloPanner.getNumChannels();
    assert(gains.capacity() >= nLdspk);
    gains.resize(nLdspk);

    // Check if the input array includes a bottom row after loudspeaker have been excluded
    bool hasBottomRow = m_hasBottomRow;
    if (m_hasBottomRow)
    {
        // Layout includes a bottom layer before any loudspeakers are exlcuded. Check if the bottom row
        // has been completely removed by zone exclusion
        for (size_t i = 0; i < m_cartesianPositions.size(); ++i)
            if (m_cartesianPositions[i].z < 0. && !excluded[i])
                hasBottomRow = true;
    }

    int Nx = m_nMaxGridPoints;
    int Ny = m_nMaxGridPoints;
    int Nz = hasBottomRow ? m_nMaxGridPoints : m_nMaxGridPoints / 2;

    int iZStart = !hasBottomRow && m_hasBottomRow ? Nz : 0;

    // Get the source coordinates, accounting for bottom row
    auto xo = position.x;
    auto yo = position.y;
    auto zo = hasBottomRow ? position.z : std::max(position.z, 0.);

    // Pre-scale the extent parameters
    auto sx = std::max(interp(sizeX, m_extentScaleInput, m_extentScaleOutput), 2. / ((double)Nx - 1.));
    auto sy = std::max(interp(sizeY, m_extentScaleInput, m_extentScaleOutput), 2. / ((double)Ny - 1.));
    auto sz = std::max(interp(sizeZ, m_extentScaleInput, m_extentScaleOutput), 2. / ((double)Nz - 1.));
    auto sEff = calculateSEff(m_cartesianPositions, m_excluded, sx, sy, sz);
    auto sMax = 2.8;
    auto p = sEff <= 0.5 ? 6. : 6. - 4. * (sEff - 0.5) / (sMax - 0.5);
    int dim = countDimensions(m_layout, excluded);
    auto mu = calculateMu(dim, xo, yo, zo, sx, sy, sz);

    for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
    {
        m_fx[iLdspk] = 0.;
        m_fy[iLdspk] = 0.;
        m_fz[iLdspk] = 0.;
    }

    double b_floor = 0., b_ceil = 0., b_left = 0., b_right = 0., b_front = 0., b_back = 0.;

    for (size_t iX = 0; iX < m_xs.size(); ++iX)
        for (size_t iY = 0; iY < m_ys.size(); ++iY)
            for (size_t iZ = iZStart; iZ < m_zs.size(); ++iZ)
                if ((m_zs[iZ] < 0. && hasBottomRow) || m_zs[iZ] >= 0.) // if there is a non-excluded bottom row and the grid point is below 0
                {
                    auto w = calculateWeights(m_xs[iX], m_ys[iY], m_zs[iZ], xo, yo, zo, sx, sy, sz);
                    auto wx = std::get<0>(w);
                    auto wy = std::get<1>(w);
                    auto wz = std::get<2>(w);
                    // Calculate the gains for the current grid position
                    // TODO: Store the gains. Only recompute when 'excluded' changes?
                    CartesianPosition gridPosition = { m_xs[iX], m_ys[iY], m_zs[iZ] };
                    m_alloPanner.CalculateIndividualGains(gridPosition, excluded, m_gx, m_gy, m_gz);

                    for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
                    {
                        m_fx[iLdspk] += std::pow(wx * m_gx[iLdspk], p);
                        m_fy[iLdspk] += std::pow(wy * m_gy[iLdspk], p);
                        m_fz[iLdspk] += std::pow(wz * m_gz[iLdspk], p);
                    }

                    // Compute the intermediate boundary gains once at each boundary
                    if (iZ == iZStart && iX == 0 && iY == 0) // b_floor, b_left and b_back
                    {
                        for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
                        {
                            m_bFloor[iLdspk] = dim == 4 ? std::pow(m_gz[iLdspk] * wz, p) : 0.;
                            m_bLeft[iLdspk] = std::pow(m_gx[iLdspk] * wx, p);
                            m_bBack[iLdspk] = dim > 1 ? std::pow(m_gy[iLdspk] * wy, p) : 0.;
                        }
                    }
                    else if (iZ == m_zs.size() - 1 && iX == 0 && iY == 0) // b_ceil
                    {
                        for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
                            m_bCeil[iLdspk] = dim >= 3 ? std::pow(m_gz[iLdspk] * wz, p) : 0.;
                    }
                    else if (iX == m_xs.size() - 1 && iZ == iZStart && iY == 0) // b_right
                    {
                        for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
                            m_bRight[iLdspk] = std::pow(m_gx[iLdspk] * wx, p);
                    }
                    else if (iY == m_xs.size() - 1 && iZ == iZStart && iX == 0) // b_front
                    {
                        for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
                            m_bFront[iLdspk] = dim > 1 ? std::pow(m_gy[iLdspk] * wy, p) : 0.;
                    }
                }
    const double verySmall = std::pow(10., -6.5);

    // Compute the inside gains
    for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
    {
        // "Additionally, very small values of f(10−6.5) are rounded down to zero to avoid floating point underflow in implementations."
        m_fx[iLdspk] = m_fx[iLdspk] < verySmall ? 0. : m_fx[iLdspk];
        m_fy[iLdspk] = m_fy[iLdspk] < verySmall ? 0. : m_fy[iLdspk];
        m_fz[iLdspk] = m_fz[iLdspk] < verySmall ? 0. : m_fz[iLdspk];
        // Calculate inside gain
        m_gInside[iLdspk] = m_fx[iLdspk] * m_fy[iLdspk] * m_fz[iLdspk];
        m_gBound[iLdspk] = m_bFloor[iLdspk] * m_fx[iLdspk] * m_fy[iLdspk] + m_bCeil[iLdspk] * m_fx[iLdspk] * m_fy[iLdspk]
            + m_bLeft[iLdspk] * m_fy[iLdspk] * m_fz[iLdspk] + m_bRight[iLdspk] * m_fy[iLdspk] * m_fz[iLdspk]
            + m_bFront[iLdspk] * m_fx[iLdspk] * m_fz[iLdspk] + m_bBack[iLdspk] * m_fx[iLdspk] * m_fz[iLdspk];
    }

    normaliseGains(m_gInside);
    // Rec. ITU-R BS.2127-1 does not mention normalising the boundary gains. However, it does use the notation (g^{\tilde})
    // for normalised gain when combining gInside and gBound. We will normalise it, sticking to the notation in the equations,
    // rather than strictly to the text. If gBound is not normalised then it will dominate gExtent when summed with gInside.
    normaliseGains(m_gBound);

    for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
        m_gExtent[iLdspk] = m_gBound[iLdspk] + mu * m_gInside[iLdspk];

    normaliseGains(m_gExtent);

    // Combining extent gains and point gains
    m_alloPanner.CalculateGains({ xo,yo,zo }, excluded, m_gPoint);
    const double sFade = 0.2;
    auto alpha = sEff < sFade ? std::cos(sEff / sFade * M_PI_2) : 0.;
    auto beta = sEff < sFade ? std::sin(sEff / sFade * M_PI_2) : 1.;

    for (unsigned int iLdspk = 0; iLdspk < nLdspk; ++iLdspk)
        gains[iLdspk] = alpha * m_gPoint[iLdspk] + beta * m_gExtent[iLdspk];

    normaliseGains(gains);
}

unsigned int CAllocentricExtent::getNumChannels()
{
    return m_alloPanner.getNumChannels();
}

double CAllocentricExtent::calculateSEff(const std::vector<CartesianPosition>& cartesianPositions, const std::vector<bool>& excluded, double sx, double sy, double sz)
{
    // Find the first non-exlcuded speaker
    unsigned int iFirstLdspk = 0;
    for (size_t i = 0; i < cartesianPositions.size(); ++i)
        if (excluded[i])
            iFirstLdspk++;
        else
            break;
    auto y = cartesianPositions[iFirstLdspk].y;
    auto z = cartesianPositions[iFirstLdspk].z;

    // Check or a single left/right row or single horizontal layer
    bool isRow = true;
    bool isLayer = true;
    for (size_t i = iFirstLdspk + 1; i < cartesianPositions.size(); ++i)
        if (!excluded[i])
        {
            isRow |= cartesianPositions[i].y == y && cartesianPositions[i].z == z;
            isLayer |= cartesianPositions[i].z == z;
        }

    if (isRow)
        return sx;
    else if (isLayer)
    {
        std::pair<double, double> s_sorted = sx > sy ? std::pair<double, double>(sx, sy) : std::pair<double, double>(sy, sx);
        return 0.75 * s_sorted.first + 0.25 * s_sorted.second;
    }

    double s_sorted[] = { sx, sy, sz };
    std::sort(s_sorted, s_sorted + 3); // sorts to ascending order
    return (6. * s_sorted[2] + 2. * s_sorted[1] + s_sorted[0]) / 9.;
}

std::tuple<double, double, double> CAllocentricExtent::calculateWeights(double xs, double ys, double zs, double xo, double yo, double zo, double sx, double sy, double sz)
{
    auto wx = std::pow(10., -std::min(std::pow(0.75 * (xs - xo) / (sx), 4.), 6.5));
    auto wy = std::pow(10., -std::min(std::pow(0.75 * (ys - yo) / (sy), 4.), 6.5));
    auto wz = std::pow(10., -std::min(std::pow(1.5 * (zs - zo) / (sz), 4.), 6.5)) * std::cos(sz * 3. * M_PI / 7.);

    return { wx, wy, wz };
}

void CAllocentricExtent::normaliseGains(std::vector<double>& gains)
{
    double tol = 1e-5;
    auto gainsNorm = norm(gains);
    if (gainsNorm > tol)
        for (auto& g : gains)
            g /= gainsNorm;
    else
        for (auto& g : gains)
            g = 0.;
}

int CAllocentricExtent::countDimensions(const Layout& layout, const std::vector<bool>& excluded)
{
    auto nLdspk = layout.channels.size();
    // Count non-excluded loudspeakers
    m_elLdspk.resize(0);
    for (size_t i = 0; i < excluded.size(); ++i)
        if (!excluded[i])
            m_elLdspk.push_back(layout.channels[i].polarPositionNominal.elevation);

    // Calculate the unique number of z-coordinates
    std::sort(m_elLdspk.begin(), m_elLdspk.end());
    int nLayers = std::unique(m_elLdspk.begin(), m_elLdspk.end()) - m_elLdspk.begin();

    if (nLayers == 1) // One layer, so check if it is a plane or a row
    {    // Find the first non-exlcuded speaker
        unsigned int iFirstLdspk = 0;
        for (size_t i = 0; i < nLdspk; ++i)
            if (excluded[i])
                iFirstLdspk++;
            else
                break;
        auto y = m_cartesianPositions[iFirstLdspk].y;
        auto z = m_cartesianPositions[iFirstLdspk].z;

        // Check or a single left/right row or single horizontal layer
        bool isRow = true;
        bool isLayer = true;
        for (size_t i = iFirstLdspk + 1; i < nLdspk; ++i)
            if (!excluded[i])
            {
                isRow |= m_cartesianPositions[i].y == y && m_cartesianPositions[i].z == z;
                isLayer |= m_cartesianPositions[i].z == z;
            }

        if (isRow)
            return 1;
        else if (isLayer)
            return 2;
    }

    // Check if there are more than 2 height layers
    m_elLdspk.resize(0);
    for (size_t i = 0; i < excluded.size(); ++i)
        if (!excluded[i] && layout.channels[i].polarPositionNominal.elevation > 0.)
            m_elLdspk.push_back(layout.channels[i].polarPositionNominal.elevation);

    // Calculate the unique number of z-coordinates above the horizontal
    std::sort(m_elLdspk.begin(), m_elLdspk.end());
    int nHeightLayers = std::unique(m_elLdspk.begin(), m_elLdspk.end()) - m_elLdspk.begin();

    if (nHeightLayers >= 2)
        return 4;

    return 3;
}

double CAllocentricExtent::calculateMu(int dim, double xo, double yo, double zo, double sx, double sy, double sz)
{
    double dBound;
    if (dim == 1)
    {
        dBound = std::min(xo + 1., 1. - xo);
        return std::pow(h(xo, sx, dBound), 3.);
    }
    else if (dim == 2)
    {
        dBound = std::min({ xo + 1., 1. - xo, yo + 1., 1. - yo });
        return h(xo, sx, dBound) * std::pow(h(yo, sy, dBound), 1.5);
    }
    else
    {
        dBound = std::min({ xo + 1., 1. - xo, yo + 1., 1. - yo , zo + 1., 1. - zo });
        return h(xo, sx, dBound) * h(yo, sy, dBound) * h(zo, sz, dBound);
    }
}

double CAllocentricExtent::h(double x, double s, double dBound)
{
    if (dBound >= s && dBound >= 0.4)
        return std::pow(std::max(2. * s, 0.4) / (0.32 * s), 1. / 3.);
    else
        return std::pow(0.5 * dBound * std::pow(dBound / 0.4, 2.), 1. / 3.);
}
