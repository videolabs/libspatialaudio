#ifndef BINAURALIZER_H
#define BINAURALIZER_H

#include <vector>

#include "AmbisonicCommons.h"
#include "BFormat.h"
#include "AmbisonicSpeaker.h"

#include "mit_hrtf_lib.h"
#include "kiss_fftr.h"


/** Binaural decoder. */

class Binauralizer
{
public:
    Binauralizer();
    ~Binauralizer();

    AmbBool Create(AmbUInt nSampleRate,
                   AmbUInt nBlockSize,
                   AmbBool bDiffused,
                   CAmbisonicSpeaker *speakers,
                   AmbUInt nSpeakers,
                   AmbUInt& tailLength);

    void Reset();
    void Process(AmbFloat** pBFSrc, AmbFloat** ppfDst);

private:
    AmbUInt m_nBlockSize;
    AmbUInt m_nTaps;
    AmbUInt m_nFFTSize;
    AmbUInt m_nFFTBins;
    AmbFloat m_fFFTScaler;
    AmbUInt m_nOverlapLength;

    kiss_fftr_cfg m_pFFT_cfg;
    kiss_fftr_cfg m_pIFFT_cfg;
    kiss_fft_cpx** m_ppcpFilters[2];
    kiss_fft_cpx* m_pcpScratch;

    AmbFloat* m_pfScratchBufferA;
    AmbFloat* m_pfScratchBufferB;
    AmbFloat* m_pfOverlap[2];

    CAmbisonicSpeaker *m_speakers;
    AmbUInt m_nSpeakers;

    void AllocateBuffers();
    void DeallocateBuffers();
};

#endif // BINAURALIZER_H
