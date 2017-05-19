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

#include <iostream>

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

AmbBool CAmbisonicZoomer::Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc)
{
	bool success = CAmbisonicBase::Create(nOrder, b3D, nMisc);
    if(!success)
        return false;

    if(m_b3D)
        m_pProcessFunction = &CAmbisonicZoomer::Process3D;
    else
        m_pProcessFunction = &CAmbisonicZoomer::Process2D;

    m_AmbDecoderFront.Create(m_nOrder, 1, kAmblib_Mono, 1);

    //Calculate all the speaker coefficients
    m_AmbDecoderFront.Refresh();

	m_fZoomRed = 0.f;

	m_AmbEncoderFront = new AmbFloat[m_nChannelCount];
	m_AmbEncoderFront_weighted = new AmbFloat[m_nChannelCount];
	a_m = new AmbFloat[m_nOrder];

	// These weights a_m are applied to the channels of a corresponding order within the Ambisonics signals.
	// When applied to the encoded channels and decoded to a particular loudspeaker direction they will create a
	// virtual microphone pattern with no rear lobes.
	// When used for decoding this is known as in-phase decoding.
    for(AmbUInt iOrder = 0; iOrder<=m_nOrder; iOrder++)
        a_m[iOrder] = (2*iOrder+1)*factorial(m_nOrder)*factorial(m_nOrder+1) / (factorial(m_nOrder+iOrder+1)*factorial(m_nOrder-iOrder));

	AmbUInt iDegree=0;
	for(AmbUInt iChannel = 0; iChannel<m_nChannelCount; iChannel++)
	{
        m_AmbEncoderFront[iChannel] = m_AmbDecoderFront.GetCoefficient(0, iChannel);
		iDegree = (int)floor(sqrt(iChannel));
		m_AmbEncoderFront_weighted[iChannel] = m_AmbEncoderFront[iChannel] * a_m[iDegree];
        std::cout << "Channel " << iChannel << " = " << m_AmbDecoderFront.GetCoefficient(0,iChannel)
                  << " " << m_AmbEncoderFront_weighted[iChannel] << std::endl;
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
	m_fWCoeff = 1.f / sqrtf(2.f) * m_fZoom;
	m_fXCoeff = sqrtf(2.f) * m_fZoom;
	m_fYZCoeff = sqrtf(1.f - m_fZoom * m_fZoom);

	m_fZoomRed = sqrtf(1.f - m_fZoom * m_fZoom);
	m_fZoomBlend = 1.f - m_fZoom;
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
	for(AmbUInt niSample = 0; niSample < nSamples; niSample++)
	{
        AmbFloat fMic = 0.f;

		for(AmbUInt iChannel=0; iChannel<m_nChannelCount; iChannel++)
		{
			// virtual microphone with polar pattern narrowing as Ambisonic order increases
			fMic += m_AmbEncoderFront_weighted[iChannel] * pBFSrcDst->m_ppfChannels[iChannel][niSample];
		}
		for(AmbUInt iChannel=0; iChannel<m_nChannelCount; iChannel++)
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

AmbFloat CAmbisonicZoomer::factorial(AmbUInt M)
{
    AmbUInt ret = 1;
    for(unsigned int i = 1; i <= M; ++i)
        ret *= i;
    return ret;
}
