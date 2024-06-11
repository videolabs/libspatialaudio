/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CBFormat - Ambisonic BFormat                                            #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      BFormat.h                                                #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _BFORMAT_H
#define _BFORMAT_H

#include "AmbisonicBase.h"
#include <memory>
#include <vector>

/// Storage for BFormat signals.

/** This object is used to store and transfer BFormat signals. Memory is
    allocated for the number of channels needed for the given Ambisonic
    configuration (order and 2D/3D) and the number of samples. */

class CBFormat : public CAmbisonicBase
{
public:
    CBFormat();

    /** Returns the number of samples.
     * @return      Number of samples in the internal buffers
     */
    unsigned GetSampleCount();

    /** Re-create the buffers needed for the given configuration. Previous
     *  buffer contents are lost.
     * @param nOrder        Order of the B-format signal.
     * @param b3D           True if the signal is 3D.
     * @param nSampleCount  Number of samples in the buffer.
     * @return              Returns true if successfully configured.
     */
    bool Configure(unsigned nOrder, bool b3D, unsigned nSampleCount);

    /** Fill the buffer with zeros. */
    void Reset();

    /** Not implemented. */
    void Refresh();

    /** Copy a number of samples to a specific channel of the BFormat. Replaces the original content.
     * @param pfData    Pointer to buffer to copy.
     * @param nChannel  Channel to copy the input to.
     * @param nSamples  Number of samples to copy. Must be less than specified in Configure().
     */
    void InsertStream(float* pfData, unsigned nChannel, unsigned nSamples);

    /** Add a number of samples to a specific channel of the BFormat. Adds to the original content.
     * @param pfData    Pointer to buffer to copy.
     * @param nChannel  Channel to copy the input to.
     * @param nSamples  Number of samples to copy. Must be less than specified in Configure().
     * @param nOffset   Start position in the internal buffers that the data should be copied to.
     * @param gain      Optional gain to apply to the signal before adding it to the stream
     */
    void AddStream(float* pfData, unsigned nChannel, unsigned nSamples, unsigned nOffset = 0, float gain = 1.f);
    /**
        Copy a number of samples from a specific channel of the BFormat.
    */

    /** Copy a number of samples from a specific channel of the BFormat.
     * @param pfData    Output holding the specified data.
     * @param nChannel  Channel to copy.
     * @param nSamples  Number of samples to copy.
     */
    void ExtractStream(float* pfData, unsigned nChannel, unsigned nSamples);

    /** Copy the content of the buffer. It is assumed that the two objects are
        of the same configuration.
     * @param bf    B-format signal to copy.
     */
    void operator = (const CBFormat &bf);

    /** Returns true if the configuration of the two objects match.
     * @param bf    B-format signal for configuration comparison.
     * @return      Returns true if configuration of both objects match.
     */
    bool operator == (const CBFormat &bf);

    /** Returns true if the configuration of the two objects don't match.
     * @param bf    B-format signal for configuration comparison.
     * @return      Returns true if configuration of both objects do not match.
     */
    bool operator != (const CBFormat &bf);
    CBFormat& operator += (const CBFormat &bf);
    CBFormat& operator -= (const CBFormat &bf);
    CBFormat& operator *= (const CBFormat &bf);
    CBFormat& operator /= (const CBFormat &bf);
    CBFormat& operator += (const float &fValue);
    CBFormat& operator -= (const float &fValue);
    CBFormat& operator *= (const float &fValue);
    CBFormat& operator /= (const float &fValue);

protected:
    unsigned m_nSamples;
    unsigned m_nDataLength;
    std::vector<float> m_pfData;
    std::unique_ptr<float*[]> m_ppfChannels;

    //friend classes cannot be pure abstract type,
    //so must list each friend class manually
    friend class CAmbisonicEncoder;
    friend class CAmbisonicEncoderDist;
    friend class CAmbisonicDecoder;
    friend class CAmbisonicSpeaker;
    friend class CAmbisonicMicrophone;
    friend class CAmbisonicProcessor;
    friend class CAmbisonicRotator;
    friend class CAmbisonicBinauralizer;
    friend class CAmbisonicZoomer;
    friend class CAmbisonicShelfFilters;
    friend class CAmbisonicOptimFilters;
    friend class CAmbisonicAllRAD;
};

#endif //_BFORMAT_H
