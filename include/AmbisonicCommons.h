/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
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

/** Struct for source positioning in soundfield. */
typedef struct PolarPoint
{
    /** horizontal positioning */
    float fAzimuth;
    /** vertical positioning */
    float fElevation;
    /** distance from centre of soundfield (radius)*/
    float fDistance;
} PolarPoint;

/** Convert degrees to radians.
 * @param fDegrees  Input angle in degrees.
 * @return          Output angle in radians.
 */
float DegreesToRadians(float fDegrees);

/** Convert radians to degrees.
 * @param fRadians  Input angle in radians.
 * @return          Output angle in degrees.
 */
float RadiansToDegrees(float fRadians);

/** Get the number of BFormat components for a given BFormat configuration.
 * @param nOrder    Ambisonic order.
 * @param b3D       True if the signal is 3D.
 * @return          The number of ambisonic components
 */
unsigned OrderToComponents(unsigned nOrder, bool b3D);

/** Returns the index component of a BFormat stream where components of a given
 *  configuration start. For example, in a BFormat stream, the components of a
 *  2nd order 3D configuration, would start at index 4.
 * @param nOrder    Ambisonic order.
 * @param b3D       True if the signal is 3D.
 * @return          Index of the first component of a particular order.
 */
unsigned OrderToComponentPosition(unsigned nOrder, bool b3D);

/** Get the recommended minimum speakers needed to decode a BFormat stream of
 *  a given configuration.
 * @param nOrder    Ambisonic order.
 * @param b3D       True if the signal is 3D.
 * @return          Recommended minimum number of speakers.
 */
unsigned OrderToSpeakers(unsigned nOrder, bool b3D);

/** Get the label for a given index component in a BFormat stream.
 * @param nComponent    Index of the component
 * @param b3D           True if the signal is 3D.
 * @return              The label of the specified component.
 */
char ComponentToChannelLabel(unsigned nComponent, bool b3D);

/** Get the degree that the channel corresponds to. E.g. Channel 0 belongs to degree 0, 1-to-3 belong to degree 1, 4-to-8 belong to degree 2
 * @param nComponent    Index of the component
 * @param b3D           True if the signal is 3D.
 * @return              The degree of the specified component.
 */
unsigned ComponentPositionToDegree(unsigned nComponent, bool b3D);

#endif //_AMBISONICCOMMONS_H
