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
#include <assert.h>
#include <cmath>

CAmbisonicEncoder::CAmbisonicEncoder() : m_coeffInterp(0)
{ }

CAmbisonicEncoder::~CAmbisonicEncoder()
{ }

bool CAmbisonicEncoder::Configure(unsigned nOrder, bool b3D, unsigned sampleRate, float fadeTimeMilliSec)
{
    bool success = CAmbisonicSource::Configure(nOrder, b3D, sampleRate);
    if(!success || fadeTimeMilliSec < 0.f)
        return false;

    m_pfCoeffCurrent.resize(m_nChannelCount);
    m_coeffInterp = CGainInterp<float>(m_nChannelCount);

    m_fadingTimeMilliSec = fadeTimeMilliSec;
    m_fadingSamples = (unsigned)std::round(0.001f * m_fadingTimeMilliSec * (float)sampleRate);

    return true;
}

void CAmbisonicEncoder::Refresh()
{
    CAmbisonicSource::Refresh();
}

void CAmbisonicEncoder::Reset()
{
    CAmbisonicSource::Reset();
    m_coeffInterp.Reset();
}

void CAmbisonicEncoder::SetPosition(PolarPoint polPosition)
{
    // Update the coefficients
    CAmbisonicSource::SetPosition(polPosition);
    CAmbisonicSource::Refresh();
    CAmbisonicSource::GetCoefficients(m_pfCoeffCurrent);
    m_coeffInterp.SetGainVector(m_pfCoeffCurrent, m_fadingSamples);
}

void CAmbisonicEncoder::Process(float* pfSrc, unsigned nSamples, CBFormat* pfDst, unsigned int nOffset)
{
    assert(nSamples + nOffset <= pfDst->GetSampleCount()); // Cannot write beyond the of the destination buffers!

    m_coeffInterp.Process(pfSrc, pfDst->m_ppfChannels.get(), nSamples, nOffset);
}

void CAmbisonicEncoder::ProcessAccumul(float* pfSrc, unsigned nSamples, CBFormat* pfDst, unsigned int nOffset, float fGain)
{
    assert(nSamples + nOffset <= pfDst->GetSampleCount()); // Cannot write beyond the of the destination buffers!

    m_coeffInterp.ProcessAccumul(pfSrc, pfDst->m_ppfChannels.get(), nSamples, nOffset, fGain);
}
