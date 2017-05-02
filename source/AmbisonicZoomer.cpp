/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicZoomer - Ambisonic Zoomer                                     #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicZoomer.cpp                                      #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicZoomer.h"

CAmbisonicZoomer::CAmbisonicZoomer()
{
	m_fZoom = 0;
	m_fWCoeff = 0;
	m_fXCoeff = 0;
	m_fYZCoeff = 0;
    m_pProcessFunction = 0;
}

CAmbisonicZoomer::~CAmbisonicZoomer()
{
	
}

bool CAmbisonicZoomer::Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc)
{
	bool success = CAmbisonicBase::Create(nOrder, b3D, nMisc);
    if(!success)
        return false;

    if(m_b3D)
        m_pProcessFunction = &CAmbisonicZoomer::Process3D;
    else
        m_pProcessFunction = &CAmbisonicZoomer::Process2D;
    
    return true;
}

void CAmbisonicZoomer::Reset()
{
	
}

void CAmbisonicZoomer::Refresh()
{
	m_fWCoeff = 1.f / sqrtf(2.f) * m_fZoom;
	m_fXCoeff = sqrtf(2.f) * m_fZoom;
	m_fYZCoeff = sqrtf(1.f - m_fZoom * m_fZoom);
}

void CAmbisonicZoomer::SetZoom(AmbFloat fZoom)
{
	m_fZoom = fZoom;
}

AmbFloat CAmbisonicZoomer::GetZoom()
{
	return m_fZoom;
}

void CAmbisonicZoomer::Process(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
    (this->*m_pProcessFunction)(pBFSrcDst, nSamples);
}

void CAmbisonicZoomer::Process2D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
    AmbFloat fW = 0.f;
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		fW = pBFSrcDst->m_ppfChannels[kW][niSample];
		pBFSrcDst->m_ppfChannels[kW][niSample] = pBFSrcDst->m_ppfChannels[kW][niSample] + m_fWCoeff * pBFSrcDst->m_ppfChannels[kX][niSample];
		pBFSrcDst->m_ppfChannels[kX][niSample] = pBFSrcDst->m_ppfChannels[kX][niSample] + m_fXCoeff * fW;
		pBFSrcDst->m_ppfChannels[kY][niSample] *= m_fYZCoeff;
	}
}

void CAmbisonicZoomer::Process3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
    AmbFloat fW = 0.f;
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		fW = pBFSrcDst->m_ppfChannels[kW][niSample];
		pBFSrcDst->m_ppfChannels[kW][niSample] = pBFSrcDst->m_ppfChannels[kW][niSample] + m_fWCoeff * pBFSrcDst->m_ppfChannels[kX][niSample];
		pBFSrcDst->m_ppfChannels[kX][niSample] = pBFSrcDst->m_ppfChannels[kX][niSample] + m_fXCoeff * fW;
		pBFSrcDst->m_ppfChannels[kY][niSample] *= m_fYZCoeff;
		pBFSrcDst->m_ppfChannels[kZ][niSample] *= m_fYZCoeff;
	}
}
