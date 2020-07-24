/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicShelfFilters - Ambisonic psychoactic optimising filters       #*/
/*#  Copyright © 2020 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicShelfFilters.cpp                                #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          23/03/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicShelfFilters.h"
#include <iostream>

CAmbisonicShelfFilters::CAmbisonicShelfFilters()
{
    m_pfScratchBufferA = nullptr;
    m_pFFT_psych_cfg = nullptr;
    m_pIFFT_psych_cfg = nullptr;
    m_ppcpPsychFilters = nullptr;
    m_pcpScratch = nullptr;
    m_pfOverlap = nullptr;
}

CAmbisonicShelfFilters::~CAmbisonicShelfFilters()
{
    if(m_pfScratchBufferA)
        delete [] m_pfScratchBufferA;
    if(m_pFFT_psych_cfg)
        kiss_fftr_free(m_pFFT_psych_cfg);
    if(m_pIFFT_psych_cfg)
        kiss_fftr_free(m_pIFFT_psych_cfg);
    if (m_ppcpPsychFilters)
    {
        for(unsigned i=0; i<=m_nOrder; i++)
            if(m_ppcpPsychFilters[i])
                delete [] m_ppcpPsychFilters[i];
        delete [] m_ppcpPsychFilters;
    }
    if(m_pcpScratch)
        delete [] m_pcpScratch;
    if(m_pfOverlap){
        for(unsigned i=0; i<m_nChannelCount; i++)
            if(m_pfOverlap[i])
                delete [] m_pfOverlap[i];
        delete [] m_pfOverlap;
    }
}

bool CAmbisonicShelfFilters::Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned nMisc)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nMisc);
    if(!success)
        return false;

    // All optimisation filters have the same number of taps so take from the first order 3D impulse response arbitrarily
    unsigned nbTaps = sizeof(first_order_3D[0]) / sizeof(int16_t);

    m_nBlockSize = nBlockSize;
    m_nTaps = nbTaps;

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

    //Allocate buffers
        m_pfOverlap = new float*[m_nChannelCount];
    for(unsigned i=0; i<m_nChannelCount; i++)
        m_pfOverlap[i] = new float[m_nOverlapLength];

    m_pfScratchBufferA = new float[m_nFFTSize];
    m_ppcpPsychFilters = new kiss_fft_cpx*[m_nOrder+1];
    for(unsigned i = 0; i <= m_nOrder; i++)
        m_ppcpPsychFilters[i] = new kiss_fft_cpx[m_nFFTBins];

    m_pcpScratch = new kiss_fft_cpx[m_nFFTBins];

    //Allocate temporary buffers for retrieving taps of psychoacoustic opimisation filters
    std::vector<std::unique_ptr<float[]>> pfPsychIR;
    for(unsigned i = 0; i <= m_nOrder; i++)
    {
        pfPsychIR.emplace_back(new float[m_nTaps]);
    }

    Reset();

    //Allocate FFT and iFFT for new size
    m_pFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 0, 0, 0);
    m_pIFFT_psych_cfg = kiss_fftr_alloc(m_nFFTSize, 1, 0, 0);

    // get impulse responses for psychoacoustic optimisation based on playback system (2D or 3D) and playback order (1 to 3)
    //Convert from short to float representation
    for (unsigned i_m = 0; i_m <= m_nOrder; i_m++){
        for(unsigned i = 0; i < m_nTaps; i++)
            if(m_b3D){
                switch(m_nOrder){
                    case 0: pfPsychIR[i_m][i] = i == 0 ? 1.f : 0.f; break;
                    case 1: pfPsychIR[i_m][i] = 2.f*first_order_3D[i_m][i] / 32767.f; break;
                    case 2: pfPsychIR[i_m][i] = 2.f*second_order_3D[i_m][i] / 32767.f; break;
                    case 3: pfPsychIR[i_m][i] = 2.f*third_order_3D[i_m][i] / 32767.f; break;
                }
                }
                else{
                    switch(m_nOrder){
                    case 0: pfPsychIR[i_m][i] = i == 0 ? 1.f : 0.f; break;
                    case 1: pfPsychIR[i_m][i] = 2.f*first_order_2D[i_m][i] / 32767.f; break;
                    case 2: pfPsychIR[i_m][i] = 2.f*second_order_2D[i_m][i] / 32767.f; break;
                    case 3: pfPsychIR[i_m][i] = 2.f*third_order_2D[i_m][i] / 32767.f; break;
                }
            }
        // Convert the impulse responses to the frequency domain
        memcpy(m_pfScratchBufferA, pfPsychIR[i_m].get(), m_nTaps * sizeof(float));
        memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
        kiss_fftr(m_pFFT_psych_cfg, m_pfScratchBufferA, m_ppcpPsychFilters[i_m]);
    }

    return true;
}

void CAmbisonicShelfFilters::Reset()
{
    for(unsigned i=0; i<m_nChannelCount; i++)
        memset(m_pfOverlap[i], 0, m_nOverlapLength * sizeof(float));
}

void CAmbisonicShelfFilters::Refresh()
{
}

void CAmbisonicShelfFilters::Process(CBFormat* pBFSrcDst)
{
    Process(pBFSrcDst, m_nBlockSize);
}

void CAmbisonicShelfFilters::Process(CBFormat* pBFSrcDst, unsigned int nSamples)
{
    kiss_fft_cpx cpTemp;

    unsigned iChannelOrder = 0;

    // Filter the Ambisonics channels
    // All  channels are filtered using linear phase FIR filters.
    // In the case of the 0th order signal (W channel) this takes the form of a delay
    // For all other channels shelf filters are used
    memset(m_pfScratchBufferA, 0, m_nFFTSize * sizeof(float));
    for (unsigned niChannel = 0; niChannel < m_nChannelCount; niChannel++)
    {

        iChannelOrder = int(sqrt(niChannel));    //get the order of the current channel

        memcpy(m_pfScratchBufferA, pBFSrcDst->m_ppfChannels[niChannel], nSamples * sizeof(float));
        memset(&m_pfScratchBufferA[nSamples], 0, (m_nFFTSize - nSamples) * sizeof(float));
        kiss_fftr(m_pFFT_psych_cfg, m_pfScratchBufferA, m_pcpScratch);
        // Perform the convolution in the frequency domain
        for (unsigned ni = 0; ni < m_nFFTBins; ni++)
        {
            cpTemp.r = m_pcpScratch[ni].r * m_ppcpPsychFilters[iChannelOrder][ni].r
                - m_pcpScratch[ni].i * m_ppcpPsychFilters[iChannelOrder][ni].i;
            cpTemp.i = m_pcpScratch[ni].r * m_ppcpPsychFilters[iChannelOrder][ni].i
                + m_pcpScratch[ni].i * m_ppcpPsychFilters[iChannelOrder][ni].r;
            m_pcpScratch[ni] = cpTemp;
        }
        // Convert from frequency domain back to time domain
        kiss_fftri(m_pIFFT_psych_cfg, m_pcpScratch, m_pfScratchBufferA);
        for (unsigned ni = 0; ni < m_nFFTSize; ni++)
            m_pfScratchBufferA[ni] *= m_fFFTScaler;
        memcpy(pBFSrcDst->m_ppfChannels[niChannel], m_pfScratchBufferA, nSamples * sizeof(float));
        for (unsigned ni = 0; ni < m_nOverlapLength; ni++)
        {
            pBFSrcDst->m_ppfChannels[niChannel][ni] += m_pfOverlap[niChannel][ni];
        }
        memcpy(m_pfOverlap[niChannel], &m_pfScratchBufferA[nSamples], m_nOverlapLength * sizeof(float));
    }
}
