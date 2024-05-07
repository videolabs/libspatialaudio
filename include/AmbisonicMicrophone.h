/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicMicrophone - Ambisonic Microphone                             #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicMicrophone.h                                    #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_MICROPHONE_H
#define _AMBISONIC_MICROPHONE_H

#include "AmbisonicSource.h"
#include "BFormat.h"

/// Ambisonic microphone

/** This is a microphone class. It is similar to ::CAmbisonicSpeaker, with the
    addition of having directivity control. */

class CAmbisonicMicrophone : public CAmbisonicSource
{
public:
    CAmbisonicMicrophone();
    ~CAmbisonicMicrophone();

    /** Recalculate coefficients, and apply normalisation factors. */
    void Refresh();

    /** Decode B-Format to microphone feed.
     * @param pBFSrc    BFormat scene to be sampled by the microphone directivity.
     * @param nSamples  Number of samples to process.
     * @param pfDst     Mono microphone signal.
     */
    void Process(CBFormat* pBFSrc, unsigned nSamples, float* pfDst);

    /** Set the microphone's directivity.
     * @param fDirectivity  Microphone directivity.
     */
    void SetDirectivity(float fDirectivity);

    /** Get the microphone's directivity.
     * @return  Microphone directivity.
     */
    float GetDirectivity();

protected:
    float m_fDirectivity;
};

#endif // _AMBISONIC_MICROPHONE_H
