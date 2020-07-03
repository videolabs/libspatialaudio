/*############################################################################*/
/*#                                                                          #*/
/*#  A decorrelator for ADM renderer.                                        #*/
/*#  CAdmDecorrelate - ADM Decorrelation                                     #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmDecorrelate.cpp                                       #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmDecorrelate.h"

namespace admrender {

	CAdmDecorrelate::CAdmDecorrelate()
	{
		m_pfScratchBufferA = nullptr;
		m_pFFT_decor_cfg = nullptr;
		m_pIFFT_decor_cfg = nullptr;
		m_ppcpDecorFilters = nullptr;
		m_pcpScratch = nullptr;
		m_pfOverlap = nullptr;
	}

	CAdmDecorrelate::~CAdmDecorrelate()
	{
        if (m_pfScratchBufferA)
            delete[] m_pfScratchBufferA;
        if (m_pFFT_decor_cfg)
            kiss_fftr_free(m_pFFT_decor_cfg);
        if (m_pIFFT_decor_cfg)
            kiss_fftr_free(m_pIFFT_decor_cfg);
        if (m_ppcpDecorFilters)
        {
            for (unsigned i = 0; i < m_nCh; i++)
                if (m_ppcpDecorFilters[i])
                    delete[] m_ppcpDecorFilters[i];
            delete[] m_ppcpDecorFilters;
        }
        if (m_pcpScratch)
            delete[] m_pcpScratch;
        if (m_pfOverlap) {
            for (unsigned i = 0; i < m_nCh; i++)
                if (m_pfOverlap[i])
                    delete[] m_pfOverlap[i];
            delete[] m_pfOverlap;
        }
        if (m_ppfDirectDelay) {
            for (unsigned i = 0; i < m_nCh; i++)
                if (m_ppfDirectDelay[i])
                    delete[] m_ppfDirectDelay[i];
            delete[] m_ppfDirectDelay;
        }
	}

    bool CAdmDecorrelate::Configure(Layout layout, unsigned int nBlockSize)
    {
        decorrelationFilters = ear::designDecorrelators(layout);

        // Set the number fo channels
        m_nCh = layout.channels.size();

        // The decorrelation filters have the same length regardless of the input block size.
        // The length is 512 samples (Rec. ITU-R BS.2127-0, sec 7.4, pg 63)
        m_nDelay = ear::decorrelatorCompensationDelay();
        unsigned nbTaps = m_nDelay * 2;

        m_nBlockSize = nBlockSize;
        m_nTaps = nbTaps;

        // Length of the delay lines used to compensate the direct signal
        m_nDelayLineLength = ear::decorrelatorCompensationDelay() + m_nBlockSize;

        //What will the overlap size be?
        m_nOverlapLength = m_nBlockSize < m_nTaps ? m_nBlockSize - 1 : m_nTaps - 1;
        //How large does the FFT need to be
        m_nFFTSize = 1;
        while (m_nFFTSize < (m_nBlockSize + m_nTaps + m_nOverlapLength))
            m_nFFTSize <<= 1;
        //How many bins is that
        m_nFFTBins = m_nFFTSize / 2 + 1;
        //What do we need to scale the result of the iFFT by
        m_fFFTScaler = 1.f / m_nFFTSize;

        //Allocate buffers
        m_pfOverlap = new float* [m_nCh];
        m_ppfDirectDelay = new float* [m_nCh];
        for (unsigned i = 0; i < m_nCh; i++)
        {
            m_pfOverlap[i] = new float[m_nOverlapLength];
            m_ppfDirectDelay[i] = new float[m_nDelayLineLength];
        }

        m_pfScratchBufferA = new float[m_nFFTSize];
        m_ppcpDecorFilters = new kiss_fft_cpx * [m_nCh];
        for (unsigned i = 0; i < m_nCh; i++)
            m_ppcpDecorFilters[i] = new kiss_fft_cpx[m_nFFTBins];

        m_pcpScratch = new kiss_fft_cpx[m_nFFTBins];

        Reset();

        //Allocate FFT and iFFT for new size
        m_pFFT_decor_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
        m_pIFFT_decor_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

        // get impulse responses for psychoacoustic optimisation based on playback system (2D or 3D) and playback order (1 to 3)
        //Convert from short to float representation
        for (unsigned i_m = 0; i_m < m_nCh; i_m++) {
            // Convert the impulse responses to the frequency domain
            memcpy(m_pfScratchBufferA, &decorrelationFilters[i_m][0], m_nTaps * sizeof(float));
            memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
            kiss_fftr(m_pFFT_decor_cfg, m_pfScratchBufferA, m_ppcpDecorFilters[i_m]);
        }

        return true;
    }

    void CAdmDecorrelate::Reset()
    {
        for (unsigned i = 0; i < m_nCh; i++)
        {
            memset(m_pfOverlap[i], 0, m_nOverlapLength * sizeof(float));
            memset(m_ppfDirectDelay[i], 0, m_nDelayLineLength * sizeof(float));
        }
    }

    void CAdmDecorrelate::Process(std::vector<std::vector<float>> &ppInDirect, std::vector<std::vector<float>> &ppInDiffuse, unsigned int nSamples)
    {
        kiss_fft_cpx cpTemp;

        // get the read position that is static across all samples
        m_nReadPos = m_nWritePos - m_nDelay;
        if (m_nReadPos < 0)
            m_nReadPos += m_nDelayLineLength;

        memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(float));
        for (unsigned int iCh = 0; iCh < m_nCh; ++iCh)
        {
            // delay the direct input
            // Write to the delay line
            WriteToDelayLine(m_ppfDirectDelay[iCh], &ppInDirect[iCh][0], m_nWritePos, nSamples);
            // Read from the delay line
            ReadFromDelayLine(m_ppfDirectDelay[iCh], &ppInDirect[iCh][0], m_nReadPos, nSamples);

            // Apply the decorrelation filters
            memcpy(m_pfScratchBufferA, &ppInDiffuse[iCh][0], m_nBlockSize * sizeof(float));
            memset(&m_pfScratchBufferA[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(float));
            kiss_fftr(m_pFFT_decor_cfg, m_pfScratchBufferA, m_pcpScratch);
            // Perform the convolution in the frequency domain
            for (unsigned ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpDecorFilters[iCh][ni].r
                    - m_pcpScratch[ni].i * m_ppcpDecorFilters[iCh][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpDecorFilters[iCh][ni].i
                    + m_pcpScratch[ni].i * m_ppcpDecorFilters[iCh][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            // Convert from frequency domain back to time domain
            kiss_fftri(m_pIFFT_decor_cfg, m_pcpScratch, m_pfScratchBufferA);
            for (unsigned ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] *= m_fFFTScaler;
            memcpy(&ppInDiffuse[iCh][0], m_pfScratchBufferA, m_nBlockSize * sizeof(float));
            for (unsigned ni = 0; ni < m_nOverlapLength; ni++)
            {
                ppInDiffuse[iCh][ni] += m_pfOverlap[iCh][ni];
            }
            memcpy(m_pfOverlap[iCh], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(float));
        }
        // Advance the read/write positions
        m_nWritePos += nSamples;
        if (m_nWritePos >= m_nDelayLineLength)
            m_nWritePos -= m_nDelayLineLength;
    }

    void CAdmDecorrelate::WriteToDelayLine(float* pDelayLine, float* pIn, int nWritePos, int nSamples)
    {
        int overrun = nWritePos + nSamples - m_nDelayLineLength;
        if (overrun > 0) {
            memcpy(&pDelayLine[nWritePos], &pIn[0], (nSamples - overrun) * sizeof(float));
            memcpy(&pDelayLine[0], &pIn[nSamples - overrun], overrun*sizeof(float));
        }
        else {
            memcpy(&pDelayLine[nWritePos], &pIn[0], nSamples*sizeof(float));
        }
    }

    void CAdmDecorrelate::ReadFromDelayLine(float* pDelayLine, float* pOut, int nReadPos, int nSamples)
    {
        int overrun = nReadPos + nSamples - m_nDelayLineLength;
        if (overrun > 0) {
            memcpy(&pOut[0], &pDelayLine[nReadPos], (nSamples - overrun)*sizeof(float));
            memcpy(&pOut[nSamples - overrun], &pDelayLine[0], overrun*sizeof(float));
        }
        else {
            memcpy(&pOut[0], &pDelayLine[nReadPos], nSamples*sizeof(float));
        }
    }
}