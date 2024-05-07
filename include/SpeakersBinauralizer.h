#ifndef BINAURALIZER_H
#define BINAURALIZER_H

#include <vector>

#include "AmbisonicCommons.h"
#include "BFormat.h"
#include "AmbisonicSpeaker.h"
#include "AmbisonicBinauralizer.h"


/** A class to convert speaker signals to binaural by applying the
appropriate static HRTFs.
*/
class SpeakersBinauralizer : public CAmbisonicBinauralizer
{
public:
    SpeakersBinauralizer();

    /** Configure the speaker binauralizer instance.
     * @param nSampleRate   Sample rate of the audio to be processed.
     * @param nBlockSize    Maximum number of samples to be processed per block.
     * @param speakers      Pointer to any array of CAmbisonicSpeaker which define the layout to be binauralized.
     * @param nSpeakers     Number of speakers.
     * @param tailLength    Return the length of the HRTF
     * @param HRTFPath      Path to the HRTF
     * @return              Returns true if correctly configured.
     */
    bool Configure(unsigned nSampleRate,
                   unsigned nBlockSize,
                   CAmbisonicSpeaker *speakers,
                   unsigned nSpeakers,
                   unsigned& tailLength,
                   std::string HRTFPath = "");

    /** Convert the source to binaural.
     * @param pBFSrc    Input speaker signals of size nSpeakers x nBlockSize as set in Configure
     * @param ppfDst    Output binaural signals of size 2 x nBlockSize
     */
    void Process(float** pBFSrc, float** ppfDst);

protected:
    unsigned m_nSpeakers;

    /** Allocate the buffers required for the convolution processing. */
    virtual void AllocateBuffers();
};

#endif // BINAURALIZER_H
