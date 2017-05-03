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
#include <iostream>

CAmbisonicProcessor::CAmbisonicProcessor()
{
	m_orientation.fYaw = 0.f;
	m_orientation.fRoll = 0.f;
	m_orientation.fPitch = 0.f;
	m_orientation.fAlpha = 0.f;
	m_orientation.fBeta = 0.f;
	m_orientation.fGamma = 0.f;
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

	m_fCosAlpha = cosf(m_orientation.fAlpha);
	m_fSinAlpha = sinf(m_orientation.fAlpha);
	m_fCosBeta = cosf(m_orientation.fBeta);
	m_fSinBeta = sinf(m_orientation.fBeta);
	m_fCosGamma = cosf(m_orientation.fGamma);
	m_fSinGamma = sinf(m_orientation.fGamma);
	m_fCos2Alpha = cosf(2.f * m_orientation.fAlpha);
	m_fSin2Alpha = sinf(2.f * m_orientation.fAlpha);
	m_fCos2Beta = cosf(2.f * m_orientation.fBeta);
	m_fSin2Beta = sinf(2.f * m_orientation.fBeta);
	m_fCos2Gamma = cosf(2.f * m_orientation.fGamma);
	m_fSin2Gamma = sinf(2.f * m_orientation.fGamma);
	m_fCos3Alpha = cosf(3.f * m_orientation.fAlpha);
	m_fSin3Alpha = sinf(3.f * m_orientation.fAlpha);
	m_fCos3Beta = cosf(3.f * m_orientation.fBeta);
	m_fSin3Beta = sinf(3.f * m_orientation.fBeta);
	m_fCos3Gamma = cosf(3.f * m_orientation.fGamma);
	m_fSin3Gamma = sinf(3.f * m_orientation.fGamma);
}

void CAmbisonicProcessor::SetOrientation(Orientation orientation)
{
	/* Conversion from yaw, pitch, roll (ZYX) to ZYZ convention to match rotation matrices 
	This method reduces the complexity of the rotation matrices since the Z0 and Z1 rotations are the same form */
	AmbFloat r33 = m_fCosPitch * m_fCosRoll;
	if (r33 == 1.f)
	{
		orientation.fBeta = 0.f;
		orientation.fGamma = 0.f;
		orientation.fAlpha = atan2(m_fSinYaw,m_fCosYaw);
	}
	else {
		if (r33 == -1.f)
		{
			orientation.fBeta = M_PI;
			orientation.fGamma = 0.f;
			orientation.fAlpha = atan2(-m_fSinYaw,m_fCosYaw);
		}
		else
		{

			AmbFloat r32 = -m_fCosYaw * m_fSinRoll + m_fCosRoll * m_fSinPitch * m_fSinYaw ;
			AmbFloat r31 = m_fCosRoll * m_fCosYaw * m_fSinPitch + m_fSinRoll * m_fSinYaw ;
			orientation.fAlpha = atan2( r32 , r31 );

			orientation.fBeta = acos( r33 );

			AmbFloat r23 = m_fCosPitch * m_fSinRoll;
			AmbFloat r13 = -m_fSinPitch;
			orientation.fGamma = atan2( r23 , -r13 );
		}
	}

	/* Display for debugging */
	//std::cout<< "sinAlpha = " << m_fSinAlpha << " cosAlpha = " << m_fCosAlpha << std::endl;
	//std::cout<< "sinBeta = " << m_fSinBeta << " cosBeta = " << m_fCosBeta << std::endl;
	//std::cout<< "sinGamma = " << m_fSinGamma << " cosGamma = " << m_fCosGamma << std::endl;
	std::cout<<"Incoming angles from VLC video module"<<std::endl;
	std::cout<<"Yaw: "<< orientation.fYaw/M_PI*180.f <<
		" Pitch: " << orientation.fPitch/M_PI*180.f <<
		" Roll: " << orientation.fRoll/M_PI*180.f << std::endl;
	std::cout<<"ZYZ Euler angles used for the rotations"<<std::endl;
	std::cout<<"Alpha: "<< orientation.fAlpha/M_PI*180.f <<
		" Beta: " << orientation.fBeta/M_PI*180.f <<
		" Gamma: " << orientation.fGamma/M_PI*180.f << std::endl;
	std::cout<< " " << std::endl;


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
		// Alpha rotation
		m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinAlpha
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosAlpha;		
		m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];		
		m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosAlpha 
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinAlpha;

		// Beta rotation
		pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];		
		pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ] * m_fCosBeta 
							+  m_pfTempSample[kX] * m_fSinBeta;		
		pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX] * m_fCosBeta
							- m_pfTempSample[kZ] * m_fSinBeta;

		// Gamma rotation
		m_pfTempSample[kY] = -pBFSrcDst->m_ppfChannels[kX][niSample] * m_fSinGamma
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fCosGamma;
		m_pfTempSample[kZ] = pBFSrcDst->m_ppfChannels[kZ][niSample];
		m_pfTempSample[kX] = pBFSrcDst->m_ppfChannels[kX][niSample] * m_fCosGamma
							+ pBFSrcDst->m_ppfChannels[kY][niSample] * m_fSinGamma;

		pBFSrcDst->m_ppfChannels[kX][niSample] = m_pfTempSample[kX];
		pBFSrcDst->m_ppfChannels[kY][niSample] = m_pfTempSample[kY];
		pBFSrcDst->m_ppfChannels[kZ][niSample] = m_pfTempSample[kZ];
	}

	// Some debugging using dummy Ambisonics channels to test that the rotations are correct
/*	AmbFloat fY = 0.f; AmbFloat fZ = 0.f; AmbFloat fX = 1.f;
	AmbFloat fY_tmp;
	AmbFloat fZ_tmp;
	AmbFloat fX_tmp;
	AmbFloat fY_tmp2;
	AmbFloat fZ_tmp2;
	AmbFloat fX_tmp2;
	// Alpha rotation
	fY_tmp = -fX * m_fSinAlpha + fY * m_fCosAlpha;
	fZ_tmp = fZ;
	fX_tmp = fX * m_fCosAlpha + fY * m_fSinAlpha;
	// Beta rotation
	fY_tmp2 = fY_tmp;
	fZ_tmp2 = fZ_tmp * m_fCosBeta +  fX_tmp * m_fSinBeta;
	fX_tmp2 = fX_tmp * m_fCosBeta - fZ_tmp * m_fSinBeta;
	// Gamma rotation
	fY = -fX_tmp2 * m_fSinGamma + fY_tmp2 * m_fCosGamma;
	fZ = fZ_tmp2;
	fX = fX_tmp2 * m_fCosGamma + fY_tmp2 * m_fSinGamma;
	std::cout << "Ynew = " << fY << ", Znew = " << fZ << ", Xnew = " << fX << std::endl; */
	/* Post testing: This verifies that the first order components are correctly rotated */

}

void CAmbisonicProcessor::ProcessOrder2_3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
	AmbFloat fSqrt3 = sqrt(3.f);

	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		// Alpha rotation
		m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Alpha
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Alpha;
		m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinAlpha 
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosAlpha;
		m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
		m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosAlpha 
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinAlpha;
		m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Alpha 
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Alpha;

		// Beta rotation
		pBFSrcDst->m_ppfChannels[kV][niSample] = -m_fSinBeta * m_pfTempSample[kT]
										+ m_fCosBeta * m_pfTempSample[kV];
		pBFSrcDst->m_ppfChannels[kT][niSample] = -m_fCosBeta * m_pfTempSample[kT]
										+ m_fSinBeta * m_pfTempSample[kV];
		pBFSrcDst->m_ppfChannels[kR][niSample] = (0.75f * m_fCos2Beta + 0.25f) * m_pfTempSample[kR]
							+ (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * m_pfTempSample[kU]
							+ (fSqrt3 * m_fSinBeta * m_fCosBeta) * m_pfTempSample[kS];
		pBFSrcDst->m_ppfChannels[kS][niSample] = m_fCos2Beta * m_pfTempSample[kS] 
							- fSqrt3 * m_fCosBeta * m_fSinBeta * m_pfTempSample[kR]
							+ m_fCosBeta * m_fSinBeta * m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kU][niSample] = (0.25f * m_fCos2Beta + 0.75f) * m_pfTempSample[kU]
							- m_fCosBeta * m_fSinBeta * m_pfTempSample[kS]
							+0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * m_pfTempSample[kR];

		// Gamma rotation
		m_pfTempSample[kV] = - pBFSrcDst->m_ppfChannels[kU][niSample] * m_fSin2Gamma
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fCos2Gamma;
		m_pfTempSample[kT] = - pBFSrcDst->m_ppfChannels[kS][niSample] * m_fSinGamma
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fCosGamma;

		m_pfTempSample[kR] = pBFSrcDst->m_ppfChannels[kR][niSample];
		m_pfTempSample[kS] = pBFSrcDst->m_ppfChannels[kS][niSample] * m_fCosGamma 
							+ pBFSrcDst->m_ppfChannels[kT][niSample] * m_fSinGamma;
		m_pfTempSample[kU] = pBFSrcDst->m_ppfChannels[kU][niSample] * m_fCos2Gamma
							+ pBFSrcDst->m_ppfChannels[kV][niSample] * m_fSin2Gamma;

		pBFSrcDst->m_ppfChannels[kR][niSample] = m_pfTempSample[kR];
		pBFSrcDst->m_ppfChannels[kS][niSample] = m_pfTempSample[kS];
		pBFSrcDst->m_ppfChannels[kT][niSample] = m_pfTempSample[kT];
		pBFSrcDst->m_ppfChannels[kU][niSample] = m_pfTempSample[kU];
		pBFSrcDst->m_ppfChannels[kV][niSample] = m_pfTempSample[kV];
	}
	// Some debugging using dummy Ambisonics channels to test that the rotations are correct
/*	AmbFloat fV = 0.f; AmbFloat fT = 0.f; AmbFloat fR = -0.5f; AmbFloat fS = 0.f; AmbFloat fU = fSqrt3/2.f;
	AmbFloat fV_tmp; AmbFloat fT_tmp; AmbFloat fR_tmp; AmbFloat fS_tmp; AmbFloat fU_tmp;
	AmbFloat fV_tmp2; AmbFloat fT_tmp2; AmbFloat fR_tmp2; AmbFloat fS_tmp2; AmbFloat fU_tmp2;
	//Alpha rotation
	fV_tmp = - fU * m_fSin2Alpha + fV * m_fCos2Alpha;
	fT_tmp = - fS * m_fSinAlpha + fT * m_fCosAlpha;
	fR_tmp = fR;
	fS_tmp = fS * m_fCosAlpha + fT * m_fSinAlpha;
	fU_tmp = fU * m_fCos2Alpha + fV * m_fSin2Alpha;
	// Beta rotation
	fV_tmp2 = -m_fSinBeta * fT_tmp + m_fCosBeta * fV_tmp;
	fT_tmp2 = -m_fCosBeta * fT_tmp + m_fSinBeta * fV_tmp;
	fR_tmp2 = (0.75f * m_fCos2Beta + 0.25f) * fR_tmp + (0.5 * fSqrt3 * pow(m_fSinBeta,2.0) ) * fU_tmp
			+ (fSqrt3 * m_fSinBeta * m_fCosBeta) * fS_tmp;
	fS_tmp2 = m_fCos2Beta * fS_tmp - fSqrt3 * m_fCosBeta * m_fSinBeta * fR_tmp
			+ m_fCosBeta * m_fSinBeta * fU_tmp;
	fU_tmp2 = (0.25f * m_fCos2Beta + 0.75f) * fU_tmp - m_fCosBeta * m_fSinBeta * fS_tmp
			+0.5 * fSqrt3 * pow(m_fSinBeta,2.0) * fR_tmp;
	//Gamma rotation
	fV = - fU_tmp2 * m_fSin2Gamma + fV_tmp2 * m_fCos2Gamma;
	fT = - fS_tmp2 * m_fSinGamma + fT_tmp2 * m_fCosGamma;
	fR = fR_tmp2;
	fS = fS_tmp2 * m_fCosGamma + fT_tmp2 * m_fSinGamma;
	fU = fU_tmp2 * m_fCos2Gamma + fV_tmp2 * m_fSin2Gamma;
	std::cout << "Vnew = " << fV << ", Tnew = " << fT << ", Rnew = " << fR
		 << " Snew = " << fS << ", Unew = " << fU << std::endl;
*/
}

void CAmbisonicProcessor::ProcessOrder3_3D(CBFormat* pBFSrcDst, AmbUInt nSamples)
{
		/* (should move these somewhere that does recompute each time) */
		AmbFloat fSqrt3_2 = sqrt(3.f/2.f);
		AmbFloat fSqrt15 = sqrt(15.f);
		AmbFloat fSqrt5_2 = sqrt(5.f/2.f);

	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
		// Alpha rotation
		m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Alpha
							+ pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Alpha;
		m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Alpha
							+ pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Alpha;
		m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinAlpha 
							+ pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosAlpha;
		m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
		m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosAlpha 
							+ pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinAlpha;
		m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Alpha 
							+ pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Alpha;
		m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Alpha
							+ pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Alpha;

		// Beta rotation
		pBFSrcDst->m_ppfChannels[kQ][niSample] = 0.125f * m_pfTempSample[kQ] * (5.f + 3.f*m_fCos2Beta)
					- fSqrt3_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
					+ 0.25f * fSqrt15 * m_pfTempSample[kM] * pow(m_fSinBeta,2.0f);
		pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO] * m_fCos2Beta
					- fSqrt5_2 * m_pfTempSample[kM] * m_fCosBeta * m_fSinBeta
					+ fSqrt3_2 * m_pfTempSample[kQ] * m_fCosBeta * m_fSinBeta;
		pBFSrcDst->m_ppfChannels[kM][niSample] = 0.125f * m_pfTempSample[kM] * (3.f + 5.f*m_fCos2Beta)
					- fSqrt5_2 * m_pfTempSample[kO] *m_fCosBeta * m_fSinBeta
					+ 0.25f * fSqrt15 * m_pfTempSample[kQ] * pow(m_fSinBeta,2.0f);
		pBFSrcDst->m_ppfChannels[kK][niSample] = 0.25f * m_pfTempSample[kK] * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
					+ 0.5f * fSqrt15 * m_pfTempSample[kN] * m_fCosBeta * pow(m_fSinBeta,2.f)
					+ 0.5f * fSqrt5_2 * m_pfTempSample[kP] * pow(m_fSinBeta,3.f)
					+ 0.125f * fSqrt3_2 * m_pfTempSample[kL] * (m_fSinBeta + 5.f * m_fSin3Beta);
		pBFSrcDst->m_ppfChannels[kL][niSample] = 0.0625f * m_pfTempSample[kL] * (m_fCosBeta + 15.f * m_fCos3Beta)
					+ 0.25f * fSqrt5_2 * m_pfTempSample[kN] * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
					+ 0.25f * fSqrt15 * m_pfTempSample[kP] * m_fCosBeta * pow(m_fSinBeta,2.f)
					- 0.125 * fSqrt3_2 * m_pfTempSample[kK] * (m_fSinBeta + 5.f * m_fSin3Beta);
		pBFSrcDst->m_ppfChannels[kN][niSample] = 0.125f * m_pfTempSample[kN] * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
					+ 0.25f * fSqrt3_2 * m_pfTempSample[kP] * (3.f + m_fCos2Beta) * m_fSinBeta
					+ 0.5f * fSqrt15 * m_pfTempSample[kK] * m_fCosBeta * pow(m_fSinBeta,2.f)
					+ 0.125 * fSqrt5_2 * m_pfTempSample[kL] * (m_fSinBeta - 3.f * m_fSin3Beta);
		pBFSrcDst->m_ppfChannels[kP][niSample] = 0.0625f * m_pfTempSample[kP] * (15.f * m_fCosBeta + m_fCos3Beta)
					- 0.25f * fSqrt3_2 * m_pfTempSample[kN] * (3.f + m_fCos2Beta) * m_fSinBeta
					+ 0.25f * fSqrt15 * m_pfTempSample[kL] * m_fCosBeta * pow(m_fSinBeta,2.f)
					- 0.5 * fSqrt5_2 * m_pfTempSample[kK] * pow(m_fSinBeta,3.f);

		// Gamma rotation
		m_pfTempSample[kQ] = - pBFSrcDst->m_ppfChannels[kP][niSample] * m_fSin3Gamma
							+ pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fCos3Gamma;
		m_pfTempSample[kO] = - pBFSrcDst->m_ppfChannels[kN][niSample] * m_fSin2Gamma
							+ pBFSrcDst->m_ppfChannels[kO][niSample] * m_fCos2Gamma;
		m_pfTempSample[kM] = - pBFSrcDst->m_ppfChannels[kL][niSample] * m_fSinGamma
							+ pBFSrcDst->m_ppfChannels[kM][niSample] * m_fCosGamma;
		m_pfTempSample[kK] = pBFSrcDst->m_ppfChannels[kK][niSample];
		m_pfTempSample[kL] = pBFSrcDst->m_ppfChannels[kL][niSample] * m_fCosGamma 
							+ pBFSrcDst->m_ppfChannels[kM][niSample] * m_fSinGamma;
		m_pfTempSample[kN] = pBFSrcDst->m_ppfChannels[kN][niSample] * m_fCos2Gamma
							+ pBFSrcDst->m_ppfChannels[kO][niSample] * m_fSin2Gamma;
		m_pfTempSample[kP] = pBFSrcDst->m_ppfChannels[kP][niSample] * m_fCos3Gamma
							+ pBFSrcDst->m_ppfChannels[kQ][niSample] * m_fSin3Gamma;

		pBFSrcDst->m_ppfChannels[kQ][niSample] = m_pfTempSample[kQ];
		pBFSrcDst->m_ppfChannels[kO][niSample] = m_pfTempSample[kO];
		pBFSrcDst->m_ppfChannels[kM][niSample] = m_pfTempSample[kM];
		pBFSrcDst->m_ppfChannels[kK][niSample] = m_pfTempSample[kK];
		pBFSrcDst->m_ppfChannels[kL][niSample] = m_pfTempSample[kL];
		pBFSrcDst->m_ppfChannels[kN][niSample] = m_pfTempSample[kN];
		pBFSrcDst->m_ppfChannels[kP][niSample] = m_pfTempSample[kP];
	}

	// Some debugging using dummy Ambisonics channels to test that the rotations are correct
/*	AmbFloat fQ = 0.f; AmbFloat fO = 0.f; AmbFloat fM = 0.f; AmbFloat fK = 0.f; AmbFloat fL = -0.6124f;
	AmbFloat fN = 0.f; AmbFloat fP = 0.7906f;
	AmbFloat fQ_tmp; AmbFloat fO_tmp; AmbFloat fM_tmp; AmbFloat fK_tmp; AmbFloat fL_tmp;
	AmbFloat fN_tmp; AmbFloat fP_tmp;
	AmbFloat fQ_tmp2; AmbFloat fO_tmp2; AmbFloat fM_tmp2; AmbFloat fK_tmp2; AmbFloat fL_tmp2;
	AmbFloat fN_tmp2; AmbFloat fP_tmp2;
	//Alpha rotation
	fQ_tmp = - fP * m_fSin3Alpha + fQ * m_fCos3Alpha;
	fO_tmp = - fN * m_fSin2Alpha + fO * m_fCos2Alpha;
	fM_tmp = - fL * m_fSinAlpha + fM * m_fCosAlpha;
	fK_tmp = fK;
	fL_tmp = fL * m_fCosAlpha + fM * m_fSinAlpha;
	fN_tmp = fN * m_fCos2Alpha + fO * m_fSin2Alpha;
	fP_tmp = fP * m_fCos3Alpha + fQ * m_fSin3Alpha;
	// Beta rotation
	fQ_tmp2 = 0.125f * fQ_tmp * (5.f + 3.f*m_fCos2Beta)- fSqrt3_2 * fO_tmp *m_fCosBeta * m_fSinBeta
			+ 0.25f * fSqrt15 * fM_tmp * pow(m_fSinBeta,2.0f);
	fO_tmp2 = fO_tmp * m_fCos2Beta - fSqrt5_2 * fM_tmp * m_fCosBeta * m_fSinBeta
			+ fSqrt3_2 * fQ_tmp * m_fCosBeta * m_fSinBeta;
	fM_tmp2 = 0.125f * fM_tmp * (3.f + 5.f*m_fCos2Beta) - fSqrt5_2 * fO_tmp *m_fCosBeta * m_fSinBeta
			+ 0.25f * fSqrt15 * fQ_tmp * pow(m_fSinBeta,2.0f);
	fK_tmp2 = 0.25f * fK_tmp * m_fCosBeta * (-1.f + 15.f*m_fCos2Beta)
				+ 0.5f * fSqrt15 * fN_tmp * m_fCosBeta * pow(m_fSinBeta,2.f)
				+ 0.5f * fSqrt5_2 * fP_tmp * pow(m_fSinBeta,3.f)
				+ 0.125f * fSqrt3_2 * fL_tmp * (m_fSinBeta + 5.f * m_fSin3Beta);
	fL_tmp2 = 0.0625f * fL_tmp * (m_fCosBeta + 15.f * m_fCos3Beta)
				+ 0.25f * fSqrt5_2 * fN_tmp * (1.f + 3.f * m_fCos2Beta) * m_fSinBeta
				+ 0.25f * fSqrt15 * fP_tmp * m_fCosBeta * pow(m_fSinBeta,2.f)
				- 0.125 * fSqrt3_2 * fK_tmp * (m_fSinBeta + 5.f * m_fSin3Beta);
	fN_tmp2 = 0.125f * fN_tmp * (5.f * m_fCosBeta + 3.f * m_fCos3Beta)
				+ 0.25f * fSqrt3_2 * fP_tmp * (3.f + m_fCos2Beta) * m_fSinBeta
				+ 0.5f * fSqrt15 * fK_tmp * m_fCosBeta * pow(m_fSinBeta,2.f)
				+ 0.125 * fSqrt5_2 * fL_tmp * (m_fSinBeta - 3.f * m_fSin3Beta);
	fP_tmp2 = 0.0625f * fP_tmp * (15.f * m_fCosBeta + m_fCos3Beta)
				- 0.25f * fSqrt3_2 * fN_tmp * (3.f + m_fCos2Beta) * m_fSinBeta
				+ 0.25f * fSqrt15 * fL_tmp * m_fCosBeta * pow(m_fSinBeta,2.f)
				- 0.5 * fSqrt5_2 * fK_tmp * pow(m_fSinBeta,3.f);
	//Alpha rotation
	fQ = - fP_tmp2 * m_fSin3Gamma + fQ_tmp2 * m_fCos3Gamma;
	fO = - fN_tmp2 * m_fSin2Gamma + fO_tmp2 * m_fCos2Gamma;
	fM = - fL_tmp2 * m_fSinGamma + fM_tmp2 * m_fCosGamma;
	fK = fK_tmp2;
	fL = fL_tmp2 * m_fCosGamma + fM_tmp2 * m_fSinGamma;
	fN = fN_tmp2 * m_fCos2Gamma + fO_tmp2 * m_fSin2Gamma;
	fP = fP_tmp2 * m_fCos3Gamma + fQ_tmp2 * m_fSin3Gamma;
	std::cout << "Qnew = " << fQ << ", Onew = " << fO << ", Mnew = " << fM
		 << " Knew = " << fK << ", Lnew = " << fL
		 << " Nnew = " << fN << ", Pnew = " << fP << std::endl;
*/
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
