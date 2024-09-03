# ADM Renderer Overview

If you are not interested in the details and just want to get coding, see the [code example](#code-example).

## Overview

`libspatialaudio` includes a renderer for the Audio Definition Model (ADM) renderer detailed in Rec. ITU-R BS.2127-1 [[1]](#ref1). It also aims to support elements of the AOM's Immersive Audio Model and Formats (IAMF) specification [[2]](#ref2).

The ADM renderer specifies rendering methods for Objects, HOA, and DirectSpeaker streams:

- Object streams consist of a mono audio signal and metadata which specify how they are spatialised. More details can be read [here](AdmObject.md).
- HOA streams are summed and decoded to the target playback format (loudspeakers or binaural). More details and an example can be found [here](AdmAmbisonics.md).
- DirectSpeaker streams are intended to be sent directly to a specific loudspeaker in the target layout, if it is present.
If it is not present then the DirectSpeaker stream will be spatialised based on the target layout and its associated metadata. More details can be read [here](AdmDirectSpeaker.md).

Rec. ITU-R BS.2127-1 [[1]](#ref1) should be consulted for full details about the rendering algorithms.

### Supported Output Formats

'AdmRenderer' supports all of the loudspeaker layouts specified in [[1]](#ref1) as well as additional ones specified in the IAMF. 

The supported ITU layouts are:

- `"0+2+0"`: BS.2051-3 System A (Stereo)
- `"0+5+0"`: BS.2051-3 System B (5.1)
- `"2+5+0"`: BS.2051-3 System C (5.1.2)
- `"4+5+0"`: BS.2051-3 System D (5.1.4)
- `"4+5+1"`: BS.2051-3 System E
- `"3+7+0"`: BS.2051-3 System F
- `"4+9+0"`: BS.2051-3 System G
- `"9+10+3"`: BS.2051-3 System H
- `"0+7+0"`: BS.2051-3 System I (7.1)
- `"4+7+0"`: BS.2051-3 System J (7.1.4)

The additional IAMF layouts are: 

- `"2+7+0"`: 7.1.2 (IAMF v1.0.0-errata)
- `"2+3+0"`: 3.1.2 (IAMF v1.0.0-errata)

A quad layout is also supported, along with the virtual layout specified in [[3]](#ref3):

- `"0+4+0"`: Quad
- `"9+10+5"`: EBU Tech 3369 (BEAR) 9+10+5 - 9+10+3 with LFE1 & LFE2 removed and B+135 & B-135 added. This is used internally for the binaural rendering.

In addition to these loudspeaker layouts, `CAdmRenderer` implements a custom binaural renderer that is inspired by the BEAR renderer specified in [[3]](#ref3). Full details on the binaural rendering processing can be read [here](AdmBinaural.md).

## CAdmRenderer

The `CAdmRenderer` is the only class you will need to interact with to render an ADM stream. It accepts audio streams and metadata to produce a signal rendered to the specified output format.

Use the class as follows:

1. Configure the class by informing it of the different streams it should expect.
2. Add each stream using the appropriate function i.e. `AddHOA()` for an Ambisonics signal.
3. Once all streams have been added use `GetRenderedAudio()`. This clears any internal buffers containing waiting audio.
4. Repeat steps 2 and 3.

### Configuration

Before calling any other functions the object must first be configured by calling `Configure()` with the appropriate values. If the values are supported then the it will return `true` and the object can now be used.

The configuration parameters are:

- **outputTarget**: The target output layout selected from `OutputLayout`.
- **hoaOrder**: The ambisonic order of the signal to be rendered. This also control the order of the HOA processing used for binaural output, so higher should be preferred.
- **nSampleRate**: Sample rate of the streams to be processed.
- **nSamples**: The maximum number of samples in an audio frame to be added to the stream.
- **channelInfo**: Information about the expected stream formats.
- **HRTFPath**: Path to an HRTF SOFA-file to be used when the output layout is binaural.
- **reproductionScreen**: (Optional) Reproduction screen details used for screen scaling/locking.
- **layoutPositions**: (Optional) Real polar positions for each of the loudspeaker in the layout. This is used if they do not exactly match the ITU specification. Note that they must be within the range allowed by the specification.

### AddObject

`AddObject()` should be called for every Object stream to be rendered. Its corresponding `ObjectMetadata` should be supplied with it.

This function can be called multiple times for each Object each time `GetRenderedAudio()` is called by using the offset parameter.
However, care must be taken to ensure that each new call would not lead to the total number of samples added that frame going beyond the value nSamples set in `Configure()`.

The inputs are:

- **pIn** Pointer to the mono object buffer to be rendered.
- **nSamples** Number of samples in the stream.
- **metadata** Metadata for the object stream.
- **nOffset** (Optional) Number of samples of delay to applied to the signal.

### AddHOA

`AddHOA()` should be called for every HOA stream to be rendered. Its corresponding `HoaMetadata` should be supplied with it.

This function can be called multiple times for each Object each time `GetRenderedAudio()` is called by using the offset parameter.
However, care must be taken to ensure that each new call would not lead to the total number of samples added that frame going beyond the value nSamples set in `Configure()`.

The inputs are:

- **pHoaIn** The HOA audio buffers to be rendered of size nAmbiCh x nSamples
- **nSamples** Number of samples in the stream.
- **metadata** Metadata for the HOA stream.
- **nOffset** (Optional) Number of samples of delay to applied to the signal.

### AddDirectSpeaker

`AddDirectSpeaker()` should be called for every DirectSpeaker stream to be rendered. Its corresponding `DirectSpeakerMetadata` should be supplied with it.

This function can be called multiple times for each Object each time `GetRenderedAudio()` is called by using the offset parameter.
However, care must be taken to ensure that each new call would not lead to the total number of samples added that frame going beyond the value nSamples set in `Configure()`.

The inputs are:

- **pIn** Pointer to the mono DirectSpeaker buffer to be rendered.
- **nSamples** Number of samples in the stream.
- **metadata** Metadata for the DirectSpeaker stream.
- **nOffset** (Optional) Number of samples of delay to applied to the signal.

### AddBinaural

`AddBinaural()` should be called for every Binaural stream to be rendered. If the output format is not set to binaural then any audio added here is discarded.

This function can be called multiple times for each Object each time `GetRenderedAudio()` is called by using the offset parameter.
However, care must be taken to ensure that each new call would not lead to the total number of samples added that frame going beyond the value nSamples set in `Configure()`.

The inputs are:

- **pHoaIn** The binaural audio buffers to be rendered of size 2 x nSamples
- **nSamples** Number of samples in the stream.
- **nOffset** (Optional) Number of samples of delay to applied to the signal.

### GetRenderedAudio

Get the rendered audio using `GetRenderedAudio()`.
After it has been called all internal buffers are reset.
Therefore, it is very important that this function only be called after all streams have been added.

- **pRender** Rendered audio output.
- **nSamples** The number of samples to get.

### Code Example

This example renders an Object, HOA and DirectSpeaker stream to binaural.
For each of the stream types the metadata is generated along with an audio stream.
The `StreamInformation` used to configure `CAdmRenderer` is generated when the metadata is "read".
`StreamInformation` keeps track of the total number of tracks as well as what type they have.
Note that when the metadata is being generated the track indices must match the ordering with which the tracks are added to the `StreamInformation`.

In this example an Object rotates around the listener (left, back, right and again to front), the HOA stream is encoded with a source at 90&deg; and the DirectSpeaker corresponds to the right-wide loudspeaker placed at -60&deg;. Replace the streams with your own mono audio to hear the motion clearer.

```c++
const unsigned int sampleRate = 48000;
const int nBlockLength = 512;
const int nBlocks = 470;
const int nSigSamples = nBlocks * nBlockLength; // Approximately 5 seconds @ 48 kHz

// Ambisonic order
const unsigned int nOrder = 3;

// Prepare the stream to hold the rendered audio (binaural)
const unsigned int nLdspk = 2;
float** renderStream = new float* [nLdspk];
for (unsigned int i = 0; i < nLdspk; ++i)
{
    renderStream[i] = new float[nSigSamples];
}

// Track index for each channel in the streams
unsigned int trackInd = 0;
// The stream information used to configure CAdmRenderer
admrender::StreamInformation streamInfo;

// Add an object stream ===============================================================================================
// Prepare the metadata for the stream
admrender::ObjectMetadata objectMetadata;
objectMetadata.trackInd = trackInd++;
objectMetadata.blockLength = nBlockLength;
objectMetadata.cartesian = false;

// Channel lock (off by default)
objectMetadata.channelLock = admrender::ChannelLock();
objectMetadata.channelLock->maxDistance = 0.f; // <- Increase this to see the signal snap to fully in the left loudspeaker

// Object divergence (off by default)
objectMetadata.objectDivergence = admrender::ObjectDivergence();
objectMetadata.objectDivergence->azimuthRange = 30.; // <- Controls the width of the divergence
objectMetadata.objectDivergence->value = 0.; // <- Increase this to apply object divergence

// Object extent (off by default)
objectMetadata.width = 0.; // <- Increase this to increase the width and spread the Object over more adjacent loudspeakers

// Configure the stream information. In this case there is only a single channel stream
streamInfo.nChannels++;
streamInfo.typeDefinition.push_back(admrender::TypeDefinition::Objects);

std::vector<float> objectStream(nSigSamples);
// Fill the vector with a sine wave
for (int i = 0; i < nSigSamples; ++i)
    objectStream[i] = (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate);

// Add an HOA stream ===============================================================================================
const unsigned int nHoaCh = OrderToComponents(nOrder, true);
// Set the fade time to the length of one block
const float fadeTimeInMilliSec = 0.f;

std::vector<float> hoaSinewave(nSigSamples);
// Fill the vector with a sine wave
for (int i = 0; i < nSigSamples; ++i)
    hoaSinewave[i] = (float)std::sin((float)M_PI * 2.f * 700.f * (float)i / (float)sampleRate);

// Destination B-format buffer
CBFormat myBFormat;
myBFormat.Configure(nOrder, true, nSigSamples);
myBFormat.Reset();

// Set up and configure the Ambisonics encoder
CAmbisonicEncoder myEncoder;
myEncoder.Configure(nOrder, true, sampleRate, fadeTimeInMilliSec);

// Set test signal's initial direction in the sound field
PolarPoint position;
position.fAzimuth = 0.5f * (float)M_PI;
position.fElevation = 0;
position.fDistance = 1.f;
myEncoder.SetPosition(position);
myEncoder.Reset();
myEncoder.Process(hoaSinewave.data(), nSigSamples, &myBFormat);

// Prepare the stream to be processed by the renderer
float** hoaStream = new float* [nHoaCh];
for (unsigned int i = 0; i < nHoaCh; ++i)
{
    hoaStream[i] = new float[nSigSamples];
    myBFormat.ExtractStream(&hoaStream[i][0], i, nSigSamples);
}

// Prepare the metadata for the stream
admrender::HoaMetadata hoaMetadata;
hoaMetadata.normalization = "SN3D";

// Configure the stream information. In this case there is only a single HOA stream of (nOrder + 1)^2 channels
for (unsigned int i = 0; i < nHoaCh; ++i)
{
    streamInfo.nChannels++;
    streamInfo.typeDefinition.push_back(admrender::TypeDefinition::HOA);

    // Set the metadata for each channel
    int order, degree;
    ComponentToOrderAndDegree(i, true, order, degree);
    hoaMetadata.degrees.push_back(degree);
    hoaMetadata.orders.push_back(order);
    hoaMetadata.trackInds.push_back(trackInd++);
}

// Add a DirectSpeaker stream ===============================================================================================
std::vector<float> speakerSinewave(nSigSamples);
// Fill the vector with a sine wave
for (int i = 0; i < nSigSamples; ++i)
    speakerSinewave[i] = (float)std::sin((float)M_PI * 2.f * 185.f * (float)i / (float)sampleRate);

// Prepare the metadata for the stream
admrender::DirectSpeakerMetadata directSpeakerMetadata;
directSpeakerMetadata.trackInd = trackInd++;
directSpeakerMetadata.speakerLabel = "M-060"; // Wide-right speaker
directSpeakerMetadata.audioPackFormatID.push_back("AP_00010009"); // 9+10+3 layout

// Configure the stream information. In this case there is only a single channel stream
streamInfo.nChannels++;
streamInfo.typeDefinition.push_back(admrender::TypeDefinition::DirectSpeakers);

// Render the stream ===============================================================================================
// Set up the ADM renderer
admrender::CAdmRenderer admRender;
admRender.Configure(admrender::OutputLayout::Binaural, nOrder, sampleRate, nBlockLength, streamInfo);

float** renderBlock = new float* [2];
float** hoaBlock = new float* [nHoaCh];

for (int iBlock = 0; iBlock < nBlocks; ++iBlock)
{
    const unsigned int iSamp = iBlock * nBlockLength;

    // Get "new" Object metadata
    objectMetadata.position.polarPosition().azimuth = (double)iBlock / (double)nBlocks * 360.;
    // Add the Object stream to be rendered
    admRender.AddObject(&objectStream[iSamp], nBlockLength, objectMetadata);

    // Add the HOA stream to be rendered
    for (unsigned int i = 0; i < nHoaCh; ++i)
        hoaBlock[i] = &hoaStream[i][iSamp];
    admRender.AddHoa(hoaBlock, nBlockLength, hoaMetadata);

    // Add the DirectSpeaker stream to be rendered
    admRender.AddDirectSpeaker(&speakerSinewave[iSamp], nBlockLength, directSpeakerMetadata);

    // Render the stream
    renderBlock[0] = &renderStream[0][iSamp];
    renderBlock[1] = &renderStream[1][iSamp];
    admRender.GetRenderedAudio(renderBlock, nBlockLength);
}

// Cleanup
for (unsigned i = 0; i < nHoaCh; ++i)
    delete hoaStream[i];
delete[] hoaStream;
for (unsigned i = 0; i < nLdspk; ++i)
    delete renderStream[i];
delete[] renderStream;
delete[] renderBlock;
delete[] hoaBlock;
```

## References

<a name="ref1">[1]</a> International Telecommunication Union. Audio Definition Model renderer for advanced sound systems. International Telecommunication Union, Geneva, Switzerland, Recommendation ITU-R BS.2127-1 edition, 2023. URL https://www.itu.int/rec/r-rec-bs.2127/en.

<a name="ref2">[2]</a> Alliance for Open Media. Interactive Audio Metadata Format (IAMF) Version 1.0.0 - Errata, 2024. URL: https://aomediacodec.github.io/iamf/v1.0.0-errata.html. Accessed: 2024-09-04.

<a name="ref3">[3]</a> European Broadcasting Union. EBU Tech 3369: Binaral EBU ADM Renderer (BEAR) for Object-Based Sound Over Headphones, Geneva, Switzerland, 2023. URL https://tech.ebu.ch/publications/tech3396.
