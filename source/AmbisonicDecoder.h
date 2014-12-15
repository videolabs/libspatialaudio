/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicDecoder - Ambisonic Decoder                                   #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicDecoder.h                                       #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_DECODER_H
#define	_AMBISONIC_DECODER_H

#include "AmbisonicBase.h"
#include "BFormat.h"
#include "AmbisonicSpeaker.h"

enum Amblib_SpeakerSetUps
{
	kAmblib_CustomSpeakerSetUp = -1,
	///2D Speaker Setup
	kAmblib_Mono, kAmblib_Stereo, kAmblib_LCR, kAmblib_Quad, kAmblib_50,
    kAmblib_Pentagon, kAmblib_Hexagon, kAmblib_HexagonWithCentre, kAmblib_Octagon, 
    kAmblib_Decadron, kAmblib_Dodecadron, 
	///3D Speaker Setup
	kAmblib_Cube,
	kAmblib_NumOfSpeakerSetUps
};

/// Ambisonic decoder

/** This is a basic decoder, handling both default and custom speaker
	configurations. */

class CAmbisonicDecoder : public CAmbisonicBase
{
public:
	CAmbisonicDecoder();
	~CAmbisonicDecoder();
	/**
		Re-create the object for the given configuration. Previous data is
		lost. nSpeakerSetUp can be any of the ::SpeakerSetUps enumerations. If 
		::kCustomSpeakerSetUp is used, then nSpeakers must also be given, 
		indicating the number of speakers in the custom speaker configuration.
		Else, if using one of the default configurations, nSpeakers does not 
		need to be specified. Function returns true if successful.
	*/
	bool Create(AmbUInt nOrder, AmbBool b3D, AmbInt nSpeakerSetUp, AmbUInt nSpeakers = 0);
	/**
		Resets all the speakers.
	*/
	void Reset();
	/**
		Refreshes all the speakers.
	*/
	void Refresh();
	/**
		Decode B-Format to speaker feeds.
	*/
	void Process(CBFormat* pBFSrc, AmbUInt nSamples, AmbFloat** ppfDst);
	/**
		Returns the current speaker setup, which is a ::SpeakerSetUps 
		enumeration.
	*/
	AmbInt GetSpeakerSetUp();
	/**
		Returns the number of speakers in the current speaker setup.
	*/
	AmbUInt GetSpeakerCount();
	/**
		Used when current speaker setup is ::kCustomSpeakerSetUp, to position 
		each speaker. Should be used by iterating nSpeaker for the number of speakers
		declared present in the current speaker setup, using polPosition to position
		each on.
	*/
	void SetPosition(AmbUInt nSpeaker, PolarPoint polPosition);
	/**
		Used when current speaker setup is ::kCustomSpeakerSetUp, it returns
		the position of the speaker of index nSpeaker, in the current speaker 
		setup.
	*/
	PolarPoint GetPosition(AmbUInt nSpeaker);
	/**
		Sets the weight [0,1] for the spherical harmonics of the given order, 
		at the given speaker.
	*/
	void SetOrderWeight(AmbUInt nSpeaker, AmbUInt nOrder, AmbFloat fWeight);
	/**
		Returns the weight [0,1] for the spherical harmonics of the given order, 
		at the given speaker.
	*/
	AmbFloat GetOrderWeight(AmbUInt nSpeaker, AmbUInt nOrder);
	/**
		Gets the coefficient of the specified channel/component of the 
		specified speaker. Useful for the Binauralizer.
	*/
	virtual AmbFloat GetCoefficient(AmbUInt nSpeaker, AmbUInt nChannel);

protected:
	void SpeakerSetUp(AmbInt nSpeakerSetUp, AmbUInt nSpeakers = 1);

	AmbInt m_nSpeakerSetUp;
	AmbUInt m_nSpeakers;
	CAmbisonicSpeaker* m_pAmbSpeakers;
};

#endif // _AMBISONIC_DECODER_H