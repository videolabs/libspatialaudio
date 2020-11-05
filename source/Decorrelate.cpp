/*############################################################################*/
/*#                                                                          #*/
/*#  A decorrelator for loudspeaker arrays                                   #*/
/*#                                                                          #*/
/*#  Filename:      AdmRenderer.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "Decorrelate.h"

#include<random>

CDecorrelate::CDecorrelate()
{
    m_pfScratchBufferA = nullptr;
    m_pFFT_decor_cfg = nullptr;
    m_pIFFT_decor_cfg = nullptr;
    m_ppcpDecorFilters = nullptr;
    m_pcpScratch = nullptr;
    m_pfOverlap = nullptr;
}

CDecorrelate::~CDecorrelate()
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

bool CDecorrelate::Configure(Layout layout, unsigned int nBlockSize)
{
    m_layout = layout;

    // Set the number fo channels
    m_nCh = (unsigned int)layout.channels.size();

    // The decorrelation filters have the same length regardless of the input block size.
    // The length is 512 samples (Rec. ITU-R BS.2127-0, sec 7.4, pg 63)
    unsigned nbTaps = m_nDelay * 2;

    m_nBlockSize = nBlockSize;
    m_nTaps = nbTaps;

    // Length of the delay lines used to compensate the direct signal
    m_nDelayLineLength = m_nDecorrelationFilterSamples + m_nBlockSize;

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

    // Get the decorrelation filter bank
    decorrelationFilters = CalculateDecorrelationFilterBank();

    for (unsigned i_m = 0; i_m < m_nCh; i_m++) {
        // Convert the impulse responses to the frequency domain
        memcpy(m_pfScratchBufferA, &decorrelationFilters[i_m][0], m_nTaps * sizeof(float));
        memset(&m_pfScratchBufferA[m_nTaps], 0, (m_nFFTSize - m_nTaps) * sizeof(float));
        kiss_fftr(m_pFFT_decor_cfg, m_pfScratchBufferA, m_ppcpDecorFilters[i_m]);
    }

    return true;
}

void CDecorrelate::Reset()
{
    for (unsigned i = 0; i < m_nCh; i++)
    {
        memset(m_pfOverlap[i], 0, m_nOverlapLength * sizeof(float));
        memset(m_ppfDirectDelay[i], 0, m_nDelayLineLength * sizeof(float));
    }
}

void CDecorrelate::Process(std::vector<std::vector<float>>& ppInDirect, std::vector<std::vector<float>>& ppInDiffuse, unsigned int nSamples)
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
    if (m_nWritePos >= (int)m_nDelayLineLength)
        m_nWritePos -= (int)m_nDelayLineLength;
}

void CDecorrelate::WriteToDelayLine(float* pDelayLine, float* pIn, int nWritePos, int nSamples)
{
    int overrun = nWritePos + nSamples - m_nDelayLineLength;
    if (overrun > 0) {
        memcpy(&pDelayLine[nWritePos], &pIn[0], (nSamples - overrun) * sizeof(float));
        memcpy(&pDelayLine[0], &pIn[nSamples - overrun], overrun * sizeof(float));
    }
    else {
        memcpy(&pDelayLine[nWritePos], &pIn[0], nSamples * sizeof(float));
    }
}

void CDecorrelate::ReadFromDelayLine(float* pDelayLine, float* pOut, int nReadPos, int nSamples)
{
    int overrun = nReadPos + nSamples - m_nDelayLineLength;
    if (overrun > 0) {
        memcpy(&pOut[0], &pDelayLine[nReadPos], (nSamples - overrun) * sizeof(float));
        memcpy(&pOut[nSamples - overrun], &pDelayLine[0], overrun * sizeof(float));
    }
    else {
        memcpy(&pOut[0], &pDelayLine[nReadPos], nSamples * sizeof(float));
    }
}

std::vector<std::vector<float>> CDecorrelate::CalculateDecorrelationFilterBank()
{
    std::vector<std::vector<float>> decorrelationFilter;
    // Get the index of the channel names in a sorted list of all channel names in the layout
    std::vector<std::string> channelNames = m_layout.channelNames();
    std::vector<std::string> channelNamesSorted = channelNames;
    std::sort(channelNamesSorted.begin(), channelNamesSorted.end());
    std::vector<unsigned int> chNameSortedInd;
    for (unsigned int iName = 0; iName < m_nCh; ++iName)
        for (unsigned int i = 0; i < m_nCh; ++i)
            if (channelNames[iName] == channelNamesSorted[i])
                decorrelationFilter.push_back(CalculateDecorrelationFilter(i));

    return decorrelationFilter;
}

std::vector<float> CDecorrelate::CalculateDecorrelationFilter(unsigned int seedIndex)
{
    int N = (int)m_nDecorrelationFilterSamples;

    // Get random numbers generated using the MT19937 pseudorandom number generator
    std::mt19937 randNumGen(seedIndex);
    std::vector<double> r;
    double randMax = (double)randNumGen.max();
    for (int i = 0; i < N / 2 - 1; ++i)
        r.push_back((double)randNumGen() / randMax);


    // Calculate the frequency domain data
    std::vector<kiss_fft_cpx> x(N / 2 + 1, { 1.,0. });
    // The first and last elements in the frequency domain data are 1 (phase = 0) so
    // only loop over the points between
    for (int i = 1; i < N / 2; ++i)
    {
        double phaseAngle = 2. * M_PI * r[i - 1];
        x[i].r = cosf((float)phaseAngle);
        x[i].i = sinf((float)phaseAngle);
    }
    // Calculate the time domain data
    std::vector<float> decorrelationFilter(m_nDecorrelationFilterSamples);
    kiss_fftr_cfg ifft_cfg = kiss_fftr_alloc(m_nDecorrelationFilterSamples, 1, 0, 0);
    kiss_fftri(ifft_cfg, &x[0], &decorrelationFilter[0]);
    for (auto& sample : decorrelationFilter)
        sample /= (float)m_nDecorrelationFilterSamples;

    kiss_fftr_free(ifft_cfg);

    return decorrelationFilter;
}