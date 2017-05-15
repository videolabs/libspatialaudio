
#include <iostream>
#include <cstddef>

#include <fhkhrtf.h>
#include <AmbisonicCommons.h>
#include <fhkhrtfdata.h>

#define FHK_HRTF_LEN 128


bool get_FHK_HRTF(float f_azimuth, float f_elevation, unsigned i_samplerate, float **p_hrtf)
{
    float (*p_dbHRTF)[FHK_HRTF_LEN] = NULL;

    if (i_samplerate != 48000)
        return false;

    if (f_elevation == DegreesToRadians(-69.1f))
    {
        if (f_azimuth == DegreesToRadians(90.f))         p_dbHRTF = hrtf_900_m691;
        else if (f_azimuth == DegreesToRadians(-90.f))   p_dbHRTF = hrtf_m900_m691;
    }
    else if (f_elevation == DegreesToRadians(-35.3f))
    {
        if (f_azimuth == DegreesToRadians(45.f))         p_dbHRTF = hrtf_450_m353;
        else if (f_azimuth == DegreesToRadians(135.f))   p_dbHRTF = hrtf_1350_m353;
        else if (f_azimuth == DegreesToRadians(-45.f))   p_dbHRTF = hrtf_m450_m353;
        else if (f_azimuth == DegreesToRadians(-135.f))  p_dbHRTF = hrtf_m1350_m353;
    }
    else if (f_elevation == DegreesToRadians(-20.9f))
    {
        if (f_azimuth == DegreesToRadians(180.f))        p_dbHRTF = hrtf_m1800_m209;
        else if (f_azimuth == DegreesToRadians(0.f))     p_dbHRTF = hrtf_0_m209;
    }
    else if (f_elevation == DegreesToRadians(0.f))
    {
        if (f_azimuth == DegreesToRadians(69.1f))        p_dbHRTF = hrtf_691_0;
        else if (f_azimuth == DegreesToRadians(110.9f))  p_dbHRTF = hrtf_1109_0;
        else if (f_azimuth == DegreesToRadians(-69.1f))  p_dbHRTF = hrtf_m691_0;
        else if (f_azimuth == DegreesToRadians(-110.9f)) p_dbHRTF = hrtf_m1109_0;
    }
    else if (f_elevation == DegreesToRadians(20.9f))
    {
        if (f_azimuth == DegreesToRadians(180.f))        p_dbHRTF = hrtf_m1800_209;
        else if (f_azimuth == DegreesToRadians(0.f))     p_dbHRTF = hrtf_0_209;
    }
    else if (f_elevation == DegreesToRadians(35.3f))
    {
        if (f_azimuth == DegreesToRadians(45.f))         p_dbHRTF = hrtf_450_353;
        else if (f_azimuth == DegreesToRadians(135.f))   p_dbHRTF = hrtf_1350_353;
        else if (f_azimuth == DegreesToRadians(-45.f))   p_dbHRTF = hrtf_m450_353;
        else if (f_azimuth == DegreesToRadians(-135.f))  p_dbHRTF = hrtf_m1350_353;
    }
    else if (f_elevation == DegreesToRadians(69.1f))
    {
        if (f_azimuth == DegreesToRadians(90.f))         p_dbHRTF = hrtf_900_691;
        else if (f_azimuth == DegreesToRadians(-90.f))   p_dbHRTF = hrtf_m900_691;
    }

    if (p_dbHRTF == NULL)
        return false;

    memcpy(p_hrtf[0], p_dbHRTF[0], FHK_HRTF_LEN * sizeof(float));
    memcpy(p_hrtf[1], p_dbHRTF[1], FHK_HRTF_LEN * sizeof(float));

    return true;
}
