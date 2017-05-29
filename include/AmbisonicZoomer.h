/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicZoomer - Ambisonic Zoomer                                     #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicZoomer.h                                        #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_ZOOMER_H
#define _AMBISONIC_ZOOMER_H

#include "AmbisonicBase.h"
#include "AmbisonicDecoder.h"
#include "BFormat.h"

/// Ambisonic zoomer.

/** This object is used to apply a zoom effect into BFormat soundfields. */

class CAmbisonicZoomer : public CAmbisonicBase
{
public:
    CAmbisonicZoomer();
    ~CAmbisonicZoomer();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. The last argument is not used, it is just there to match with
        the base class's form. Returns true if successful.
    */
    virtual AmbBool Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc);
    /**
        Not implemented.
    */
    void Reset();
    /**
        Recalculate coefficients.
    */
    void Refresh();
    /**
        Set zoom factor. This is in a range from -1 to 1, with 0 being no zoom,
        1 full forward zoom, and -1 full forward backwards.
    */
    void SetZoom(AmbFloat fZoom);
    /**
        Get zoom factor.
    */
    AmbFloat GetZoom();
    /**
        Zoom into B-Format stream.
    */
    void Process(CBFormat* pBFSrcDst, AmbUInt nSamples);
    /**
        Compute factorial of integer
    */
    AmbFloat factorial(AmbUInt M);
protected:
    CAmbisonicDecoder m_AmbDecoderFront;

    AmbFloat* m_AmbEncoderFront;
    AmbFloat* m_AmbEncoderFront_weighted;
    AmbFloat* a_m;

    AmbFloat m_fZoom;
    AmbFloat m_fZoomRed;
    AmbFloat m_AmbFrontMic;
    AmbFloat m_fZoomBlend;

    AmbFloat m_fWCoeff;
    AmbFloat m_fXCoeff;
    AmbFloat m_fYZCoeff;
    void Process2D(CBFormat* pBFSrcDst, AmbUInt nSamples);
    void Process3D(CBFormat* pBFSrcDst, AmbUInt nSamples);
    void (CAmbisonicZoomer::*m_pProcessFunction)(CBFormat* pBFSrcDst, AmbUInt nSamples);
};

#endif // _AMBISONIC_ZOOMER_H
