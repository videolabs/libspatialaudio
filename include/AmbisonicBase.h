/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicBase - Ambisonic Base                                         #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicBase.h                                          #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_BASE_H
#define    _AMBISONIC_BASE_H

#include "AmbisonicCommons.h"

/// Ambisonic base class.

/** This is the base class for most if not all of the classes that make up this 
    library. */

class CAmbisonicBase
{
public:
    CAmbisonicBase();
    virtual ~CAmbisonicBase();
    /**
        Gets the order of the current Ambisonic configuration.
    */
    AmbUInt GetOrder();
    /**
        Gets true or false depending on whether the current Ambisonic
        configuration has height(3D).
    */
    AmbUInt GetHeight();
    /**
        Gets the number of B-Format channels in the current Ambisonic
        configuration.
    */
    AmbUInt GetChannelCount();
    /**
        Re-create the object for the given configuration. Previous data is
        lost.
    */
    virtual AmbBool Create(AmbUInt nOrder, AmbBool b3D, AmbUInt nMisc);
    /**
        Not implemented.
    */
    virtual void Reset() = 0;
    /**
        Not implemented.
    */
    virtual void Refresh() = 0;

protected:
    AmbUInt m_nOrder;
    AmbBool m_b3D;
    AmbUInt m_nChannelCount;
    AmbBool m_bOpt;
};

#endif //_AMBISONIC_BASE_H
