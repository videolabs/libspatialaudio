/*############################################################################*/
/*#                                                                          #*/
/*#  A biquad IIR filter with options for low- and high-pass                 #*/
/*#                                                                          #*/
/*#                                                                          #*/
/*#  Filename:      IIRFilter.cpp                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/04/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "IIRFilter.h"
#include "Tools.h" // for M_PI

CIIRFilter::CIIRFilter()
{
    m_b.resize(3, 0.);
    m_a.resize(2, 0.);
}

CIIRFilter::~CIIRFilter()
{
}

bool CIIRFilter::Configure(unsigned int nCh, unsigned int sampleRate, float frequency, float q, FilterType filterType)
{
    // Must process at least one channel, have a positive, non-zero sample rate and
    // the cutoff frequency must not be above Nyquist
    if (nCh < 1 || sampleRate <= 0 || frequency >= static_cast<float>(sampleRate) / 2.f)
        return false;

    m_nCh = nCh;

    // Allocate state for each of the channels
    m_state1.resize(nCh, 0.f);
    m_state2.resize(nCh, 0.f);

    // Calculate the filter coefficients using the RBJ Cookbook equations: https://www.musicdsp.org/en/latest/Filters/197-rbj-audio-eq-cookbook.html
    auto w0 = 2.f * static_cast<float>(M_PI) * frequency / static_cast<float>(sampleRate);
    auto sinw0 = std::sin(w0);
    auto cosw0 = std::cos(w0);
    auto alpha = sinw0 / (2.f * q);
    auto a0 = 1.f + alpha;
    switch (filterType)
    {
    case FilterType::LowPass:
        m_b[0] = (1.f - cosw0) / (2.f * a0);
        m_b[1] = (1.f - cosw0) / a0;
        m_b[2] = (1.f - cosw0) / (2.f * a0);
        m_a[0] = -2.f * cosw0 / a0;
        m_a[1] = (1.f - alpha) / a0;
        break;
    case FilterType::HighPass:
        m_b[0] = (1.f + cosw0) / (2.f * a0);
        m_b[1] = -(1.f + cosw0) / a0;
        m_b[2] = (1.f + cosw0) / (2.f * a0);
        m_a[0] = -2.f * cosw0 / a0;
        m_a[1] = (1.f - alpha) / a0;
        break;
    default:
        break;
    }

    Reset();

    return true;
}

void CIIRFilter::Reset()
{
    // Clear the states for all channels
    for (int iCh = 0; iCh < m_nCh; ++iCh)
    {
        m_state1[iCh] = 0.f;
        m_state2[iCh] = 0.f;
    }
}

void CIIRFilter::Process(float** pIn, float** pOut, unsigned int nSamples)
{
    auto b0 = m_b[0];
    auto b1 = m_b[1];
    auto b2 = m_b[2];
    auto a1 = m_a[0];
    auto a2 = m_a[1];

    for (int iCh = 0; iCh < m_nCh; ++iCh)
    {
        auto& state1 = m_state1[iCh];
        auto& state2 = m_state2[iCh];
        for (unsigned int iSamp = 0; iSamp < nSamples; ++iSamp)
        {
            float w = pIn[iCh][iSamp] - a1 * state1 - a2 * state2;
            pOut[iCh][iSamp] = b0 * w + b1 * state1 + b2 * state2;

            state2 = state1;
            state1 = w;
        }
        // If state is very small then snap to zero to avoid denormals
        if (std::abs(state1) < 1e-8f)
            state1 = 0.f;
        if (std::abs(state2) < 1e-8f)
            state2 = 0.f;
    }
}
