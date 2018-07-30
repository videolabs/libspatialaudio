/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicZoomer - Ambisonic Zoomer                                     #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicZoomer.cpp                                      #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL (+ Proprietary)                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicZoomer.h"

#include <iostream>
#include <algorithm>

CAmbisonicZoomer::CAmbisonicZoomer()
{
    m_fZoom = 0;
}

bool CAmbisonicZoomer::Configure(unsigned nOrder, bool b3D, unsigned nMisc)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;

    m_AmbDecoderFront.Configure(m_nOrder, 1, kAmblib_Mono, 1);

    //Calculate all the speaker coefficients
    m_AmbDecoderFront.Refresh();

    m_fZoomRed = 0.f;

    m_AmbEncoderFront.reset(new float[m_nChannelCount]);
    m_AmbEncoderFront_weighted.reset(new float[m_nChannelCount]);
    a_m.reset(new float[m_nOrder + 1]);

    // These weights a_m are applied to the channels of a corresponding order within the Ambisonics signals.
    // When applied to the encoded channels and decoded to a particular loudspeaker direction they will create a
    // virtual microphone pattern with no rear lobes.
    // When used for decoding this is known as in-phase decoding.
    for(unsigned iOrder = 0; iOrder < m_nOrder + 1; iOrder++)
        a_m[iOrder] = (2*iOrder+1)*factorial(m_nOrder)*factorial(m_nOrder+1) / (factorial(m_nOrder+iOrder+1)*factorial(m_nOrder-iOrder));

    unsigned iDegree=0;
    for(unsigned iChannel = 0; iChannel<m_nChannelCount; iChannel++)
    {
        m_AmbEncoderFront[iChannel] = m_AmbDecoderFront.GetCoefficient(0, iChannel);
        iDegree = (int)floor(sqrt(iChannel));
        m_AmbEncoderFront_weighted[iChannel] = m_AmbEncoderFront[iChannel] * a_m[iDegree];
        // Normalisation factor
        m_AmbFrontMic += m_AmbEncoderFront[iChannel] * m_AmbEncoderFront_weighted[iChannel];
    }

    return true;
}

void CAmbisonicZoomer::Reset()
{

}

void CAmbisonicZoomer::Refresh()
{
    m_fZoomRed = sqrtf(1.f - m_fZoom * m_fZoom);
    m_fZoomBlend = 1.f - m_fZoom;
}

void CAmbisonicZoomer::SetZoom(float fZoom)
{
    // Limit the zoom value to always preserve the spacial effect.
    m_fZoom = std::min(fZoom, 0.99f);
}

float CAmbisonicZoomer::GetZoom()
{
    return m_fZoom;
}

void CAmbisonicZoomer::Process(CBFormat* pBFSrcDst, unsigned nSamples)
{
    for(unsigned niSample = 0; niSample < nSamples; niSample++)
    {
        float fMic = 0.f;

        for(unsigned iChannel=0; iChannel<m_nChannelCount; iChannel++)
        {
            // virtual microphone with polar pattern narrowing as Ambisonic order increases
            fMic += m_AmbEncoderFront_weighted[iChannel] * pBFSrcDst->m_ppfChannels[iChannel][niSample];
        }
        for(unsigned iChannel=0; iChannel<m_nChannelCount; iChannel++)
        {
            if(abs(m_AmbEncoderFront[iChannel])>1e-6)
            {
                // Blend original channel with the virtual microphone pointed directly to the front
                // Only do this for Ambisonics components that aren't zero for an encoded frontal source
                pBFSrcDst->m_ppfChannels[iChannel][niSample] = (m_fZoomBlend * pBFSrcDst->m_ppfChannels[iChannel][niSample]
                    + m_AmbEncoderFront[iChannel]*m_fZoom*fMic) / (m_fZoomBlend + fabs(m_fZoom)*m_AmbFrontMic);
            }
            else{
                // reduce the level of the Ambisonic components that are zero for a frontal source
                pBFSrcDst->m_ppfChannels[iChannel][niSample] = pBFSrcDst->m_ppfChannels[iChannel][niSample] * m_fZoomRed;
            }
        }
    }
}

float CAmbisonicZoomer::factorial(unsigned M)
{
    unsigned ret = 1;
    for(unsigned int i = 1; i <= M; ++i)
        ret *= i;
    return ret;
}
