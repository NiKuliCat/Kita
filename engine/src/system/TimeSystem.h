#pragma once
#include "core/Timestep.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
namespace Kita {


	struct TimeSystemSettings
	{
		double MaxDeltaSeconds = 0.100;
		size_t SmoothingWindowSize = 120;
		double FpsRefreshIntervalSeconds = 0.25;
		double TimeScale = 1.0;
		bool StartPaused = false;
	};

	struct TimeSystemStatistics
	{
		uint64_t FrameIndex = 0;
		double RawDeltaSeconds = 0.0;
		double UnscaleDeltaSeconds = 0.0;
		double ScaledDeltaSeconds = 0.0;

		double SmoothUnscaleDeltaSeconds = 0.0;
		double InstantFPS = 0.0;
		double SmoothFPS = 0.0;
		double DisplayDeltaSeconds = 0.0;
		double DisplayFPS = 0.0;

		double RealTimeSeconds = 0.0;
		double UnscaledTimeSeconds = 0.0;
		double ScaledTimeSeconds = 0.0;

		double TimeScale = 1.0;
		double AppliedTimeScale = 1.0;
		bool Paused = false;
	};


	class TimeSystem
	{
	public:
		using Clock = std::chrono::steady_clock;

	public:

		TimeSystem();
		explicit TimeSystem(const TimeSystemSettings& settings);

		void Reset();

		const Timestep& Tick();

		void SetTimeScale(double scale);
		double GetTimeScale() const;
		double GetAppliedTimeScale() const;

		void SetPaused(bool paused);
		void Pause();
		void Resume();
		bool IsPaused() const;

		void SetMaxDeltaSeconds(double maxDeltaSeconds);
		double GetMaxDeltaSeconds() const;

		void SetSmoothingWindowSize(std::size_t sampleCount);
		std::size_t GetSmoothingWindowSize() const;


		const Timestep& GetTimestep() const;
		const TimeSystemStatistics& GetStatistics() const;


		uint64_t GetFrameIndex() const;

		double GetRawDeltaSeconds() const;
		double GetUnscaledDeltaSeconds() const;
		double GetDeltaSeconds() const;

		double GetSmoothUnscaledDeltaSeconds() const;
		double GetInstantFPS() const;
		double GetSmoothFPS() const;

		double GetRealTimeSeconds() const;
		double GetUnscaledTimeSeconds() const;
		double GetScaledTimeSeconds() const;

	private:
		void ApplySettings(const TimeSystemSettings& settings);
		void PushDeltaSample(double deltaSeconds);
		void TrimDeltaSamples();
		void RecalculateSmoothDelta();

	private:
		TimeSystemSettings m_Settings{};
		TimeSystemStatistics m_Statistics{};
		Timestep m_CurrentTimestep{};

		Clock::time_point m_StartTime{};
		Clock::time_point m_LastFrameTime{};
		bool m_Initialized = false;

		std::deque<double> m_DeltaSamples;
		double m_DeltaSampleSum = 0.0;
		double m_FpsRefreshAccumulatedSeconds = 0.0;
		uint32_t m_FpsRefreshAccumulatedFrames = 0;
	};
}
