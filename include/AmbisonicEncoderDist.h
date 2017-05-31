/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoderDist - Ambisonic Encoder with distance                 #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicEncoderDist.h                                   #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_ENCODER_DIST_H
#define _AMBISONIC_ENCODER_DIST_H

#include "AmbisonicEncoder.h"

const AmbUInt knSpeedOfSound = 344;
const AmbUInt knMaxDistance = 150;

/// Ambisonic encoder with distance cues.

/** This is similar to a normal the ambisonic encoder, but takes the source's
    distance into account, delaying the signal, adjusting its gain, and
    implementing "W-Panning"(interior effect). If distance is not an issue,
    then use CAmbisonicEncoder which is more efficient. */

class CAmbisonicEncoderDist : public CAmbisonicEncoder
{
public:
    CAmbisonicEncoderDist();
    ~CAmbisonicEncoderDist();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. Returns true if successful.
    */
    virtual bool Configure(AmbUInt nOrder, AmbBool b3D, AmbUInt nSampleRate);
    /**
        Resets members such as delay lines.
    */
    virtual void Reset();
    /**
        Refreshes coefficients.
    */
    virtual void Refresh();
    /**
        Encode mono stream to B-Format.
    */
    void Process(AmbFloat* pfSrc, AmbUInt nSamples, CBFormat* pBFDst);
    /**
        Set the radius of the intended playback speaker setup which is used for
        the interior effect (W-Panning).
    */
    void SetRoomRadius(AmbFloat fRoomRadius);
    /**
        Returns the radius of the intended playback speaker setup, which is
        used for the interior effect (W-Panning).
    */
    AmbFloat GetRoomRadius();

protected:
    AmbUInt m_nSampleRate;
    AmbFloat m_fDelay;
    AmbInt m_nDelay;
    AmbUInt m_nDelayBufferLength;
    AmbFloat* m_pfDelayBuffer;
    AmbInt m_nIn;
    AmbInt m_nOutA;
    AmbInt m_nOutB;
    AmbFloat m_fRoomRadius;
    AmbFloat m_fInteriorGain;
    AmbFloat m_fExteriorGain;
};

#endif // _AMBISONIC_ENCODER_DIST_H
