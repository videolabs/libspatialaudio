/*############################################################################*/
/*#                                                                          #*/
/*#  A simple Linkwitz-Riley IIR filter                                      #*/
/*#                                                                          #*/
/*#                                                                          #*/
/*#  Filename:      LinkwitzRileyIIR.h                                       #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/04/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "IIRFilter.h"
#include <vector>

/** A simple 4th-order Linkwitz-Riley filter to low- and high-pass a signal. The outputs sum to have flat magnitude response.
*/
class CLinkwitzRileyIIR
{
public:
    CLinkwitzRileyIIR();
    ~CLinkwitzRileyIIR();

    /** Configure the filter with the sample rate and cross-over frequency.
    * @param nCh               The number of channels to be processed
    * @param sampleRate        The sample rate of the signal to be processed
    * @param crossoverFreq     The cross-over frequency between the low- and high-pass bands
    * @return                  Returns true on successful configuration
    */
    bool Configure(unsigned int nCh, unsigned int sampleRate, float crossoverFreq);

    /** Reset the filter. */
    void Reset();

    /** Filter the multichannel input signal.
     * @param pIn       2D array containing the input signal. Size nCh x nSamples
     * @param pOutLP    2D array containing the low-pass filtered signal. Size nCh x nSamples
     * @param pOutHP    2D array containing the high-pass filtered signal. Size nCh x nSamples
     * @param nSamples  The number of samples to process
     */
    void Process(float** pIn, float** pOutLP, float** pOutHP, unsigned int nSamples);

private:
    // The two low- and high-pass Butterworth biquads that make up the 4th order Linkwitz-Riley filter
    CIIRFilter m_lp[2];
    CIIRFilter m_hp[2];
};
