#include "kita_pch.h"
#include "OpenGLRendererAPI.h"
#include <glad/glad.h>
namespace Kita {



	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}



	void OpenGLRendererAPI::SetBlend(bool value)
	{
		if (value)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void OpenGLRendererAPI::SetDepthTest(bool value)
	{
		if (value)
		{
			glEnable(GL_DEPTH_TEST);
			glCullFace(GL_BACK);
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}
	}

	void OpenGLRendererAPI::SetCullMode(CullMode cullmode)
	{
		switch (cullmode)
		{
			case CullMode::Back:
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			}
			case CullMode::Front:
			{
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			}
			case CullMode::None:
			{
				glDisable(GL_CULL_FACE);
				break;
			}
		}
	}

	void OpenGLRendererAPI::SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		vertexArray->Bind();
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader)
	{
		vertexArray->Bind();
		shader->Bind();
		glDrawElements(GL_TRIANGLES, vertexArray->GetIndexCount(), GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t count)
	{
		vertexArray->Bind();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}



}