/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.cpp                                #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "config.h"

#include <iostream>

#include "AmbisonicBinauralizer.h"


CAmbisonicBinauralizer::CAmbisonicBinauralizer()
    : m_pFFT_cfg(nullptr, kiss_fftr_free)
    , m_pIFFT_cfg(nullptr, kiss_fftr_free)
{
    m_nBlockSize = 0;
    m_nTaps = 0;
    m_nFFTSize = 0;
    m_nFFTBins = 0;
    m_fFFTScaler = 0.f;
    m_nOverlapLength = 0;
}

bool CAmbisonicBinauralizer::Configure(unsigned nOrder,
                                       bool b3D,
                                       unsigned nSampleRate,
                                       unsigned nBlockSize,
                                       unsigned& tailLength,
                                       std::string HRTFPath)
{
    //Iterators
    unsigned niEar = 0;
    unsigned niChannel = 0;
    unsigned niSpeaker = 0;
    unsigned niTap = 0;

    HRTF *p_hrtf = getHRTF(nSampleRate, HRTFPath);
    if (p_hrtf == nullptr)
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

    CAmbisonicBase::Configure(nOrder, b3D, 0);
    //Position speakers and recalculate coefficients
    ArrangeSpeakers();

    unsigned nSpeakers = m_AmbDecoder.GetSpeakerCount();

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
        ppfAccumulator[niEar] = new float*[m_nChannelCount];
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            ppfAccumulator[niEar][niChannel] = new float[m_nTaps]();
    }

    for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {
        for(niSpeaker = 0; niSpeaker < nSpeakers; niSpeaker++)
        {
            //What is the position of the current speaker
            PolarPoint position = m_AmbDecoder.GetPosition(niSpeaker);

            bool b_found = p_hrtf->get(position.fAzimuth, position.fElevation, pfHRTF);
            if (!b_found)
                return false;

            //Scale the HRTFs by the coefficient of the current channel/component
            // The spherical harmonic coefficients are multiplied by (2*order + 1) to provide the correct decoder
            // for SN3D normalised Ambisonic inputs.
            float fCoefficient = m_AmbDecoder.GetCoefficient(niSpeaker, niChannel) * (2*floor(sqrt(niChannel)) + 1);
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
    float fMax = 0;

    // encode a source at azimuth 90deg and elevation 0
    CAmbisonicEncoder myEncoder;
    myEncoder.Configure(m_nOrder, true, 0);

    PolarPoint position90;
    position90.fAzimuth = DegreesToRadians(90.f);
    position90.fElevation = 0.f;
    position90.fDistance = 5.f;
    myEncoder.SetPosition(position90);
    myEncoder.Refresh();

    float* pfLeftEar90;
    pfLeftEar90 = new float[m_nTaps]();
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
    float fUpperSample = 1.f;
    float fScaler = fUpperSample / fMax;
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
            memcpy(m_pfScratchBufferA.data(), ppfAccumulator[niEar][niChannel], m_nTaps * sizeof(float));
            memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
            kiss_fftr(m_pFFT_cfg.get(), m_pfScratchBufferA.data(), m_ppcpFilters[niEar][niChannel].get());
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
    delete[] pfLeftEar90;

    return true;
}

void CAmbisonicBinauralizer::Reset()
{
    memset(m_pfOverlap[0].data(), 0, m_nOverlapLength * sizeof(float));
    memset(m_pfOverlap[1].data(), 0, m_nOverlapLength * sizeof(float));
}

void CAmbisonicBinauralizer::Refresh()
{

}

void CAmbisonicBinauralizer::Process(CBFormat* pBFSrc,
                                     float** ppfDst)
{
    unsigned niEar = 0;
    unsigned niChannel = 0;
    unsigned ni = 0;
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
    bool bLowCPU = true;
    if(bLowCPU){
        // Perform the convolutions for the left ear and generate the right ear from a modified accumulation of these channels
        niEar = 0;
        memset(m_pfScratchBufferA.data(), 0, m_nFFTSize * sizeof(float));
        memset(m_pfScratchBufferC.data(), 0, m_nFFTSize * sizeof(float));
        for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
        {
            memcpy(m_pfScratchBufferB.data(), pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(float));
            memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(float));
            kiss_fftr(m_pFFT_cfg.get(), m_pfScratchBufferB.data(), m_pcpScratch.get());
            for(ni = 0; ni < m_nFFTBins; ni++)
            {
                cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                            - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                            + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                m_pcpScratch[ni] = cpTemp;
            }
            kiss_fftri(m_pIFFT_cfg.get(), m_pcpScratch.get(), m_pfScratchBufferB.data());
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
        memcpy(ppfDst[0], m_pfScratchBufferA.data(), m_nBlockSize * sizeof(float));
        memcpy(ppfDst[1], m_pfScratchBufferC.data(), m_nBlockSize * sizeof(float));
        for(ni = 0; ni < m_nOverlapLength; ni++){
            ppfDst[0][ni] += m_pfOverlap[0][ni];
            ppfDst[1][ni] += m_pfOverlap[1][ni];
        }
        memcpy(m_pfOverlap[0].data(), &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(float));
        memcpy(m_pfOverlap[1].data(), &m_pfScratchBufferC[m_nBlockSize], m_nOverlapLength * sizeof(float));
    }
    else
    {
        // Perform the convolution on both ears. Potentially more realistic results but requires double the number of
        // convolutions.
        for(niEar = 0; niEar < 2; niEar++)
        {
            memset(m_pfScratchBufferA.data(), 0, m_nFFTSize * sizeof(float));
            for(niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            {
                memcpy(m_pfScratchBufferB.data(), pBFSrc->m_ppfChannels[niChannel], m_nBlockSize * sizeof(float));
                memset(&m_pfScratchBufferB[m_nBlockSize], 0, (m_nFFTSize - m_nBlockSize) * sizeof(float));
                kiss_fftr(m_pFFT_cfg.get(), m_pfScratchBufferB.data(), m_pcpScratch.get());
                for(ni = 0; ni < m_nFFTBins; ni++)
                {
                    cpTemp.r = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].r
                                - m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].i;
                    cpTemp.i = m_pcpScratch[ni].r * m_ppcpFilters[niEar][niChannel][ni].i
                                + m_pcpScratch[ni].i * m_ppcpFilters[niEar][niChannel][ni].r;
                    m_pcpScratch[ni] = cpTemp;
                }
                kiss_fftri(m_pIFFT_cfg.get(), m_pcpScratch.get(), m_pfScratchBufferB.data());
                for(ni = 0; ni < m_nFFTSize; ni++)
                    m_pfScratchBufferA[ni] += m_pfScratchBufferB[ni];
            }
            for(ni = 0; ni < m_nFFTSize; ni++)
                m_pfScratchBufferA[ni] *= m_fFFTScaler;
            memcpy(ppfDst[niEar], m_pfScratchBufferA.data(), m_nBlockSize * sizeof(float));
            for(ni = 0; ni < m_nOverlapLength; ni++)
                ppfDst[niEar][ni] += m_pfOverlap[niEar][ni];
            memcpy(m_pfOverlap[niEar].data(), &m_pfScratchBufferA[m_nBlockSize], m_nOverlapLength * sizeof(float));
        }
    }
}

void CAmbisonicBinauralizer::ArrangeSpeakers()
{
    unsigned nSpeakerSetUp;
    //How many speakers will be needed? Add one for right above the listener
    unsigned nSpeakers = OrderToSpeakers(m_nOrder, m_b3D);
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

    m_AmbDecoder.Configure(m_nOrder, m_b3D, nSpeakerSetUp, nSpeakers);

    //Calculate all the speaker coefficients
    m_AmbDecoder.Refresh();
}


HRTF *CAmbisonicBinauralizer::getHRTF(unsigned nSampleRate, std::string HRTFPath)
{
    HRTF *p_hrtf;

#ifdef HAVE_MYSOFA
# ifdef HAVE_MIT_HRTF
    if (HRTFPath == "")
        p_hrtf = new MIT_HRTF(nSampleRate);
    else
# endif
        p_hrtf = new SOFA_HRTF(HRTFPath, nSampleRate);
#else
# ifdef HAVE_MIT_HRTF
    p_hrtf = new MIT_HRTF(nSampleRate);
# else
# error At least MySOFA or MIT_HRTF need to be enabled
# endif
#endif

    if (p_hrtf == nullptr)
        return nullptr;

    if (!p_hrtf->isLoaded())
    {
        delete p_hrtf;
        return nullptr;
    }

    return p_hrtf;
}


void CAmbisonicBinauralizer::AllocateBuffers()
{
    //Allocate scratch buffers
    m_pfScratchBufferA.resize(m_nFFTSize);
    m_pfScratchBufferB.resize(m_nFFTSize);
    m_pfScratchBufferC.resize(m_nFFTSize);

    //Allocate overlap-add buffers
    m_pfOverlap[0].resize(m_nOverlapLength);
    m_pfOverlap[1].resize(m_nOverlapLength);

    //Allocate FFT and iFFT for new size
    m_pFFT_cfg.reset(kiss_fftr_alloc(m_nFFTSize, 0, 0, 0));
    m_pIFFT_cfg.reset(kiss_fftr_alloc(m_nFFTSize, 1, 0, 0));

    //Allocate the FFTBins for each channel, for each ear
    for(unsigned niEar = 0; niEar < 2; niEar++)
    {
        m_ppcpFilters[niEar].resize(m_nChannelCount);
        for(unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
            m_ppcpFilters[niEar][niChannel].reset(new kiss_fft_cpx[m_nFFTBins]);
    }

    m_pcpScratch.reset(new kiss_fft_cpx[m_nFFTBins]);
}
