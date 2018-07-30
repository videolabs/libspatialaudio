/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.cpp                                #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "SpeakersBinauralizer.h"


SpeakersBinauralizer::SpeakersBinauralizer()
    : m_nSpeakers(0)
{
}

bool SpeakersBinauralizer::Configure(unsigned nSampleRate,
                             unsigned nBlockSize,
                             CAmbisonicSpeaker *speakers,
                             unsigned nSpeakers,
                             unsigned& tailLength,
                             std::string HRTFPath)
{
        //Iterators
        unsigned niEar = 0;
        unsigned niTap = 0;

        HRTF *p_hrtf = getHRTF(nSampleRate, HRTFPath);
        if (p_hrtf == nullptr)
            return false;

        m_nTaps = tailLength = p_hrtf->getHRTFLen();
        m_nBlockSize = nBlockSize;

        //What will the overlap size be?
        m_nOverlapLength = m_nBlockSize < m_nTaps ? m_nBlockSize - 1 : m_nTaps - 1;
        //How large does the FFT need to be
        m_nFFTSize = 1;
        while(m_nFFTSize < (m_nBlockSize + m_nTaps + m_nOverlapLength))
                m_nFFTSize <<= 1;
        //How many bins is that
        m_nFFTBins = m_nFFTSize / 2 + 1;
        //What do we need to scale the result of the iFFT by
        m_fFFTScaler = 1.f / m_nFFTSize;

        m_nSpeakers = nSpeakers;

        //Allocate buffers with new settings
        AllocateBuffers();

        //Allocate temporary buffers for retrieving taps from mit_hrtf_lib
        float* pfHRTF[2];
        for(niEar = 0; niEar < 2; niEar++)
            pfHRTF[niEar] = new float[m_nTaps];

        //Allocate buffers for HRTF accumulators
        float** ppfAccumulator[2];
        for(niEar = 0; niEar < 2; niEar++)
        {
            ppfAccumulator[niEar] = new float*[nSpeakers];
            for (unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                ppfAccumulator[niEar][niChannel] = new float[m_nTaps];
                memset(ppfAccumulator[niEar][niChannel], 0, m_nTaps * sizeof(float));
            }
        }

        for(unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
        {
            PolarPoint position = speakers[niChannel].GetPosition();

            bool b_found = p_hrtf->get(position.fAzimuth, position.fElevation, pfHRTF);
            if (!b_found)
                return false;

            //Accumulate channel/component HRTF
            for(niTap = 0; niTap < m_nTaps; niTap++)
            {
                ppfAccumulator[0][niChannel][niTap] += pfHRTF[0][niTap];
                ppfAccumulator[1][niChannel][niTap] += pfHRTF[1][niTap];
            }
        }
        delete p_hrtf;

        //Find the maximum tap
        float fMax = 0;
        for(niEar = 0; niEar < 2; niEar++)
        {
            for (unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                for(niTap = 0; niTap < m_nTaps; niTap++)
                {
                    fMax = fabs(ppfAccumulator[niEar][niChannel][niTap]) > fMax ?
                                fabs(ppfAccumulator[niEar][niChannel][niTap]) : fMax;
                }
            }
        }

        //Normalize to pre-defined value
        float fUpperSample = 1.f;
        float fScaler = fUpperSample / fMax;
        fScaler *= 0.35f;
        for(niEar = 0; niEar < 2; niEar++)
        {
            for (unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                for(niTap = 0; niTap < m_nTaps; niTap++)
                {
                        ppfAccumulator[niEar][niChannel][niTap] *= fScaler;
                }
            }
        }

        //Convert frequency domain filters
        for(niEar = 0; niEar < 2; niEar++)
        {
            for (unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                memcpy(m_pfScratchBufferA.data(), ppfAccumulator[niEar][niChannel], m_nTaps * sizeof(float));
                memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
                kiss_fftr(m_pFFT_cfg.get(), m_pfScratchBufferA.data(), m_ppcpFilters[niEar][niChannel].get());
            }
        }

        for(niEar = 0; niEar < 2; niEar++)
            delete [] pfHRTF[niEar];

        for(niEar = 0; niEar < 2; niEar++)
        {
            for(unsigned niChannel = 0; niChannel < nSpeakers; niChannel++)
                delete [] ppfAccumulator[niEar][niChannel];
            delete [] ppfAccumulator[niEar];
        }

    return true;
}


void SpeakersBinauralizer::Process(float** pBFSrc, float** ppfDst)
{
    kiss_fft_cpx cpTemp;

    for (unsigned niEar = 0; niEar < 2; niEar++)
    {
        memset(m_pfScratchBufferA.data(), 0, m_nFFTSize * sizeof(float));
        for (unsigned niChannel = 0; niChannel < m_nSpeakers; niChannel++)
        {
            memcpy(m_pfScratchBufferB.data(), pBFSrc[niChannel], m_nBlockSize * sizeof(float));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(float));
            kiss_fftr(m_pFFT_cfg.get(), m_pfScratchBufferB.data(), m_pcpScratch.get());
            for(unsigned ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                    - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                    + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            kiss_fftri(m_pIFFT_cfg.get(), m_pcpScratch.get(), m_pfScratchBufferB.data());
            for (unsigned ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
        }

        for (unsigned ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
        memcpy(ppfDst[niEar], m_pfScratchBufferA.data(), m_nBlockSize * sizeof(float));
        for (unsigned ni = 0; ni < m_nOverlapLength; ni++)
            ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
        memcpy(m_pfOverlap[niEar].data(), &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(float));
    }
}


void SpeakersBinauralizer::AllocateBuffers()
{
    CAmbisonicBinauralizer::AllocateBuffers();

    //Allocate the FFTBins for each channel, for each ear
    for(unsigned niEar = 0; niEar < 2; niEar++)
    {
        m_ppcpFilters[niEar].resize(m_nSpeakers);
        for(unsigned niChannel = 0; niChannel < m_nSpeakers; niChannel++)
            m_ppcpFilters[niEar][niChannel].reset(new kiss_fft_cpx[m_nFFTBins]);
    }
}
