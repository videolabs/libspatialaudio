/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicAllRAD - Ambisonic AllRAD decoder                             #*/
/*#  Copyright © 2024 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicAllRAD.h                                        #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/05/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_ALLRAD_H
#define _AMBISONIC_ALLRAD_H

#include <string>
#include "AmbisonicBase.h"
#include "BFormat.h"
#include "AmbisonicOptimFilters.h"
#include "LoudspeakerLayouts.h";

/// Ambisonic AllRAD decoder

/** This is an AllRAD decoder for ITU BS.2051-3 layouts (and some extra ones). */

class CAmbisonicAllRAD : public CAmbisonicBase
{
public:
    CAmbisonicAllRAD();
    ~CAmbisonicAllRAD();

    /** Re-create the object for the given configuration. Previous data is
     *  lost. Set the ambisonic order, the maximum block size and the sample rate.
     *  A decoder will be generated for the specified layout name in the format X+Y+Z
     * @param nOrder        Ambisonic order of signal to be decoded.
     * @param nBlockSize    Maximum number of samples to be decoded.
     * @param sampleRate    Sample rate of the signal to be decoded.
     * @param layoutName    Loudspeaker layout name in the format X+Y+Z.
     * @param useLFE        (Optional) If true (and the layout contains one) the LFE channel will be rendered. If not, the LFE channel will be removed.
     * @param useOptimFilts (Optional) If true then psychacoustic optimisation filtering will be applied before decoding. This is false by default.
     * @return              Returns true if successfully configured.
     */
    bool Configure(unsigned nOrder, unsigned nBlockSize, unsigned sampleRate, const std::string& layoutName, bool useLFE = true, bool useOptimFilts = false);

    /** Resets the internal state. */
    void Reset();

    /** Not implemented. */
    void Refresh();

    /** Decode B-Format to speaker feeds.
     * @param pBFSrc    BFormat signal to decode.
     * @param nSamples  The number of samples to be decoded.
     * @param ppfDst    Decoded output of size nSpeakers x nSamples.
     */
    void Process(const CBFormat* pBFSrc, unsigned nSamples, float** ppfDst);

    /** Returns the number of speakers in the current speaker setup.
     * @return  Number of speakers.
     */
    unsigned GetSpeakerCount();

    /** Returns true if psychoacoustic optimisation filters are enabled. */
    bool GetUseOptimFilters();

private:
    CAmbisonicOptimFilters m_shelfFilters;
    // A temp version of the input signal
    CBFormat m_pBFSrcTmp;

    // Loudspeaker layout
    Layout m_layout;

    // IIR low-pass for the LFE
    CIIRFilter m_lowPassIIR;

    std::vector<std::vector<float>> m_decMat;

    /** Configure AllRAD decoding matrix */
    void ConfigureAllRADMatrix();

    // Maximum block size
    unsigned m_nBlockSize = 0;
    // Sample rate of the signal to process
    unsigned m_sampleRate = 0;

    // Apply psychoacoustic optimisation shelf filtering or not
    bool m_useOptimFilters = false;
};

#endif // _AMBISONIC_ALLRAD_H
