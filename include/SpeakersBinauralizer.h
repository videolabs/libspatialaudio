#ifndef BINAURALIZER_H
#define BINAURALIZER_H

#include <vector>

#include "AmbisonicCommons.h"
#include "BFormat.h"
#include "AmbisonicSpeaker.h"
#include "AmbisonicBinauralizer.h"


/** Binaural decoder. */

class SpeakersBinauralizer : public CAmbisonicBinauralizer
{
public:
    AmbBool Create(AmbUInt nSampleRate,
                   AmbUInt nBlockSize,
                   CAmbisonicSpeaker *speakers,
                   AmbUInt nSpeakers,
                   AmbUInt& tailLength,
                   std::string HRTFPath = "");

    void Process(AmbFloat** pBFSrc, AmbFloat** ppfDst);

protected:
    CAmbisonicSpeaker *m_speakers;
    AmbUInt m_nSpeakers;

    virtual void AllocateBuffers();
    virtual void DeallocateBuffers();
};

#endif // BINAURALIZER_H
