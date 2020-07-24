/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoder - Ambisonic Encoder                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicEncoder.cpp                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicEncoder.h"


CAmbisonicEncoder::CAmbisonicEncoder()
{ }

CAmbisonicEncoder::~CAmbisonicEncoder()
{ }

bool CAmbisonicEncoder::Configure(unsigned nOrder, bool b3D, unsigned nMisc)
{
    bool success = CAmbisonicSource::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;
    //SetOrderWeight(0, 1.f / sqrtf(2.f)); // Removed as seems to break SN3D normalisation
    
    return true;
}

void CAmbisonicEncoder::Refresh()
{
    CAmbisonicSource::Refresh();
}

void CAmbisonicEncoder::SetPosition(PolarPoint polPosition, float interpDur)
{
    // Set the interpolation duration
    m_fInterpDur = interpDur;
    // Store the last set coefficients
    m_pfCoeffOld = m_pfCoeff;
    // Update the coefficients
    CAmbisonicSource::SetPosition(polPosition);
    Refresh();
}

void CAmbisonicEncoder::Process(float* pfSrc, unsigned nSamples, CBFormat* pfDst)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    if (m_fInterpDur > 0.f)
    {
        // Number of samples expected per frame
        for (niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            unsigned int nInterpSamples = (int)roundf(m_fInterpDur * nSamples);
            float deltaCoeff = (m_pfCoeff[niChannel] - m_pfCoeffOld[niChannel]) / ((float)nInterpSamples);
            for (niSample = 0; niSample < nInterpSamples; niSample++)
            {
                float fInterp = niSample * deltaCoeff;
                pfDst->m_ppfChannels[niChannel][niSample] = pfSrc[niSample] * (fInterp*m_pfCoeff[niChannel] + (1.f-fInterp)*m_pfCoeffOld[niChannel]);
            }
            // once interpolation has finished
            for (niSample = nInterpSamples; niSample < nSamples; niSample++)
            {
                pfDst->m_ppfChannels[niChannel][niSample] = pfSrc[niSample] * m_pfCoeff[niChannel];
            }
        }
        // Set interpolation duration to zero so none is applied on next call
        m_fInterpDur = 0.f;
    }
    else
    {
        for (niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            for (niSample = 0; niSample < nSamples; niSample++)
            {
                pfDst->m_ppfChannels[niChannel][niSample] = pfSrc[niSample] * m_pfCoeff[niChannel];
            }
        }
    }
}

void CAmbisonicEncoder::ProcessAccumul(float* pfSrc, unsigned nSamples, CBFormat* pfDst, unsigned int nOffset)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    if (m_fInterpDur > 0.f)
    {
        // Number of samples expected per frame
        for (niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            unsigned int nInterpSamples = (int)roundf(m_fInterpDur * nSamples);
            float deltaCoeff = 1.f / (float)nInterpSamples;
            for (niSample = 0; niSample < nInterpSamples; niSample++)
            {
                float fInterp = niSample * deltaCoeff;
                pfDst->m_ppfChannels[niChannel][niSample + nOffset] += pfSrc[niSample] * (fInterp * m_pfCoeff[niChannel] + (1.f - fInterp) * m_pfCoeffOld[niChannel]);
            }
            // once interpolation has finished
            for (niSample = nInterpSamples; niSample < nSamples; niSample++)
            {
                pfDst->m_ppfChannels[niChannel][niSample + nOffset] += pfSrc[niSample] * m_pfCoeff[niChannel];
            }
        }
        // Set interpolation duration to zero so none is applied on next call
        m_fInterpDur = 0.f;
    }
    else
    {
        for (niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            for (niSample = 0; niSample < nSamples; niSample++)
            {
                pfDst->m_ppfChannels[niChannel][niSample + nOffset] += pfSrc[niSample] * m_pfCoeff[niChannel];
            }
        }
    }
}