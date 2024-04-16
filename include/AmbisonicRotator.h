/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  CAmbisonicRotator - Ambisonic sound field rotation                      #*/
/*#  Copyright © 2024 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicRotator.h                                       #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          05/04/2024                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/


#ifndef _AMBISONIC_ROTATOR_H
#define    _AMBISONIC_ROTATOR_H

#include "AmbisonicBase.h"
#include "BFormat.h"

struct RotationOrientation
{
    float yaw = 0.f;
    float pitch = 0.f;
    float roll = 0.f;
};

/** This class is used to rotate the sound field by the angles specified in RotationOrientation.
 *  It includes rotation matrix coefficient smoothing to minimise artefacts when applying real-time rotations.
 */
class CAmbisonicRotator : public CAmbisonicBase
{
public:

    enum class RotationOrder
    {
        YawPitchRoll,
        YawRollPitch,
        PitchYawRoll,
        PitchRollYaw,
        RollYawPitch,
        RollPitchYaw
    };

    CAmbisonicRotator();
    ~CAmbisonicRotator();

    /** Configure the object with the specified settings.
     *
     * @param nOrder            The order of the signal the Rotator should process.
     * @param b3D               true if the signal to be processed is 3D.
     * @param nBlockSize        The maximum number of samples to process during any one call of Process().
     * @param sampleRate        The sample rate of the signal.
     * @param fadeTimeMilliSec  The time to fade from the current rotation matrix to the next when the orientation is updated.
     * @return                  Returns true if the class was correctly configured.
     */
    bool Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned sampleRate, float fadeTimeMilliSec);

    /** Reset the rotator. Ensures that the matrix coefficients are updated and smoothing is reset. */
    void Reset();

    /** Base class function. Not implemented. */
    void Refresh();
    
    /** Set the yaw, pitch and roll angles of the rotation in radians. The coefficients are updated
     *  and will smoothly transition from the old values to those set here.
     *
     *  A positive yaw-rotation turns the listener to the left (anti-clockwise as viewed from above)
     *  A positive pitch-rotation tilts the listener down (anti-clockwise as viewed from the left)
     *  A positive roll-angle tilts the listener to the right (anti-clockwise as viewed from the front)
     *
     * @param orientation   The yaw, pitch and roll orientation to be applied to the signal.
     */
    void SetOrientation(RotationOrientation orientation);

    /** Set the order the rotations set in SetOrientation are applied.
     *  If no order is set then yaw-pitch-roll is used by default.
     *
     * @param rotOrder  The order in which the rotations are to be applied
     */
    void SetRotationOrder(RotationOrder rotOrder);
    
    /** Get the rotation orientation angles (yaw, pitch and roll) in radians. */
    RotationOrientation GetOrientation();
    
    /** Rotate the B-format audio stream.
     *
     * @param pBFSrcDst     The B-format stream to be rotated. This is replaced by the rotated signal.
     * @param nSamples      The number of samples to be processed. This must be less than nBlockSize set in Configure().
     */
    void Process(CBFormat* pBFSrcDst, unsigned nSamples);

private:
    RotationOrder m_rotOrder = RotationOrder::YawPitchRoll;
    RotationOrientation m_orientation;
    CBFormat m_tempBuffer;
    std::vector<std::vector<float>> m_targetMatrix;
    std::vector<std::vector<float>> m_targetMatrixTmp;
    std::vector<std::vector<float>> m_currentMatrix;

    // The time to fade from the previous orientation to the target orientation
    float m_fadingTimeMilliSec = 0.f;
    unsigned int m_fadingSamples = 0;
    unsigned int m_fadingCounter = 0;
    // The size of the steps taken during fading for each matrix coefficient
    std::vector<std::vector<float>> m_deltaMatrix;

    // Temp matrices for the individual yaw, pitch and roll rotations
    std::vector<std::vector<float>> m_yawMatrix, m_pitchMatrix, m_rollMatrix;

    // Some constants that are used frequently in rotation matrix equations
    float m_sqrt3_2, m_sqrt6_4, m_sqrt10_4, m_sqrt15_4, m_sqrt15_2;

    /** Returns the rotation matrix for a yaw rotation.
     *  The rotation is applied so that a positive yaw will lead to a sound source
     *  encoded to the front moving in an anti-clockwise direction as viewed from above the listener.
     *
     *  @param yaw      The yaw rotation angle in radians.
     *  @param yawMat   The spherical harmonic yaw rotation matrix.
     */
    void getYawMatrix(float yaw, std::vector<std::vector<float>>& yawMat);

    /** Returns the rotation matrix for a pitch rotation.
     *  The rotation is applied so that a positive pitch will lead to a sound source
     *  encoded to the front moving in an anti-clockwise direction as viewed from the left of the listener.
     *
     *  @param pitch    The pitch rotation angle in radians.
     *  @param pitchMat The spherical harmonic pitch rotation matrix.
     */
    void getPitchMatrix(float pitch, std::vector<std::vector<float>>& pitchMat);

    /** Returns the rotation matrix for a roll rotation.
     *  The rotation is applied so that a positive roll will lead to a sound source
     *  encoded to the left moving in a clockwise direction as viewed from the front of the listener.
     *
     *  @param roll     The roll rotation angle in radians.
     *  @param rollMat  The spherical harmonic roll rotation matrix.
     */
    void getRollMatrix(float roll, std::vector<std::vector<float>>& rollMat);

    /** Update the target rotation matrix using the current RotationOrder and RotationOrientation. */
    void updateTargetRotationMatrix();
};

#endif // _AMBISONIC_ROTATOR_H
