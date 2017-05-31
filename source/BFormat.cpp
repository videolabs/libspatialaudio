/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CBFormat - Ambisonic BFormat                                            #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      BFormat.cpp                                              #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "BFormat.h"

CBFormat::CBFormat()
{
    m_nSamples = 0;
    m_nDataLength = 0;
    m_pfData = nullptr;
    m_ppfChannels = nullptr;
}

CBFormat::~CBFormat()
{
    if(m_pfData)
        delete [] m_pfData;
    if(m_ppfChannels)
        delete [] m_ppfChannels;
}

AmbUInt CBFormat::GetSampleCount()
{
    return m_nSamples;
}

bool CBFormat::Configure(AmbUInt nOrder, AmbBool b3D, AmbUInt nSampleCount)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nSampleCount);
    if(!success)
        return false;

    if(m_pfData)
        delete [] m_pfData;
    if(m_ppfChannels)
        delete [] m_ppfChannels;

    m_nSamples = nSampleCount;
    m_nDataLength = m_nSamples * m_nChannelCount;

    m_pfData = new AmbFloat[m_nDataLength];
    memset(m_pfData, 0, m_nDataLength * sizeof(AmbFloat));
    m_ppfChannels = new AmbFloat*[m_nChannelCount];

    for(AmbUInt niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        m_ppfChannels[niChannel] = &m_pfData[niChannel * m_nSamples];
    }

    return true;
}

void CBFormat::Reset()
{
    memset(m_pfData, 0, m_nDataLength * sizeof(AmbFloat));
}

void CBFormat::Refresh()
{

}

void CBFormat::InsertStream(AmbFloat* pfData, AmbUInt nChannel, AmbUInt nSamples)
{
    memcpy(m_ppfChannels[nChannel], pfData, nSamples * sizeof(AmbFloat));
}

void CBFormat::ExtractStream(AmbFloat* pfData, AmbUInt nChannel, AmbUInt nSamples)
{
    memcpy(pfData, m_ppfChannels[nChannel], nSamples * sizeof(AmbFloat));
}

void CBFormat::operator = (const CBFormat &bf)
{
    memcpy(m_pfData, bf.m_pfData, m_nDataLength * sizeof(AmbFloat));
}

AmbBool CBFormat::operator == (const CBFormat &bf)
{
    if(m_b3D == bf.m_b3D && m_nOrder == bf.m_nOrder && m_nDataLength == bf.m_nDataLength)
        return true;
    else
        return false;
}

AmbBool CBFormat::operator != (const CBFormat &bf)
{
    if(m_b3D != bf.m_b3D || m_nOrder != bf.m_nOrder || m_nDataLength != bf.m_nDataLength)
        return true;
    else
        return false;
}

CBFormat& CBFormat::operator += (const CBFormat &bf)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] += bf.m_ppfChannels[niChannel][niSample];
        }
    }

    return *this;
}

CBFormat& CBFormat::operator -= (const CBFormat &bf)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] -= bf.m_ppfChannels[niChannel][niSample];
        }
    }

    return *this;
}

CBFormat& CBFormat::operator *= (const CBFormat &bf)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] *= bf.m_ppfChannels[niChannel][niSample];
        }
    }

    return *this;
}

CBFormat& CBFormat::operator /= (const CBFormat &bf)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] /= bf.m_ppfChannels[niChannel][niSample];
        }
    }

    return *this;
}

CBFormat& CBFormat::operator += (const AmbFloat &fValue)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] += fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator -= (const AmbFloat &fValue)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] -= fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator *= (const AmbFloat &fValue)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] *= fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator /= (const AmbFloat &fValue)
{
    AmbUInt niChannel = 0;
    AmbUInt niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] /= fValue;
        }
    }

    return *this;
}
