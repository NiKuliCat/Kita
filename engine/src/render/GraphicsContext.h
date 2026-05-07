#pragma once

#include "RendererAPI.h"

namespace Kita {

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;
		virtual void OnResize(uint32_t width, uint32_t height) = 0;
		virtual RendererAPI::API GetAPI() const = 0;
	};

}
