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
#include "AmbisonicOptimFilters.h"

enum class Amblib_SpeakerSetUps
{
    kAmblib_CustomSpeakerSetUp = -1,
    ///2D Speaker Setup
    kAmblib_Mono, kAmblib_Stereo, kAmblib_LCR, kAmblib_Quad, kAmblib_50, kAmblib_70, kAmblib_51, kAmblib_71,
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

    /** Re-create the object for the given configuration. Previous data is
     *  lost. nSpeakerSetUp can be any of the ::SpeakerSetUps enumerations. If
     *  ::kCustomSpeakerSetUp is used, then nSpeakers must also be given,
     *  indicating the number of speakers in the custom speaker configuration.
     *  Else, if using one of the default configurations, nSpeakers does not
     *  need to be specified. Function returns true if successful.
     *
     * @param nOrder        Ambisonic order of signal to be decoded.
     * @param b3D           True if the signal to be decoded is 3D.
     * @param nBlockSize    Maximum number of samples to be decoded.
     * @param sampleRate    Sample rate of the signal to be decoded.
     * @param nSpeakerSetUp Selection of the speaker layout from Amblib_SpeakerSetUps.
     * @param nSpeakers     Number of speakers in the layout if Amblib_SpeakerSetUps::kCustomSpeakerSetUp is selected.
     * @return              Returns true if the configuration was successful.
     */
    bool Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned sampleRate, Amblib_SpeakerSetUps nSpeakerSetUp, unsigned nSpeakers = 0);

    /** Resets the internal state. */
    void Reset();

    /** Refreshes the internal state. This should be called if the speaker positions are changed. */
    void Refresh();

    /** Decode B-Format to speaker feeds.
     * @param pBFSrc    BFormat signal to decode.
     * @param nSamples  The number of samples to be decoded.
     * @param ppfDst    Decoded output of size nSpeakers x nSamples.
     */
    void Process(CBFormat* pBFSrc, unsigned nSamples, float** ppfDst);

    /** Returns the current speaker setup, which is a Amblib_SpeakerSetUps enumeration.
     * @return      The current speaker layout.
     */
    Amblib_SpeakerSetUps GetSpeakerSetUp();

    /** Returns the number of speakers in the current speaker setup.
     * @return  Number of speakers.
     */
    unsigned GetSpeakerCount();

    /** Used when current speaker setup is ::kCustomSpeakerSetUp, to position
     *  each speaker. Should be used by iterating nSpeaker for the number of speakers
     *  declared present in the current speaker setup, using polPosition to position
     *  each one. Refresh() should be called once all speaker positions are set.
     * @param nSpeaker      Index of the speaker to reposition.
     * @param polPosition   Position of the speaker.
     */
    void SetPosition(unsigned nSpeaker, PolarPoint polPosition);

    /** Used when current speaker setup is ::kCustomSpeakerSetUp, it returns
     *  the position of the speaker of index nSpeaker, in the current speaker
     *  setup.
     * @param nSpeaker  Speaker index.
     * @return          Position of the desired speaker.
     */
    PolarPoint GetPosition(unsigned nSpeaker);

    /** Sets the weight for the spherical harmonics of the given order,
     *  at the given speaker.
     * @param nSpeaker  Index of the speaker to adjust.
     * @param nOrder    Order of the components to be weighted.
     * @param fWeight   Weight to be applied.
     * @see GetOrderWeight
     */
    void SetOrderWeight(unsigned nSpeaker, unsigned nOrder, float fWeight);

    /** Returns the weight for the spherical harmonics of the given order,
     *  at the given speaker.
     * @param nSpeaker  Speaker index
     * @param nOrder    Order of the components to get the weight for.
     * @return          Weight applied to the specified speaker and components.
     */
    float GetOrderWeight(unsigned nSpeaker, unsigned nOrder);

    /** Gets the coefficient of the specified channel/component of the
     *  specified speaker. Useful for the Binauralizer.
     * @param nSpeaker  Speaker index.
     * @param nChannel  Ambisonic channel.
     * @return          Spherical harmonic coefficient for specified speaker and component.
     */
    virtual float GetCoefficient(unsigned nSpeaker, unsigned nChannel);

    /** Sets the coefficient of the specified channel/component of the
     *  specified speaker. Useful for presets for irregular physical loudspeakery arrays
     * @param nSpeaker  Speaker index.
     * @param nChannel  Spherical harmonic coefficient to set.
     * @param fCoeff    Value of the coefficient to set.
     */
    void SetCoefficient(unsigned nSpeaker, unsigned nChannel, float fCoeff);

    /** Gets whether a preset has been loaded or if the coefficients are calculated.
     * @return  Returns true if a layout is selected that has a preset decoder.
     */
    bool GetPresetLoaded();

protected:
    void SpeakerSetUp(Amblib_SpeakerSetUps nSpeakerSetUp, unsigned nSpeakers = 1);

    /** Checks if the current speaker arrangement is one that has a pre-defined preset.
     *  If true, sets the m_nSpeakerSetUp to the correct value
     */
    void CheckSpeakerSetUp();

    /** Load a pre-defined decoder preset if the speaker set-up is a supported layout. */
    void LoadDecoderPreset();

    Amblib_SpeakerSetUps m_nSpeakerSetUp;
    unsigned m_nSpeakers;
    CAmbisonicSpeaker* m_pAmbSpeakers;
    bool m_bPresetLoaded;

private:
    CAmbisonicOptimFilters m_shelfFilters;
    // A temp version of the input when optimisation filtering is applied to avoid overwriting the input
    CBFormat m_pBFSrcTmp;

    /** Configure decoder matrix to account for the speaker layout
    */
    void ConfigureDecoderMatrix();

    // Flag if the selected layout is 2D
    bool m_is2dLayout = false;
    // The maximum order supported by the selected layout
    unsigned m_maxLayoutOrder = 3;

    // Maximum block size
    unsigned m_nBlockSize = 0;
    // Sample rate of the signal to process
    unsigned m_sampleRate = 0;
};

#endif // _AMBISONIC_DECODER_H
