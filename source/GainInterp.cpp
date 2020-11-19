/*############################################################################*/
/*#                                                                          #*/
/*#  Apply a vector of gains to a mono input with interpolation              #*/
/*#                                                                          #*/
/*#  Filename:      GainInterp.cpp		                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "GainInterp.h"

CGainInterp::CGainInterp()
{
}

CGainInterp::~CGainInterp()
{
}

void CGainInterp::SetGainVector(std::vector<double> newGainVec, unsigned int interpTimeInSamples)
{
	if (m_targetGainVec != newGainVec)
	{
		if (!m_isFirstCall)
		{
			m_gainVec = m_targetGainVec;
			m_targetGainVec = newGainVec;
			m_interpDurInSamples = interpTimeInSamples;
			// Reset the interpolation counter to start interpolation
			m_iInterpCount = 0;
		}
		else if (m_isFirstCall)
		{
			m_gainVec = newGainVec;
			m_targetGainVec = newGainVec;
			m_interpDurInSamples = 0;
			m_iInterpCount = 0;
		}
	}
}

void CGainInterp::ProcessAccumul(float* pIn, std::vector<std::vector<float>>& ppOut, unsigned int nSamples, unsigned int nOffset)
{
	unsigned int nCh = (unsigned int)m_targetGainVec.size();
	// The number of samples to interpolate over in this block
	unsigned int nInterpSamples = std::max(std::min(m_interpDurInSamples - m_iInterpCount, nSamples), (unsigned int)0);

	float deltaCoeff = 1.f / m_interpDurInSamples;

	if (nInterpSamples > 0)
	{
		for (unsigned int i = 0; i < nInterpSamples; ++i)
		{
			float fInterp = (float)(i + m_iInterpCount) * deltaCoeff;
			float fOneMinusInterp = 1.f - fInterp;
			for (unsigned int iCh = 0; iCh < nCh; ++iCh)
				ppOut[iCh][i + nOffset] += pIn[i] * (fOneMinusInterp * (float)m_gainVec[iCh] + fInterp * (float)m_targetGainVec[iCh]);
		}

		m_iInterpCount += nInterpSamples;
		// If interpolation ends then set the m_gainVec to equal the target
		if (m_iInterpCount >= m_interpDurInSamples)
			m_gainVec = m_targetGainVec;
	}

	for (unsigned int iCh = 0; iCh < nCh; ++iCh)
		for (unsigned int i = nInterpSamples; i < nSamples; ++i)
			ppOut[iCh][i + nOffset] += pIn[i] * (float)m_gainVec[iCh];

	m_isFirstCall = false;
}

void CGainInterp::Reset()
{
	m_iInterpCount = m_interpDurInSamples;
	m_gainVec = m_targetGainVec;
	m_isFirstCall = true;
}
