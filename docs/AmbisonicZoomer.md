# Ambisonic Zoomer

If you are not interested in the theory you can read about how to use `CAmbisonicZoomer` [here](#cambisoniczoomer).

## Theory and Implementation Details

The `CAmbisonicZoomer` class allows for a kind of acoustic zoom to the front of the sound field.
It was implemented for use with the zoom function with 360-videos in VLC Media Player to place emphasis on sounds in the view direction.

It samples the sound field to the front with a virtual cardioid microphone with order matching the order of the input signal.
This mono virtual microphone signal is then re-encoded back to a signal at the front of the sound field and blended with the full sound field based on the zoom value set in `CAmbisonicZoomer::SetZoom`.

A zoom value of 0 will perform no transformation to the ambisonic signal while a zoom of 1 will lead to an extreme zoom, which results in mono signal encoded to the front of the sound field.

Another method for achieving a similar effect would be to decode the Ambisonics signal to a set of points regularly spaced on the sphere, applying a directional weight to these points, then re-encoding the points to Ambisonics.
However, this requires a transformation from and to the Ambisonics domain over a large number of points. This makes it more CPU intensive than the simple method used here, and so it is not implemented in `libspatialaudio`.

## CAmbisonicZoomer

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **nOrder**: The ambisonic order from 1 to 3.
- **b3D**: A bool to indicate if the signal is to be decoded is 2D (azimuth only) or 3D (azimuth and elevation).
- **nBlockSize**: The maximum block size `Process()` is expected to handle.
- **nMisc**: Unused parameter to match base class's form.

### Setting the Zoom Amount

The amount of zoom can be set from 0 to 1 using 'SetZoom()'. A value of 0 corresponds to no zoom and the signal will be returned unchanged after processing.
A value of 1 means that the maximum amount of zooming will be applied and almost all of the signal will be from the re-encoded forward-facing virtual microphone.

### Zooming a Signal

A B-format signal can have zooming applied using the `Process()` function. The input signal is replaced by the processed signal.

The inputs are:

- **pBFSrcDst**: A pointer to the source B-format signal.
- **nSamples**: The number of samples to process.

### Code Example

This example shows how to apply zooming to an Ambisonics signal.

```c++
#include "Ambisonics.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;

// Higher ambisonic order means higher spatial resolution and more channels required
const unsigned int nOrder = 1;

std::vector<float> sinewave(nBlockLength);
// Fill the vector with a sine wave
for (int i = 0; i < nBlockLength; ++i)
    sinewave[i] = (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate);

// B-format buffer
CBFormat myBFormat;
myBFormat.Configure(nOrder, true, nBlockLength);
myBFormat.Reset();

// Encode the signal to Ambisonics
CAmbisonicEncoder myEncoder;
myEncoder.Configure(nOrder, true, sampleRate, 0);
PolarPoint position;
position.fAzimuth = 0;
position.fElevation = 0;
position.fDistance = 1.f;
myEncoder.SetPosition(position);
myEncoder.Reset();
myEncoder.Process(sinewave.data(), nBlockLength, &myBFormat);

// Set up the zoomer processor
CAmbisonicZoomer myZoomer;
myZoomer.Configure(nOrder, true, nBlockLength, 0);
myZoomer.SetZoom(0.5f);

// Apply zooming to the Ambisonics signal
myZoomer.Process(&myBFormat, nBlockLength);
```
