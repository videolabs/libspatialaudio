/*############################################################################*/
/*#                                                                          #*/
/*#  A decorrelator for loudspeaker arrays                                   #*/
/*#                                                                          #*/
/*#  Filename:      AdmRenderer.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "LoudspeakerLayouts.h"
#include "Tools.h"
#include "kiss_fftr.h"

/**
	This class applied decorrelation to the output speaker layouts.
	It allows allows for compensation delay to be applied to the direct signal.

	It is based on Rec. ITU-R BS.2127-0 section 7.4 pg.63
*/
class CDecorrelate
{
public:
	CDecorrelate();
	~CDecorrelate();

	/** Re-create the object for the given configuration. Previous data is lost.
	 *
	 * @param layout		Target speaker layout
	 * @param nBlockSize	Maximum number of samples to be passed to Process()
	 * @return				Returns true if correctly configured
	 */
	bool Configure(Layout layout, unsigned int nBlockSize);
	/**
		Not implemented.
	*/
	void Reset();

	/** Apply decorrelation filters to the input signal
	 *
	 * @param ppInDirect	The direct input signal to be processed. A delay will be applied to compensate for the delay in the diffuse FIR filters.
	 * @param ppInDiffuse	The diffuse output signal.
	 * @param nSamples		The number of samples to process.
	 */
	void Process(float** ppInDirect, float** ppInDiffuse, unsigned int nSamples);

private:
	// The time-domain decorrelation filters
	std::vector<std::vector<float>> decorrelationFilters;
	// Output layout
	Layout m_layout;
	// The number of channels in the output array
	unsigned int m_nCh;

	// According to Rec. ITU-R BS.2127-0 sec. 7.4 the decorrelation filter length is 512 samples
	const unsigned int m_nDecorrelationFilterSamples = 512;

	kiss_fftr_cfg m_pFFT_decor_cfg;
	kiss_fftr_cfg m_pIFFT_decor_cfg;

	float* m_pfScratchBufferA;
	float** m_pfOverlap;
	unsigned m_nFFTSize;
	unsigned m_nBlockSize;
	unsigned m_nTaps;
	unsigned m_nOverlapLength;
	unsigned m_nFFTBins;
	float m_fFFTScaler;

	kiss_fft_cpx** m_ppcpDecorFilters;
	kiss_fft_cpx* m_pcpScratch;

	// Buffers to hold the delayed direct signals
	float** m_ppfDirectDelay;
	unsigned int m_nDelayLineLength;
	int m_nDelay = (m_nDecorrelationFilterSamples - 1) / 2;
	int m_nReadPos = 0;
	int m_nWritePos = 0;

	/** Write a circular signal to a delay line
	 *
	 * @param pDelayLine	Delay line buffer.
	 * @param pIn			Signal to be written to the delay line.
	 * @param nWritePos		The position in the delay line that the signal is to be written to.
	 * @param nSamples		Number of samples to be written to the delay line
	 */
	void WriteToDelayLine(float* pDelayLine, const float* pIn, int nWritePos, int nSamples);

	/** Read from a circular delay line
	 *
	 * @param pDelayLine	Delay line buffer.
	 * @param pOut			Signal read from the delay line.
	 * @param nReadPos		The position from which the signal is to be read.
	 * @param nSamples		Number of samples to be read from the delay line.
	 */
	void ReadFromDelayLine(const float* pDelayLine, float* pOut, int nReadPos, int nSamples);

	/** Calculate decorrelation filters using the method described in Rec. ITU-R BS.2127-0 sec. 7.4
	 *	The filters depend on the layout set at construction.
	 * @return Returns the decorrelation filters of size m_nCh x m_nDecorrelationFilterSamples
	 */
	std::vector<std::vector<float>> CalculateDecorrelationFilterBank();

	/** Calculate the time-domain decorrelation filter for the a particular channel index seed.
	 *
	 * @param seedIndex		The seed index for the random number generator.
	 * @return				Returns the time-domain decorrelation FIR.
	 */
	std::vector<float> CalculateDecorrelationFilter(unsigned int seedIndex);
};