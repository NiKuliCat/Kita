#pragma once

#include "render/GraphicsContext.h"
struct GLFWwindow;
namespace Kita {

	class VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual RendererAPI::API GetAPI() const override { return RendererAPI::API::Vulkan; }

	private:
		GLFWwindow* m_WindowHandle;
	};
}