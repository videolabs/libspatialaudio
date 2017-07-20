/*############################################################################*/
/*#                                                                          #*/
/*#  MIT HRTF C Library                                                      #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:  mit_hrtf_lib.c                                               #*/
/*#  Version:   0.1                                                          #*/
/*#  Date:      04/05/2007                                                   #*/
/*#  Author(s): Aristotel Digenis                                            #*/
/*#  Credit:    Bill Gardner and Keith Martin                                #*/
/*#  Licence:   MIT                                                          #*/
/*#                                                                          #*/
/*############################################################################*/

#include "config.h"

#ifdef HAVE_MIT_HRTF

#include "../include/mit_hrtf_lib.h"
#include "normal/mit_hrtf_normal_44100.h"
#include "normal/mit_hrtf_normal_48000.h"
#include "normal/mit_hrtf_normal_88200.h"
#include "normal/mit_hrtf_normal_96000.h"



/*    Internal functions for handling the indexing of the -/+40 degree elevation
    data which has irregular azimuth positions. */
int mit_hrtf_findAzimuthFor40Elev(int azimuth);
int mit_hrtf_findIndexFor40Elev(int azimuth);



unsigned int mit_hrtf_availability(int azimuth, int elevation, unsigned int samplerate)
{
    if(elevation > 90 || elevation < -40)
        return 0;

    if(azimuth > 180 || azimuth < -180)
        return 0;

    if(samplerate == 44100)
        return MIT_HRTF_44_TAPS;
    else if(samplerate == 48000)
        return MIT_HRTF_48_TAPS;
    else if(samplerate == 88200)
        return MIT_HRTF_88_TAPS;
    else if(samplerate == 96000)
        return MIT_HRTF_96_TAPS;

    return 0;
}



unsigned int mit_hrtf_get(int* pAzimuth, int* pElevation, unsigned int samplerate, short* psLeft, short* psRight)
{
    int nInternalElevation = 0;
    float fAzimuthIncrement = 0;
    int nInternalAzimuth = 0;
    int nSwitchLeftRight = 0;
    int nAzimuthIndex = 0;
    const mit_hrtf_filter_set_44* pFilter44 = 0;
    const mit_hrtf_filter_set_48* pFilter48 = 0;
    const mit_hrtf_filter_set_88* pFilter88 = 0;
    const mit_hrtf_filter_set_96* pFilter96 = 0;
    const short* psLeftTaps = 0;
    const short* psRightTaps = 0;
    const short* psTempTaps = 0;
    unsigned int nTotalTaps = 0;
    unsigned int niTap = 0;

    //Check if the requested HRTF exists
    if(!mit_hrtf_availability(*pAzimuth, *pElevation, samplerate))
        return 0;

    //Snap elevation to the nearest available elevation in the filter set
    if(*pElevation < 0)
        nInternalElevation = ((*pElevation - 5) / 10) * 10;
    else
        nInternalElevation = ((*pElevation + 5) / 10) * 10;

    // Elevation of 50 has a maximum 176 in the azimuth plane so we need to handle that.
    if(nInternalElevation == 50)
    {
        if(*pAzimuth < 0)
            *pAzimuth = *pAzimuth < -176 ? -176 : *pAzimuth;
        else
            *pAzimuth = *pAzimuth > 176 ? 176 : *pAzimuth;
    }

    //Snap azimuth to the nearest available azimuth in the filter set.
    switch(nInternalElevation)
    {
    case 0:        fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_00 - 1);        break;    // 180 5
    case 10:
    case -10:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_10 - 1);        break;    // 180 5
    case 20:
    case -20:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_20 - 1);        break;    // 180 5
    case 30:
    case -30:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_30 - 1);        break;    // 180 6
    case 40:
    case -40:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_40 - 1);        break;    // 180 6.43
    case 50:    fAzimuthIncrement = 176.f / (MIT_HRTF_AZI_POSITIONS_50 - 1);        break;    // 176 8
    case 60:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_60 - 1);        break;    // 180 10
    case 70:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_70 - 1);        break;    // 180 15
    case 80:    fAzimuthIncrement = 180.f / (MIT_HRTF_AZI_POSITIONS_80 - 1);        break;    // 180 30
    case 90:    fAzimuthIncrement = 0;                                                break;    // 0   1
    };

    if(*pAzimuth < 0)
    {
        nInternalAzimuth = (int)((int)((-*pAzimuth + fAzimuthIncrement / 2.f) / fAzimuthIncrement) * fAzimuthIncrement + 0.5f);
        nSwitchLeftRight = 1;
    }
    else
    {
        nInternalAzimuth = (int)((int)((*pAzimuth + fAzimuthIncrement / 2.f) / fAzimuthIncrement) * fAzimuthIncrement + 0.5f);
    }

    //Determine array index for azimuth based on elevation
    switch(nInternalElevation)
    {
    case 0:        nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_00 - 1));    break;
    case 10:
    case -10:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_10 - 1));    break;
    case 20:
    case -20:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_20 - 1));    break;
    case 30:
    case -30:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_30 - 1));    break;
    case 40:
    case -40:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_40 - 1));    break;
    case 50:    nAzimuthIndex = (int)((nInternalAzimuth / 176.f) * (MIT_HRTF_AZI_POSITIONS_50 - 1));    break;
    case 60:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_60 - 1));    break;
    case 70:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_70 - 1));    break;
    case 80:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_80 - 1));    break;
    case 90:    nAzimuthIndex = (int)((nInternalAzimuth / 180.f) * (MIT_HRTF_AZI_POSITIONS_90 - 1));    break;
    };

    //The azimuths for +/- elevations need special handling
    if(nInternalElevation == 40 || nInternalElevation == -40)
    {
        nInternalAzimuth = mit_hrtf_findAzimuthFor40Elev(nInternalAzimuth);
        nAzimuthIndex = mit_hrtf_findIndexFor40Elev(nInternalAzimuth);
    }

    //Assign pointer to appropriate array depending on saple rate, normal filters, elevation, and azimuth index.
    switch(samplerate)
    {
    case 44100:
        pFilter44 = &normal_44;

        switch(nInternalElevation)
        {
        case -10:    psLeftTaps = pFilter44->e_10[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e_10[nAzimuthIndex].right;        break;
        case -20:    psLeftTaps = pFilter44->e_20[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e_20[nAzimuthIndex].right;        break;
        case -30:    psLeftTaps = pFilter44->e_30[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e_30[nAzimuthIndex].right;        break;
        case -40:    psLeftTaps = pFilter44->e_40[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e_40[nAzimuthIndex].right;        break;
        case 0:        psLeftTaps = pFilter44->e00[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e00[nAzimuthIndex].right;        break;
        case 10:    psLeftTaps = pFilter44->e10[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e10[nAzimuthIndex].right;        break;
        case 20:    psLeftTaps = pFilter44->e20[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e20[nAzimuthIndex].right;        break;
        case 30:    psLeftTaps = pFilter44->e30[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e30[nAzimuthIndex].right;        break;
        case 40:    psLeftTaps = pFilter44->e40[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e40[nAzimuthIndex].right;        break;
        case 50:    psLeftTaps = pFilter44->e50[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e50[nAzimuthIndex].right;        break;
        case 60:    psLeftTaps = pFilter44->e60[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e60[nAzimuthIndex].right;        break;
        case 70:    psLeftTaps = pFilter44->e70[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e70[nAzimuthIndex].right;        break;
        case 80:    psLeftTaps = pFilter44->e80[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e80[nAzimuthIndex].right;        break;
        case 90:    psLeftTaps = pFilter44->e90[nAzimuthIndex].left;
                    psRightTaps = pFilter44->e90[nAzimuthIndex].right;        break;
        };

        //How many taps to copy later to user's buffers
        nTotalTaps = MIT_HRTF_44_TAPS;
        break;
    case 48000:
        pFilter48 = &normal_48;

        switch(nInternalElevation)
        {
        case -10:    psLeftTaps = pFilter48->e_10[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e_10[nAzimuthIndex].right;        break;
        case -20:    psLeftTaps = pFilter48->e_20[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e_20[nAzimuthIndex].right;        break;
        case -30:    psLeftTaps = pFilter48->e_30[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e_30[nAzimuthIndex].right;        break;
        case -40:    psLeftTaps = pFilter48->e_40[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e_40[nAzimuthIndex].right;        break;
        case 0:        psLeftTaps = pFilter48->e00[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e00[nAzimuthIndex].right;        break;
        case 10:    psLeftTaps = pFilter48->e10[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e10[nAzimuthIndex].right;        break;
        case 20:    psLeftTaps = pFilter48->e20[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e20[nAzimuthIndex].right;        break;
        case 30:    psLeftTaps = pFilter48->e30[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e30[nAzimuthIndex].right;        break;
        case 40:    psLeftTaps = pFilter48->e40[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e40[nAzimuthIndex].right;        break;
        case 50:    psLeftTaps = pFilter48->e50[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e50[nAzimuthIndex].right;        break;
        case 60:    psLeftTaps = pFilter48->e60[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e60[nAzimuthIndex].right;        break;
        case 70:    psLeftTaps = pFilter48->e70[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e70[nAzimuthIndex].right;        break;
        case 80:    psLeftTaps = pFilter48->e80[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e80[nAzimuthIndex].right;        break;
        case 90:    psLeftTaps = pFilter48->e90[nAzimuthIndex].left;
                    psRightTaps = pFilter48->e90[nAzimuthIndex].right;        break;
        };

        //How many taps to copy later to user's buffers
        nTotalTaps = MIT_HRTF_48_TAPS;
        break;
    case 88200:
        pFilter88 = &normal_88;

        switch(nInternalElevation)
        {
        case -10:    psLeftTaps = pFilter88->e_10[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e_10[nAzimuthIndex].right;        break;
        case -20:    psLeftTaps = pFilter88->e_20[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e_20[nAzimuthIndex].right;        break;
        case -30:    psLeftTaps = pFilter88->e_30[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e_30[nAzimuthIndex].right;        break;
        case -40:    psLeftTaps = pFilter88->e_40[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e_40[nAzimuthIndex].right;        break;
        case 0:        psLeftTaps = pFilter88->e00[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e00[nAzimuthIndex].right;        break;
        case 10:    psLeftTaps = pFilter88->e10[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e10[nAzimuthIndex].right;        break;
        case 20:    psLeftTaps = pFilter88->e20[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e20[nAzimuthIndex].right;        break;
        case 30:    psLeftTaps = pFilter88->e30[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e30[nAzimuthIndex].right;        break;
        case 40:    psLeftTaps = pFilter88->e40[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e40[nAzimuthIndex].right;        break;
        case 50:    psLeftTaps = pFilter88->e50[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e50[nAzimuthIndex].right;        break;
        case 60:    psLeftTaps = pFilter88->e60[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e60[nAzimuthIndex].right;        break;
        case 70:    psLeftTaps = pFilter88->e70[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e70[nAzimuthIndex].right;        break;
        case 80:    psLeftTaps = pFilter88->e80[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e80[nAzimuthIndex].right;        break;
        case 90:    psLeftTaps = pFilter88->e90[nAzimuthIndex].left;
                    psRightTaps = pFilter88->e90[nAzimuthIndex].right;        break;
        };

        //How many taps to copy later to user's buffers
        nTotalTaps = MIT_HRTF_88_TAPS;
        break;
    case 96000:
        pFilter96 = &normal_96;

        switch(nInternalElevation)
        {
        case -10:    psLeftTaps = pFilter96->e_10[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e_10[nAzimuthIndex].right;        break;
        case -20:    psLeftTaps = pFilter96->e_20[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e_20[nAzimuthIndex].right;        break;
        case -30:    psLeftTaps = pFilter96->e_30[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e_30[nAzimuthIndex].right;        break;
        case -40:    psLeftTaps = pFilter96->e_40[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e_40[nAzimuthIndex].right;        break;
        case 0:        psLeftTaps = pFilter96->e00[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e00[nAzimuthIndex].right;        break;
        case 10:    psLeftTaps = pFilter96->e10[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e10[nAzimuthIndex].right;        break;
        case 20:    psLeftTaps = pFilter96->e20[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e20[nAzimuthIndex].right;        break;
        case 30:    psLeftTaps = pFilter96->e30[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e30[nAzimuthIndex].right;        break;
        case 40:    psLeftTaps = pFilter96->e40[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e40[nAzimuthIndex].right;        break;
        case 50:    psLeftTaps = pFilter96->e50[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e50[nAzimuthIndex].right;        break;
        case 60:    psLeftTaps = pFilter96->e60[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e60[nAzimuthIndex].right;        break;
        case 70:    psLeftTaps = pFilter96->e70[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e70[nAzimuthIndex].right;        break;
        case 80:    psLeftTaps = pFilter96->e80[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e80[nAzimuthIndex].right;        break;
        case 90:    psLeftTaps = pFilter96->e90[nAzimuthIndex].left;
                    psRightTaps = pFilter96->e90[nAzimuthIndex].right;        break;
        };

        //How many taps to copy later to user's buffers
        nTotalTaps = MIT_HRTF_96_TAPS;
        break;
    };

    //Switch left and right ear if the azimuth is to the left of front centre (azimuth < 0)
    if(nSwitchLeftRight)
    {
        psTempTaps = psRightTaps;
        psRightTaps = psLeftTaps;
        psLeftTaps = psTempTaps;
    }

    //Copy taps to user's arrays
    for(niTap = 0; niTap < nTotalTaps; niTap++)
    {
        psLeft[niTap] = psLeftTaps[niTap];
        psRight[niTap] = psRightTaps[niTap];
    }

    //Assign the real azimuth and elevation used
    *pAzimuth = nInternalAzimuth;
    *pElevation = nInternalElevation;

    return nTotalTaps;
}



int mit_hrtf_findAzimuthFor40Elev(int azimuth)
{
    if(azimuth >= 0 && azimuth < 4)
        return 0;
    else if(azimuth >= 4 && azimuth < 10)
        return 6;
    else if(azimuth >= 10 && azimuth < 17)
        return 13;
    else if(azimuth >= 17 && azimuth < 23)
        return 19;
    else if(azimuth >= 23 && azimuth < 30)
        return 26;
    else if(azimuth >= 30 && azimuth < 36)
        return 32;
    else if(azimuth >= 36 && azimuth < 43)
        return 39;
    else if(azimuth >= 43 && azimuth < 49)
        return 45;
    else if(azimuth >= 49 && azimuth < 55)
        return 51;
    else if(azimuth >= 55 && azimuth < 62)
        return 58;
    else if(azimuth >= 62 && azimuth < 68)
        return 64;
    else if(azimuth >= 68 && azimuth < 75)
        return 71;
    else if(azimuth >= 75 && azimuth < 81)
        return 77;
    else if(azimuth >= 81 && azimuth < 88)
        return 84;
    else if(azimuth >= 88 && azimuth < 94)
        return 90;
    else if(azimuth >= 94 && azimuth < 100)
        return 96;
    else if(azimuth >= 100 && azimuth < 107)
        return 103;
    else if(azimuth >= 107 && azimuth < 113)
        return 109;
    else if(azimuth >= 113 && azimuth < 120)
        return 116;
    else if(azimuth >= 120 && azimuth < 126)
        return 122;
    else if(azimuth >= 126 && azimuth < 133)
        return 129;
    else if(azimuth >= 133 && azimuth < 139)
        return 135;
    else if(azimuth >= 139 && azimuth < 145)
        return 141;
    else if(azimuth >= 145 && azimuth < 152)
        return 148;
    else if(azimuth >= 152 && azimuth < 158)
        return 154;
    else if(azimuth >= 158 && azimuth < 165)
        return 161;
    else if(azimuth >= 165 && azimuth < 171)
        return 167;
    else if(azimuth >= 171 && azimuth < 178)
        return 174;
    else
        return 180;
};



int mit_hrtf_findIndexFor40Elev(int azimuth)
{
    if(azimuth >= 0 && azimuth < 4)
        return 0;
    else if(azimuth >= 4 && azimuth < 10)
        return 1;
    else if(azimuth >= 10 && azimuth < 17)
        return 2;
    else if(azimuth >= 17 && azimuth < 23)
        return 3;
    else if(azimuth >= 23 && azimuth < 30)
        return 4;
    else if(azimuth >= 30 && azimuth < 36)
        return 5;
    else if(azimuth >= 36 && azimuth < 43)
        return 6;
    else if(azimuth >= 43 && azimuth < 49)
        return 7;
    else if(azimuth >= 49 && azimuth < 55)
        return 8;
    else if(azimuth >= 55 && azimuth < 62)
        return 9;
    else if(azimuth >= 62 && azimuth < 68)
        return 10;
    else if(azimuth >= 68 && azimuth < 75)
        return 11;
    else if(azimuth >= 75 && azimuth < 81)
        return 12;
    else if(azimuth >= 81 && azimuth < 88)
        return 13;
    else if(azimuth >= 88 && azimuth < 94)
        return 14;
    else if(azimuth >= 94 && azimuth < 100)
        return 15;
    else if(azimuth >= 100 && azimuth < 107)
        return 16;
    else if(azimuth >= 107 && azimuth < 113)
        return 17;
    else if(azimuth >= 113 && azimuth < 120)
        return 18;
    else if(azimuth >= 120 && azimuth < 126)
        return 19;
    else if(azimuth >= 126 && azimuth < 133)
        return 20;
    else if(azimuth >= 133 && azimuth < 139)
        return 21;
    else if(azimuth >= 139 && azimuth < 145)
        return 22;
    else if(azimuth >= 145 && azimuth < 152)
        return 23;
    else if(azimuth >= 152 && azimuth < 158)
        return 24;
    else if(azimuth >= 158 && azimuth < 165)
        return 25;
    else if(azimuth >= 165 && azimuth < 171)
        return 26;
    else if(azimuth >= 171 && azimuth < 178)
        return 27;
    else
        return 28;
}

#endif
