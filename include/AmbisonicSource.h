/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicSource - Ambisonic Source                                     #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicSource.h                                        #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_SOURCE_H
#define	_AMBISONIC_SOURCE_H

#include "AmbisonicBase.h"

/// Base class for encoder and speaker

/** This acts as a base class for single point 3D objects such as encoding a 
	mono stream into a 3D soundfield, or a single speaker for decoding a 3D 
	soundfield. */

class CAmbisonicSource : public CAmbisonicBase
{
public:
	CAmbisonicSource();
	~CAmbisonicSource();
	/**
		Re-create the object for the given configuration. Previous data is
        lost. The last argument is not used, it is just there to match with
        the base class's form. Returns true if successful.
	*/
	virtual bool Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc);
	/**
		Not implemented.
	*/
	virtual void Reset();
	/**
		Recalculates coefficients.
	*/
	virtual void Refresh();
	/**
		Set azimuth, elevation, and distance settings.
	*/
	virtual void SetPosition(PolarPoint polPosition);
	/**
		Get azimuth, elevation, and distance settings.
	*/
	virtual PolarPoint GetPosition();
	/**
		Sets the weight [0,1] for the spherical harmonics of the given order.
	*/
	virtual void SetOrderWeight(AmbUInt nOrder, AmbFloat fWeight);
	/**
		Sets the weight [0,1] for the spherical harmonics of all orders.
	*/
	virtual void SetOrderWeightAll(AmbFloat fWeight);
	/**
		Sets the spherical harmonic coefficient for a given component
		Can be used for preset decoders to non-regular arrays where a Sampling decoder is sub-optimal
	*/
	virtual void SetCoefficient(AmbUInt nChannel, AmbFloat fCoeff);
	/**
		Gets the weight [0,1] for the spherical harmonics of the given order.
	*/
	virtual AmbFloat GetOrderWeight(AmbUInt nOrder);
	/**
		Gets the coefficient of the specified channel/component. Useful for the
		Binauralizer.
	*/
	virtual AmbFloat GetCoefficient(AmbUInt nChannel);
	/**
		Sets the source's gain.
	*/
	virtual void SetGain(AmbFloat fGain);
	/**
		Gets the source's gain.
	*/
	virtual AmbFloat GetGain();

protected:
	AmbFloat* m_pfCoeff;
	AmbFloat* m_pfOrderWeights;
	PolarPoint m_polPosition;
	AmbFloat m_fGain;
};

#endif // _AMBISONIC_SOURCE_H
