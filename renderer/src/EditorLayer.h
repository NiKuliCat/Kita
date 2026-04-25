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
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);

		void TryPickObject();

	
	private:


		Ref<FrameBuffer> m_SceneMSAAFrameBuffer = nullptr;
		Ref<FrameBuffer> m_SceneResolveFrameBuffer = nullptr;
		Ref<FrameBuffer> m_PickingFrameBuffer = nullptr;


		Transform m_CameraTransform;
		ViewportCamera* m_ViewportCamera = nullptr;


		Ref<Scene> m_Scene = nullptr;
		Light* m_DirectLight = nullptr;
		Transform m_LightTransform;

		uint32_t m_SceneTexID = 0;
		glm::vec2 m_ViewportSize{};
		glm::vec2 m_ViewportBounds[2];
		bool m_ViewportOpen = true;




		//ui
		SceneHierarchyPanel m_SceneHierarchyPanel;
		int32_t m_GizmoControlType = -1;

	};
}
