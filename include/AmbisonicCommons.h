/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  Copyright � 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicCommons.h                                       #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis, Peter Stitt                           #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONICCOMMONS_H
#define    _AMBISONICCOMMONS_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <memory.h>

#define DEFAULT_ORDER    1
#define    DEFAULT_HEIGHT    true
#define DEFAULT_BFORMAT_SAMPLECOUNT    512
#define DEFAULT_SAMPLERATE 44100
#define DEFAULT_BLOCKSIZE 512
#define DEFAULT_HRTFSET_DIFFUSED false


typedef float AmbFloat;
typedef bool AmbBool;
typedef int AmbInt;
typedef unsigned int AmbUInt;

//TODO
enum BFormatChannels3D
{
    kW,
    kY, kZ, kX,
    kV, kT, kR, kS, kU,
    kQ, kO, kM, kK, kL, kN, kP,
    kNumOfBformatChannels3D
};

/*enum BFormatChannels2D
{
    kW,
    kX, kY,
    kU, kV,
    kP, kQ,
    kNumOfBformatChannels2D
};*/

/// Struct for source positioning in soundfield.
typedef struct PolarPoint
{
    /** horizontal positioning */
    AmbFloat fAzimuth;
    /** vertical positioning */
    AmbFloat fElevation;
    /** distance from centre of soundfield (radius)*/
    AmbFloat fDistance;
} PolarPoint;

/**
    Convert degrees to radians.
*/
AmbFloat DegreesToRadians(AmbFloat fDegrees);

/**
    Convert radians to degrees.
*/
AmbFloat RadiansToDegrees(AmbFloat fRadians);

/**
    Get the number of BFormat components for a given BFormat configuration.
*/
AmbUInt OrderToComponents(AmbUInt nOrder, AmbBool b3D);

/**
    Returns the index component of a BFormat stream where components of a given
    configuration start. For example, in a BFormat stream, the components of a
    2nd order 3D configuration, would start at index 4.
*/
AmbUInt OrderToComponentPosition(AmbUInt nOrder, AmbBool b3D);

/**
    Get the recommended minimum speakers needed to decode a BFormat stream of
    a given configuration.
*/
AmbUInt OrderToSpeakers(AmbUInt nOrder, AmbBool b3D);

/**
    Get the label for a given index component in a BFormat stream.
*/
char ComponentToChannelLabel(AmbUInt nComponent, AmbBool b3D);

#endif //_AMBISONICCOMMONS_H
