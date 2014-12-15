/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicSpeaker - Ambisonic Speaker                                   #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicSpeaker.cpp                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicSpeaker.h"


CAmbisonicSpeaker::CAmbisonicSpeaker()
{
    Create(DEFAULT_ORDER, DEFAULT_HEIGHT, 0);
	Refresh();
}

CAmbisonicSpeaker::~CAmbisonicSpeaker()
{

}

bool CAmbisonicSpeaker::Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc)
{
    bool success = CAmbisonicSource::Create(nOrder, b3D, nMisc);
    if(!success)
        return false;
    SetOrderWeight(0, sqrtf(2.f));
    
    return true;
}

void CAmbisonicSpeaker::Refresh()
{
	CAmbisonicSource::Refresh();
}

void CAmbisonicSpeaker::Process(CBFormat* pBFSrc, AmbUInt nSamples, AmbFloat* pfDst)
{
	AmbUInt niChannel = 0;
	AmbUInt niSample = 0;
	memset(pfDst, 0, nSamples * sizeof(AmbFloat));
	for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
	{
		for(niSample = 0; niSample < nSamples; niSample++)
		{
			pfDst[niSample] += pBFSrc->m_ppfChannels[niChannel][niSample] * m_pfCoeff[niChannel];
		}
	}
}