/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicSource - Ambisonic Source                                     #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicSource.h                                        #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_SOURCE_H
#define _AMBISONIC_SOURCE_H

#include "AmbisonicBase.h"

#include <vector>

/// Base class for encoder and speaker

/** This acts as a base class for single point 3D objects such as encoding a
    mono stream into a 3D soundfield, or a single speaker for decoding a 3D
    soundfield. */

class CAmbisonicSource : public CAmbisonicBase
{
public:
    CAmbisonicSource();
    /**
        Re-create the object for the given configuration. Previous data is
        lost. The last argument is not used, it is just there to match with
        the base class's form. Returns true if successful.
    */
    virtual bool Configure(unsigned nOrder, bool b3D, unsigned nMisc);
    /**
        Not implemented.
    */
    virtual void Reset();
    /**
        Recalculates coefficients.
    */
    virtual void Refresh();
    /**
        Set azimuth, elevation, and distance settings.
    */
    virtual void SetPosition(PolarPoint polPosition);
    /**
        Get azimuth, elevation, and distance settings.
    */
    virtual PolarPoint GetPosition();
    /**
        Sets the weight [0,1] for the spherical harmonics of the given order.
    */
    virtual void SetOrderWeight(unsigned nOrder, float fWeight);
    /**
        Sets the weight [0,1] for the spherical harmonics of all orders.
    */
    virtual void SetOrderWeightAll(float fWeight);
    /**
        Sets the spherical harmonic coefficient for a given component
        Can be used for preset decoders to non-regular arrays where a Sampling decoder is sub-optimal
    */
    virtual void SetCoefficient(unsigned nChannel, float fCoeff);
    /**
        Gets the weight [0,1] for the spherical harmonics of the given order.
    */
    virtual float GetOrderWeight(unsigned nOrder);
    /**
        Gets the coefficient of the specified channel/component. Useful for the
        Binauralizer.
    */
    virtual float GetCoefficient(unsigned nChannel);
    /**
        Sets the source's gain.
    */
    virtual void SetGain(float fGain);
    /**
        Gets the source's gain.
    */
    virtual float GetGain();

protected:
    std::vector<float> m_pfCoeff;
    std::vector<float> m_pfOrderWeights;
    PolarPoint m_polPosition;
    float m_fGain;
};

#endif // _AMBISONIC_SOURCE_H
