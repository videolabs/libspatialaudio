# Ambisonic Rotation

If you are not interested in the theory you can read about how to use `CAmbisonicRotator` [here](#cambisonicrotator).

## Theory and Implementation Details

Any signal in the ambisonic domain can be rotated by application of a rotation matrix.
This allows (for example) head tracking data to be used to rotate the sound field in the opposite direction to how the listener is moving their head, which will keep the sound sources in the correct absolute position in space, rather than them moving with the listeners head.

Rotation is performed by multiplying the Ambisonic signal $`\textbf{b}_{N}(t)`$ by a rotation matrix:

```math
\textbf{b}'_{N}(t) = \textbf{R}_{N}(\alpha, \beta, \gamma) \textbf{b}_{N}(t).
```

The rotation angles are yaw $`\alpha`$, pitch $`\beta`$ and roll $`\gamma`$ around the z-axis, y-axis and x-axis respectively.

A positive yaw angle will cause a sound source to rotated in a clockwise direction with respect to the origin when viewed from along the z-axis (above). From the point-of-view of the listener this is as if they turn to their left.

A positive pitch angle will cause a sound source to move above the listener. From the point-of-view of the listener this is as if they tilt their head forward.

A positive roll angle will rotate a sound from the left to the right underneath the listener. From the point-of-view of the listener this is as if they roll their head to the right.

Rotations can be applied in any order but changing the order of the rotation will not result in the same output, so care must be taken.
The yaw-pitch-roll order, where rotations around the z-axis are performed first, is common in Ambisonics, but `CAmbisonicRotator` gives the option for any combination to be used.

Rotation of the sound field is particularly beneficial when working with a binaural renderer because it fixes the sound sources in space.
This can help to reduce front-to-back confusions or with sound source externalisation.

Note that `AmbisonicRotator` uses hard-coded equations to generate the individual yaw, pitch and roll rotation matrices. However, for higher orders a recurrence relation based method can be used to compute $`\textbf{R}_{N}(\alpha, \beta, \gamma)`$ [[1]](#ref1).

## CAmbisonicRotator

The `CAmbisonicRotator` class is used to change the orientation of a supplied Ambisonic signal in real-time. In order to avoid clicks while rotating the rotation matrices are interpolated over a specified length of time set during configuration of the object.

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **nOrder**: The ambisonic order from 1 to 3.
- **b3D**: `CAmbisonicRotator` only supports rotation of 3D sound scenes. This should be set to `true` or else configuration will fail.
- **nBlockSize**: The maximum block size `Process()` is expected to handle.
- **sampleRate**: The sample rate of the audio being used e.g. 44100 Hz, 48000 Hz etc. This must be an integer value greater than zero.
- **fadeTimeMilliSec**: The time in milliseconds to fade from an old matrix to another. Lower values will lead to lower latency at the expense of possible audio artefacts. Higher values will lead to increased latency before the source reaches the new orientation. A value of 10 ms is usually a good starting point.

### Set Rotation Order and Orientation

The default rotation ordering is yaw-pitch-roll. However any desired order can be specified using `SetRotationOrder()`. Changing the rotation ordering will cause the rotation matrix to be updated using the new ordering.

The yaw, pitch and roll values are set by passing a `RotationOrientation` to `SetOrientation()`. The rotation angles should be supplied in radians.

### Rotating an Ambisonic Signal

A B-format signal can be rotated using the `Process()` function. The input signal is replaced by the rotated signal. The inputs are:

- **pBFSrcDst**: A pointer to the source B-format signal that is replaced with the processed signal.
- **nSamples**: The length of the input signal in samples.

### Code Example

This example shows how to rotate an Ambisonics signal by 90 degrees ($'\pi/2'$ radians) so that it is heard as coming from the right of the listener when decoded.

```c++
#include "Ambisonics.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;

// Higher ambisonic order means higher spatial resolution and more channels required
const unsigned int nOrder = 1;
// Set the fade time to the length of one block
const float fadeTimeInMilliSec = 1000.f * (float)nBlockLength / (float)sampleRate;

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

// Set up the rotator
CAmbisonicRotator myRotator;
myRotator.Configure(nOrder, true, nBlockLength, sampleRate, fadeTimeInMilliSec);
RotationOrientation rotOri;
rotOri.yaw = 0.5 * M_PI; // pi/2 radians = 90 degrees
myRotator.SetOrientation(rotOri);

// Rotate the Ambisonics signal
myRotator.Process(&myBFormat, nBlockLength);
```

## References

<a name="ref1">[1]</a> Joseph Ivanic and Klaus Ruedenberg. Rotation matrices for real spherical harmonics. direct determination by recursion. The Journal of Physical Chemistry, 100(15):6342–6347, 1996.
