/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicProcessor - Ambisonic Processor                               #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicProcessor.cpp                                   #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicProcessor.h"

CAmbisonicProcessor::CAmbisonicProcessor()
{
	m_orientation.fYaw = 0.f;
	m_orientation.fRoll = 0.f;
	m_orientation.fPitch = 0.f;
	m_pfTempSample = 0;
    Create(DEFAULT_ORDER, DEFAULT_HEIGHT, 0);
	Reset();
}

CAmbisonicProcessor::~CAmbisonicProcessor()
{
	if(m_pfTempSample)
		delete [] m_pfTempSample;
}

bool CAmbisonicProcessor::Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc)
{
	bool success = CAmbisonicBase::Create(nOrder, b3D, nMisc);
    if(!success)
        return false;
	if(m_pfTempSample)
		delete [] m_pfTempSample;
	m_pfTempSample = new AmbFloat[m_nChannelCount];
	memset(m_pfTempSample, 0, m_nChannelCount * sizeof(AmbFloat));
    
    return true;
}

void CAmbisonicProcessor::Reset()
{
	
}

void CAmbisonicProcessor::Refresh()
{
	m_fCosYaw = cosf(m_orientation.fYaw);
	m_fSinYaw = sinf(m_orientation.fYaw);
	m_fCosRoll = cosf(m_orientation.fRoll);
	m_fSinRoll = sinf(m_orientation.fRoll);
	m_fCosPitch = cosf(m_orientation.fPitch);
	m_fSinPitch = sinf(m_orientation.fPitch);
	m_fCos2Yaw = cosf(2.f * m_orientation.fYaw);
	m_fSin2Yaw = sinf(2.f * m_orientation.fYaw);
	m_fCos2Roll = cosf(2.f * m_orientation.fRoll);
	m_fSin2Roll = sinf(2.f * m_orientation.fRoll);
	m_fCos2Pitch = cosf(2.f * m_orientation.fPitch);
	m_fSin2Pitch = sinf(2.f * m_orientation.fPitch);
}

void CAmbisonicProcessor::SetOrientation(Orientation orientation)
{
	m_orientation = orientation;
}

Orientation CAmbisonicProcessor::GetOrientation()
{
	return m_orientation;
}

void CAmbisonicProcessor::Process(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	if(m_b3D)
	{
		if(m_nOrder >= 1)
			ProcessOrder1_3D(pBFSrcDst, nSamples);
		if(m_nOrder >= 2)
			ProcessOrder2_3D(pBFSrcDst, nSamples);
		if(m_nOrder >= 3)
			ProcessOrder3_3D(pBFSrcDst, nSamples);
	}
	else
	{
		if(m_nOrder >= 1)
			ProcessOrder1_2D(pBFSrcDst, nSamples);
		if(m_nOrder >= 2)
			ProcessOrder2_2D(pBFSrcDst, nSamples);
		if(m_nOrder >= 3)
			ProcessOrder3_2D(pBFSrcDst, nSamples);
	}	
}

void CAmbisonicProcessor::ProcessOrder1_3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		//Yaw
		m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosYaw 
							- pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinYaw;
		m_pfTempSample[kY] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinYaw 
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosYaw;
		m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
		
		//Roll
		pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
		pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY] * m_fCosRoll 
												- m_pfTempSample[kZ] * m_fSinRoll;
		pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kY] * m_fSinRoll 
												+ m_pfTempSample[kZ] * m_fCosRoll;

		//Pitch
		m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosPitch 
							- pBFSrcDst->m_ppfChannels[kZ][niSample] * m_fSinPitch;
		m_pfTempSample[kY] = pBFSrcDst->m_ppfChannels[kY][niSample];
		m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinPitch 
							+ pBFSrcDst->m_ppfChannels[kZ][niSample] * m_fCosPitch;

		pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
		pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
		pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ];
	}
}

void CAmbisonicProcessor::ProcessOrder2_3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		//Yaw
		m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
		m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosYaw 
							- pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinYaw;
		m_pfTempSample[kT] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinYaw 
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosYaw;
		m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Yaw 
							- pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Yaw;
		m_pfTempSample[kV] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Yaw 
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Yaw;

		//Roll
		pBFSrcDst->m_ppfChannels[kR][niSample] = (0.75f * m_fCos2Roll + 0.25f) * m_pfTempSample[kR]
												+ (0.75f * m_fSin2Roll) * m_pfTempSample[kT]
												+ (0.375f * m_fCos2Roll - 0.375f) * m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kS][niSample] = m_fCosRoll * m_pfTempSample[kS]
												+ m_fSinRoll * m_pfTempSample[kV];
		pBFSrcDst->m_ppfChannels[kT][niSample] = -m_fSin2Roll * m_pfTempSample[kR]
												+ m_fCos2Roll * m_pfTempSample[kT]
												- (0.5f * m_fSin2Roll) * m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kU][niSample] = (0.5f * m_fCos2Roll - 0.5f) * m_pfTempSample[kR]
												+ (0.5f * m_fSin2Roll) * m_pfTempSample[kT]
												+ (0.25f * m_fCos2Roll + 0.75f) * m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kV][niSample] = -m_fSinRoll * m_pfTempSample[kS]
												+ m_fCosRoll * m_pfTempSample[kV];

		//Pitch
		m_pfTempSample[kR] = (0.75f * m_fCos2Pitch + 0.25f) * pBFSrcDst->m_ppfChannels[kR][niSample]
							+ (0.75f * m_fSin2Pitch) * pBFSrcDst->m_ppfChannels[kS][niSample]
							+ (0.375f - 0.375f * m_fCos2Pitch) * pBFSrcDst->m_ppfChannels[kU][niSample];
		m_pfTempSample[kS] = -m_fSin2Pitch * pBFSrcDst->m_ppfChannels[kR][niSample]
							+ m_fCos2Pitch * pBFSrcDst->m_ppfChannels[kS][niSample]
							+ (0.5f * m_fSin2Pitch) * pBFSrcDst->m_ppfChannels[kU][niSample];
		m_pfTempSample[kT] = m_fCosPitch * pBFSrcDst->m_ppfChannels[kT][niSample]
							+ m_fSinPitch * pBFSrcDst->m_ppfChannels[kV][niSample];
		m_pfTempSample[kU] = (0.5f - 0.5f * m_fCos2Pitch) * pBFSrcDst->m_ppfChannels[kR][niSample]
							- (0.5f * m_fSin2Pitch) * pBFSrcDst->m_ppfChannels[kS][niSample]
							+ (0.25f * m_fCos2Pitch + 0.75f) * pBFSrcDst->m_ppfChannels[kU][niSample];
		m_pfTempSample[kV] = -m_fSinPitch * pBFSrcDst->m_ppfChannels[kT][niSample]
							+ m_fCosPitch * pBFSrcDst->m_ppfChannels[kV][niSample];

		pBFSrcDst->m_ppfChannels[kR][niSample] = m_pfTempSample[kR];
		pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
		pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
		pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
	}
}

void CAmbisonicProcessor::ProcessOrder3_3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	//TODO
}

void CAmbisonicProcessor::ProcessOrder1_2D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		//Yaw
		m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosYaw 
							- pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinYaw;
		m_pfTempSample[kY] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinYaw 
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosYaw;
		
		pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
		pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
	}
}

void CAmbisonicProcessor::ProcessOrder2_2D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		//Yaw
		m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosYaw 
							- pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinYaw;
		m_pfTempSample[kT] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinYaw 
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosYaw;
		m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Yaw 
							- pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Yaw;
		m_pfTempSample[kV] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Yaw 
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Yaw;
	
		pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
		pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
		pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
	}
}

void CAmbisonicProcessor::ProcessOrder3_2D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	//TODO
}