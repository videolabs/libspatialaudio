/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicSpeaker - Ambisonic Speaker                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicSpeaker.cpp                                     #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL (+ Proprietary)                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicSpeaker.h"


CAmbisonicSpeaker::CAmbisonicSpeaker()
{ }

CAmbisonicSpeaker::~CAmbisonicSpeaker()
{ }

bool CAmbisonicSpeaker::Configure(unsigned nOrder, bool b3D, unsigned nMisc)
{
    bool success = CAmbisonicSource::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;
    
    return true;
}

void CAmbisonicSpeaker::Refresh()
{
    CAmbisonicSource::Refresh();
}

void CAmbisonicSpeaker::Process(CBFormat* pBFSrc, unsigned nSamples, float* pfDst)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    memset(pfDst, 0, nSamples * sizeof(float));
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        if(m_b3D){ /* Decode to a 3D loudspeaker array */
            for(niSample = 0; niSample < nSamples; niSample++)
            {
                // The spherical harmonic coefficients are multiplied by (2*order + 1) to provide the correct decoder
                // for SN3D normalised Ambisonic inputs.
                pfDst[niSample] += pBFSrc->m_ppfChannels[niChannel][niSample]
                            * m_pfCoeff[niChannel] * (2*floor(sqrt(niChannel)) + 1);
            }
        }
        else
        {    /* Decode to a 2D loudspeaker array */
            for(niSample = 0; niSample < nSamples; niSample++)
            {
                // The spherical harmonic coefficients are multiplied by 2 to provide the correct decoder
                // for SN3D normalised Ambisonic inputs decoded to a horizontal loudspeaker array
                pfDst[niSample] += pBFSrc->m_ppfChannels[niChannel][niSample]
                            * m_pfCoeff[niChannel] * 2.f;
            }
        }
    }
}
