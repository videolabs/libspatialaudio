/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CBFormat - Ambisonic BFormat                                            #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
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
}

unsigned CBFormat::GetSampleCount()
{
    return m_nSamples;
}

bool CBFormat::Configure(unsigned nOrder, bool b3D, unsigned nSampleCount)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nSampleCount);
    if(!success)
        return false;

    m_nSamples = nSampleCount;
    m_nDataLength = m_nSamples * m_nChannelCount;

    m_pfData.resize(m_nDataLength);
    memset(m_pfData.data(), 0, m_nDataLength * sizeof(float));
    m_ppfChannels.reset(new float*[m_nChannelCount]);

    for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        m_ppfChannels[niChannel] = &m_pfData[niChannel * m_nSamples];
    }

    return true;
}

void CBFormat::Reset()
{
    memset(m_pfData.data(), 0, m_nDataLength * sizeof(float));
}

void CBFormat::Refresh()
{

}

void CBFormat::InsertStream(float* pfData, unsigned nChannel, unsigned nSamples)
{
    memcpy(m_ppfChannels[nChannel], pfData, nSamples * sizeof(float));
}

void CBFormat::ExtractStream(float* pfData, unsigned nChannel, unsigned nSamples)
{
    memcpy(pfData, m_ppfChannels[nChannel], nSamples * sizeof(float));
}

void CBFormat::operator = (const CBFormat &bf)
{
    memcpy(m_pfData.data(), bf.m_pfData.data(), m_nDataLength * sizeof(float));
}

bool CBFormat::operator == (const CBFormat &bf)
{
    if(m_b3D == bf.m_b3D && m_nOrder == bf.m_nOrder && m_nDataLength == bf.m_nDataLength)
        return true;
    else
        return false;
}

bool CBFormat::operator != (const CBFormat &bf)
{
    if(m_b3D != bf.m_b3D || m_nOrder != bf.m_nOrder || m_nDataLength != bf.m_nDataLength)
        return true;
    else
        return false;
}

CBFormat& CBFormat::operator += (const CBFormat &bf)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
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
    unsigned niChannel = 0;
    unsigned niSample = 0;
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
    unsigned niChannel = 0;
    unsigned niSample = 0;
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
    unsigned niChannel = 0;
    unsigned niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] /= bf.m_ppfChannels[niChannel][niSample];
        }
    }

    return *this;
}

CBFormat& CBFormat::operator += (const float &fValue)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] += fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator -= (const float &fValue)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] -= fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator *= (const float &fValue)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] *= fValue;
        }
    }

    return *this;
}

CBFormat& CBFormat::operator /= (const float &fValue)
{
    unsigned niChannel = 0;
    unsigned niSample = 0;
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSample = 0; niSample < m_nSamples; niSample++)
        {
            m_ppfChannels[niChannel][niSample] /= fValue;
        }
    }

    return *this;
}
