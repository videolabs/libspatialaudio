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
	/** rotation around the Z axis (rotate) */
	AmbFloat fYaw;
	/** rotation around the X axis (tilt) */
	AmbFloat fRoll;
	/** rotation around the Y axis (tumble) */
	AmbFloat fPitch;
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
	bool Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc);
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

protected:
	Orientation m_orientation;
	AmbFloat* m_pfTempSample;
	
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
};

#endif // _AMBISONIC_PROCESSOR_H