/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicOptimFilters - Ambisonic psychoactic optimising filters       #*/
/*#  Copyright © 2024 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicOptimFilters.h                                  #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/04/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_OPTIM_FILTERS_H
#define    _AMBISONIC_OPTIM_FILTERS_H

#include "AmbisonicBase.h"
#include "BFormat.h"
#include "LinkwitzRileyIIR.h"

/** This class takes an ambisonic signal and applies shelf filtering that psychoacoustically
 *  optimise the high frequency band.
 */
class CAmbisonicOptimFilters : public CAmbisonicBase
{
public:
    CAmbisonicOptimFilters();
    ~CAmbisonicOptimFilters();

    /** Configure the object for the specified inputs
     * @param nOrder        The ambisonic order of the signal to be processed
     * @param b3D           Set true if the signal to process is 3D
     * @param nBlockSize    The maximum number of samples to process in a block
     * @param sampleRate    The sample rate of the signal to be processed
     * @return              Returns true on successful configuration
     */
    bool Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned sampleRate);
    
    /** Reset the internal state of the filters */
    void Reset();
    /** No implementation. Pure virtual base-class function. */
    void Refresh();

    /** Apply shelf filters to the B-format stream to apply psychoacoustic optimisation in the high frequency band
     * @param pBFSrcDst     The B-format stream to process
     * @param nSamples      The number of samples to process
     */
    void Process(CBFormat* pBFSrcDst, unsigned int nSamples);

protected:
    // Filter the signal into low- and high-frequency bands
    CLinkwitzRileyIIR m_bandFilterIIR;

    // The gains for the and high-frequency band max-rE optimisation
    std::vector<float> m_gMaxRe;

    // A temp buffer holding the low-passed signal
    CBFormat m_lowPassOut;

    // The maximum number of samples the class can process at once
    unsigned int m_nMaxBlockSize = 0;
};

#endif // _AMBISONIC_OPTIM_FILTERS_H
