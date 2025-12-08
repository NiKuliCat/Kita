#pragma once
#include <Engine.h>

namespace Kita {

	class EditorLayer :public Layer{

	public:
		EditorLayer();


		virtual ~EditorLayer() = default;


		virtual void OnCreate()  override;
		virtual void OnUpdate(float daltaTime)  override;
		virtual void OnDestroy()  override;

		virtual void OnImGuiRender()  override;
		virtual void OnEvent(Event& event)  override;


	
	private:
		Ref<FrameBuffer> m_FrameBuffer = nullptr;
		Ref<VertexArray> m_VertexArray = nullptr;
		Ref<Shader> m_Shader = nullptr;
		Ref<Texture> m_Texture = nullptr;
		Ref<UniformBuffer> m_UniformBuffer = nullptr;
		OrthographicCamera*  m_Camera = nullptr;

		uint32_t m_SceneTexID = 0;
		glm::vec2 m_ViewportSize;

	};
}