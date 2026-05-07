#pragma once

#include "core/Core.h"

#include <glm/glm.hpp>

namespace Kita {

	class VertexArray;
	class Shader;

	enum class CullMode
	{
		None = 0,
		Front,
		Back
	};

	enum class DepthTestMode
	{
		Never = 0,
		Less,
		Equal,
		Lequal,
		Greater,
		NotEqual,
		Gequal,
		Always
	};

	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};

		virtual ~RendererAPI() = default;

		virtual void Clear() = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void SetBlend(bool enable) = 0;
		virtual void SetCullMode(CullMode cullMode) = 0;
		virtual void SetDepthTest(bool enable) = 0;
		virtual void SetDepthWrite(bool enable) = 0;
		virtual void SetDepthTestMode(DepthTestMode testMode) = 0;
		virtual void SetColorAttachmentWriteMask(const std::vector<bool>& enabledAttachments) = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count) = 0;
		virtual void DrawLine(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, uint32_t vertexCount, float lineWidth) = 0;
		virtual void DrawGizmoPoints(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, uint32_t count) = 0;

		static API GetAPI() { return s_API; }
		static void SetAPI(API api) { s_API = api; }

	private:
		inline static API s_API = API::None;
	};

}
