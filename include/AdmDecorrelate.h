/*############################################################################*/
/*#                                                                          #*/
/*#  A renderer for ADM streams.                                             #*/
/*#  CAdmRenderer - ADM Renderer                                             #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmRenderer.h                                            #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _ADM_DECORRELATE_H
#define    _ADM_DECORRELATE_H

#include "AdmLayouts.h"
#include "kiss_fftr.h"
#include "decorrelate.hpp" // from libear

namespace admrender {

	/**
		This class applied decorrelation to the output speaker layouts.
		It allows allows for compensation delay to be applied to the direct signal
	*/

	class CAdmDecorrelate
	{
	public:
		/*
			layout is the target loudspeaker layout
			nSamples is the maximum block size expected
		*/
		CAdmDecorrelate();
		~CAdmDecorrelate();

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
		//void Process(CBFormat* pBFSrcDst);
		void Process(std::vector<std::vector<float>> &ppInDirect, std::vector<std::vector<float>> &ppInDiffuse, unsigned int nSamples);

	private:
		// The time-domain decorrelation filters
		std::vector<std::vector<float>> decorrelationFilters;
		// The compensation delay to apply to the direct signals
		int compensationDelay = ear::decorrelatorCompensationDelay();
		// The number of channels in the output array
		unsigned int m_nCh;

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
		int m_nDelay = 0;
		int m_nReadPos = 0;
		int m_nWritePos = 0;

		void WriteToDelayLine(float* pDelayLine, float* pIn, int nWritePos, int nSamples);
		void ReadFromDelayLine(float* pDelayLine, float* pOut, int nReadPos, int nSamples);
	};

}

#endif //_ADM_DECORRELATE_H
