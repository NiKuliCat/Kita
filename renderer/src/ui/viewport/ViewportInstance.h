#pragma once

#include <EngineCore.h>

#include "scene/EditorRenderer.h"
#include "scene/ViewportCamera.h"

#include "ui/viewport/EditorGizmoController.h"
#include "ui/viewport/EditorPickRegistry.h"
#include "ui/viewport/EditorViewportPanel.h"
#include "ui/viewport/EditorViewportSurface.h"

namespace Kita {

	class EditorSelectionContext;

	class ViewportInstance
	{
	public:
		ViewportInstance() = default;
		ViewportInstance(
			VulkanContext& context,
			VulkanResourceFactory&resFactory,
			PipelineFactory& pipelineFactory,
			const Ref<Scene>& scene,
			const Ref<EditorSelectionContext>& selectionContext,
			std::string windowName = "Viewport");

		ViewportInstance(const ViewportInstance&) = delete;
		ViewportInstance& operator=(const ViewportInstance&) = delete;

		ViewportInstance(ViewportInstance&&) noexcept = default;
		ViewportInstance& operator=(ViewportInstance&&) noexcept = default;

		~ViewportInstance();

		void OnUpdate(Timestep ts);
		void OnRender();
		void OnImGuiRender();
		void OnEvent(Event& event);
		bool HandleGizmoShortcut(KeyPressedEvent& event);

		void SetActive(bool isActive);

		bool IsOpen() const;
		bool IsImageHovered() const;
		bool IsWindowFocused() const;

		EditorViewportPanel* GetPanel() const { return m_Panel.get(); }
		EditorViewportSurface* GetSurface() const { return m_Surface.get(); }
		EditorRenderer* GetRenderer() const { return m_Renderer.get(); }
		ViewportCamera* GetViewportCamera() const { return m_ViewportCamera.get(); }

	private:
		Object FindSceneObjectByUUID(UUID uuid) const;
		EditorGizmoContext BuildGizmoContext() const;
		void ProcessPendingPickRequest();
		void ApplyPickResult(uint32_t pickId);
		void FocusCameraOnSelectedObject();
		void SyncViewportOverlaySettings();
		static glm::vec3 GetObjectFocusPoint(Object object);
		static float GetObjectFocusRadius(Object object);

	private:
		VulkanContext* m_Context = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		Unique<ViewportCamera> m_ViewportCamera = nullptr;
		Ref<EditorSelectionContext> m_SelectionContext = nullptr;
		Unique<EditorGizmoController> m_GizmoController = nullptr;
		Unique<EditorPickRegistry> m_PickRegistry = nullptr;
		Unique<EditorViewportPanel> m_Panel = nullptr;
		Unique<EditorViewportSurface> m_Surface = nullptr;
		Unique<EditorRenderer> m_Renderer = nullptr;
	};

}
