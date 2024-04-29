/*############################################################################*/
/*#                                                                          #*/
/*#  A gain calculator with ADM metadata with speaker or HOA output			 #*/
/*#                                                                          #*/
/*#  Filename:      GainCalculator.h	                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          28/10/2020                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL + proprietary                                       #*/
/*#                                                                          #*/
/*############################################################################*/

#pragma once

#include "Coordinates.h"
#include "Tools.h"
#include "AdmMetadata.h"
#include "PolarExtent.h"
#include "AmbisonicEncoder.h"
#include "Screen.h"

namespace admrender {

	class ChannelLockHandler
	{
	public:
		ChannelLockHandler(Layout layout);
		~ChannelLockHandler();

		/**
			If the Object has a valid channelLock distance then determines the new direction of the object
		*/
		CartesianPosition handle(ChannelLock channelLock, CartesianPosition position);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;

		std::vector<double> m_l2norm;
		std::vector<unsigned int> m_closestSpeakersInd;
		std::vector<unsigned int> m_equalDistanceSpeakers;
		std::vector<std::vector<double>> m_tuple;
		std::vector<std::vector<double>> m_tupleSorted;
		int m_activeTuples = 0;
	};

	class ZoneExclusionHandler
	{
	public:
		ZoneExclusionHandler(Layout layout);
		~ZoneExclusionHandler();

		/**
			Calculate the gain vector once the appropriate loudspeakers have been exlcuded.

			The gains are replaced with the processed version.
		*/
		void handle(const std::vector<PolarExclusionZone>& exclusionZones, std::vector<double>& gainsInOut);

	private:
		unsigned int m_nCh = 0;
		Layout m_layout;
		std::vector<std::vector<std::set<unsigned int>>> m_downmixMapping;
		std::vector<std::vector<unsigned int>> m_downmixMatrix;

		int GetLayerPriority(const std::string& inputChannelName, const std::string& outputChannelName);

		// Downmix matrix
		std::vector<std::vector<double>> m_D;
		// Vector holding the exclusion state of each channel
		std::vector<bool> m_isExcluded;
		// Temp vector of the gains
		std::vector<double> m_gainsTmp;

		std::vector<unsigned int> m_notExcludedElements;
		std::vector<unsigned int> m_setElements;

	};

	class CGainCalculator
	{
	public:
		CGainCalculator(Layout outputLayoutNoLFE);
		~CGainCalculator();

		/*
			Calculate the panning (loudspeaker or HOA) gains to apply to a
			mono signal for spatialisation based on the input metadata
		*/
		void CalculateGains(const ObjectMetadata& metadata, std::vector<double>& directGains, std::vector<double>& diffuseGains);

	private:
		// The output layout
		Layout m_outputLayout;
		// number of output channels
		unsigned int m_nCh;

		CPointSourcePannerGainCalc m_pspGainCalculator;
		CPolarExtentHandler m_extentPanner;
		CAmbisonicPolarExtentHandler m_ambiExtentPanner;

		CScreenScaleHandler m_screenScale;
		CScreenEdgeLock m_screenEdgeLock;

		ChannelLockHandler m_channelLockHandler;
		ZoneExclusionHandler m_zoneExclusionHandler;

		std::vector<double> m_gains;

		std::vector<CartesianPosition> m_divergedPos;
		std::vector<double> m_divergedGains;
		std::vector<std::vector<double>> m_gains_for_each_pos;
	};
} // namespace admrenderer
