#pragma once
namespace Kita {
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};

	public:
		inline static  API GetAPI() { return s_API; }

	private:
		static API s_API;

	};
}
