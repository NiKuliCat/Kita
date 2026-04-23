#pragma once
#include <Engine.h>
#include "scene/ViewportCamera.h"
#include "ui/SceneHierarchyPanel.h"
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
		bool OnKeyPressed(KeyPressedEvent& event);


	
	private:
		Ref<FrameBuffer> m_FrameBuffer = nullptr;
		Transform m_CameraTransform;
		ViewportCamera* m_ViewportCamera = nullptr;


		Ref<Scene> m_Scene = nullptr;
		Light* m_DirectLight = nullptr;
		Transform m_LightTransform;

		uint32_t m_SceneTexID = 0;
		glm::vec2 m_ViewportSize{};
		bool m_ViewportOpen = true;

		//skybox
		Ref<Texture> m_Cubemap = nullptr;



		//ui
		SceneHierarchyPanel m_SceneHierarchyPanel;
		int32_t m_GizmoControlType = -1;

	};
}
