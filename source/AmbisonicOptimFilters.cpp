/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicOptimFilters - Ambisonic psychoactic optimising filters       #*/
/*#  Copyright © 2020 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicShelfFilters.cpp                                #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          23/03/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicOptimFilters.h"
#include <cmath>
#include <assert.h>

// Precalculated max-rE gains
const float maxReGains3D[][4] = {
    {1.417794018951694f, 0.814424156449370f, 0.f, 0.f},
    {1.583040780613530f, 1.225234967342221f, 0.630932597243196f, 0.f},
    {1.669215604860955f, 1.437112458085760f, 1.021316810756924f, 0.507430850075628f }
};

const float maxReGains2D[][4] = {
    {1.224744871391589f, 0.866025403784439f, 0.f, 0.f},
    {1.290994448735806f, 1.118033988749895f, 0.645497224367903f, 0.f},
    {1.322875655532295f, 1.222177742203739f, 0.935414346693485f, 0.506242596451317f}
};

CAmbisonicOptimFilters::CAmbisonicOptimFilters()
{
}

CAmbisonicOptimFilters::~CAmbisonicOptimFilters()
{
}

bool CAmbisonicOptimFilters::Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned sampleRate)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, 0);
    if(!success)
        return false;

    // The cross-over frequency depends on the order. From:
    // [1] S. Bertet, J. Daniel, E. Parizet, and O. Warusfel,
    // “Investigation on localisation accuracy for first and higher
    // order Ambisonics reproduced sound sources,” Acta Acust. united
    // with Acust., vol. 99, no. 4, pp. 642–657, 2013.
    float headRadius = 0.09f;
    float speedOfSound = 343.f;
    float M = static_cast<float>(nOrder);
    float crossoverFreq = speedOfSound * M / (4.f * headRadius * (M + 1) * std::sin((float)M_PI / (2.f * M + 2.f)));

    success = m_bandFilterIIR.Configure(GetChannelCount(), sampleRate, crossoverFreq);
    if (!success)
        return false;

    // Calculate the gains to be applied to each sub-set of channels, one for each order
    m_gMaxRe.resize(nOrder + 1, 1.f); // high-frequency uses max-rE decoding

    // The gains depend whether or not the processing is 2D or 3D.
    for (unsigned int i = 0; i <= nOrder; ++i)
        m_gMaxRe[i] = b3D ? maxReGains3D[nOrder - 1][i] : maxReGains2D[nOrder - 1][i];

    m_nMaxBlockSize = nBlockSize;
    m_lowPassOut.Configure(nOrder, b3D, nBlockSize);

    return true;
}

void CAmbisonicOptimFilters::Reset()
{
    m_bandFilterIIR.Reset();
}

void CAmbisonicOptimFilters::Refresh()
{
}

void CAmbisonicOptimFilters::Process(CBFormat* pBFSrcDst, unsigned int nSamples)
{
    assert(nSamples <= m_nMaxBlockSize);

    float** inOutHP = reinterpret_cast<float**>(pBFSrcDst->m_ppfChannels.get());
    float** outLP = reinterpret_cast<float**>(m_lowPassOut.m_ppfChannels.get());
    m_bandFilterIIR.Process(inOutHP, outLP, inOutHP, nSamples);

    // Multiply the high-pass channels by the appropriate max-rE gain and add it to the output
    for (unsigned int iCh = 0; iCh < pBFSrcDst->GetChannelCount(); ++iCh)
    {
        float gMaxRe = m_gMaxRe[ComponentPositionToDegree(iCh, m_b3D)];
        float* chDataHP = inOutHP[iCh];
        float* chDataLP = outLP[iCh];
        // Scale the high-passed data and add the low-passed signal
        for (unsigned iSamp = 0; iSamp < nSamples; ++iSamp)
            chDataHP[iSamp] = gMaxRe * chDataHP[iSamp] + chDataLP[iSamp];
    }
}
