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

/// Struct for soundfield rotation.
typedef struct Orientation
{
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
} Orientation;

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

	AmbFloat m_fCosYaw;
	AmbFloat m_fSinYaw;
	AmbFloat m_fCosRoll;
	AmbFloat m_fSinRoll;
	AmbFloat m_fCosPitch;
	AmbFloat m_fSinPitch;
	AmbFloat m_fCos2Yaw;
	AmbFloat m_fSin2Yaw;
	AmbFloat m_fCos2Roll;
	AmbFloat m_fSin2Roll;
	AmbFloat m_fCos2Pitch;
	AmbFloat m_fSin2Pitch;

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
