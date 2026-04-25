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
		void DrawSelectedBezierHelpers();
		void EnsureHelperLineBuffer(uint32_t vertexCount);
		void DrawSelectedBezierHandles();
		void EnsureHandlePointBuffer(uint32_t pointCount);

	
	private:
		Ref<FrameBuffer> m_FrameBuffer = nullptr;
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
		Ref<VertexArray> m_HelperLineVAO = nullptr;
		Ref<VertexBuffer> m_HelperLineVBO = nullptr;
		BufferLayout m_HelperLineLayout;
		uint32_t m_HelperLineCapacity = 0;
		Ref<VertexArray> m_HandlePointVAO = nullptr;
		Ref<VertexBuffer> m_HandlePointVBO = nullptr;
		BufferLayout m_HandlePointLayout;
		uint32_t m_HandlePointCapacityBytes = 0;

	};
}
