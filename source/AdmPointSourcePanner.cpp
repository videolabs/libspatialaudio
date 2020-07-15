/*############################################################################*/
/*#                                                                          #*/
/*#  A point source panner for ADM renderer.                                 #*/
/*#  CAdmPointSourcePanner - ADM Point Source Panner						 #*/
/*#  Copyright © 2020 Peter Stitt                                            #*/
/*#                                                                          #*/
/*#  Filename:      AdmPointSourcePanner.cpp							     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          23/06/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#include "AdmPointSourcePanner.h"
#include<cmath>
#include<string>
#include <map>

namespace admrender {

	CAdmPointSourcePanner::CAdmPointSourcePanner(Layout targetLayout) : m_gainCalculator(getLayoutWithoutLFE(targetLayout)),
		channelLockHandler(targetLayout), zoneExclusionHandler(targetLayout)
	{
		m_layout = targetLayout;
		m_nCh = 0;
		for (unsigned int iCh = 0; iCh < m_layout.channels.size(); ++iCh)
			if (!m_layout.channels[iCh].isLFE)
				m_nCh++;
	}

	CAdmPointSourcePanner::~CAdmPointSourcePanner()
	{

	}

	void CAdmPointSourcePanner::ProcessAccumul(ObjectMetadata metadata, float* pIn, std::vector<std::vector<float>> &ppDirect, std::vector<std::vector<float>>& ppDiffuse,
		unsigned int nSamples, unsigned int nOffset)
	{
		int nInterpSamples = 0;

		std::vector<double> gains(m_nCh);

		if (!(metadata == m_metadata) || m_gains.size() == 0)
		{
			m_gains.resize(m_nCh, 0.);
			// Get the panning direction
			PolarPosition direction;
			if (metadata.cartesian)
			{
				// If cartesian = true then convert the position to polar coordinates.
				// Note: Rec. ITU-R BS.2127-0 defines a different set of processing when
				// this flag is set but that is not yet implemented here. Instead, the polar
				// position path is used regardless of this flag.
				direction = CartesianToPolar(metadata.cartesianPosition);
			}
			else
				direction = metadata.polarPosition;

			// TODO: Apply screenEdgeLock and screenScaling

			// Apply channelLock to modify the position of the source, if required
			direction = channelLockHandler.handle(metadata.channelLock, direction);

			// Apply divergence
			auto divergedData = divergedPositionsAndGains(metadata.objectDivergence, direction);
			auto diverged_positions = divergedData.first;
			auto diverged_gains = divergedData.second;
			unsigned int nDivergedGains = (unsigned int)diverged_gains.size();

			// Calculate the new gains to be applied
			std::vector<std::vector<double>> gains_for_each_pos(nDivergedGains);
			for (unsigned int iGain = 0; iGain < nDivergedGains; ++iGain)
				gains_for_each_pos[iGain] = m_gainCalculator.CalculateGains(diverged_positions[iGain]);

			// Power summation of the gains
			for (int i = 0; i < (int)m_nCh; ++i)
			{
				double g_tmp = 0.;
				for (unsigned int j = 0; j < nDivergedGains; ++j)
					g_tmp += diverged_gains[j] * gains_for_each_pos[j][i] * gains_for_each_pos[j][i];
				gains[i] = sqrt(g_tmp);
			}

			// Zone exclusion downmix
			// See Rec. ITU-R BS.2127-0 sec. 7.3.12, pg 60
			gains = zoneExclusionHandler.handle(metadata.zoneExclusionPolar, gains);

			// Apply the overall gain to the spatialisation gains
			for (auto& g : gains)
				g *= metadata.gain;

			// Set the interpolation duration based on the conditions on page 35 of Rec. ITU-R BS.2127-0
			if (metadata.jumpPosition.flag && !m_bFirstFrame)
			{
				nInterpSamples = metadata.jumpPosition.interpolationLength;
			}
		}
		else // if the metadata input is the same as the last set
		{
			gains = m_gains;
		}

		// Calculate the direct and diffuse gains
		// See Rec. ITU-R BS.2127-0 sec.7.3.1 page 39
		float directCoefficient = (float)std::sqrt(1. - metadata.diffuse);
		float diffuseCoefficient = (float)std::sqrt(metadata.diffuse);

		unsigned int iCh = 0;
		// Apply the gains and add them to the non-LFE channels
		for (unsigned int i = 0; i < m_layout.channels.size(); ++i)
		{
			if (!m_layout.channels[i].isLFE) // if not LFE then skip the output channel
			{
				float deltaCoeff = 1.f / ((float)nInterpSamples);
				int iSample = 0;
				for (iSample = 0; iSample < nInterpSamples; ++iSample)
				{
					float fInterp = (float)iSample * deltaCoeff;
					float sampleData = pIn[iSample] * (fInterp * (float)gains[iCh] + (1.f - fInterp) * (float)m_gains[iCh]);
					ppDirect[i][iSample + nOffset] += sampleData * directCoefficient;
					ppDiffuse[i][iSample + nOffset] += sampleData * diffuseCoefficient;
				}
				for (iSample = nInterpSamples; iSample < (int)nSamples; ++iSample)
				{
					float sampleData = pIn[iSample] * (float)gains[iCh];
					ppDirect[i][iSample + nOffset] += sampleData * directCoefficient;
					ppDiffuse[i][iSample + nOffset] += sampleData * diffuseCoefficient;
				}
				iCh++;
			}
		}

		// Store the last calculated gains
		m_gains = gains;
		// Store the last input metadata
		m_metadata = metadata;
		// Flag that at least one frame has been processed
		m_bFirstFrame = false;
	}
}
