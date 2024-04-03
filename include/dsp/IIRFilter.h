/*############################################################################*/
/*#                                                                          #*/
/*#  A biquad IIR filter with options for low- and high-pass                 #*/
/*#                                                                          #*/
/*#                                                                          #*/
/*#  Filename:      IIRFilter.h                                              #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          03/04/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include <vector>

/** A simple biquad IIR filter that creates either low- or high-pass Butterworth filters
*/
class CIIRFilter
{
public:
    CIIRFilter();
    ~CIIRFilter();

    enum class FilterType
    {
        LowPass = 0,
        HighPass
    };

    /** Configure the IIR filter. Sets the coefficients and prepares the filter state
     * @param nCh               The number of channels to process
     * @param sampleRate        The sample rate of the signal to be processed
     * @param frequency         The cutoff frequency of the filter
     * @param q                 The q-factor of the filter
     * @param filterType        The type of filter. Either low- or high-pass
     * @return                  Returns true on successful configuration
     */
    bool Configure(unsigned int nCh, unsigned int sampleRate, float frequency, float q, FilterType filterType);

    /** Reset the filter. */
    void Reset();

    /** Filter the multichannel input signal.
     * @param pIn       2D array containing the input signal. Size nCh x nSamples
     * @param pOut      2D array containing the filtered signal. Size nCh x nSamples
     * @param nSamples  The number of samples to process
     */
    void Process(float** pIn, float** pOut, unsigned int nSamples);

private:
    // The filter coefficients
    std::vector<float> m_b, m_a;

    // The filter states, one for each input channel
    std::vector<float> m_state1, m_state2;

    // The number of channels to process
    int m_nCh = 0;
};
