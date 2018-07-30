/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicSource - Ambisonic Source                                     #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicSource.cpp                                      #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicSource.h"

#define fSqrt32 sqrt(3.f)/2.f
#define fSqrt58 sqrt(5.f/8.f)
#define fSqrt152 sqrt(15.f)/2.f
#define fSqrt38 sqrt(3.f/8.f)

CAmbisonicSource::CAmbisonicSource()
{
    m_polPosition.fAzimuth = 0.f;
    m_polPosition.fElevation = 0.f;
    m_polPosition.fDistance = 1.f;
    m_fGain = 1.f;
}

bool CAmbisonicSource::Configure(unsigned nOrder, bool b3D, unsigned nMisc)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;

    m_pfCoeff.resize( m_nChannelCount, 0 );
    // for a Basic Ambisonics decoder all of the gains are set to 1.f
    m_pfOrderWeights.resize( m_nOrder + 1, 1.f );

    return true;
}

void CAmbisonicSource::Reset()
{
    //memset(m_pfCoeff, 0, m_nChannelCount * sizeof(float));
}

void CAmbisonicSource::Refresh()
{
    float fCosAzim = cosf(m_polPosition.fAzimuth);
    float fSinAzim = sinf(m_polPosition.fAzimuth);
    float fCosElev = cosf(m_polPosition.fElevation);
    float fSinElev = sinf(m_polPosition.fElevation);

    float fCos2Azim = cosf(2.f * m_polPosition.fAzimuth);
    float fSin2Azim = sinf(2.f * m_polPosition.fAzimuth);
    float fSin2Elev = sinf(2.f * m_polPosition.fElevation);

    if(m_b3D)
    {
        // Uses ACN channel ordering and SN3D normalization scheme (AmbiX format)
        if(m_nOrder >= 0)
        {
            m_pfCoeff[0] = 1.f * m_pfOrderWeights[0]; // W
        }
        if(m_nOrder >= 1)
        {
            m_pfCoeff[1] = (fSinAzim * fCosElev) * m_pfOrderWeights[1]; // Y
            m_pfCoeff[2] = (fSinElev) * m_pfOrderWeights[1]; // Z
            m_pfCoeff[3] = (fCosAzim * fCosElev) * m_pfOrderWeights[1]; // X
        }
        if(m_nOrder >= 2)
        {
            m_pfCoeff[4] = fSqrt32*(fSin2Azim * powf(fCosElev, 2)) * m_pfOrderWeights[2]; // V
            m_pfCoeff[5] = fSqrt32*(fSinAzim * fSin2Elev) * m_pfOrderWeights[2]; // T
            m_pfCoeff[6] = (1.5f * powf(fSinElev, 2.f) - 0.5f) * m_pfOrderWeights[2]; // R
            m_pfCoeff[7] = fSqrt32*(fCosAzim * fSin2Elev) * m_pfOrderWeights[2]; // S
            m_pfCoeff[8] = fSqrt32*(fCos2Azim * powf(fCosElev, 2)) * m_pfOrderWeights[2]; // U
        }
        if(m_nOrder >= 3)
        {
            m_pfCoeff[9] = fSqrt58*(sinf(3.f * m_polPosition.fAzimuth) * powf(fCosElev, 3.f)) * m_pfOrderWeights[3]; // Q
            m_pfCoeff[10] = fSqrt152*(fSin2Azim * fSinElev * powf(fCosElev, 2.f)) * m_pfOrderWeights[3]; // O
            m_pfCoeff[11] = fSqrt38*(fSinAzim * fCosElev * (5.f * powf(fSinElev, 2.f) - 1.f)) * m_pfOrderWeights[3]; // M
            m_pfCoeff[12] = (fSinElev * (5.f * powf(fSinElev, 2.f) - 3.f) * 0.5f) * m_pfOrderWeights[3]; // K
            m_pfCoeff[13] = fSqrt38*(fCosAzim * fCosElev * (5.f * powf(fSinElev, 2.f) - 1.f)) * m_pfOrderWeights[3]; // L
            m_pfCoeff[14] = fSqrt152*(fCos2Azim * fSinElev * powf(fCosElev, 2.f)) * m_pfOrderWeights[3]; // N
            m_pfCoeff[15] = fSqrt58*(cosf(3.f * m_polPosition.fAzimuth) * powf(fCosElev, 3.f)) * m_pfOrderWeights[3]; // P

        }
    }
    else
    {
        if(m_nOrder >= 0)
        {
            m_pfCoeff[0] = m_pfOrderWeights[0];
        }
        if(m_nOrder >= 1)
        {
            m_pfCoeff[1] = (fCosAzim * fCosElev) * m_pfOrderWeights[1];
            m_pfCoeff[2] = (fSinAzim * fCosElev) * m_pfOrderWeights[1];
        }
        if(m_nOrder >= 2)
        {
            m_pfCoeff[3] = (fCos2Azim * powf(fCosElev, 2)) * m_pfOrderWeights[2];
            m_pfCoeff[4] = (fSin2Azim * powf(fCosElev, 2)) * m_pfOrderWeights[2];
        }
        if(m_nOrder >= 3)
        {
            m_pfCoeff[5] = (cosf(3.f * m_polPosition.fAzimuth) * powf(fCosElev, 3.f)) * m_pfOrderWeights[3];
            m_pfCoeff[6] = (sinf(3.f * m_polPosition.fAzimuth) * powf(fCosElev, 3.f)) * m_pfOrderWeights[3];
        }
    }

    for(unsigned ni = 0; ni < m_nChannelCount; ni++)
        m_pfCoeff[ni] *= m_fGain;
}

void CAmbisonicSource::SetPosition(PolarPoint polPosition)
{
    m_polPosition = polPosition;
}

PolarPoint CAmbisonicSource::GetPosition()
{
    return m_polPosition;
}

void CAmbisonicSource::SetOrderWeight(unsigned nOrder, float fWeight)
{
    m_pfOrderWeights[nOrder] = fWeight;
}

void CAmbisonicSource::SetOrderWeightAll(float fWeight)
{
    for(unsigned niOrder = 0; niOrder < m_nOrder + 1; niOrder++)
    {
        m_pfOrderWeights[niOrder] = fWeight;
    }
}

void CAmbisonicSource::SetCoefficient(unsigned nChannel, float fCoeff)
{
    m_pfCoeff[nChannel] = fCoeff;
}

float CAmbisonicSource::GetOrderWeight(unsigned nOrder)
{
    return m_pfOrderWeights[nOrder];
}

float CAmbisonicSource::GetCoefficient(unsigned nChannel)
{
    return m_pfCoeff[nChannel];
}

void CAmbisonicSource::SetGain(float fGain)
{
    m_fGain = fGain;
}

float CAmbisonicSource::GetGain()
{
    return m_fGain;
}
