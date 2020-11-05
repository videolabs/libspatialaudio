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

/*
	A class to handle the interpolation from one gain vector applied to a mono input over a specified duration
*/
class CGainInterp
{
public:
	CGainInterp();
	~CGainInterp();

	/*
		Set the gain vector target and the time in samples to interpolate to it
	*/
	void SetGainVector(std::vector<double> newGainVec, unsigned int interpTimeInSamples);

	/*
		Apply the gains to the mono input signal and _add_ them to the output buffer
	*/
	void ProcessAccumul(float* pIn, std::vector<std::vector<float>>& ppOut, unsigned int nSamples, unsigned int nOffset);

	/*
		Resets the gain interpolator by setting the gain vector to the target and making sure there is no interpolation processing
	*/
	void Reset();

private:
	// The gain vector and the target gain vector to interpolate towards
	std::vector<double> m_gainVec, m_targetGainVec;

	// Flag if the process function has been called. If it has not then any time SetGainVector() is called
	// the gain vector will be set to the current and there is no interpolation i.e. first call of ProcessingAccumul()
	// is applies a constant gain vector
	bool m_isFirstCall = true;

	// The interpolation duration in samples
	unsigned int m_interpDurInSamples = 0;
	// The number of samples interpolated over
	unsigned int m_iInterpCount = 0;

};
