#ifndef SOFA_HRTF_H
#define SOFA_HRTF_H

#include "config.h"

#ifdef HAVE_MYSOFA

#include <string>

#include <mysofa.h>

#include "hrtf.h"


class SOFA_HRTF : public HRTF
{
public:
    SOFA_HRTF(std::string path, unsigned i_sampleRate);
    ~SOFA_HRTF();
    bool get(float f_azimuth, float f_elevation, float **pfHRTF);

private:
    struct MYSOFA_EASY *hrtf;

    unsigned i_filterExtraLength;
    int i_internalLength;
};

#endif

#endif // SOFA_HRTF_H
