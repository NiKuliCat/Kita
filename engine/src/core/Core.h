#pragma once
#include <memory>
namespace Kita {
	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}


#ifdef KITA_DEBUG
	#define KITA_ASSERT(x,...) { if(!x){KITA_ERROR("Assert Failed : {0}",__VA_ARGS__); __debugbreak();}}
	#define KITA_CORE_ASSERT(x,...) { if(!x){KITA_CORE_ERROR("Assert Failed : {0}",__VA_ARGS__); __debugbreak();}}
#else
	#define KITA_ASSERT(x, ...)
	#define KITA_CORE_ASSERT(x,...)
#endif


#define BIT(x)  (1 << x)
#define BIND_EVENT_FUNC(fn) [this](auto&&... args) ->decltype(auto)  {return this->fn(std::forward<decltype(args)>(args)...);}
