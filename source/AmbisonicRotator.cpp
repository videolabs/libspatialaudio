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


#include "AmbisonicRotator.h"
#include <assert.h>
#include <cmath>

#include "Tools.h"

CAmbisonicRotator::CAmbisonicRotator()
{
    m_sqrt3_2 = 0.5f * std::sqrt(3.f);
    m_sqrt6_4 = 0.25f * std::sqrt(6.f);
    m_sqrt10_4 = 0.25f * std::sqrt(10.f);
    m_sqrt15_4 = 0.25f * std::sqrt(15.f);
    m_sqrt15_2 = 0.5f * std::sqrt(15.f);
}

CAmbisonicRotator::~CAmbisonicRotator()
{

}

bool CAmbisonicRotator::Configure(unsigned nOrder, bool b3D, unsigned nBlockSize, unsigned sampleRate, float fadeTimeMilliSec)
{
    bool success = CAmbisonicBase::Configure(nOrder, b3D, nBlockSize);
    if (!success)
        return false;

    if (fadeTimeMilliSec < 0.f || !b3D)
        return false;

    m_tempBuffer.Configure(nOrder, b3D, nBlockSize);

    auto nAmbiCh = GetChannelCount();
    m_targetMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_targetMatrixTmp.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_currentMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_deltaMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_yawMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_pitchMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));
    m_rollMatrix.resize(nAmbiCh, std::vector<float>(nAmbiCh, 0.f));

    m_fadingTimeMilliSec = fadeTimeMilliSec;
    m_fadingSamples = (unsigned)std::round(0.001f * m_fadingTimeMilliSec * (float)sampleRate);

    Reset();

    return true;
}

void CAmbisonicRotator::Reset()
{
    updateTargetRotationMatrix();
    m_currentMatrix = m_targetMatrix;
    m_fadingCounter = m_fadingSamples;
}

void CAmbisonicRotator::Refresh()
{
}

void CAmbisonicRotator::SetOrientation(RotationOrientation orientation)
{
    if (m_orientation.yaw != orientation.yaw || m_orientation.pitch != orientation.pitch
        || m_orientation.roll != orientation.roll)
    {
        m_orientation = orientation;

        updateTargetRotationMatrix();

        // Update the coefficient step size to go from the current matrix to the new target.
        // If the fading time is set to zero then m_deltaMatrix is all zeros
        for (unsigned i = 0; i < m_nChannelCount; ++i)
            for (unsigned j = 0; j < m_nChannelCount; ++j)
            {
                m_deltaMatrix[i][j] = m_fadingSamples == 0 ? 0.f : (m_targetMatrix[i][j] - m_currentMatrix[i][j]) / (float)m_fadingSamples;
            }

        // Restart the cross-fading
        m_fadingCounter = 0;
    }
}

void CAmbisonicRotator::SetRotationOrder(RotationOrder rotOrder)
{
    if (m_rotOrder != rotOrder)
    {
        m_rotOrder = rotOrder;
        SetOrientation(m_orientation);
    }
}

RotationOrientation CAmbisonicRotator::GetOrientation()
{
    return m_orientation;
}

void CAmbisonicRotator::Process(CBFormat* pBFSrcDst, unsigned nSamples)
{
    // Make a copy of the input to use during the matrix multiplication
    m_tempBuffer = *pBFSrcDst;

    // Clear the output buffer
    pBFSrcDst->Reset();

    // The number of samples to fade, which might not be a full frame
    unsigned nFadeSamp = std::min(nSamples, m_fadingSamples - m_fadingCounter);

    if (m_fadingCounter < m_fadingSamples)
    {
        for (unsigned iOut = 0; iOut < m_nChannelCount; ++iOut)
            for (unsigned iIn = 0; iIn < m_nChannelCount; ++iIn)
                if (std::abs(m_currentMatrix[iOut][iIn]) > 1e-6f && std::abs(m_targetMatrix[iOut][iIn]) > 1e-6f)
                    for (unsigned iSamp = 0; iSamp < nFadeSamp; ++iSamp)
                    {
                        pBFSrcDst->m_ppfChannels[iOut][iSamp] += m_currentMatrix[iOut][iIn] * m_tempBuffer.m_ppfChannels[iIn][iSamp];
                        m_currentMatrix[iOut][iIn] += m_deltaMatrix[iOut][iIn];
                    }
        m_fadingCounter += nFadeSamp;
    }

    // Process any remaining signal after cross-fading
    for (unsigned iOut = 0; iOut < m_nChannelCount; ++iOut)
        for (unsigned iIn = 0; iIn < m_nChannelCount; ++iIn)
            if (std::abs(m_targetMatrix[iOut][iIn]) > 1e-6f)
                for (unsigned iSamp = nFadeSamp; iSamp < nSamples; ++iSamp)
                {
                    pBFSrcDst->m_ppfChannels[iOut][iSamp] += m_targetMatrix[iOut][iIn] * m_tempBuffer.m_ppfChannels[iIn][iSamp];
                }
}

void CAmbisonicRotator::getYawMatrix(float yaw, std::vector<std::vector<float>>& yawMat)
{
    yawMat[0][0] = 1.f;
    if (m_nOrder > 0)
    {
        float cosYaw = std::cos(yaw);
        float sinYaw = std::sin(yaw);
        yawMat[1][1] = cosYaw;
        yawMat[1][3] = -sinYaw;
        yawMat[2][2] = 1.f;
        yawMat[3][1] = sinYaw;
        yawMat[3][3] = cosYaw;

        if (m_nOrder > 1)
        {
            float cos2Yaw = std::cos(2.f * yaw);
            float sin2Yaw = std::sin(2.f * yaw);
            yawMat[4][4] = cos2Yaw;
            yawMat[4][8] = -sin2Yaw;
            yawMat[5][5] = cosYaw;
            yawMat[5][7] = -sinYaw;
            yawMat[6][6] = 1.f;
            yawMat[7][5] = sinYaw;
            yawMat[7][7] = cosYaw;
            yawMat[8][4] = sin2Yaw;
            yawMat[8][8] = cos2Yaw;

            if (m_nOrder > 2)
            {
                assert(m_nOrder <= 3);

                float cos3Yaw = std::cos(3.f * yaw);
                float sin3Yaw = std::sin(3.f * yaw);
                yawMat[9][9] = cos3Yaw;
                yawMat[9][15] = -sin3Yaw;
                yawMat[10][10] = cos2Yaw;
                yawMat[10][14] = -sin2Yaw;
                yawMat[11][11] = cosYaw;
                yawMat[11][13] = -sinYaw;
                yawMat[12][12] = 1.f;
                yawMat[13][11] = sinYaw;
                yawMat[13][13] = cosYaw;
                yawMat[14][10] = sin2Yaw;
                yawMat[14][14] = cos2Yaw;
                yawMat[15][9] = sin3Yaw;
                yawMat[15][15] = cos3Yaw;
            }
        }
    }
}

void CAmbisonicRotator::getPitchMatrix(float pitch, std::vector<std::vector<float>>& pitchMat)
{
    pitchMat[0][0] = 1.f;
    if (m_nOrder > 0)
    {
        float cosPitch = std::cos(pitch);
        float sinPitch = std::sin(pitch);
        pitchMat[1][1] = 1.f;
        pitchMat[2][2] = cosPitch;
        pitchMat[2][3] = sinPitch;
        pitchMat[3][2] = -sinPitch;
        pitchMat[3][3] = cosPitch;

        if (m_nOrder > 1)
        {
            float cos2Pitch = std::cos(2.f * pitch);
            float sin2Pitch = std::sin(2.f * pitch);
            float cosPitchSq = cosPitch * cosPitch;
            float sinPitchSq = sinPitch * sinPitch;
            pitchMat[4][4] = cosPitch;
            pitchMat[4][5] = -sinPitch;
            pitchMat[5][4] = sinPitch;
            pitchMat[5][5] = cosPitch;
            pitchMat[6][6] = 1.f - 1.5f * sinPitchSq;
            pitchMat[6][7] = m_sqrt3_2 * sin2Pitch;
            pitchMat[6][8] = m_sqrt3_2 * sinPitchSq;
            pitchMat[7][6] = -m_sqrt3_2 * sin2Pitch;
            pitchMat[7][7] = cos2Pitch;
            pitchMat[7][8] = 0.5f * sin2Pitch;
            pitchMat[8][6] = m_sqrt3_2 * sinPitchSq;
            pitchMat[8][7] = -0.5f * sin2Pitch;
            pitchMat[8][8] = 0.5f * (1.f + cosPitchSq);

            if (m_nOrder > 2)
            {
                assert(m_nOrder <= 3);

                float cosPitchCu = cosPitchSq * cosPitch;
                float sinPitchCu = sinPitchSq * sinPitch;
                pitchMat[9][9] = 0.25f * (3.f * cosPitchSq + 1.f);
                pitchMat[9][10] = -m_sqrt6_4 * sin2Pitch;
                pitchMat[9][11] = m_sqrt15_4 * sinPitchSq;
                pitchMat[10][9] = m_sqrt6_4 * sin2Pitch;
                pitchMat[10][10] = cos2Pitch;
                pitchMat[10][11] = -m_sqrt10_4 * sin2Pitch;
                pitchMat[11][9] = m_sqrt15_4 * sinPitchSq;
                pitchMat[11][10] = m_sqrt10_4 * sin2Pitch;
                pitchMat[11][11] = 1.f - 0.25f * 5.f * sinPitchSq;

                pitchMat[12][12] = 0.5f * cosPitch * (5.f * cosPitchSq - 3.f);
                pitchMat[12][13] = -m_sqrt6_4 * sinPitch * (5.f * sinPitchSq - 4.f);
                pitchMat[12][14] = -m_sqrt15_2 * cosPitch * (cosPitchSq - 1.f);
                pitchMat[12][15] = m_sqrt10_4 * sinPitchCu;
                pitchMat[13][12] = m_sqrt6_4 * sinPitch * (5.f * sinPitchSq - 4.f);
                pitchMat[13][13] = 0.25f * cosPitch * (15.f * cosPitchSq - 11.f);
                pitchMat[13][14] = -m_sqrt10_4 * sinPitch * (3.f * sinPitchSq - 2.f);
                pitchMat[13][15] = -m_sqrt15_4 * cosPitch * (cosPitchSq - 1.f);
                pitchMat[14][12] = -m_sqrt15_2 * cosPitch * (cosPitchSq - 1.f);
                pitchMat[14][13] = m_sqrt10_4 * sinPitch * (3.f * sinPitchSq - 2.f);
                pitchMat[14][14] = 0.5f * cosPitch * (3.f * cosPitchSq - 1.f);
                pitchMat[14][15] = -m_sqrt6_4 * sinPitch * (sinPitchSq - 2.f);
                pitchMat[15][12] = -m_sqrt10_4 * sinPitchCu;
                pitchMat[15][13] = -m_sqrt15_4 * cosPitch * (cosPitchSq - 1.f);
                pitchMat[15][14] = m_sqrt6_4 * sinPitch * (sinPitchSq - 2.f);
                pitchMat[15][15] = 0.25f * cosPitch * (cosPitchSq + 3.f);
            }
        }
    }
}

void CAmbisonicRotator::getRollMatrix(float roll, std::vector<std::vector<float>>& rollMat)
{
    rollMat[0][0] = 1.f;
    if (m_nOrder > 0)
    {
        float cosRoll = std::cos(roll);
        float sinRoll = std::sin(roll);
        rollMat[1][1] = cosRoll;
        rollMat[1][2] = sinRoll;
        rollMat[2][1] = -sinRoll;
        rollMat[2][2] = cosRoll;
        rollMat[3][3] = 1.f;

        if (m_nOrder > 1)
        {
            float cos2Roll = std::cos(2.f * roll);
            float sin2Roll = std::sin(2.f * roll);
            float cosRollSq = cosRoll * cosRoll;
            float sinRollSq = sinRoll * sinRoll;
            rollMat[4][4] = cosRoll;
            rollMat[4][7] = sinRoll;
            rollMat[5][5] = cos2Roll;
            rollMat[5][6] = m_sqrt3_2 * sin2Roll;
            rollMat[5][8] = 0.5f * sin2Roll;
            rollMat[6][5] = -m_sqrt3_2 * sin2Roll;
            rollMat[6][6] = 1.f - 1.5f * sinRollSq;
            rollMat[6][8] = -m_sqrt3_2 * sinRollSq;
            rollMat[7][4] = -sinRoll;
            rollMat[7][7] = cosRoll;
            rollMat[8][5] = -0.5f * sin2Roll;
            rollMat[8][6] = -m_sqrt3_2 * sinRollSq;
            rollMat[8][8] = 0.5f * (cosRollSq + 1.f);

            if (m_nOrder > 2)
            {
                assert(m_nOrder <= 3);
                float sinRollCu = sinRollSq * sinRoll;

                rollMat[9][9] = 0.25f * cosRoll * (cosRollSq + 3.f);
                rollMat[9][11] = m_sqrt15_4 * cosRoll * (cosRollSq - 1.f);
                rollMat[9][12] = -m_sqrt10_4 * sinRollCu;
                rollMat[9][14] = -m_sqrt6_4 * sinRoll * (sinRollSq - 2.f);
                rollMat[10][10] = cos2Roll;
                rollMat[10][13] = m_sqrt10_4 * sin2Roll;
                rollMat[10][15] = m_sqrt6_4 * sin2Roll;
                rollMat[11][9] = m_sqrt15_4 * cosRoll * (cosRollSq - 1.f);
                rollMat[11][11] = 0.25f * cosRoll * (15.f * cosRollSq - 11.f);
                rollMat[11][12] = -m_sqrt6_4 * sinRoll * (5.f * sinRollSq - 4.f);
                rollMat[11][14] = -m_sqrt10_4 * sinRoll * (3.f * sinRollSq - 2.f);
                rollMat[12][9] = m_sqrt10_4 * sinRollCu;
                rollMat[12][11] = m_sqrt6_4 * sinRoll * (5.f * sinRollSq - 4.f);
                rollMat[12][12] = 0.5f * cosRoll * (5.f * cosRollSq - 3.f);
                rollMat[12][14] = m_sqrt15_2 * cosRoll * (cosRollSq - 1.f);
                rollMat[13][10] = -m_sqrt10_4 * sin2Roll;
                rollMat[13][13] = 1.f - 1.25f * sinRollSq;
                rollMat[13][15] = -m_sqrt15_4 * sinRollSq;
                rollMat[14][9] = m_sqrt6_4 * sinRoll * (sinRollSq - 2.f);
                rollMat[14][11] = m_sqrt10_4 * sinRoll * (3.f * sinRollSq - 2.f);
                rollMat[14][12] = m_sqrt15_2 * cosRoll * (cosRollSq - 1.f);
                rollMat[14][14] = 0.5f * cosRoll * (3.f * cosRollSq - 1.f);
                rollMat[15][10] = -m_sqrt6_4 * sin2Roll;
                rollMat[15][13] = -m_sqrt15_4 * sinRollSq;
                rollMat[15][15] = 0.25f * (3.f * cosRollSq + 1);
            }
        }
    }
}

void CAmbisonicRotator::updateTargetRotationMatrix()
{
    // Calculate the yaw matrix using the inverse
    getYawMatrix(m_orientation.yaw, m_yawMatrix);
    // Calculate the pitch matrix
    getPitchMatrix(m_orientation.pitch, m_pitchMatrix);
    // Calculate the roll matrix
    getRollMatrix(m_orientation.roll, m_rollMatrix);

    switch (m_rotOrder)
    {
    case CAmbisonicRotator::RotationOrder::YawPitchRoll:
        // R_yp = R_pitch * R_yaw
        multiplyMat(m_pitchMatrix, m_yawMatrix, m_targetMatrixTmp);
        // R_ypr = R_roll * R_yp
        multiplyMat(m_rollMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    case CAmbisonicRotator::RotationOrder::YawRollPitch:
        // R_yr = R_roll * R_yaw
        multiplyMat(m_rollMatrix, m_yawMatrix, m_targetMatrixTmp);
        // R_yrp = R_pitch * R_yp
        multiplyMat(m_pitchMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    case CAmbisonicRotator::RotationOrder::PitchYawRoll:
        // R_py = R_yaw * R_pitch
        multiplyMat(m_yawMatrix, m_pitchMatrix, m_targetMatrixTmp);
        // R_pyr = R_roll * R_ry
        multiplyMat(m_rollMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    case CAmbisonicRotator::RotationOrder::PitchRollYaw:
        // R_pr = R_roll * R_pitch
        multiplyMat(m_rollMatrix, m_pitchMatrix, m_targetMatrixTmp);
        // R_pry = R_yaw * R_ry
        multiplyMat(m_yawMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    case CAmbisonicRotator::RotationOrder::RollYawPitch:
        // R_ry = R_yaw * R_roll
        multiplyMat(m_yawMatrix, m_rollMatrix, m_targetMatrixTmp);
        // R_pyr = R_pitch * R_ry
        multiplyMat(m_pitchMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    case CAmbisonicRotator::RotationOrder::RollPitchYaw:
        // R_rp = R_pitch * R_roll
        multiplyMat(m_pitchMatrix, m_rollMatrix, m_targetMatrixTmp);
        // R_rpy = R_yaw * R_ry
        multiplyMat(m_yawMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    default: // Default to YPR
        // R_yp = R_pitch * R_yaw
        multiplyMat(m_pitchMatrix, m_yawMatrix, m_targetMatrixTmp);
        // R_ypr = R_roll * R_yp
        multiplyMat(m_rollMatrix, m_targetMatrixTmp, m_targetMatrix);
        break;
    }
}
