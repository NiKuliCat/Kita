#include "kita_pch.h"
#include "TimeSystem.h"

#include <algorithm>
namespace Kita {

	namespace
	{
		constexpr double k_MinPositiveSeconds = 1e-8;
		constexpr double k_DefaultMaxDeltaSeconds = 0.100;
		constexpr std::size_t k_DefaultSmoothingWindowSize = 120;
		constexpr double k_DefaultFpsRefreshIntervalSeconds = 0.25;
	}






	TimeSystem::TimeSystem()
	{
		ApplySettings(TimeSystemSettings{});
		Reset();
	}

	TimeSystem::TimeSystem(const TimeSystemSettings& settings)
	{
		ApplySettings(settings);
		Reset();
	}

	void TimeSystem::Reset()
	{
		m_Statistics = {};
		m_CurrentTimestep = Timestep{};
		m_DeltaSamples.clear();
		m_DeltaSampleSum = 0.0;
		m_FpsRefreshAccumulatedSeconds = 0.0;
		m_FpsRefreshAccumulatedFrames = 0;

		m_Initialized = false;
		m_StartTime = {};
		m_LastFrameTime = {};
	}

	const Timestep& TimeSystem::Tick()
	{
		const Clock::time_point now = Clock::now();

		if (!m_Initialized)
		{
			m_Initialized = true;

			m_StartTime = now;
			m_LastFrameTime = now;

			m_Statistics.FrameIndex = 1;
			m_Statistics.TimeScale = m_Settings.TimeScale;
			m_Statistics.AppliedTimeScale = GetAppliedTimeScale();
			m_Statistics.Paused = m_Settings.StartPaused;
			m_Statistics.DisplayDeltaSeconds = 0.0;
			m_Statistics.DisplayFPS = 0.0;

			m_CurrentTimestep = Timestep(0.0, m_Statistics.AppliedTimeScale);
			return m_CurrentTimestep;
		}

		const double  rawDeltaSeconds = std::chrono::duration<double>(now - m_LastFrameTime).count();
		m_LastFrameTime = now;

		const double clampedUnscaleDeltaSeconds = std::clamp(rawDeltaSeconds,0.0, m_Settings.MaxDeltaSeconds);

		PushDeltaSample(clampedUnscaleDeltaSeconds);

		RecalculateSmoothDelta();

		const double appliedTimeScale = GetAppliedTimeScale();

		m_CurrentTimestep = Timestep(clampedUnscaleDeltaSeconds, appliedTimeScale);

		++m_Statistics.FrameIndex;
		m_Statistics.RawDeltaSeconds = rawDeltaSeconds;
		m_Statistics.UnscaleDeltaSeconds = clampedUnscaleDeltaSeconds;
		m_Statistics.ScaledDeltaSeconds = m_CurrentTimestep.GetSeconds();

		m_Statistics.InstantFPS = rawDeltaSeconds > k_MinPositiveSeconds ? 1.0 / rawDeltaSeconds : 0.0;

		m_Statistics.SmoothFPS = m_Statistics.SmoothUnscaleDeltaSeconds > k_MinPositiveSeconds ?
			1.0 / m_Statistics.SmoothUnscaleDeltaSeconds : 0.0;

		m_FpsRefreshAccumulatedSeconds += clampedUnscaleDeltaSeconds;
		++m_FpsRefreshAccumulatedFrames;

		if (m_FpsRefreshAccumulatedSeconds >= m_Settings.FpsRefreshIntervalSeconds)
		{
			m_Statistics.DisplayDeltaSeconds =
				m_FpsRefreshAccumulatedSeconds / static_cast<double>(m_FpsRefreshAccumulatedFrames);

			m_Statistics.DisplayFPS = m_Statistics.DisplayDeltaSeconds > k_MinPositiveSeconds
				? 1.0 / m_Statistics.DisplayDeltaSeconds
				: 0.0;

			m_FpsRefreshAccumulatedSeconds = 0.0;
			m_FpsRefreshAccumulatedFrames = 0;
		}

		m_Statistics.RealTimeSeconds = std::chrono::duration<double>(now - m_StartTime).count();
		m_Statistics.UnscaledTimeSeconds += clampedUnscaleDeltaSeconds;
		m_Statistics.ScaledTimeSeconds += m_CurrentTimestep.GetSeconds();

		m_Statistics.TimeScale = m_Settings.TimeScale;
		m_Statistics.AppliedTimeScale = appliedTimeScale;
		m_Statistics.Paused = m_Settings.StartPaused;


		return m_CurrentTimestep;
	}

	void TimeSystem::SetTimeScale(double scale)
	{
		m_Settings.TimeScale = std::max(0.0, scale);
		m_Statistics.TimeScale = m_Settings.TimeScale;
		m_Statistics.AppliedTimeScale = GetAppliedTimeScale();
	}

	double TimeSystem::GetTimeScale() const
	{
		return m_Settings.TimeScale;
	}

	double TimeSystem::GetAppliedTimeScale() const
	{
		return m_Settings.StartPaused ? 0.0 : m_Settings.TimeScale;
	}

	void TimeSystem::SetPaused(bool paused)
	{
		m_Settings.StartPaused = paused;
		m_Statistics.Paused = paused;
		m_Statistics.AppliedTimeScale = GetAppliedTimeScale();
	}

	void TimeSystem::Pause()
	{
		SetPaused(true);
	}

	void TimeSystem::Resume()
	{
		SetPaused(false);
	}

	bool TimeSystem::IsPaused() const
	{
		return m_Settings.StartPaused;
	}

	void TimeSystem::SetMaxDeltaSeconds(double maxDeltaSeconds)
	{
		m_Settings.MaxDeltaSeconds = maxDeltaSeconds > 0.0 ? maxDeltaSeconds : k_DefaultMaxDeltaSeconds;
	}

	double TimeSystem::GetMaxDeltaSeconds() const
	{
		return m_Settings.MaxDeltaSeconds;
	}

	void TimeSystem::SetSmoothingWindowSize(std::size_t sampleCount)
	{
		m_Settings.SmoothingWindowSize = sampleCount > 0 ? sampleCount: k_DefaultSmoothingWindowSize;

		TrimDeltaSamples();
		RecalculateSmoothDelta();
	}

	std::size_t TimeSystem::GetSmoothingWindowSize() const
	{
		return m_Settings.SmoothingWindowSize;
	}

	const Timestep& TimeSystem::GetTimestep() const
	{
		return m_CurrentTimestep;
	}

	const TimeSystemStatistics& TimeSystem::GetStatistics() const
	{
		return m_Statistics;
	}

	uint64_t TimeSystem::GetFrameIndex() const
	{
		return m_Statistics.FrameIndex;
	}

	double TimeSystem::GetRawDeltaSeconds() const
	{
		return m_Statistics.RawDeltaSeconds;
	}

	double TimeSystem::GetUnscaledDeltaSeconds() const
	{
		return m_Statistics.UnscaleDeltaSeconds;
	}

	double TimeSystem::GetDeltaSeconds() const
	{
		return m_Statistics.ScaledDeltaSeconds;
	}

	double TimeSystem::GetSmoothUnscaledDeltaSeconds() const
	{
		return m_Statistics.SmoothUnscaleDeltaSeconds;
	}

	double TimeSystem::GetInstantFPS() const
	{
		return m_Statistics.InstantFPS;
	}

	double TimeSystem::GetSmoothFPS() const
	{
		return m_Statistics.SmoothFPS;
	}

	double TimeSystem::GetRealTimeSeconds() const
	{
		return m_Statistics.RealTimeSeconds;
	}

	double TimeSystem::GetUnscaledTimeSeconds() const
	{
		return m_Statistics.UnscaledTimeSeconds;
	}

	double TimeSystem::GetScaledTimeSeconds() const
	{
		return m_Statistics.ScaledTimeSeconds;
	}

	void TimeSystem::ApplySettings(const TimeSystemSettings& settings)
	{
		m_Settings.MaxDeltaSeconds = settings.MaxDeltaSeconds > 0.0
			? settings.MaxDeltaSeconds
			: k_DefaultMaxDeltaSeconds;

		m_Settings.SmoothingWindowSize = settings.SmoothingWindowSize > 0
			? settings.SmoothingWindowSize
			: k_DefaultSmoothingWindowSize;

		m_Settings.FpsRefreshIntervalSeconds = settings.FpsRefreshIntervalSeconds > 0.0
			? settings.FpsRefreshIntervalSeconds
			: k_DefaultFpsRefreshIntervalSeconds;

		m_Settings.TimeScale = std::max(0.0, settings.TimeScale);
		m_Settings.StartPaused = settings.StartPaused;
	}

	void TimeSystem::PushDeltaSample(double deltaSeconds)
	{
		m_DeltaSamples.push_back(std::max(0.0, deltaSeconds));
		m_DeltaSampleSum += m_DeltaSamples.back();
		TrimDeltaSamples();
	}

	void TimeSystem::TrimDeltaSamples()
	{
		while (m_DeltaSamples.size() > m_Settings.SmoothingWindowSize)
		{
			m_DeltaSampleSum -= m_DeltaSamples.front();
			m_DeltaSamples.pop_front();
		}
	}

	void TimeSystem::RecalculateSmoothDelta()
	{
		if (m_DeltaSamples.empty())
		{
			m_Statistics.SmoothUnscaleDeltaSeconds = 0.0;
			return;
		}

		m_Statistics.SmoothUnscaleDeltaSeconds =
			m_DeltaSampleSum / static_cast<double>(m_DeltaSamples.size());
	}

}
