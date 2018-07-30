/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicEncoder - Ambisonic Encoder                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicEncoder.h                                       #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_ENCODER_H
#define _AMBISONIC_ENCODER_H

#include "AmbisonicSource.h"
#include "BFormat.h"

/// Ambisonic encoder.

/** This is a basic encoder that only takes the source's azimuth an elevation
    into account. If distance cues are going to be used, then use
    CAmbisonicEncoderDist instead. */

class CAmbisonicEncoder : public CAmbisonicSource
{
public:
    CAmbisonicEncoder();
    ~CAmbisonicEncoder();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. Returns true if successful.
    */
    virtual bool Configure(unsigned nOrder, bool b3D, unsigned nMisc);
    /**
        Recalculate coefficients, and apply normalisation factors.
    */
    void Refresh();
    /**
        Encode mono stream to B-Format.
    */
    void Process(float* pfSrc, unsigned nSamples, CBFormat* pBFDst);
};

#endif // _AMBISONIC_ENCODER_H
