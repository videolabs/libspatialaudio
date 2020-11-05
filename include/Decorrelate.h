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
	/*
		layout is the target loudspeaker layout
		nSamples is the maximum block size expected
	*/
	CDecorrelate();
	~CDecorrelate();

	/**
		Re-create the object for the given configuration. Previous data is
		lost. Returns true if successful.
	*/
	bool Configure(Layout layout, unsigned int nBlockSize);
	/**
		Not implemented.
	*/
	void Reset();
	/**
		Filter otate B-Format stream.
	*/
	void Process(std::vector<std::vector<float>> &ppInDirect, std::vector<std::vector<float>> &ppInDiffuse, unsigned int nSamples);

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

	/**
		Read and write data to delay lines
	*/
	void WriteToDelayLine(float* pDelayLine, float* pIn, int nWritePos, int nSamples);
	void ReadFromDelayLine(float* pDelayLine, float* pOut, int nReadPos, int nSamples);

	/**
		Calculate decorrelation filters using the method described in Rec. ITU-R BS.2127-0 sec. 7.4
		The filters depend on the layout set at construction
	*/
	std::vector<std::vector<float>> CalculateDecorrelationFilterBank();

	/**
		Calculate the time-domain decorrelation filter for the a particular channel index seed
	*/
	std::vector<float> CalculateDecorrelationFilter(unsigned int seedIndex);
};