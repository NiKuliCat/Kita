#pragma once
#include <Engine.h>
#include "scene/ViewportCamera.h"
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
		Transform m_CameraTransform;
		PerspectiveCamera*  m_Camera = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;


		Ref<Scene> m_Scene = nullptr;
		Light* m_DirectLight = nullptr;
		Transform m_LightTransform;

		uint32_t m_SceneTexID = 0;
		glm::vec2 m_ViewportSize{};
		bool m_DisableFaceCulling = true;
		uint32_t m_MeshVertexCount = 0;
		uint32_t m_MeshIndexCount = 0;
		glm::vec3 m_MeshBoundsMin = glm::vec3(0.0f);
		glm::vec3 m_MeshBoundsMax = glm::vec3(0.0f);

	};
}
