#pragma once


namespace Kita {


	class Timestep
	{
	public:

		constexpr Timestep() = default;

		constexpr explicit Timestep(double unscaleSeconds, double timeScale = 1.0)
			:m_UnscaleSeconds(unscaleSeconds), m_TimeScale(timeScale)
		{

		}

		constexpr double GetSeconds() const { return m_UnscaleSeconds * m_TimeScale; }
		constexpr double GetMilliSeconds() const { return GetSeconds() * 1000.0; }

		constexpr float GetSecondsF() const { return static_cast<float>(GetSeconds()); }
		constexpr float GetMilliSecondsF() const { return static_cast<float>(GetMilliSeconds());}

		constexpr double GetUnscaledSeconds() const{return m_UnscaleSeconds;}
		constexpr double GetUnscaledMilliSeconds() const{return m_UnscaleSeconds * 1000.0;}

		constexpr float GetUnscaledSecondsF() const{return static_cast<float>(m_UnscaleSeconds);}
		constexpr float GetUnscaledMilliSecondsF() const{return static_cast<float>(GetUnscaledMilliSeconds());}

		constexpr double GetTimeScale() const { return m_TimeScale; }

		constexpr bool IsPaused() const { return m_TimeScale <= 0.0; }
		constexpr bool IsZero() const { return m_UnscaleSeconds <= 0.0 || m_TimeScale <= 0.0; }

		constexpr explicit operator float() const{ return static_cast<float>(GetSeconds()); }

		constexpr explicit operator double() const{ return GetSeconds(); }


	public:
		static constexpr Timestep FromSeconds(double seconds){return Timestep(seconds, 1.0);}
		static constexpr Timestep FromMilliseconds(double milliseconds){return Timestep(milliseconds * 0.001, 1.0);}

	private:
		double m_UnscaleSeconds = 0.0;
		double m_TimeScale = 1.0;
	};
}