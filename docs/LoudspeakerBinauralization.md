# Loudspeaker Binauralization

If you are not interested in the theory you can read about how to use `CSpeakerBinauralization` [here](#cspeakersbinauralizer).

## Theory and Implementation Details

Binauralization of a loudspeaker array uses a simple method.
First, a pair of head-related impulse responses (HRIRs) for each of the loudspeaker directions is convolved with the signal of that loudspeaker.
The convolved signals are summed to give a final 2-channel binaural signal for listening over headphones.

```math
z_{ear}(t) = \sum_{m = 0}^{M - 1} h_{m,ear}(t)\circledast x_{m}(t)
```

where $`z_{ear}(t)`$ is the signal at the left or right ear, $`h_{ear}`$ is the corresponding HRIR for the left/right ear, $`x_{m}(t)`$ is the signal of the $m$-th loudspeaker and $M$ is the total number of loudspeakers.

`CSpeakersBinauralizer` provides a simple convolution of a set of supplied loudspeaker signals with a pair of HRTFs (one for each ear).
The convolution is implemented in the frequency domain using an overlap-add algorithm.

If `libmysofa` is activated at compile time then the HRTF can be loaded from a .SOFA file, allowing for the use of custom HRTFs.
Custom HRTFs allow for users to potentially load their own HRTF, giving highly personalised rendering.

## CSpeakersBinauralizer

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **sampleRate**: The sample rate of the audio being used e.g. 44100 Hz, 48000 Hz etc. This must be an integer value greater than zero.
- **nBlockSize**: The maximum number of samples the decoder is expected to process at a time.
- **speakers**: A pointer to an array of `CAmbisonicSpeaker` holding the speaker directions.
- **nSpeakers**: The number of speakers to be binauralized.
- **tailLength**: The value is replaced with the length of the HRIRs used for the binauralisation.
- **HRTFPath**: An optional path to the .SOFA file containing the HRTF. If no path is supplied and the `HAVE_MIT_HRTF` compiler flag is used then the MIT HRTF will be used.

### Binauralizing a Signal

A set of loudspeaker signals can be decoded to the binaural signals using the `Process()` function. The processing is non-replacing, so the original loudspeaker signals are unchanged and the decoded signal is contained in the output array.

The inputs are:

- **pBFSrc**: A pointer to the source B-format signal.
- **ppDst**: Array of pointers of size 2 x nBlockSize containing the binaurally decoded signal.

### Code Example

This example shows how to convert a set of loudspeaker signals to a 2-channel binaural signal.

```c++
#include "SpeakersBinauralizer.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;

// Configure the speaker layout
const unsigned int nSpeakers = 5;
CAmbisonicSpeaker speakers[nSpeakers];
speakers[0].SetPosition({ 30.f / 180.f * (float)M_PI, 0.f, 1.f }); // L
speakers[1].SetPosition({ -30.f / 180.f * (float)M_PI, 0.f, 1.f }); // R
speakers[2].SetPosition({ 0.f / 180.f * (float)M_PI, 0.f, 1.f }); // C
speakers[3].SetPosition({ 110.f / 180.f * (float)M_PI, 0.f, 1.f }); // Ls
speakers[4].SetPosition({ -110.f / 180.f * (float)M_PI, 0.f, 1.f }); // Rs

// Configure buffers to hold the 5.0 signal
float** ldspkInput = new float* [5];
for (int iLdspk = 0; iLdspk < ; ++iLdspk)
{
    ldspkInput[iLdspk] = new float[nBlockLength];
    // Fill L and C channels, panning the signal to half way between both speakers
    for (int i = 0; i < nBlockLength; ++i)
        ldspkInput[iLdspk][i] = iLdspk == 0 || iLdspk == 2 ? (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate) : 0.f;
}

// Set up the binauralizer
SpeakersBinauralizer myBinaural;
unsigned int tailLength = 0;
myBinaural.Configure(sampleRate, nBlockLength, speakers, nSpeakers, tailLength);

// Configure buffers to hold the decoded signal
const unsigned int nEar = 2;
float** earOut = new float* [nEar];
for (int iEar = 0; iEar < nEar; ++iEar)
    earOut[iEar] = new float[nBlockLength];

// Decode the Ambisonics signal
myBinaural.Process(&ldspkInput[0], earOut);

// Cleanup
for (unsigned iEar = 0; iEar < nEar; ++iEar)
    delete earOut[iEar];
delete[] earOut;
for (unsigned iLdspk = 0; iLdspk < nSpeakers; ++iLdspk)
    delete ldspkInput[iLdspk];
delete[] ldspkInput;
```
