/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.h                                  #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_BINAURALIZER_H
#define _AMBISONIC_BINAURALIZER_H

#include <string>
#include <vector>

#include "AmbisonicShelfFilters.h"
#include "AmbisonicDecoder.h"
#include "AmbisonicEncoder.h"
#include "kiss_fftr.h"

#include "mit_hrtf.h"
#include "sofa_hrtf.h"

/// Ambisonic binauralizer

/** B-Format to binaural decoder. */

class CAmbisonicBinauralizer : public CAmbisonicBase
{
public:
    CAmbisonicBinauralizer();

    /** Re-create the object for the given configuration. Previous data is
     *  lost. The tailLength variable it updated with the number of taps
     *  used for the processing, and this can be used to offset the delay
     *  this causes. The function returns true if the call is successful.
     *
     * @param nOrder        The order of the signal to be processed.
     * @param b3D           Set to true if the signal to be processed is 3D. Must be true.
     * @param nSampleRate   Sample rate of the signal to binauralize.
     * @param nBlockSize    The maximum number of samples in a block to be processed.
     * @param tailLength    Returns the length of the HRTF in samples.
     * @param HRTFPath      Path to the HRTF to be used.
     * @param lowCpuMode    If true then uses a symmetric head assumption to reduce CPU use.
     * @return              Returns true if correctly configured.
     */
    virtual bool Configure(unsigned nOrder,
                           bool b3D,
                           unsigned nSampleRate,
                           unsigned nBlockSize,
                           unsigned& tailLength,
                           std::string HRTFPath = "",
                           bool lowCpuMode = true);

    /** Resets the state of the binauralizer. */
    virtual void Reset();

    /** Base class pure virtual function. Not implemented here. */
    virtual void Refresh() override;

    /** Decode B-Format to binaural feeds.
     *
     * @param pBFSrc    the B-format audio to be rendered to binaural
     * @param ppfDst    the output destination
     * @param nSamples = the number of samples to be in the input output. Useful if
     *  working with variable sizes of buffers. Must be less than the max size
     *  set at Configure
     */
    void Process(CBFormat* pBFSrc, float** ppfDst);
    void Process(CBFormat* pBFSrc, float** ppfDst, unsigned int nSamples);

protected:
    CAmbisonicDecoder m_AmbDecoder;

    bool m_useSymHead = true;

    unsigned m_nBlockSize;
    unsigned m_nSampleRate;
    unsigned m_nTaps;
    unsigned m_nFFTSize;
    unsigned m_nFFTBins;
    float m_fFFTScaler;
    unsigned m_nOverlapLength;

    std::unique_ptr<struct kiss_fftr_state, decltype(&kiss_fftr_free)> m_pFFT_cfg;
    std::unique_ptr<struct kiss_fftr_state, decltype(&kiss_fftr_free)> m_pIFFT_cfg;
    std::vector<std::unique_ptr<kiss_fft_cpx[]>> m_ppcpFilters[2];
    std::unique_ptr<kiss_fft_cpx[]> m_pcpScratch;

    std::vector<float> m_pfScratchBufferA;
    std::vector<float> m_pfScratchBufferB;
    std::vector<float> m_pfScratchBufferC;
    std::vector<float> m_pfOverlap[2];

    HRTF *getHRTF(unsigned nSampleRate, std::string HRTFPath);
    virtual void ArrangeSpeakers();
    virtual void AllocateBuffers();
};

#endif // _AMBISONIC_BINAURALIZER_H
