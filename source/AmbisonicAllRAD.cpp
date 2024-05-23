/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicAllRAD - Ambisonic AllRAD decoder                             #*/
/*#  Copyright © 2024 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicAllRAD.cpp                                      #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/05/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AmbisonicAllRAD.h"
#include "PointSourcePannerGainCalc.h"
#include "AmbisonicCommons.h"
#include "AmbisonicSource.h"
#include "t_design_5200.h"
#include <assert.h>

CAmbisonicAllRAD::CAmbisonicAllRAD()
{
}

CAmbisonicAllRAD::~CAmbisonicAllRAD()
{
}

bool ::CAmbisonicAllRAD::Configure(unsigned nOrder, unsigned nBlockSize, unsigned sampleRate, const std::string& layoutName, bool useLFE, bool useOptimFilts)
{
    bool success = CAmbisonicBase::Configure(nOrder, true, 0);
    if(!success)
        return false;

    m_nBlockSize = nBlockSize;
    m_sampleRate = sampleRate;

    m_layout = GetMatchingLayout(layoutName);
    if (m_layout.channels.size() == 0)
        return false; // No valid layout

    if (!useLFE)
        m_layout = getLayoutWithoutLFE(m_layout);

    // Set up the ambisonic shelf filters
    m_useOptimFilters = useOptimFilts;
    if (m_useOptimFilters)
        m_shelfFilters.Configure(nOrder, m_b3D, nBlockSize, sampleRate);

    // Set up the low pass IIR
    unsigned int nLFE = 0;
    for (auto& c : m_layout.channels)
        if (c.isLFE)
            nLFE++;
    m_lowPassIIR.Configure(nLFE, sampleRate, 200.f, std::sqrt(0.5), CIIRFilter::FilterType::LowPass);

    m_pBFSrcTmp.Configure(nOrder, m_b3D, nBlockSize);

    ConfigureAllRADMatrix();
    Refresh();

    return true;
}

void CAmbisonicAllRAD::Reset()
{
    m_shelfFilters.Reset();
    m_pBFSrcTmp.Reset();
}

void CAmbisonicAllRAD::Refresh()
{
    m_shelfFilters.Refresh();
    m_pBFSrcTmp.Refresh();
}

void CAmbisonicAllRAD::Process(const CBFormat* pBFSrc, unsigned nSamples, float** ppfDst)
{
        // Process a copy of the input to avoid overwriting it
        m_pBFSrcTmp = *pBFSrc;
        if (m_useOptimFilters)
            m_shelfFilters.Process(&m_pBFSrcTmp, nSamples);

        // Decode the input signal
        unsigned int ii = 0;
        unsigned int iLFE = 0;
        size_t nLdspk = m_layout.channels.size();
        for (size_t niSpeaker = 0; niSpeaker < nLdspk; niSpeaker++)
        {
            if (m_layout.channels[niSpeaker].isLFE)
            {
                // Filter the W channel for the LFE and scale by -6 dB
                m_lowPassIIR.Process(pBFSrc->m_ppfChannels[0], ppfDst[niSpeaker], nSamples, iLFE);
                for (unsigned int niSample = 0; niSample < nSamples; niSample++)
                    ppfDst[niSpeaker][niSample] *= 0.5f;
                iLFE++;
            }
            else
            {
                memset(ppfDst[niSpeaker], 0, nSamples * sizeof(float));
                for (size_t niChannel = 0; niChannel < m_decMat[0].size(); niChannel++)
                {
                    float* in = m_pBFSrcTmp.m_ppfChannels[niChannel];
                    float* out = ppfDst[niSpeaker];

                    const float coeff = m_decMat[ii][niChannel];
                    for (unsigned int niSample = 0; niSample < nSamples; niSample++)
                        *out++ += (*in++) * coeff;
                }
                ii++;
            }
        }
}

unsigned CAmbisonicAllRAD::GetSpeakerCount()
{
    return (unsigned)m_layout.channels.size();
}

bool CAmbisonicAllRAD::GetUseOptimFilters()
{
    return m_useOptimFilters;
}

void CAmbisonicAllRAD::ConfigureAllRADMatrix()
{
    // Set up the point source panner
    CPointSourcePannerGainCalc psp(m_layout);

    unsigned int nLdspk = psp.getNumChannels();
    unsigned int nGrid = tDesign5200::nTdesignPoints;
    float recipNumGrid = 1.f / (float)nGrid;

    // Set up the virtual source grid spherical harmonics Y with N3D normalisation
    CAmbisonicSource ambiSrc;
    ambiSrc.Configure(m_nOrder, m_b3D, 0);
    std::vector<std::vector<float>> Y((m_nOrder + 1) * (m_nOrder + 1), std::vector<float>(nGrid, 0.f));
    std::vector<std::vector<float>> YT(nGrid, std::vector<float>((m_nOrder + 1) * (m_nOrder + 1), 0.f));
    std::vector<double> pspGainsTmp(psp.getNumChannels(), 0.);
    std::vector<std::vector<float>> G(nLdspk, std::vector<float>(nGrid, 0.f));
    for (unsigned i = 0; i < nGrid; ++i)
    {
        float azRad = tDesign5200::points[i][0];
        float elRad = tDesign5200::points[i][1];
        ambiSrc.SetPosition({ azRad, elRad, 1.f });
        ambiSrc.Refresh();
        // Store transpose of Y
        ambiSrc.GetCoefficients(YT[i]);

        // Convert to N3D
        for (size_t iCoeff = 0; iCoeff < YT[i].size(); ++iCoeff)
        {
            float n2sn = (float)std::sqrt(2 * ComponentPositionToOrder(iCoeff, m_b3D) + 1);
            YT[i][iCoeff] *= n2sn;
        }

        // Store Y for normalisation
        for (size_t j = 0; j < YT[i].size(); ++j)
            Y[j][i] = YT[i][j];

        // Divide YT by the number of grid points
        for (auto& y : YT[i])
            y *= recipNumGrid;

        // Calculate the point source panning vectors for each source grid direction G
        psp.CalculateGains(PolarPosition{ (double)RadiansToDegrees(azRad), (double)RadiansToDegrees(elRad), 1. }, pspGainsTmp);

        // Store the gain matrix
        for (size_t j = 0; j < nLdspk; ++j)
            G[j][i] = pspGainsTmp[j];
    }

    // Multiply G * Y^T to get the decoder
    m_decMat = multiplyMat(G, YT);

    // Calculate the normalisation value
    auto sampleMat = multiplyMat(m_decMat, Y); // Decode the sampling matrix to get the loudspeaker gains at each grid position
    // Take the Frobenius norm of sampleMat
    float froNorm = 0.f;
    for (size_t iLdspk = 0; iLdspk < sampleMat.size(); ++iLdspk)
        for (size_t iGrid = 0; iGrid < sampleMat[0].size(); ++iGrid)
            froNorm += sampleMat[iLdspk][iGrid] * sampleMat[iLdspk][iGrid];
    froNorm = std::sqrt(froNorm);

    // Normalise the decoder and convert to a decoder for SN3D normalised signals
    float normFactor = std::sqrt((float)nGrid) / froNorm;
    for (size_t iCoeff = 0; iCoeff < m_decMat[0].size(); ++iCoeff)
    {
        float n2snDec = (float)std::sqrt(2 * ComponentPositionToOrder(iCoeff, m_b3D) + 1);
        for (size_t iLdspk = 0; iLdspk < m_decMat.size(); ++iLdspk)
            m_decMat[iLdspk][iCoeff] *= normFactor * n2snDec;
    }
}
