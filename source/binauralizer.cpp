/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.cpp                                #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "binauralizer.h"


Binauralizer::Binauralizer()
{
    m_nBlockSize = 0;
    m_nTaps = 0;
    m_nFFTSize = 0;
    m_nFFTBins = 0;
    m_fFFTScaler = 0.f;
    m_nOverlapLength = 0;
    m_nSpeakers = 0;

    m_pfScratchBufferA = NULL;
    m_pfScratchBufferB = NULL;
    m_pfOverlap[0] = NULL;
    m_pfOverlap[1] = NULL;

    m_pFFT_cfg = NULL;	//TODO: Remove all the NULL dependencies
    m_pIFFT_cfg = NULL;
    m_ppcpFilters[0] = NULL;
    m_ppcpFilters[1] = NULL;
    m_pcpScratch = NULL;

    AmbUInt tail = 0;

    Create(DEFAULT_SAMPLERATE, DEFAULT_BLOCKSIZE, DEFAULT_HRTFSET_DIFFUSED,
           NULL, 0, tail);
}

Binauralizer::~Binauralizer()
{
    DeallocateBuffers();
}

AmbBool Binauralizer::Create(AmbUInt nSampleRate,
                             AmbUInt nBlockSize,
                             AmbBool bDiffused,
                             CAmbisonicSpeaker *speakers,
                             AmbUInt nSpeakers,
                             AmbUInt& tailLength)
{
        //Iterators
        AmbUInt niEar = 0;
        AmbUInt niTap = 0;

        //How many taps will there be in the HRTFs
        tailLength = mit_hrtf_availability(0, 0, nSampleRate, bDiffused);
        if(!tailLength)
                return false;

        m_nTaps = tailLength;
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

        //Deallocate any buffers with previous settings
        DeallocateBuffers();

        m_speakers = speakers;
        m_nSpeakers = nSpeakers;

        //Allocate buffers with new settings
        AllocateBuffers();

        //Allocate temporary buffers for retrieving taps from mit_hrtf_lib
        short* psHRTF[2];
        AmbFloat* pfHRTF[2];
        for(niEar = 0; niEar < 2; niEar++)
        {
            psHRTF[niEar] = new short[m_nTaps];
            pfHRTF[niEar] = new AmbFloat[m_nTaps];
        }

        //Allocate buffers for HRTF accumulators
        AmbFloat** ppfAccumulator[2];
        for(niEar = 0; niEar < 2; niEar++)
        {
            ppfAccumulator[niEar] = new AmbFloat*[nSpeakers];
            for (AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                ppfAccumulator[niEar][niChannel] = new AmbFloat[m_nTaps];
                memset(ppfAccumulator[niEar][niChannel], 0, m_nTaps * sizeof(AmbFloat));
            }
        }

        for(AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
        {
            //What is the position of the current speaker
            PolarPoint position = speakers[niChannel].GetPosition();
            AmbInt nAzimuth = (AmbInt)RadiansToDegrees(-position.fAzimuth);
            if(nAzimuth > 180)
                nAzimuth -= 360;
            AmbInt nElevation = (AmbInt)RadiansToDegrees(position.fElevation);
            //Get HRTFs for given position
            AmbUInt nResult = mit_hrtf_get(&nAzimuth, &nElevation, nSampleRate, bDiffused, psHRTF[0], psHRTF[1]);
            if(!nResult)
                nResult = nResult;
            //Convert from short to float representation
            for(niTap = 0; niTap < m_nTaps; niTap++)
            {
                pfHRTF[0][niTap] = psHRTF[0][niTap] / 32767.f;
                pfHRTF[1][niTap] = psHRTF[1][niTap] / 32767.f;
            }
            //Accumulate channel/component HRTF
            for(niTap = 0; niTap < m_nTaps; niTap++)
            {
                ppfAccumulator[0][niChannel][niTap] += pfHRTF[0][niTap];
                ppfAccumulator[1][niChannel][niTap] += pfHRTF[1][niTap];
            }
        }

        //Find the maximum tap
        AmbFloat fMax = 0;
        for(niEar = 0; niEar < 2; niEar++)
        {
            for (AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                for(niTap = 0; niTap < m_nTaps; niTap++)
                {
                    fMax = fabs(ppfAccumulator[niEar][niChannel][niTap]) > fMax ?
                                fabs(ppfAccumulator[niEar][niChannel][niTap]) : fMax;
                }
            }
        }

        //Normalize to pre-defined value
        AmbFloat fUpperSample = 1.f;
        AmbFloat fScaler = fUpperSample / fMax;
        fScaler *= 0.35f;
        for(niEar = 0; niEar < 2; niEar++)
        {
            for (AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
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
            for (AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
            {
                memcpy(m_pfScratchBufferA, ppfAccumulator[niEar][niChannel], m_nTaps * sizeof(AmbFloat));
                memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(AmbFloat));
                kiss_fftr(m_pFFT_cfg, m_pfScratchBufferA, m_ppcpFilters[niEar][niChannel]);
            }
        }

        for(niEar = 0; niEar < 2; niEar++)
        {
            delete [] psHRTF[niEar];
            delete [] pfHRTF[niEar];
        }

        for(niEar = 0; niEar < 2; niEar++)
        {
            for(AmbUInt niChannel = 0; niChannel < nSpeakers; niChannel++)
                delete [] ppfAccumulator[niEar][niChannel];
            delete [] ppfAccumulator[niEar];
        }

    return true;
}

void Binauralizer::Reset()
{
    memset(m_pfOverlap[0], 0, m_nOverlapLength * sizeof(AmbFloat));
    memset(m_pfOverlap[1], 0, m_nOverlapLength * sizeof(AmbFloat));
}


void Binauralizer::Process(AmbFloat** pBFSrc, AmbFloat** ppfDst)
{
    kiss_fft_cpx cpTemp;

    for (AmbUInt niEar = 0; niEar < 2; niEar++)
    {
        memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(AmbFloat));
        for (AmbUInt niChannel = 0; niChannel < m_nSpeakers; niChannel++)
        {
            memcpy(m_pfScratchBufferB, pBFSrc[niChannel], m_nBlockSize * sizeof(AmbFloat));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(AmbFloat));
            kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);
            for(AmbUInt ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                    - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                    + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);
            for (AmbUInt ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
        }

        for (AmbUInt ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
        memcpy(ppfDst[niEar], m_pfScratchBufferA, m_nBlockSize * sizeof(AmbFloat));
        for (AmbUInt ni = 0; ni < m_nOverlapLength; ni++)
            ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
        memcpy(m_pfOverlap[niEar], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(AmbFloat));
    }
}


void Binauralizer::AllocateBuffers()
{
    //Allocate scratch buffers
    m_pfScratchBufferA = new AmbFloat[m_nFFTSize];
    m_pfScratchBufferB = new AmbFloat[m_nFFTSize];

    //Allocate overlap-add buffers
    m_pfOverlap[0] = new AmbFloat[m_nOverlapLength];
    m_pfOverlap[1] = new AmbFloat[m_nOverlapLength];

    //Allocate FFT and iFFT for new size
    m_pFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    //Allocate the FFTBins for each channel, for each ear
    for(AmbUInt niEar = 0; niEar < 2; niEar++)
    {
        m_ppcpFilters[niEar] = new kiss_fft_cpx*[m_nSpeakers];
        for(AmbUInt niChannel = 0; niChannel < m_nSpeakers; niChannel++)
            m_ppcpFilters[niEar][niChannel] = new kiss_fft_cpx[m_nFFTBins];
    }

    m_pcpScratch = new kiss_fft_cpx[m_nFFTBins];
}


void Binauralizer::DeallocateBuffers()
{
    if(m_pfScratchBufferA)
        delete [] m_pfScratchBufferA;
    if(m_pfScratchBufferB)
        delete [] m_pfScratchBufferB;

    if(m_pfOverlap[0])
        delete [] m_pfOverlap[0];
    if(m_pfOverlap[1])
        delete [] m_pfOverlap[1];

    if(m_pFFT_cfg)
        kiss_fftr_free(m_pFFT_cfg);
    if(m_pIFFT_cfg)
        kiss_fftr_free(m_pIFFT_cfg);

    for(AmbUInt niEar = 0; niEar < 2; niEar++)
    {
        for(AmbUInt niChannel = 0; niChannel < m_nSpeakers; niChannel++)
        {
            if(m_ppcpFilters[niEar][niChannel])
                delete [] m_ppcpFilters[niEar][niChannel];
        }
        if(m_ppcpFilters[niEar])
            delete [] m_ppcpFilters[niEar];
    }

    if(m_pcpScratch)
        delete [] m_pcpScratch;
}
