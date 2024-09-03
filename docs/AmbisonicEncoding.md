# Ambisonic Encoding

If you are not interested in the theory you can read about how to use `CAmbisonicEncoder` [here](#cambisonicencoder).

## Theory and Implementation Details

Ambisonic encoding is the conversion of a mono signal to Ambisonics of a specified order.
Ambisonic order governs the spatial resolution of the sound scene.
Higher orders lead to higher resolution at the expense of increased processing requirements.
The number of encoded channels is $1(N + 1)^{2}1$ meaning 1st, 2nd and 3rd order require 4, 9 and 16 channels respectively.

A mono input signal is converted to Ambisonics through multiplication with a series of encoding gains.
`libspatialaudio`uses the AmbiX specification [[1]](#ref1), which orders the channels in the ACN format and uses SN3D normalisation.
AmbiX is currently the most widely used Ambisonics format.

The encoded signal is thus calculated as

```math
\textbf{b}_{N}(t) = \textbf{y}_{N}(\theta,\phi)s(t)
```

where $`s(t)`$ is the mono input signal at time $`t`$ and $`\textbf{y}_{N}(\theta,\phi)`$ is a column vector of length $`(N + 1)^{2}`$ containing the SN3D-weighted spherical harmonic gains $`[Y_{0}^{0}(\theta, \phi), Y_{1}^{-1}(\theta, \phi), Y_{1}^{0}(\theta, \phi),\ldots,Y_{N}^{N}(\theta, \phi)]^{\mathrm{T}}`$. The angles $`\theta`$ and $`\phi`$ are the direction of the sound source and are expected in degrees. The azimuth $`\theta`$ is positive in the anti-clockwise direction, so +90&deg; is to the left of the listener. The elevation $`\phi`$ ranges from -90&deg; below the listener to +90&deg; above.

### Encoding Gain Interpolation

In order to allow to real-time changes in source direction with minimal audio artefacts ("zipper" sounds) as the direction changes, `CAmbisonicEncoder` uses an instance of `CGainInterp` internally.
Every time the direction of the source is changed the gain vector $`\textbf{y}_{N}(\theta,\phi)`$ is updated and a linear interpolation is applied going from the current to new values.
This length of the interpolation can be specified by the user.
Its ideal length will depend on the signals but 10 ms will reduce most "zipper" sounds.

## CAmbisonicEncoder

The `CAmbisonicEncoder` class is used to convert a mono signal to Ambisonics. When the encoding position is modified it internally smooths the encoding gains $`\textbf{y}_{N}(\theta,\phi)`$ to avoid unwanted clicks in the output.

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **nOrder**: The ambisonic order from 1 to 3.
- **b3D**: A bool to indicate if the signal is to be encoded to 2D (azimuth only) or 3D (azimuth and elevation). 3D should be preferred.
- **sampleRate**: The sample rate of the audio being used e.g. 44100 Hz, 48000 Hz etc. This must be an integer value greater than zero.
- **fadeTimeMilliSec**: The time in milliseconds to fade from an old set of encoding gains to another. Lower values will lead to lower latency at the expense of possible audio artefacts. Higher values will lead to increased latency before the source reaches the new encoded position. A value of 10 ms is usually a good starting point.

### Set Encoding Direction

The encoding direction is set as a polar direction in radians using the `SetPosition()` function. It takes a `PolarPoint` as an input.

Note: the distance is ignored. Only the encoding direction is set.

### Encoding a Signal

An array of floats can be encoded using either the `Process()` or `ProcessAccumul()` functions. These two functions process the input signal in the same way. The only difference is that `ProcessAccumul()` will add the newly encoded signal to the output with an optional gain, whereas `Process()` will replace the destination signal with the encoded signal.

The inputs are:

- **pfSrc**: A pointer to the mono input signal.
- **nSamples**: The length of the input signal in samples.
- **pBFDst**: A pointer to the destination B-format signal.
- **nOffset**   Optional offset position when writing to the output. When set to zero this will write the signal to the start of `pBFDst`. Any non-zero value will write to the output with a delay of the specified number of samples, leaving any preceding samples unchanged. The offset and input signal length must not be such that the encoded signal would be written beyond the end of `pBFDst` i.e. `nSamples + nOffset <= pfDst->GetSampleCount()`.
- (`ProcessAccumul()` only) **fGain**: Optional gain to apply to the output before it is added to the signal in `pBFDst`.

### Code Example

This example shows how to convert a mono sine wave to an Ambisonics signal that rotates around the listener from the front and then to the left, back, right and back to the front.

```c++
#include "Ambisonics.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;
const int nBlocks = 94;
const int nSigSamples = nBlocks * nBlockLength; // Approximately 1 second @ 48 kHz

// Higher ambisonic order means higher spatial resolution and more channels required
const unsigned int nOrder = 1;
// Set the fade time to the length of one block
const float fadeTimeInMilliSec = 1000.f * (float)nBlockLength / (float)sampleRate;

std::vector<float> sinewave(nSigSamples);
// Fill the vector with a sine wave
for (int i = 0; i < nSigSamples; ++i)
    sinewave[i] = (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate);

// Destination B-format buffer
CBFormat myBFormat;
myBFormat.Configure(nOrder, true, nSigSamples);
myBFormat.Reset();

// Set up and configure the Ambisonics encoder
CAmbisonicEncoder myEncoder;
myEncoder.Configure(nOrder, true, sampleRate, fadeTimeInMilliSec);

// Set test signal's initial direction in the sound field
PolarPoint position;
position.fAzimuth = 0;
position.fElevation = 0;
position.fDistance = 1.f;
myEncoder.SetPosition(position);
myEncoder.Reset();

for (int iBlock = 0; iBlock < nBlocks; ++iBlock)
{
    // Update the encoding position to reach by the end of the block
    position.fAzimuth = (float)(iBlock + 1) / (float)nBlocks * 2.f * (float)M_PI;
    myEncoder.SetPosition(position);

    // Encode the first block, writing to the appropriate point of the destination buffer
    const unsigned int iSamp = iBlock * nBlockLength;
    myEncoder.Process(&sinewave[iSamp], nBlockLength, &myBFormat, iSamp);
}
```

## References

<a name="ref1">[1]</a> Christian Nachbar, Franz Zotter, Etienne Deleflie, and Alois Sontacchi. Ambix - A Suggested Ambisonics Format. In Ambisonics Symposium, volume 3, pages 1–11, Lexington, KY, 2011
