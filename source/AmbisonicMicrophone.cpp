/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicMicrophone - Ambisonic Microphone                             #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicMicrophone.cpp                                  #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicMicrophone.h"

CAmbisonicMicrophone::CAmbisonicMicrophone()
{
	m_fDirectivity = 1.f;
    Create(DEFAULT_ORDER, DEFAULT_HEIGHT, 0);
	Refresh();
}

CAmbisonicMicrophone::~CAmbisonicMicrophone()
{

}

void CAmbisonicMicrophone::Refresh()
{
	CAmbisonicSource::Refresh();

	m_pfCoeff[0] *= (2.f - m_fDirectivity) * sqrtf(2.f);
}

void CAmbisonicMicrophone::Process(CBFormat* pBFSrc, AmbUInt nSamples, AmbFloat* pfDst)
{
	AmbUInt niChannel = 0;
	AmbUInt niSample = 0;
	AmbFloat fTempA = 0;
	AmbFloat fTempB = 0;
	for(niSample = 0; niSample < nSamples; niSample++)
	{
		fTempA = pBFSrc->m_ppfChannels[0][niSample] * m_pfCoeff[0];
		fTempB = 0;
		for(niChannel = 1; niChannel < m_nChannelCount; niChannel++)
		{
			fTempB += pBFSrc->m_ppfChannels[niChannel][niSample] * m_pfCoeff[niChannel]; 
		}
		pfDst[niSample] = 0.5f * (fTempA + fTempB * m_fDirectivity);
	}
}

void CAmbisonicMicrophone::SetDirectivity(AmbFloat fDirectivity)
{
	m_fDirectivity = fDirectivity;
}

AmbFloat CAmbisonicMicrophone::GetDirectivity()
{
	return m_fDirectivity;
}