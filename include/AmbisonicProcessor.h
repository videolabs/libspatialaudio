/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicProcessor - Ambisonic Processor                               #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicProcessor.h                                     #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL (+ Proprietary)                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_PROCESSOR_H
#define    _AMBISONIC_PROCESSOR_H

#include "AmbisonicBase.h"
#include "BFormat.h"
#include "kiss_fftr.h"
#include "AmbisonicPsychoacousticFilters.h"
#include "AmbisonicZoomer.h"

enum ProcessorDOR
{
    kYaw, kRoll, kPitch, kNumProcessorDOR //(Degrees of Rotation)
};

enum ProcessorModes
{
    kYawRollPitch, kYawPitchRoll,
    kRollYawPitch, kRollPitchYaw,
    kPitchYawRoll, kPitchRollYaw,
    kNumProcessorModes
};


class CAmbisonicProcessor;


/// Struct for soundfield rotation.
class Orientation
{
public:
    Orientation(float fYaw, float fPitch, float fRoll)
        : fYaw(fYaw), fPitch(fPitch), fRoll(fRoll)
    {
        float fCosYaw = cosf(fYaw);
        float fSinYaw = sinf(fYaw);
        float fCosRoll = cosf(fRoll);
        float fSinRoll = sinf(fRoll);
        float fCosPitch = cosf(fPitch);
        float fSinPitch = sinf(fPitch);

        /* Conversion from yaw, pitch, roll (ZYX) to ZYZ convention to match rotation matrices
        This method reduces the complexity of the rotation matrices since the Z0 and Z1 rotations are the same form */
        float r33 = fCosPitch * fCosRoll;
        if (r33 == 1.f)
        {
            fBeta = 0.f;
            fGamma = 0.f;
            fAlpha = atan2(fSinYaw, fCosYaw);
        }
        else
        {
            if (r33 == -1.f)
            {
                fBeta = M_PI;
                fGamma = 0.f;
                fAlpha = atan2(-fSinYaw, fCosYaw);
            }
            else
            {

                float r32 = -fCosYaw * fSinRoll + fCosRoll * fSinPitch * fSinYaw ;
                float r31 = fCosRoll * fCosYaw * fSinPitch + fSinRoll * fSinYaw ;
                fAlpha = atan2( r32 , r31 );

                fBeta = acos( r33 );

                float r23 = fCosPitch * fSinRoll;
                float r13 = -fSinPitch;
                fGamma = atan2( r23 , -r13 );
            }
        }
    }

    friend class CAmbisonicProcessor;

private:
    /** rotation around the Z axis (yaw) */
    float fYaw;
    /** rotation around the Y axis (pitch) */
    float fPitch;
    /** rotation around the X axis (roll) */
    float fRoll;

    /** These angles are obtained from Yaw, Pitch and Roll (ZYX convention)**/
    /** They follow the ZYZ convention to match the rotation equations **/
    /** rotation around the Z axis */
    float fAlpha;
    /** rotation around the X axis */
    float fBeta;
    /** rotation around the new Z axis */
    float fGamma;
};


/// Ambisonic processor.

/** This object is used to rotate the BFormat signal around all three axes.
    Orientation structs are used to define the the soundfield's orientation. */

class CAmbisonicProcessor : public CAmbisonicBase
{
public:
    CAmbisonicProcessor();
    ~CAmbisonicProcessor();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. The last argument is not used, it is just there to match with 
        the base class's form. Returns true if successful.
    */
    bool Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned nMisc);
    /**
        Not implemented.
    */
    void Reset();
    /**
        Recalculate coefficients.
    */
    void Refresh();
    /**
        Set yaw, roll, and pitch settings.
    */
    void SetOrientation(Orientation orientation);
    /**
        Get yaw, roll, and pitch settings.
    */
    Orientation GetOrientation();
    /**
        Rotate B-Format stream.
    */
    void Process(CBFormat* pBFSrcDst, unsigned nSamples);

private:
    void ProcessOrder1_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder2_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder3_3D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder1_2D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder2_2D(CBFormat* pBFSrcDst, unsigned nSamples);
    void ProcessOrder3_2D(CBFormat* pBFSrcDst, unsigned nSamples);

    void ShelfFilterOrder(CBFormat* pBFSrcDst, unsigned nSamples);

protected:
    Orientation m_orientation;
    float* m_pfTempSample;

    kiss_fftr_cfg m_pFFT_psych_cfg;
    kiss_fftr_cfg m_pIFFT_psych_cfg;

    float* m_pfScratchBufferA;
    float** m_pfOverlap;
    unsigned m_nFFTSize;
    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nOverlapLength;
    unsigned m_nFFTBins;
    float m_fFFTScaler;

    kiss_fft_cpx** m_ppcpPsychFilters;
    kiss_fft_cpx* m_pcpScratch;

    float m_fCosAlpha;
    float m_fSinAlpha;
    float m_fCosBeta;
    float m_fSinBeta;
    float m_fCosGamma;
    float m_fSinGamma;
    float m_fCos2Alpha;
    float m_fSin2Alpha;
    float m_fCos2Beta;
    float m_fSin2Beta;
    float m_fCos2Gamma;
    float m_fSin2Gamma;
    float m_fCos3Alpha;
    float m_fSin3Alpha;
    float m_fCos3Beta;
    float m_fSin3Beta;
    float m_fCos3Gamma;
    float m_fSin3Gamma;
};

#endif // _AMBISONIC_PROCESSOR_H
