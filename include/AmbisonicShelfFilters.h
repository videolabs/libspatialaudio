/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicShelfFilters - Ambisonic psychoactic optimising filters       #*/
/*#  Copyright © 2020 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicShelfFilters.h                                  #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          23/03/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_SHELF_FILTERS_H
#define    _AMBISONIC_SHELF_FILTERS_H

#include "AmbisonicBase.h"
#include "BFormat.h"
#include "kiss_fftr.h"
#include "AmbisonicPsychoacousticFilters.h"

/// Ambisonic processor.

/** This object is used to rotate the BFormat signal around all three axes.
    Orientation structs are used to define the the soundfield's orientation. */

class CAmbisonicShelfFilters;

class CAmbisonicShelfFilters : public CAmbisonicBase
{
public:
    CAmbisonicShelfFilters();
    ~CAmbisonicShelfFilters();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. The last argument is not used, it is just there to match with
        the base class's form. Returns true if successful.
    */
    bool Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned nMisc);
    /**
        Not implemented.
    */
    void Reset();
    /**
        Recalculate coefficients.
    */
    void Refresh();
    /**
        Filter B-Format stream.
        Overload with number of samples (nSamples < m_nBlockSize) to process shorter block sizes
    */
    void Process(CBFormat* pBFSrcDst);
    void Process(CBFormat* pBFSrcDst, unsigned int nSamples);

protected:
    kiss_fftr_cfg m_pFFT_psych_cfg;
    kiss_fftr_cfg m_pIFFT_psych_cfg;

    float* m_pfScratchBufferA;
    float** m_pfOverlap;
    unsigned m_nFFTSize;
    unsigned m_nBlockSize;
    unsigned m_nTaps;
    unsigned m_nOverlapLength;
    unsigned m_nFFTBins;
    float m_fFFTScaler;

    kiss_fft_cpx** m_ppcpPsychFilters;
    kiss_fft_cpx* m_pcpScratch;
};

#endif // _AMBISONIC_SHELF_FILTERS_H
