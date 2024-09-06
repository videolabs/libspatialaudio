/*############################################################################*/
/*#                                                                          #*/
/*#  Apply a vector of gains to a mono input with interpolation              #*/
/*#                                                                          #*/
/*#  Filename:      GainInterp.h		                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          30/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include <vector>

/**
*	A class to handle the interpolation from one gain vector applied to a mono input over a specified duration
*/
template <typename T>
class CGainInterp
{
public:

	/** Constructor
	 * @param nCh	The number of channels of gain to apply
	 */
	CGainInterp(unsigned int nCh);
	~CGainInterp();

	/** Set the gain vector target and the time in samples to interpolate to it.
	 *
	 * @param newGainVec			Vector of new gains. Must have the same length as number of channels set in the constructor.
	 * @param interpTimeInSamples	The number of channels over which to interpolate to the new gain vector.
	 */
	void SetGainVector(const std::vector<T>& newGainVec, unsigned int interpTimeInSamples);

	/** Apply the gains to the mono input signal and write them to the output buffer.
	 *
	 * @param pIn		Pointer to mono input buffer.
	 * @param ppOut		Output containing pIn multiplied by the gain vector.
	 * @param nSamples	The number of samples to process.
	 * @param nOffset	Number of samples of delay to applied to the signal.
	 */
	void Process(const float* pIn, float** ppOut, unsigned int nSamples, unsigned int nOffset);

	/** Apply the gains to the mono input signal and _add_ them to the output buffer.
	 *
	 * @param pIn		Pointer to mono input buffer.
	 * @param ppOut		Output containing its original content plus pIn multiplied by the gain vector.
	 * @param nSamples	The number of samples to process.
	 * @param nOffset	Optional number of samples of delay to applied to the signal.
	 * @param gain		Optional gain to be applied to the signal before it is added to the output.
	 */
	void ProcessAccumul(const float* pIn, float** ppOut, unsigned int nSamples, unsigned int nOffset = 0, T gain = 1.f);

	/** Resets the gain interpolator by setting the gain vector to the target and making sure there is no interpolation processing pending.	*/
	void Reset();

private:
	// The gain vector, the target gain vector to interpolate towards, and a vector holding the change per sample
	std::vector<T> m_currentGainVec, m_targetGainVec, m_deltaGainVec;

	// The interpolation duration in samples
	unsigned int m_interpDurInSamples = 0;
	// The number of samples interpolated over
	unsigned int m_iInterpCount = 0;

	// Flag if it is the first call of Process or ProcessAccumul to avoid fade in from zero
	bool m_isFirstCall = true;
};
