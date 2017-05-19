#ifndef MIT_HRTF_H
#define MIT_HRTF_H

#include <hrtf.h>


class MIT_HRTF : public HRTF
{
public:
    MIT_HRTF(unsigned i_sampleRate, bool b_diffused);
    bool get(float f_azimuth, float f_elevation, float **pfHRTF);

private:
    bool b_diffused;
};

#endif // MIT_HRTF_H
