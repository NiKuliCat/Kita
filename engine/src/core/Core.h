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


#define BIT(x)  (1 << x)
#define BIND_EVENT_FUNC(fn) [this](auto&&... args) ->decltype(auto)  {return this->fn(std::forward<decltype(args)>(args)...);}
