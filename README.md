Ambisonic encoding / decoding and binauralization library
=============

**libspatialaudio** is an open-source and cross-platform C++ library for **Ambisonic** encoding and decoding, filtering and **binaural** rendering.
It is targetted to render *High-Order Ambisonic* (**HOA**) and *VR/3D audio* samples in multiple environments, from headphones to classic loudspeakers.
Its *binaural rendering* can be used for classical 5.1/7.1 spatial channels as well as Ambisonics inputs.

Originally it is a fork of **ambisonic-lib** from Aristotel Digenis. This version was developed to support Higher Order Ambisonics *HOA* and to support **ACN/SN3D** Ambisonics audio streams following the **Google spatial audio** specification:
https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md and the **IETF codec Ambisonics** specification https://tools.ietf.org/html/draft-ietf-codec-ambisonics

The library allows you to encode, decode, rotate, zoom **HOA Ambisonics** audio streams up to the *3rd order*. It can output to standard and custom loudspeakers arrays. To playback with headphones, the binauralizer applies an HRTF *(either a SOFA file or  the included MIT HRTF)* to provide a spatial binaural rendering effect. The binauralization can also be used to render multichannels streams (5.1, 7.1...).

A central part of the library is the CBFormat object which acts as a buffer for B-Format. There are several other objects, each with a specific tasks, such as encoding, decoding, and processing for Ambisonics. All of these objects handle CBFormat objects at some point.


## Features
### Encoder (CAmbisonicEncoder):
Simple encoder up to 3rd order 3D, without any distance cues.

### Encoder with distance (CAmbisonicEncoderDist):
As the simple encoder, but with the addition of the following:
* Distance level-simulation
* Fractional delay lines
* Interior effect (W-Panning)

### Decoder (CAmbisonicDecoder):
Simple decoder up to the 3rd Order 3D with:
* Preset & custom speaker arrays
* Decoder that improves the rendering with a 5.1 speaker set

### Processor (CAmbisonicProcessor):
Up to 3rd order 3D yaw/roll/pitch of the soundfield

Up to 3rd order psychoacoustic optimisation shelf-filters for 2D and 3D playback

### Binauralizer (CAmbisonicBinauralizer):
Up to 3rd order 3D decoding to headphones

Optional symmetric head decoder to reduce the number of convolutions

### Zoomer (CAmbisonicZoomer):
Up to 1st order 3D front-back dominance control of the soundfield

## Overview of the Implemented Algorithms
### Psychoacoustic Optimisation Shelf-filters
Implemented as linear phase FIR shelf-filters ensure basic and max rE decodes in low- and high-frequency ranges respectively. See [[1]](#ref1) for more details why and [[2]](#ref2) for the mathematical theory used for higher orders.

The transition frequency between the two decoder types depends on the order being decoded, increasing with Ambisonic order.

The frequency is given by [[3]](#ref3):
```c

f_lim = speedofsound*M / (4*R*(M+1)*sin(PI / (2*M+2)))
```
where speedofsound = 343 m/s, R = 0.09 m (roughly the radius of human head) and M = Ambisonic order.

A different gain is applied to each of the channels of a particular order. These are given by [[2](#ref2),[4](#ref4)]:
```c
2D: g_m = cos(pi*m/(2M + 2))
3D: g_m = legendre(m, cos(137.9*(PI/180)/(M+1.51)))
```
where m = floor(sqrt(Channel Number)) and legendre(m,x) is a Legendre polynomial of degree m evaluated for a value of x.


### Binaural Decoding
The binaural decoder uses a two different virtual loudspeaker arrays depending on the order:
* 1st order: cuboid loudspeaker array
* 2nd and 3rd order: Dodecahedron

To keep the number of convolutions to a minimum, the HRTFs are decomposed into spherical harmonics. This gives a pair of HRTF filters for each of the Ambisonics channel.
The advantage of this method is that the number of convolutions is limited to the number of Ambisonic channels, regardless of the number of virtual loudspeakers used.

### Symmetric Head Binaural Decoder
The binaural decoder can reduce the number of convolutions needed for the binaural decoding by two.

The Ambisonic input channels are convolved with the corresponding HRTF channel for the left ear. The left ear signal is the sum of these convolved channels. To generate the right ear signal the soundfield is reflected left-right by multiplying several of the convolved channels by -1. These are then summed to produce the right ear signal.

## How do I use it?

The following sample code shows the encoding of a sine wave into an Ambisonic soundfield, and then decoding that soundfield over a Quad speaker setup.

```c
// Generation of mono test signal
float sinewave[512];
for(int ni = 0; ni < 512; ni++)
    sinewave[ni] = (float)sin((ni / 128.f) * (M_PI * 2));

// CBFormat as 1st order 3D, and 512 samples
CBFormat myBFormat;

// Ambisonic encoder, also 3rd order 3D
CAmbisonicEncoder myEncoder;
myEncoder.Configure(1, true, 0);

// Set test signal's position in the soundfield
PolarPoint position;
position.fAzimuth = 0;
position.fElevation = 0;
position.fDistance = 5;
myEncoder.SetPosition(position);
myEncoder.Refresh();

// Encode test signal into BFormat buffer
myEncoder.Process(sinewave, 512, &myBFormat);

// Ambisonic decoder, also 1st order 3D, for a 5.0 setup
CAmbisonicDecoder myDecoder;
myDecoder.Configure(1, true, kAmblib_50, 5);

// Allocate buffers for speaker feeds
float** ppfSpeakerFeeds = new float*[5];
for(int niSpeaker = 0; niSpeaker < 5; niSpeaker++)
    ppfSpeakerFeeds[niSpeaker] = new float[512];

// Decode to get the speaker feeds
myDecoder.Process(&myBFormat, 512, ppfSpeakerFeeds);

// De-allocate speaker feed buffers
for(int niSpeaker = 0; niSpeaker < 5; niSpeaker++)
    delete [] ppfSpeakerFeeds[niSpeaker];
delete [] ppfSpeakerFeeds;
```

## References

<a name="ref1">[1] M. A. Gerzon, “Practical Periphony: The Reproduction of Full-Sphere Sound,” in Audio Engineering Society Convention, 1980, pp. 1–12.</a>

<a name="ref2">[2] J. Daniel, “Représentation de champs acoustiques, application à la transmission et à la reproduction de scènes sonores complexes dans un contexte multimédia,” University of Paris, 2000.</a>

<a name="ref3">[3] S. Bertet, J. Daniel, E. Parizet, and O. Warusfel, “Investigation on localisation accuracy for first and higher order Ambisonics reproduced sound sources,” Acta Acust. united with Acust., vol. 99, no. 4, pp. 642–657, 2013.</a>

<a name="ref4">[4] F. Zotter and M. Frank, “All-round ambisonic panning and decoding,” J. Audio Eng. Soc., vol. 60, no. 10, pp. 807–820, 2012.</a>
