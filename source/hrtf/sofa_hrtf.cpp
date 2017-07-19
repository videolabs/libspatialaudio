
#include <iostream>

#include <sofa_hrtf.h>

#ifdef HAVE_MYSOFA

#include <cmath>
#include <AmbisonicCommons.h>


SOFA_HRTF::SOFA_HRTF(std::string path, unsigned i_sampleRate)
    : HRTF(i_sampleRate), hrtf(nullptr)
{
    int err;

    hrtf = mysofa_open(path.c_str(), i_sampleRate, &i_internalLength, &err);
    if (hrtf == nullptr)
    {
        std::cout << "Could not load the SOFA HRTF." << std::endl;
        return;
    }

    i_filterExtraLength = i_internalLength / 2;
    i_len = i_internalLength + i_filterExtraLength;
}


SOFA_HRTF::~SOFA_HRTF()
{
    if (hrtf != nullptr)
        mysofa_close(hrtf);
}


bool SOFA_HRTF::get(float f_azimuth, float f_elevation, float** pfHRTF)
{
    float delaysSec[2]; // unit is second.
    unsigned delaysSamples[2]; // unit is samples.
    float pfHRTFNotDelayed[2][i_internalLength] = {0};

    float p[3] = {RadiansToDegrees(f_azimuth), RadiansToDegrees(f_elevation), 1.f};
    mysofa_s2c(p);

    mysofa_getfilter_float(hrtf, p[0], p[1], p[2],
        pfHRTFNotDelayed[0], pfHRTFNotDelayed[1], &delaysSec[0], &delaysSec[1]);
    delaysSamples[0] = std::roundf(delaysSec[0] * i_sampleRate);
    delaysSamples[1] = std::roundf(delaysSec[1] * i_sampleRate);

    if (delaysSamples[0] > i_filterExtraLength
        || delaysSamples[1] > i_filterExtraLength)
    {
        std::cout << "Too big HRTF delay for the buffer length." << std::endl;
        return false;
    }

    std::fill(pfHRTF[0], pfHRTF[0] + i_len, 0);
    std::fill(pfHRTF[1], pfHRTF[1] + i_len, 0);

    std::copy(pfHRTFNotDelayed[0], pfHRTFNotDelayed[0] + i_internalLength, pfHRTF[0] + delaysSamples[0]);
    std::copy(pfHRTFNotDelayed[1], pfHRTFNotDelayed[1] + i_internalLength, pfHRTF[1] + delaysSamples[1]);

    return true;
}

#endif
