ambisonic-lib - Ambisonic C Library
=============

A library for Ambisonic encoding, decoding, rotating, zooming, and other processing, up to 3rd order. It takes an object oriented approach.

## What is this?

This is a C++ library for Ambisonic encoding, decoding, and other processing. It takes an object oriented approach.

A central part of the library is the CBFormat object which acts as a buffer for B-Format. There are several other objects, each with a specific tasks, such as encoding, decoding, and processing for Ambisonics. All of these objects handle CBFormat objects at some point.

## How do I use it?

The following sample code shows the encoding of a sine wave into an Ambisonic soundfield, and then decoding that soundfield over a Quad speaker setup.

```c
// Generation of mono test signal
float sinewave[512];
for(int ni = 0; ni < 512; ni++)
{    sinewave[ni] = (float)sin((ni / 128.f) * (M_PI * 2));
}

// CBFormat as 1st order 3D, and 512 samples
CBFormat myBFormat;

// Ambisonic encoder, also 3rd order 3D
CAmbisonicEncoder myEncoder;

//Set test signal's position in the soundfield
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

// Allocate buffers for speaker feeds
float** ppfSpeakerFeeds = new float*[5];
for(int niSpeaker = 0; niSpeaker < 5; niSpeaker++)
{
    ppfSpeakerFeeds[niSpeaker] = new float[512];
}
	
// Decode to get the speaker feeds
myDecoder.Process(&myBFormat, 512, ppfSpeakerFeeds);

// De-allocate speaker feed buffers
for(int niSpeaker = 0; niSpeaker < 5; niSpeaker++)
{
    delete [] ppfSpeakerFeeds[niSpeaker];
}
delete [] ppfSpeakerFeeds;
```

## Features
###Encoder (CAmbisonicEncoder):
Simple encoder up to 3rd order 3D, without any distance cues

###Encoder with distance (CAmbisonicEncoderDist):
As the simple encoder, but with the addition of the following:  
* Distance level-simulation  
* Fractional delay lines  
* Interior effect (W-Panning)

###Decoder (CAmbisonicDecoder):
 With:  
* Up to 3rd Order 3D  
* Preset & custom speaker arrays

###Processor (CAmbisonicProcessor):
Up to 2nd order 3D yaw/roll/pitch of the soundfield

###Binauralizer (CAmbisonicBinauralizer):
Up to 1st order 3D decoding to headphones

###Zoomer (CAmbisonicZoomer):
Up to 1st order 3D front-back dominance control of the soundfield