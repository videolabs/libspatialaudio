# Ambisonic Decoding to Binaural

If you are not interested in the theory you can read about how to use `CAmbisonicBinauralizer` [here](#cambisonicbinauralizer).

## Theory and Implementation Details

There are several methods for decoding from Ambisonics to binaural [[1]](#ref1)[[2]](#ref2)[[3]](#ref3).
In general, decoding is performed by convolution of the ambisonic signal with a matrix of filters.
`CAmbisonicBinauralizer` uses a virtual-speaker approach [[1]](#ref1).

The virtual loudspeaker approach essentially decodes the signal to known loudspeaker layout and then applies an HRTF to each of the loudspeakers, which are summed to make the final 2-channel binaural output.
This requires $2\times M$ convolutions, where $M$ is the number of virtual loudspeakers.

In order to reduce the number of convolutions the HRTF of each virtual speaker is scaled by the corresponding ambisonic gains and summed to create a filter for each spherical harmonic. For an HRIR of length $k$ samples this is calculated using

```math
\mathbf{Z}_{N,ear}^{\mathrm{bin}} = \frac{1}{\sqrt{M}} \left[ \tilde{\textbf{y}}_{N}(\theta_{1}, \phi_{1}), \tilde{\textbf{y}}_{N}(\theta_{2}, \phi_{2}), \ldots, \tilde{\textbf{y}}_{N}(\theta_{M}, \phi_{M}) \right] \mathbf{H}_{\mathrm{ear}}
```

where $`\mathbf{Z}_{N,ear}^{\mathrm{bin}}`$ is a matrix of size $`(N+1)^{2} \times k`$ containing the spherical harmonic decomposition of the loudspeaker array HRIRs, $`\mathbf{H}_{\mathrm{ear}}`$ is an $`M\times k`$ matrix containing the HRIRs corresponding to each of the loudspeaker directions for the left or right ear.
This leads to $2\times (N + 1)^2$ convolutions which can be calculated using

```math
x_{\mathrm{bin}}^{ear}(t) = \sum_{i = 0}^{(N+1)^{2}} q_{i} z_{i}^{ear}(t) \circledast b_{i}(t)
```

where $`z_{i}^{ear}(t)`$ is the filter in the $`i`$-th row of $`\mathbf{Z}_{\mathrm{N,ear}}^{\mathrm{bin}}`$, $`b_{i}(t)`$ is the $i$-th term in the ambisonic signal $`\textbf{b}_{N}(t)`$, and $`q_{i}`$ is the $i$-th element of $`\mathbf{q}_{N}`$ which contains gains that convert the input signal from SN3D to N3D.

The number of convolutions can be reduced further to only $(N+1)^2$ convolutions if the head is assumed to be symmetric [[4]](#ref4).
This is done by calculating the convolutions in the above equation for one ear and storing them.
They are summed for the first ear directly.
For the opposite ear the spherical harmonics that are left-right symmetric have their polarity inverted and before being summed.

### Virtual Loudspeaker Layout Choices

For first-order signals `CAmbisonicBinauralizer` uses a virtual layout on the vertices of a cube (8 virtual loudspeakers).
For second- and third-order a virtual layout on the vertices of a dodecahedron (20 virtual loudspeakers) is used.
Therefore, in both cases, preprocessing the HRTFs reduces the number of convolutions required (from 8 to 4 for first-order and 20 to 9 or 16 for second- and third-order respectively).
This meets a requirement of `libspatialaudio` to minimise its CPU use.

A cube was chosen for first-order because it is a mathematically ideal layout for this order.
The dodocahedron was chosen because it provides good results for second- and third-order order and 8 of its 20 loudspeaker directions coincide directly with the cube layout.
This allows for the number of HRTFs stored for both layouts to be only 20, meeting a requirement that `libspatialaudio` be lightweight.

![The quality measures for a basic decoder (top row) and max $`r_{\mathrm{E}}`$ decoder (bottom row) as a function of the source direction across the whole sphere for a third-order decoding to a dodecahedral loudspeaker array.
    The directions of the loudspeakers are shown as red circles.](images/dodeca_decoder.png)

The above figure shows the total loudspeaker gain and energy for the dodecahedron for third-order (first- and second-order are now shown because they have equal performance for all source directions).
It also shows the difference between the panning direction and velocity and energy vectors [[5]](#ref5), representing a prediction of perceived direction for low and high frequencies respectively.
Finally, it also shows the velocity and energy vector magnitudes, for which a value of 1 is ideal (although an energy vector magnitude of 1 is not possible for a sound source reproduced by more than one loudspeaker).

The figure shows that both amplitude and energy are constant for all source directions. This is ensures that there are no unwanted changes in level for different source directions.
The velocity vector has perfect localisation and magnitude, indicating that at low frequencies the decoder is also performing ideally.
The max $r_{\mathrm{E}}$ decoder for high frequencies shows small deviations between the source direction and the energy vector prediction.
However, the maximum error is only 6.5\deg.
The energy vector magnitude also exhibits some variation with source position.
This may result in small variations in the perceived source width, but this effect will be small.

### Psychoacoustic Optimisation

As outlined [here](AmbisonicOptimisation.md) shelf filtering can be applied to Ambisonics signal so that they are decoded in a psychoacoustically optimised manner.

In order to reduce the CPU use `CAmbisonicBinauralizer` pre-filters the spherical harmonic impulse responses in $`\mathbf{Z}_{N,ear}^{\mathrm{bin}}`$ rather than applying the shelf filtering at run-time.

This allows the psychoacoustic optimisation to be applied for "free" at runtime, at the minor cost of performing the optimisation filtering during configuration.

## CAmbisonicBinauralizer

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **nOrder**: The ambisonic order from 1 to 3.
- **b3D**: A bool to indicate if the signal is to be decoded is 2D (azimuth only) or 3D (azimuth and elevation).
- **nBlockSize**: The maximum number of samples the decoder is expected to process at a time.
- **sampleRate**: The sample rate of the audio being used e.g. 44100 Hz, 48000 Hz etc. This must be an integer value greater than zero.
- **tailLength**: The value is replaced with the length of the HRIRs used for the binauralisation.
- **HRTFPath**: An optional path to the .SOFA file containing the HRTF. If no path is supplied and the `HAVE_MIT_HRTF` compiler flag is used then the MIT HRTF will be used.
- **lowCpuMode**: (Optional) If this is set to true (its default value) then the symmetric head assumption is used to reduce CPU used [[4]](#ref4).

### Decoding a Signal

A B-format signal can be decoded to the binaural signals using the `Process()` function. The processing is non-replacing, so the original B-format signal is unchanged and the decoded signal is contained in the output array.

The inputs are:

- **pBFSrc**: A pointer to the source B-format signal.
- **ppDst**: Array of pointers of size 2 x nSamples containing the binaurally decoded signal.
- **nSamples**: (Optional) The length of the input signal in samples. If this is not supplied then `nBlockSize` samples are assumed.


### Code Example

This example shows how to decode an Ambisonics signal to a binaural.

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
position.fAzimuth = (float)M_PI * 0.5f;
position.fElevation = 0;
position.fDistance = 1.f;
myEncoder.SetPosition(position);
myEncoder.Reset();
myEncoder.Process(sinewave.data(), nBlockLength, &myBFormat);

// Set up the binaural decoder
CAmbisonicBinauralizer myDecoder;
unsigned int tailLength = 0;
myDecoder.Configure(nOrder, true, sampleRate, nBlockLength, tailLength);

// Configure buffers to hold the decoded signal
const unsigned int nEar = 2;
float** earOut = new float* [nEar];
for (int iEar = 0; iEar < nEar; ++iEar)
    earOut[iEar] = new float[nBlockLength];

// Decode the Ambisonics signal
myDecoder.Process(&myBFormat, earOut, nBlockLength);

// Cleanup
for (unsigned iEar = 0; iEar < nEar; ++iEar)
    delete earOut[iEar];
delete[] earOut;
```

## References

<a name="ref1">[1]</a> Markus Noisternig, Alois Sontacchi, Thomas Musil, and Robert Höldrich. A 3D Ambisonic Based Binaural Sound Reproduction System. In 24th International Conference of the Audio Engineering Society, pages 1–5, June 2003.

<a name="ref2">[2]</a> Christian Schörkhuber, Markus Zaunschirm, and Robert Höldrich. Binaural rendering of ambisonic signals via magnitude least squares. In Proceedings of the DAGA, volume 44, pages 339–342, 2018.

<a name="ref3">[3]</a> Markus Zaunschirm, Christian Schörkhuber, and Robert Höldrich. Binaural rendering of Ambisonic signals by head-related impulse response time alignment and a diffuseness constraint. The Journal of the Acoustical Society of America, 143(6):3616–3627, 2018. ISSN 0001-4966. doi:10.1121/1.5040489.

<a name="ref3">[4]</a> Archontis Politis and David Poirier-Quinot. JSAmbisonics: A web audio library for interactive spatial sound processing on the web. In Interactive Audio Systems Symposium, 2016.

<a name="ref6">[5]</a> Michael Gerzon. General metatheory of auditory localisation. In 92nd Convention of the Audio Engineering Society, Vienna, March 1992.
