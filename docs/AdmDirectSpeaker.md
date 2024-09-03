# ADM DirectSpeaker Rendering

If you are not interested in the details and just want to get coding, see the [code example](#code-example).

## Introduction

A mono audio stream with DirectSpeaker metadata is spatialised by sending it directly to the specified loudspeaker i.e. a DirectSpeaker stream labelled as belonging to the left loudspeaker will be sent to it directly.
If the specified loudspeaker is not present in the reproduction layout then it checks a set of MappingRules.
In the case a suitable mapping rule exists, it will be used to calculate the gains to be applied to the stream.

As a fallback, or if no label is supplied, the position of the DirectSpeaker stream can be specified.
After screen edge locking, the position of the stream is checked and if there is a sufficiently close loudspeaker then the signal will be sent directly to it.
If no loudspeaker is close enough then the DirectSpeaker stream is panned as a point source to its specified direction.

Full details can be found in section 8 of Rec. ITU-R BS.2127-1.

## Code Example

In this example a DirectSpeaker signal for the wide-left (M+060) loudspeaker from a 9+10+3 layout is rendered to a 5.1 layout. The stream and its metadata are generated as part of the example but would normally be received from the stream to be rendered.

The stream is first added to be rendered using `AddDirectSpeaker()`. When `GetRenderedAudio()` is called the stream is converted (in this case) to binaural and the DirectSpeaker buffer internally is cleared, ready for the next `AddDirectSpeaker()` call.

```c++
#include "AdmRenderer.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;

// Ambisonic order (not used in this example but expected in Configure())
const unsigned int nOrder = 1;

std::vector<float> sinewave(nBlockLength);
// Fill the vector with a sine wave
for (int i = 0; i < nBlockLength; ++i)
    sinewave[i] = (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate);

// Prepare the stream to hold the rendered audio (5.1)
const unsigned int nLdspk = 6;
float** renderStream = new float* [nLdspk];
for (unsigned int i = 0; i < nLdspk; ++i)
{
    renderStream[i] = new float[nBlockLength];
}

// Prepare the metadata for the stream
admrender::DirectSpeakerMetadata directSpeakerMetadata;
directSpeakerMetadata.trackInd = 0;
directSpeakerMetadata.speakerLabel = "M+060"; // Wide-left speaker
directSpeakerMetadata.audioPackFormatID.push_back("AP_00010009"); // 9+10+3 layout

// Configure the stream information. In this case there is only a single channel stream
admrender::StreamInformation streamInfo;
streamInfo.nChannels = 1;
streamInfo.typeDefinition.push_back(admrender::TypeDefinition::DirectSpeakers);

// Set up the ADM renderer
admrender::CAdmRenderer admRender;
admRender.Configure(admrender::OutputLayout::ITU_0_5_0, nOrder, sampleRate, nBlockLength, streamInfo);

// Add the DirectSpeaker stream to be rendered
admRender.AddDirectSpeaker(sinewave.data(), nBlockLength, directSpeakerMetadata);

// Render the stream
admRender.GetRenderedAudio(renderStream, nBlockLength);

// Cleanup
for (unsigned i = 0; i < nLdspk; ++i)
    delete renderStream[i];
delete[] renderStream;
```
