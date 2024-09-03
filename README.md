# Spatial audio encoding / decoding and binauralization library

**libspatialaudio** is an open-source and cross-platform C++ library for spatial audio encoding and decoding, filtering and **binaural** rendering.
It is able to render *High-Order Ambisonic* (**HOA**) and *VR/3D audio* samples in multiple environments, from headphones to classic loudspeakers.
Its *binaural rendering* can be used for classical 5.1/7.1 spatial channels as well as Ambisonics inputs.
**libspatialaudio** also includes a renderer for the Audio Definition Model (ADM) that outputs to a number of ITU loudspeaker layouts as well as binaural.

Originally it is a fork of **ambisonic-lib** from Aristotel Digenis. In 2017 this version was developed to support Higher Order Ambisonics *HOA* and to support **ACN/SN3D** Ambisonics audio streams following the **Google spatial audio** specification:
https://github.com/google/spatial-media/blob/master/docs/spatial-audio-rfc.md and the **IETF codec Ambisonics** specification https://tools.ietf.org/html/draft-ietf-codec-ambisonics

It has since been expanded to include an ADM renderer based on Rec. ITU-R BS.2127-1.

The library allows you to encode, decode, rotate, zoom **HOA Ambisonics** audio streams up to the *3rd order*. It can output to standard and custom loudspeakers arrays. To playback with headphones, the binauralizer applies an HRTF *(either a SOFA file or  the included MIT HRTF)* to provide a spatial binaural rendering effect. The binauralization can also be used to render multichannel streams (5.1, 7.1...).

## Features

The library has three main uses:

- **Ambisonics**: encoding, rotation, zooming, psychoacoustic optimisation and decoding of HOA signals is provided. These function up to 3rd order Ambisonics. For more details on the Ambisonics capabilities of **libspatialaudio** please go [here](docs/AmbisonicsOverview.md).
- **Loudspeaker Binauralization**: convert loudspeaker signals to binaural, with the option to use custom HRTFs loaded via a SOFA file. Read more on how to binauralise loudspeaker signals [here](docs/LoudspeakerBinauralization.md).
- **ADM Renderer**: render Object, DirectSpeaker, HOA and Binaural streams using their metadata based on Rec. ITU-R BS.2127-1. For a guide on how to use the ADM renderer see [here](docs/AdmRendererOverview.md).

## Learning About libspatialaudio

The documentation includes elements of background theory (mathematical, psychoacoustic, DSP), implementation details (layout out the choices made while writing the library) and code examples (overviews of the main classes and code examples).

All background theory sections have been written such that it should be accessible to anyone with some mathematical background.
However, even without a mathematical background you should still be able to follow what is going on.

To learn more about the Ambisonics processors start [here](docs/AmbisonicsOverview.md).

To learn more about the ADM renderer start [here](docs/AdmRendererOverview.md).

These two documents provide full examples of both signal flows, as well as links to more details for those who want to learn more or get more specific details.

For those who do not need to know the background theory, most documentation pages contain a link at the top to take you directly to details of the code, as well as an example of how to implement it.

## Building libspatialaudio

To build **libspatialaudio**, follow these steps:

1. Clone the repository (if you haven't already):

    ```bash
    git clone https://github.com/videolabs/libspatialaudio.git
    cd libspatialaudio
    ```

2. Create a `build` directory and navigate into it:

    ```bash
    mkdir build
    cd build
    ```

3. Run CMake to configure the project:

    ```bash
    cmake ..
    ```

4. Build the project using Make:

    ```bash
    make
    ```

## License

libspatialaudio is released under LGPLv2.1 (or later) and is also available
under a commercial license. For full details see [here](LICENSE).
