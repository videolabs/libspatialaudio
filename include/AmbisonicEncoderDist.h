/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoderDist - Ambisonic Encoder with distance                 #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
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

const unsigned knSpeedOfSound = 344;
const unsigned knMaxDistance = 150;

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
    virtual bool Configure(unsigned nOrder, bool b3D, unsigned nSampleRate);
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
    void Process(float* pfSrc, unsigned nSamples, CBFormat* pBFDst);
    /**
        Set the radius of the intended playback speaker setup which is used for
        the interior effect (W-Panning).
    */
    void SetRoomRadius(float fRoomRadius);
    /**
        Returns the radius of the intended playback speaker setup, which is
        used for the interior effect (W-Panning).
    */
    float GetRoomRadius();

protected:
    unsigned m_nSampleRate;
    float m_fDelay;
    int m_nDelay;
    unsigned m_nDelayBufferLength;
    float* m_pfDelayBuffer;
    int m_nIn;
    int m_nOutA;
    int m_nOutB;
    float m_fRoomRadius;
    float m_fInteriorGain;
    float m_fExteriorGain;
};

#endif // _AMBISONIC_ENCODER_DIST_H
