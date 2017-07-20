#include "config.h"

#ifdef HAVE_MIT_HRTF

#include <AmbisonicCommons.h>

#include <mit_hrtf.h>
#include <mit_hrtf_lib.h>


MIT_HRTF::MIT_HRTF(unsigned i_sampleRate)
    : HRTF(i_sampleRate)
{
    i_len = mit_hrtf_availability(0, 0, i_sampleRate);
}


bool MIT_HRTF::get(float f_azimuth, float f_elevation, float** pfHRTF)
{
    int nAzimuth = (int)RadiansToDegrees(-f_azimuth);
    if(nAzimuth > 180)
        nAzimuth -= 360;
    int nElevation = (int)RadiansToDegrees(f_elevation);
    //Get HRTFs for given position
    short psHRTF[2][i_len];
    unsigned ret = mit_hrtf_get(&nAzimuth, &nElevation, i_sampleRate, psHRTF[0], psHRTF[1]);
    if (ret == 0)
        return false;

    //Convert from short to float representation
    for (unsigned t = 0; t < i_len; t++)
    {
        pfHRTF[0][t] = psHRTF[0][t] / 32767.f;
        pfHRTF[1][t] = psHRTF[1][t] / 32767.f;
    }

    return true;
}

#endif
