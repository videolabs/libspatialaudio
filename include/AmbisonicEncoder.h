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

#include <algorithm> // for std::max

/// Ambisonic encoder.

/** This is a basic encoder that only takes the source's azimuth an elevation
    into account. If distance cues are going to be used, then use
    CAmbisonicEncoderDist instead. */

class CAmbisonicEncoder : public CAmbisonicSource
{
public:
    CAmbisonicEncoder();
    ~CAmbisonicEncoder();

    /** Re-create the object for the given configuration. Previous data is
     *  lost. Returns true if successful.
     * @param nOrder    Ambisonic order.
     * @param b3D       Flag true if encoding is to be 3D.
     * @param nMisc     Unused.
     * @return          Returns true if encoder is correctly configured.
     */
    virtual bool Configure(unsigned nOrder, bool b3D, unsigned nMisc);

    /** Recalculate coefficients, and apply normalisation factors. */
    void Refresh();

    /** Set the position of the source with the option to interpolate over a duration
     *  of the frame to the new position.
     *  The duration is in the range 0.f to 1.f where 1.f interpolates over a full frame.
     * @param polPosition   New polar position to encode.
     * @param interpDur     Proportion (from 0 to 1) of the next audio frame over which encoding interpolation is to take place.
     */
    void SetPosition(PolarPoint polPosition, float interpDur = 0.f);

    /** Encode mono stream to B-Format.
     * @param pfSrc     Pointer to the signal to encode.
     * @param nSamples  The number of samples to encode.
     * @param pBFDst    The BFormat encoded output.
     */
    void Process(float* pfSrc, unsigned nSamples, CBFormat* pBFDst);

    /** Encode mono stream to B-Format and *adds* it to the pBFDst buffer.
     *  Allows an optional offset for the position in samples at which the output is to be written.
     * @param pfSrc     Pointer to the signal to encode.
     * @param nSamples  The number of samples to encode.
     * @param pBFDst    The BFormat encoded output.
     * @param nOffset   Optional offset position when writing to the output.
     * @param fGain     Optional gain to apply to the output.
     */
    void ProcessAccumul(float* pfSrc, unsigned nSamples, CBFormat* pBFDst, unsigned int nOffset = 0, float fGain = 1.f);

private:
    // The last set HOA coefficients
    std::vector<float> m_pfCoeffOld;
    // The duration [0,1] of the interpolation from the old to the new HOA coefficients
    float m_fInterpDur = 0.f;
};

#endif // _AMBISONIC_ENCODER_H
