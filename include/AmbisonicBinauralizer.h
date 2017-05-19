/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBinauralizer - Ambisonic Binauralizer                         #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBinauralizer.h                                  #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_BINAURALIZER_H
#define	_AMBISONIC_BINAURALIZER_H

#include "AmbisonicDecoder.h"
#include "kiss_fftr.h"

/// Ambisonic binauralizer

/** B-Format to binaural decoder. */

class CAmbisonicBinauralizer : public CAmbisonicBase
{
public:
	CAmbisonicBinauralizer();
	~CAmbisonicBinauralizer();
	/**
		Re-create the object for the given configuration. Previous data is
		lost. The tailLength variable it updated with the number of taps 
        used for the processing, and this can be used to offset the delay 
        this causes. The function returns true if the call is successful.
	*/
	virtual AmbBool Create(	AmbUInt nOrder, 
							AmbBool b3D, 
							AmbUInt nSampleRate, 
							AmbUInt nBlockSize,
							AmbBool bDiffused,
                            AmbUInt& tailLength);
	/**
		Resets members.
	*/
	virtual void Reset();
	/**
		Refreshes coefficients.
	*/
	virtual void Refresh();
	/**
		Decode B-Format to binaural feeds. There is no arguement for the number
		of samples to process, as this is determined by the nBlockSize argument
		in the constructor and Create() function. It is the responsibility of
		program using this library to handle the blocks of the signal by FIFO
		buffers or other means.
	*/
	void Process(CBFormat* pBFSrc, AmbFloat** ppfDst);
	
protected:
	CAmbisonicDecoder m_AmbDecoder;
	
	AmbUInt m_nBlockSize;
	AmbUInt m_nTaps;
	AmbUInt m_nFFTSize;
	AmbUInt m_nFFTBins;
	AmbFloat m_fFFTScaler;
	AmbUInt m_nOverlapLength;

	kiss_fftr_cfg m_pFFT_cfg;
	kiss_fftr_cfg m_pIFFT_cfg;
	kiss_fft_cpx** m_ppcpFilters[2];
	kiss_fft_cpx* m_pcpScratch;

	AmbFloat* m_pfScratchBufferA;
	AmbFloat* m_pfScratchBufferB;
	AmbFloat* m_pfScratchBufferC;
	AmbFloat* m_pfOverlap[2];

	void ArrangeSpeakers();
	void AllocateBuffers();
	void DeallocateBuffers();
};

#endif // _AMBISONIC_BINAURALIZER_H
