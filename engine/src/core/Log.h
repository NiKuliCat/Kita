#pragma once
#include "Core.h"
#include "spdlog/spdlog.h"
namespace Kita {
	class Log
	{
	public:
		static void Init();
		inline static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }


	private:

		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};
}

//Core Log macros
#define KITA_CORE_TRACE(...)		::Kita::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define KITA_CORE_DEBUG(...)		::Kita::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define KITA_CORE_INFO(...)			::Kita::Log::GetCoreLogger()->info(__VA_ARGS__)
#define KITA_CORE_WARN(...)			::Kita::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define KITA_CORE_ERROR(...)		::Kita::Log::GetCoreLogger()->error(__VA_ARGS__)
#define KITA_CORE_CRITICAL(...)		::Kita::Log::GetCoreLogger()->critical(__VA_ARGS__)


// Clent Log macros
#define KITA_CLENT_TRACE(...)				::Kita::Log::GetClientLogger()->trace(__VA_ARGS__)
#define KITA_CLENT_DEBUG(...)				::Kita::Log::GetClientLogger()->debug(__VA_ARGS__)
#define KITA_CLENT_INFO(...)				::Kita::Log::GetClientLogger()->info(__VA_ARGS__)
#define KITA_CLENT_WARN(...)				::Kita::Log::GetClientLogger()->warn(__VA_ARGS__)
#define KITA_CLENT_ERROR(...)				::Kita::Log::GetClientLogger()->error(__VA_ARGS__)
#define KITA_CLENT_CRITICAL(...)			::Kita::Log::GetClientLogger()->critical(__VA_ARGS__)
