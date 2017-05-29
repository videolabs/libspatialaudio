/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicProcessor - Ambisonic Processor                               #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicProcessor.h                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_PROCESSOR_H
#define	_AMBISONIC_PROCESSOR_H

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
    Orientation(AmbFloat fYaw, AmbFloat fPitch, AmbFloat fRoll)
        : fYaw(fYaw), fPitch(fPitch), fRoll(fRoll)
    {
        AmbFloat fCosYaw = cosf(fYaw);
        AmbFloat fSinYaw = sinf(fYaw);
        AmbFloat fCosRoll = cosf(fRoll);
        AmbFloat fSinRoll = sinf(fRoll);
        AmbFloat fCosPitch = cosf(fPitch);
        AmbFloat fSinPitch = sinf(fPitch);

        /* Conversion from yaw, pitch, roll (ZYX) to ZYZ convention to match rotation matrices
        This method reduces the complexity of the rotation matrices since the Z0 and Z1 rotations are the same form */
        AmbFloat r33 = fCosPitch * fCosRoll;
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

                AmbFloat r32 = -fCosYaw * fSinRoll + fCosRoll * fSinPitch * fSinYaw ;
                AmbFloat r31 = fCosRoll * fCosYaw * fSinPitch + fSinRoll * fSinYaw ;
                fAlpha = atan2( r32 , r31 );

                fBeta = acos( r33 );

                AmbFloat r23 = fCosPitch * fSinRoll;
                AmbFloat r13 = -fSinPitch;
                fGamma = atan2( r23 , -r13 );
            }
        }
    }

    friend class CAmbisonicProcessor;

private:
	/** rotation around the Z axis (yaw) */
	AmbFloat fYaw;
	/** rotation around the Y axis (pitch) */
	AmbFloat fPitch;
	/** rotation around the X axis (roll) */
	AmbFloat fRoll;

	/** These angles are obtained from Yaw, Pitch and Roll (ZYX convention)**/
	/** They follow the ZYZ convention to match the rotation equations **/
	/** rotation around the Z axis */
	AmbFloat fAlpha;
	/** rotation around the X axis */
	AmbFloat fBeta;
	/** rotation around the new Z axis */
	AmbFloat fGamma;
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
	bool Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nBlockSize, AmbUInt nMisc);
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
	void Process(CBFormat* pBFSrcDst, AmbUInt nSamples);

private:
	void ProcessOrder1_3D(CBFormat* pBFSrcDst, AmbUInt nSamples);
	void ProcessOrder2_3D(CBFormat* pBFSrcDst, AmbUInt nSamples);
	void ProcessOrder3_3D(CBFormat* pBFSrcDst, AmbUInt nSamples);
	void ProcessOrder1_2D(CBFormat* pBFSrcDst, AmbUInt nSamples);
	void ProcessOrder2_2D(CBFormat* pBFSrcDst, AmbUInt nSamples);
	void ProcessOrder3_2D(CBFormat* pBFSrcDst, AmbUInt nSamples);

	void ShelfFilterOrder(CBFormat* pBFSrcDst, AmbUInt nSamples);

protected:
	Orientation m_orientation;
	AmbFloat* m_pfTempSample;

	kiss_fftr_cfg m_pFFT_psych_cfg;
	kiss_fftr_cfg m_pIFFT_psych_cfg;
	
	AmbFloat* m_pfScratchBufferA;
	AmbFloat** m_pfOverlap;
	AmbUInt m_nFFTSize;
	AmbUInt m_nBlockSize;
	AmbUInt m_nTaps;
	AmbUInt m_nOverlapLength;
	AmbUInt m_nFFTBins;
	AmbFloat m_fFFTScaler;

	kiss_fft_cpx** m_ppcpPsychFilters;
	kiss_fft_cpx* m_pcpScratch;

	AmbFloat m_fCosAlpha;
	AmbFloat m_fSinAlpha;
	AmbFloat m_fCosBeta;
	AmbFloat m_fSinBeta;
	AmbFloat m_fCosGamma;
	AmbFloat m_fSinGamma;
	AmbFloat m_fCos2Alpha;
	AmbFloat m_fSin2Alpha;
	AmbFloat m_fCos2Beta;
	AmbFloat m_fSin2Beta;
	AmbFloat m_fCos2Gamma;
	AmbFloat m_fSin2Gamma;
	AmbFloat m_fCos3Alpha;
	AmbFloat m_fSin3Alpha;
	AmbFloat m_fCos3Beta;
	AmbFloat m_fSin3Beta;
	AmbFloat m_fCos3Gamma;
	AmbFloat m_fSin3Gamma;
};

#endif // _AMBISONIC_PROCESSOR_H
