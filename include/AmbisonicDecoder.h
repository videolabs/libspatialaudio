/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicDecoder - Ambisonic Decoder                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicDecoder.h                                       #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_DECODER_H
#define _AMBISONIC_DECODER_H

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
    kAmblib_Dodecahedron,
    kAmblib_Cube2,
    kAmblib_MonoCustom,
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
    bool Configure(unsigned nOrder, bool b3D, int nSpeakerSetUp, unsigned nSpeakers = 0);
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
    void Process(CBFormat* pBFSrc, unsigned nSamples, float** ppfDst);
    /**
        Returns the current speaker setup, which is a ::SpeakerSetUps
        enumeration.
    */
    int GetSpeakerSetUp();
    /**
        Returns the number of speakers in the current speaker setup.
    */
    unsigned GetSpeakerCount();
    /**
        Used when current speaker setup is ::kCustomSpeakerSetUp, to position
        each speaker. Should be used by iterating nSpeaker for the number of speakers
        declared present in the current speaker setup, using polPosition to position
        each on.
    */
    void SetPosition(unsigned nSpeaker, PolarPoint polPosition);
    /**
        Used when current speaker setup is ::kCustomSpeakerSetUp, it returns
        the position of the speaker of index nSpeaker, in the current speaker
        setup.
    */
    PolarPoint GetPosition(unsigned nSpeaker);
    /**
        Sets the weight [0,1] for the spherical harmonics of the given order,
        at the given speaker.
    */
    void SetOrderWeight(unsigned nSpeaker, unsigned nOrder, float fWeight);
    /**
        Returns the weight [0,1] for the spherical harmonics of the given order,
        at the given speaker.
    */
    float GetOrderWeight(unsigned nSpeaker, unsigned nOrder);
    /**
        Gets the coefficient of the specified channel/component of the
        specified speaker. Useful for the Binauralizer.
    */
    virtual float GetCoefficient(unsigned nSpeaker, unsigned nChannel);
    /**
        Sets the coefficient of the specified channel/component of the
        specified speaker. Useful for presets for irregular physical loudspeakery arrays
    */
    void SetCoefficient(unsigned nSpeaker, unsigned nChannel, float fCoeff);

protected:
    void SpeakerSetUp(int nSpeakerSetUp, unsigned nSpeakers = 1);

    int m_nSpeakerSetUp;
    unsigned m_nSpeakers;
    CAmbisonicSpeaker* m_pAmbSpeakers;
};

#endif // _AMBISONIC_DECODER_H
