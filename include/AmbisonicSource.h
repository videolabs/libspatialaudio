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

    /** Re-create the object for the given configuration. Previous data is
     *  lost. The last argument is not used, it is just there to match with
     *  the base class's form. Returns true if successful.
     * @param nOrder    Ambisonic order.
     * @param b3D       Flag true if the signal is 3D.
     * @param nMisc     Unused.
     * @return          Returns true if successfully configured.
     */
    virtual bool Configure(unsigned nOrder, bool b3D, unsigned nMisc);

    /** Not implemented. */
    virtual void Reset();

    /** Recalculates coefficients. */
    virtual void Refresh();

    /** Set azimuth, elevation, and distance settings.
     * @param polPosition   Polar position of the source.
     */
    virtual void SetPosition(PolarPoint polPosition);

    /** Get azimuth, elevation, and distance settings.
     * @return  Returns source position in polar coordinates.
     */
    virtual PolarPoint GetPosition();

    /** Sets the weight for the spherical harmonics of the given order.
     * @param nOrder    The order to set the weights for
     * @param fWeight   Weight to be applied to all coefficients of the specified order.
     */
    virtual void SetOrderWeight(unsigned nOrder, float fWeight);

    /** Sets the weight for the spherical harmonics of all orders.
     * @param fWeight Weight to be applied to all coefficients.
     */
    virtual void SetOrderWeightAll(float fWeight);

    /** Sets the spherical harmonic coefficient for a given component
     *  Can be used for preset decoders to non-regular arrays where a Sampling decoder is sub-optimal
     * @param nChannel  Channel/coefficient index
     * @param fCoeff    Coefficient value
     */
    virtual void SetCoefficient(unsigned nChannel, float fCoeff);

    /** Gets the weight for the spherical harmonics of the given order.
     * @param nOrder    Order of coefficients
     * @return          Weight applied to coefficients of the specified order.
     */
    virtual float GetOrderWeight(unsigned nOrder);

    /** Gets the coefficient of the specified channel/component. Useful for the
     *  Binauralizer.
     *  This does not include any weights, only the raw coefficient.
     * @param nChannel  Index of the channel/coefficient.
     * @return          Value of the specified coefficient.
     */
    virtual float GetCoefficient(unsigned nChannel);

    /** Get a vector of all coefficients.
     *  Note that this will allocate so should not be called during real-time audio processing.
     * @return  Vector containing all coefficients.
     */
    virtual std::vector<float> GetCoefficients();

    /** Sets the source's gain.
     * @param fGain   Gain to apply to the source.
     */
    virtual void SetGain(float fGain);

    /** Gets the source's gain.
     * @return  Gain applied to the source.
     */
    virtual float GetGain();

protected:
    std::vector<float> m_pfCoeff;
    std::vector<float> m_pfOrderWeights;
    PolarPoint m_polPosition;
    float m_fGain;
};

#endif // _AMBISONIC_SOURCE_H
