Ambisonic encoding / decoding and binauralization library
=============

**libspatialaudio** is a cross-platform C++ library for Ambisonic encoding, decoding, and other processing.
Originally it is a fork of **ambisonic-lib** from Aristotel Digenis. Our version was developped to support **ACN/SN3D** Ambisonics audio streams following the Google spatial audio specification:
https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md

It allows to encode, decode, rotate, zoom... **HOA Ambisonics** audio streams up to the *3rd order*. It can output to standard and custom speakers arrays. For the playback with a headset, the binauralizer applies the MIT HRTF to provide a spatial effect. The binauralization can also be used to render multichannels streams (5.1, 7.1...).

A central part of the library is the CBFormat object which acts as a buffer for B-Format. There are several other objects, each with a specific tasks, such as encoding, decoding, and processing for Ambisonics. All of these objects handle CBFormat objects at some point.

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
myEncoder.Create(1, true, 0);

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
myDecoder.Create(1, true, kAmblib_50, 5);

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

## Features
### Encoder (CAmbisonicEncoder):
Simple encoder up to 3rd order 3D, without any distance cues

### Encoder with distance (CAmbisonicEncoderDist):
As the simple encoder, but with the addition of the following:
* Distance level-simulation
* Fractional delay lines
* Interior effect (W-Panning)

### Decoder (CAmbisonicDecoder):
Simple decoder up to the 3rd Order 3D with:
* Preset & custom speaker arrays
* Filters that improves the rendering with a 5.1 speaker set

### Processor (CAmbisonicProcessor):
Up to 3rd order 3D yaw/roll/pitch of the soundfield

### Binauralizer (CAmbisonicBinauralizer):
Up to 3rd order 3D decoding to headphones

### Zoomer (CAmbisonicZoomer):
Up to 1st order 3D front-back dominance control of the soundfield

