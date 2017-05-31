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

#include <iostream>

#include "AmbisonicBinauralizer.h"


CAmbisonicBinauralizer::CAmbisonicBinauralizer()
{
    m_nBlockSize = 0;
    m_nTaps = 0;
    m_nFFTSize = 0;
    m_nFFTBins = 0;
    m_fFFTScaler = 0.f;
    m_nOverlapLength = 0;

    m_pfScratchBufferA = NULL;
    m_pfScratchBufferB = NULL;
    m_pfScratchBufferC = NULL;
    m_pfOverlap[0] = NULL;
    m_pfOverlap[1] = NULL;

    m_pFFT_cfg = NULL;
    m_pIFFT_cfg = NULL;
    m_ppcpFilters[0] = NULL;
    m_ppcpFilters[1] = NULL;
    m_pcpScratch = NULL;
}

CAmbisonicBinauralizer::~CAmbisonicBinauralizer()
{
    DeallocateBuffers();
}

AmbBool CAmbisonicBinauralizer::Create(AmbUInt nOrder,
                                       AmbBool b3D,
                                       AmbUInt nSampleRate,
                                       AmbUInt nBlockSize,
                                       AmbBool bDiffused,
                                       AmbUInt& tailLength,
                                       std::string HRTFPath)
{    
    //Iterators
    AmbUInt niEar = 0;
    AmbUInt niChannel = 0;
    AmbUInt niSpeaker = 0;
    AmbUInt niTap = 0;

    HRTF *p_hrtf = getHRTF(nSampleRate, HRTFPath);
    if (p_hrtf == NULL)
        return false;

    tailLength = m_nTaps = p_hrtf->getHRTFLen();
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

    CAmbisonicBase::Create(nOrder, b3D, bDiffused);
    //Position speakers and recalculate coefficients
    ArrangeSpeakers();

    AmbUInt nSpeakers = m_AmbDecoder.GetSpeakerCount();

    //Allocate buffers with new settings
    AllocateBuffers();

    //Allocate temporary buffers for retrieving taps from mit_hrtf_lib
    AmbFloat* pfHRTF[2];
    for(niEar = 0; niEar < 2; niEar++)
        pfHRTF[niEar] = new AmbFloat[m_nTaps];

    //Allocate buffers for HRTF accumulators
    AmbFloat** ppfAccumulator[2];
    for(niEar = 0; niEar < 2; niEar++)
    {
        ppfAccumulator[niEar] = new AmbFloat*[m_nChannelCount];
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            ppfAccumulator[niEar][niChannel] = new AmbFloat[m_nTaps]();
    }

    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSpeaker = 0; niSpeaker < nSpeakers; niSpeaker++)
        {
            //What is the position of the current speaker
            PolarPoint position = m_AmbDecoder.GetPosition(niSpeaker);

            bool b_found = p_hrtf->get(position.fAzimuth, position.fElevation, pfHRTF);
            if (!b_found)
            {
                DeallocateBuffers();
                return false;
            }

            //Scale the HRTFs by the coefficient of the current channel/component
            // The spherical harmonic coefficients are multiplied by (2*order + 1) to provide the correct decoder
            // for SN3D normalised Ambisonic inputs.
            AmbFloat fCoefficient = m_AmbDecoder.GetCoefficient(niSpeaker, niChannel) * (2*floor(sqrt(niChannel)) + 1);
            for(niTap = 0; niTap < m_nTaps; niTap++)
            {
                pfHRTF[0][niTap] *= fCoefficient;
                pfHRTF[1][niTap] *= fCoefficient;
            }
            //Accumulate channel/component HRTF
            for(niTap = 0; niTap < m_nTaps; niTap++)
            {
                ppfAccumulator[0][niChannel][niTap] += pfHRTF[0][niTap];
                ppfAccumulator[1][niChannel][niTap] += pfHRTF[1][niTap];
            }
        }
    }

    delete p_hrtf;

    //Find the maximum tap
    AmbFloat fMax = 0;

    // encode a source at azimuth 90deg and elevation 0
    CAmbisonicEncoder myEncoder;
    myEncoder.Create(m_nOrder, true, 0);

    PolarPoint position90;
    position90.fAzimuth = DegreesToRadians(90.f);
    position90.fElevation = 0.f;
    position90.fDistance = 5.f;
    myEncoder.SetPosition(position90);
    myEncoder.Refresh();

    AmbFloat* pfLeftEar90;
    pfLeftEar90 = new AmbFloat[m_nTaps]();
    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        for(niTap = 0; niTap < m_nTaps; niTap++)
            pfLeftEar90[niTap] += myEncoder.GetCoefficient(niChannel) * ppfAccumulator[0][niChannel][niTap];

    //Find the maximum value for a source encoded at 90degrees
    for(niTap = 0; niTap < m_nTaps; niTap++)
    {
        float val = fabs(pfLeftEar90[niTap]);
        fMax = val > fMax ? val : fMax;
    }

    //Normalize to pre-defined value
    AmbFloat fUpperSample = 1.f;
    AmbFloat fScaler = fUpperSample / fMax;
    fScaler *= 0.35f;
    for(niEar = 0; niEar < 2; niEar++)
    {
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
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
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            memcpy(m_pfScratchBufferA, ppfAccumulator[niEar][niChannel], m_nTaps * sizeof(AmbFloat));
            memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(AmbFloat));
            kiss_fftr(m_pFFT_cfg, m_pfScratchBufferA, m_ppcpFilters[niEar][niChannel]);
        }
    }

    for(niEar = 0; niEar < 2; niEar++)
        delete [] pfHRTF[niEar];

    for(niEar = 0; niEar < 2; niEar++)
    {
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            delete [] ppfAccumulator[niEar][niChannel];
        delete [] ppfAccumulator[niEar];
    }

    return true;
}

void CAmbisonicBinauralizer::Reset()
{
    memset(m_pfOverlap[0], 0, m_nOverlapLength * sizeof(AmbFloat));
    memset(m_pfOverlap[1], 0, m_nOverlapLength * sizeof(AmbFloat));
}

void CAmbisonicBinauralizer::Refresh()
{

}

void CAmbisonicBinauralizer::Process(CBFormat* pBFSrc, 
                                     AmbFloat** ppfDst)
{
    AmbUInt niEar = 0;
    AmbUInt niChannel = 0;
    AmbUInt ni = 0;
    kiss_fft_cpx cpTemp;


    /* If CPU load needs to be reduced then perform the convolution for each of the Ambisonics/spherical harmonic
    decompositions of the loudspeakers HRTFs for the left ear. For the left ear the results of these convolutions
    are summed to give the ear signal. For the right ear signal, the properties of the spherical harmonic decomposition
    can be use to to create the ear signal. This is done by either adding or subtracting the correct channels.
    Channels 1, 4, 5, 9, 10 and 11 are subtracted from the accumulated signal. All others are added.
    For example, for a first order signal the ears are generated from:
        SignalL = W x HRTF_W + Y x HRTF_Y + Z x HRTF_Z + X x HRTF_X
        SignalR = W x HRTF_W - Y x HRTF_Y + Z x HRTF_Z + X x HRTF_X
    where 'x' is a convolution, W/Y/Z/X are the Ambisonic signal channels and HRTF_x are the spherical harmonic
    decompositions of the virtual loudspeaker array HRTFs.
    This has the effect of assuming a completel symmetric head. */

    /* TODO: This bool flag should be either an automatic or user option depending on CPU. It should be 'true' if
    CPU load needs to be limited */
    AmbBool bLowCPU = true;
    if(bLowCPU){
        // Perform the convolutions for the left ear and generate the right ear from a modified accumulation of these channels
        niEar = 0;
        memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(AmbFloat));
        memset(m_pfScratchBufferC, 0, m_nFFTSize * sizeof(AmbFloat));
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            memcpy(m_pfScratchBufferB, pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(AmbFloat));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(AmbFloat));
            kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);
            for(ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                            - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                            + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);
            for(ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];

            for(ni = 0; ni < m_nFFTSize; ni++){
                // Subtract certain channels (such as Y) to generate right ear.
                if((niChannel==1) || (niChannel==4) || (niChannel==5) ||
                    (niChannel==9) || (niChannel==10)|| (niChannel==11))
                {
                    m_pfScratchBufferC[ni] -= m_pfScratchBufferB[ni];
                }
                else{
                    m_pfScratchBufferC[ni] += m_pfScratchBufferB[ni];
                }
            }
        }
        for(ni = 0; ni < m_nFFTSize; ni++){
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
            m_pfScratchBufferC[ni] *= m_fFFTScaler;
        }
        memcpy(ppfDst[0], m_pfScratchBufferA, m_nBlockSize * sizeof(AmbFloat));
        memcpy(ppfDst[1], m_pfScratchBufferC, m_nBlockSize * sizeof(AmbFloat));
        for(ni = 0; ni < m_nOverlapLength; ni++){
            ppfDst[0][ni] += m_pfOverlap[0][ni];
            ppfDst[1][ni] += m_pfOverlap[1][ni];
        }
        memcpy(m_pfOverlap[0], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(AmbFloat));
        memcpy(m_pfOverlap[1], &m_pfScratchBufferC[m_nBlockSize], m_nOverlapLength * sizeof(AmbFloat));
    }
    else
    {
        // Perform the convolution on both ears. Potentially more realistic results but requires double the number of
        // convolutions.
        for(niEar = 0; niEar < 2; niEar++)
        {
            memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(AmbFloat));
            for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            {
                memcpy(m_pfScratchBufferB, pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(AmbFloat));
                memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(AmbFloat));
                kiss_fftr(m_pFFT_cfg, m_pfScratchBufferB, m_pcpScratch);
                for(ni = 0; ni < m_nFFTBins; ni++)
                {
                    cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                                - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                    cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                                + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                    m_pcpScratch[ni] = cpTemp;
                }
                kiss_fftri(m_pIFFT_cfg, m_pcpScratch, m_pfScratchBufferB);
                for(ni = 0; ni < m_nFFTSize; ni++)
                    m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
            }
            for(ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] *= m_fFFTScaler;
            memcpy(ppfDst[niEar], m_pfScratchBufferA, m_nBlockSize * sizeof(AmbFloat));
            for(ni = 0; ni < m_nOverlapLength; ni++)
                ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
            memcpy(m_pfOverlap[niEar], &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(AmbFloat));
        }
    }
}

void CAmbisonicBinauralizer::ArrangeSpeakers()
{
    AmbUInt nSpeakerSetUp;
    //How many speakers will be needed? Add one for right above the listener
    AmbUInt nSpeakers = OrderToSpeakers(m_nOrder, m_b3D);
    //Custom speaker setup
    // Select cube layout for first order a dodecahedron for 2nd and 3rd
    if (m_nOrder == 1)
    {
        std::cout << "Getting first order cube" << std::endl;
        nSpeakerSetUp = kAmblib_Cube2;
    }
    else
    {
        std::cout << "Getting second/third order dodecahedron" << std::endl;
        nSpeakerSetUp = kAmblib_Dodecahedron;
    }

    m_AmbDecoder.Create(m_nOrder, m_b3D, nSpeakerSetUp, nSpeakers);

    //Calculate all the speaker coefficients
    m_AmbDecoder.Refresh();
}


HRTF *CAmbisonicBinauralizer::getHRTF(AmbUInt nSampleRate, std::string HRTFPath)
{
    HRTF *p_hrtf;

#ifdef HAVE_MYSOFA
    if (HRTFPath == "")
        p_hrtf = new MIT_HRTF(nSampleRate, false);
    else
        p_hrtf = new SOFA_HRTF(HRTFPath, nSampleRate);
#else
    if (HRTFPath != "")
        return false;
    p_hrtf = new MIT_HRTF(nSampleRate, bDiffused);
#endif

    if (p_hrtf == NULL)
        return NULL;

    if (!p_hrtf->isLoaded())
    {
        delete p_hrtf;
        return NULL;
    }

    return p_hrtf;
}


void CAmbisonicBinauralizer::AllocateBuffers()
{
    //Allocate scratch buffers
    m_pfScratchBufferA = new AmbFloat[m_nFFTSize];
    m_pfScratchBufferB = new AmbFloat[m_nFFTSize];
    m_pfScratchBufferC = new AmbFloat[m_nFFTSize];

    //Allocate overlap-add buffers
    m_pfOverlap[0] = new AmbFloat[m_nOverlapLength];
    m_pfOverlap[1] = new AmbFloat[m_nOverlapLength];

    //Allocate FFT and iFFT for new size
    m_pFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    //Allocate the FFTBins for each channel, for each ear
    for(AmbUInt niEar = 0; niEar < 2; niEar++)
    {
        m_ppcpFilters[niEar] = new kiss_fft_cpx*[m_nChannelCount];
        for(AmbUInt niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            m_ppcpFilters[niEar][niChannel] = new kiss_fft_cpx[m_nFFTBins];
    }

    m_pcpScratch = new kiss_fft_cpx[m_nFFTBins];
}

void CAmbisonicBinauralizer::DeallocateBuffers()
{
    if (m_pfScratchBufferA)
    {
        delete [] m_pfScratchBufferA;
        m_pfScratchBufferA = nullptr;
    }

    if (m_pfScratchBufferB)
    {
        delete [] m_pfScratchBufferB;
        m_pfScratchBufferB = nullptr;
    }

    if (m_pfScratchBufferC)
    {
        delete [] m_pfScratchBufferC;
        m_pfScratchBufferC = nullptr;
    }

    if (m_pfOverlap[0])
    {
        delete [] m_pfOverlap[0];
        m_pfOverlap[0] = nullptr;
    }

    if (m_pfOverlap[1])
    {
        delete [] m_pfOverlap[1];
        m_pfOverlap[1] = nullptr;
    }

    if (m_pFFT_cfg)
    {
        kiss_fftr_free(m_pFFT_cfg);
        m_pFFT_cfg = nullptr;
    }

    if (m_pIFFT_cfg)
    {
        kiss_fftr_free(m_pIFFT_cfg);
        m_pIFFT_cfg = nullptr;
    }

    for(AmbUInt niEar = 0; niEar < 2; niEar++)
    {
        if(m_ppcpFilters[niEar])
        {
            for(AmbUInt niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            {
                if(m_ppcpFilters[niEar][niChannel])
                    delete [] m_ppcpFilters[niEar][niChannel];
            }
            delete [] m_ppcpFilters[niEar];
            m_ppcpFilters[niEar] = nullptr;
        }
    }

    if(m_pcpScratch)
    {
        delete [] m_pcpScratch;
        m_pcpScratch = nullptr;
    }
}
