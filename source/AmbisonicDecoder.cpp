/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicDecoder - Ambisonic Decoder                                   #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicDecoder.cpp                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicDecoder.h"

CAmbisonicDecoder::CAmbisonicDecoder()
{
	m_nSpeakerSetUp = 0;
	m_nSpeakers = 0;
	m_pAmbSpeakers = 0;

	Create(DEFAULT_ORDER, DEFAULT_HEIGHT, kAmblib_50, 0);
}

CAmbisonicDecoder::~CAmbisonicDecoder()
{
	if(m_pAmbSpeakers)
		delete [] m_pAmbSpeakers;
}

bool CAmbisonicDecoder::Create(AmbUInt nOrder, AmbBool b3D, AmbInt nSpeakerSetUp, AmbUInt nSpeakers)
{
	bool success = CAmbisonicBase::Create(nOrder, b3D, nSpeakerSetUp);
    if(!success)
        return false;
	SpeakerSetUp(nSpeakerSetUp, nSpeakers);
	Refresh();
    
    return true;
}

void CAmbisonicDecoder::Reset()
{
	for(AmbUInt niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		m_pAmbSpeakers[niSpeaker].Reset();
}

void CAmbisonicDecoder::Refresh()
{
	for(AmbUInt niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		m_pAmbSpeakers[niSpeaker].Refresh();	
}

void CAmbisonicDecoder::Process(CBFormat* pBFSrc, AmbUInt nSamples, AmbFloat** ppfDst)
{
	for(AmbUInt niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
	{
		m_pAmbSpeakers[niSpeaker].Process(pBFSrc, nSamples, ppfDst[niSpeaker]);
	}
}

AmbInt CAmbisonicDecoder::GetSpeakerSetUp()
{
	return m_nSpeakerSetUp;
}

AmbUInt CAmbisonicDecoder::GetSpeakerCount()
{
	return m_nSpeakers;
}

void CAmbisonicDecoder::SetPosition(AmbUInt nSpeaker, PolarPoint polPosition)
{
	m_pAmbSpeakers[nSpeaker].SetPosition(polPosition);
}

PolarPoint CAmbisonicDecoder::GetPosition(AmbUInt nSpeaker)
{
	return m_pAmbSpeakers[nSpeaker].GetPosition();
}

void CAmbisonicDecoder::SetOrderWeight(AmbUInt nSpeaker, AmbUInt nOrder, AmbFloat fWeight)
{
	m_pAmbSpeakers[nSpeaker].SetOrderWeight(nOrder, fWeight);
}

AmbFloat CAmbisonicDecoder::GetOrderWeight(AmbUInt nSpeaker, AmbUInt nOrder)
{
	return m_pAmbSpeakers[nSpeaker].GetOrderWeight(nOrder);
}

AmbFloat CAmbisonicDecoder::GetCoefficient(AmbUInt nSpeaker, AmbUInt nChannel)
{
	return m_pAmbSpeakers[nSpeaker].GetCoefficient(nChannel);
}

void CAmbisonicDecoder::SpeakerSetUp(AmbInt nSpeakerSetUp, AmbUInt nSpeakers)
{
	m_nSpeakerSetUp = nSpeakerSetUp;

	if(m_pAmbSpeakers)
		delete [] m_pAmbSpeakers;

	PolarPoint polPosition = {0.f, 0.f, 1.f};
	AmbUInt niSpeaker = 0;
	AmbFloat fSpeakerGain = 0.f;

	switch(m_nSpeakerSetUp)
	{
	case kAmblib_CustomSpeakerSetUp:
		m_nSpeakers = nSpeakers;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
		}
		break;
	case kAmblib_Mono:
		m_nSpeakers = 1;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		break;
	case kAmblib_Stereo:
		m_nSpeakers = 2;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		polPosition.fAzimuth = DegreesToRadians(30.f);
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-30.f);
		m_pAmbSpeakers[1].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[1].SetPosition(polPosition);
		break;
	case kAmblib_LCR:
		m_nSpeakers = 3;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		polPosition.fAzimuth = DegreesToRadians(30.f);
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(0.f);
		m_pAmbSpeakers[1].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[1].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-30.f);
		m_pAmbSpeakers[2].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[2].SetPosition(polPosition);
		break;
	case kAmblib_Quad:
		m_nSpeakers = 4;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		polPosition.fAzimuth = DegreesToRadians(45.f);
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-45.f);
		m_pAmbSpeakers[1].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[1].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(135.f);
		m_pAmbSpeakers[2].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[2].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-135.f);
		m_pAmbSpeakers[3].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[3].SetPosition(polPosition);
		break;
	case kAmblib_50:
		m_nSpeakers = 5;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		polPosition.fAzimuth = DegreesToRadians(30.f);
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-30.f);
		m_pAmbSpeakers[1].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[1].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(0.f);
		m_pAmbSpeakers[2].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[2].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(110.f);
		m_pAmbSpeakers[3].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[3].SetPosition(polPosition);
		polPosition.fAzimuth = DegreesToRadians(-110.f);
		m_pAmbSpeakers[4].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[4].SetPosition(polPosition);
		break;
	case kAmblib_Pentagon:
		m_nSpeakers = 5;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	case kAmblib_Hexagon:
		m_nSpeakers = 6;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers + 30.f);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
    case kAmblib_HexagonWithCentre:
		m_nSpeakers = 6;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	case kAmblib_Octagon:
		m_nSpeakers = 8;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	case kAmblib_Decadron:
		m_nSpeakers = 10;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	case kAmblib_Dodecadron:
		m_nSpeakers = 12;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / m_nSpeakers);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	case kAmblib_Cube:
		m_nSpeakers = 8;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		polPosition.fElevation = DegreesToRadians(45.f);
		for(niSpeaker = 0; niSpeaker < m_nSpeakers / 2; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians(niSpeaker * 360.f / (m_nSpeakers / 2) + 45.f);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		polPosition.fElevation = DegreesToRadians(-45.f);
		for(niSpeaker = m_nSpeakers / 2; niSpeaker < m_nSpeakers; niSpeaker++)
		{
			polPosition.fAzimuth = -DegreesToRadians((niSpeaker - 4) * 360.f / (m_nSpeakers / 2) + 45.f);
			m_pAmbSpeakers[niSpeaker].Create(m_nOrder, m_b3D, 0);
			m_pAmbSpeakers[niSpeaker].SetPosition(polPosition);
		}
		break;
	default:
		m_nSpeakers = 1;
		m_pAmbSpeakers = new CAmbisonicSpeaker[m_nSpeakers];
		m_pAmbSpeakers[0].Create(m_nOrder, m_b3D, 0);
		m_pAmbSpeakers[0].SetPosition(polPosition);
		break;
	};

	fSpeakerGain = 1.f / sqrtf((float)m_nSpeakers);
	for(niSpeaker = 0; niSpeaker < m_nSpeakers; niSpeaker++)
		m_pAmbSpeakers[niSpeaker].SetGain(fSpeakerGain);
}