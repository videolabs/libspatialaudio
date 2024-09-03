# ADM Ambisonics Rendering

If you are not interested in the details and just want to get coding, see the [code example](#code-example).

## Introduction

`AdmRenderer` allows for Higher Order Ambisonics (HOA) streams to be decoded to any of the specified loudspeaker layouts (see [here](AdmRendererOverview.md#supported-output-formats)).
It supports decoding up to a maximum of order 3.
Decoding is performed using the AllRAD method, using the`AmbisonicAllRAD` class internally. See [here](AmbisonicDecoding.md#cambisonicallrad) for more details about the AllRAD method.

The HOA stream has the "order" and "degree" of each channel defined in its metadata which is used to sort the channels of the stream.
`AdmRenderer` uses the ACN channel sorting internally.

The normalisation of the orders can be SN3D, N3D or FuMa.
The signals are converted to SN3D, if they are not already.

Converting to ACN channel sorting and SN3D normalisation ensures the signal is in the AmbiX format, ensuring the signals are compatible with the `libspatialaudio` Ambisonics processors.

Full details can be found in section 9 of Rec. ITU-R BS.2127-1.

## Code Example

In this example an HOA signal is decoded to binaural. The AmbiX (SN3D/ACN) HOA stream and its metadata are generated as part of the example but would normally be received from the stream to be rendered.

The HOA stream is then added to the renderer using `AddHOA()`, which adds it to an internal buffer. `AddHOA()` can be called multiple times if there are multiple HOA streams to be rendered. When `GetRenderedAudio()` is called the accumulated HOA buffer is decoded to the specified output format. The internal HOA buffer is then cleared so that new streams can be added to the next frame to be rendered.

For a more complete example with multiple different stream types being rendered see [here](AdmRendererOverview.md#code-example).

```c++
#include "AdmRenderer.h"

const unsigned int sampleRate = 48000;
const int nBlockLength = 512;

// Higher ambisonic order means higher spatial resolution and more channels required
const unsigned int nOrder = 1;
const unsigned int nHoaCh = OrderToComponents(nOrder, true);
// Set the fade time to the length of one block
const float fadeTimeInMilliSec = 1000.f * (float)nBlockLength / (float)sampleRate;

std::vector<float> sinewave(nBlockLength);
// Fill the vector with a sine wave
for (int i = 0; i < nBlockLength; ++i)
    sinewave[i] = (float)std::sin((float)M_PI * 2.f * 440.f * (float)i / (float)sampleRate);

// Destination B-format buffer
CBFormat myBFormat;
myBFormat.Configure(nOrder, true, nBlockLength);
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
myEncoder.Process(sinewave.data(), nBlockLength, &myBFormat);

// Prepare the stream to be processed by the renderer
float** hoaStream = new float* [nHoaCh];
for (unsigned int i = 0; i < nHoaCh; ++i)
{
    hoaStream[i] = new float[nBlockLength];
    myBFormat.ExtractStream(hoaStream[i], i, nBlockLength);
}

// Prepare the stream to hold the rendered audio (binaural)
float** renderStream = new float* [2];
for (unsigned int i = 0; i < 2; ++i)
{
    renderStream[i] = new float[nBlockLength];
}

// Prepare the metadata for the stream
admrender::HoaMetadata hoaMetadata;
hoaMetadata.normalization = "SN3D";

// Configure the stream information. In this case there is only a single HOA stream of (nOrder + 1)^2 channels
admrender::StreamInformation streamInfo;
for (unsigned int i = 0; i < nHoaCh; ++i)
{
    streamInfo.nChannels++;
    streamInfo.typeDefinition.push_back(admrender::TypeDefinition::HOA);

    // Set the metadata for each channel
    int order, degree;
    ComponentToOrderAndDegree(i, true, order, degree);
    hoaMetadata.degrees.push_back(degree);
    hoaMetadata.orders.push_back(order);
    hoaMetadata.trackInds.push_back(i);
}

// Set up the ADM renderer
admrender::CAdmRenderer admRender;
admRender.Configure(admrender::OutputLayout::Binaural, nOrder, sampleRate, nBlockLength, streamInfo);

// Add the HOA stream to be rendered
admRender.AddHoa(hoaStream, nBlockLength, hoaMetadata);

// Render the stream
admRender.GetRenderedAudio(renderStream, nBlockLength);

// Cleanup
for (unsigned i = 0; i < nHoaCh; ++i)
    delete hoaStream[i];
delete[] hoaStream;
for (unsigned i = 0; i < 2; ++i)
    delete renderStream[i];
delete[] renderStream;
```
