#pragma once

#include "Engine.h"
#include "SceneSelectionContext.h"
#include "scene/ViewportCamera.h"

#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanRenderTarget.h"
#include "render/VulkanRenderer.h"
#include "render/VulkanShader.h"

namespace Kita {

	class SceneViewportPanel
	{
	public:
		SceneViewportPanel(
			const Ref<Scene>& scene,
			const Ref<SceneSelectionContext>& selectionContext,
			std::string windowName = "Viewport");

		SceneViewportPanel(const SceneViewportPanel&) = delete;
		SceneViewportPanel& operator=(const SceneViewportPanel&) = delete;

		~SceneViewportPanel();

		void Simulate(float daltaTime);
		void OnImGuiRender();
		void OnEvent(Event& event);
		void Render();
		void SetActive(bool isActive) { m_IsActive = isActive; }
		bool IsImageHovered() const { return m_IsImageHovered; }
		bool IsWindowFocused() const { return m_IsFocused; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		void InitRenderResources();
		bool EnsureDemoRenderResources();
		void ResizeRenderTargetIfNeeded();
		void RecreateViewportTexture();
		void RenderDemoMeshToTarget(VkCommandBuffer commandBuffer);
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void TryPickObject();
		bool IsMouseInsideImageBounds() const;

	private:
		std::string m_WindowName = "Viewport";
		Unique<ViewportCamera> m_ViewportCamera = nullptr;

		Ref<Scene> m_SceneContext = nullptr;
		Ref<SceneSelectionContext> m_SelectionContext = nullptr;

		Unique<VulkanRenderTarget> m_RenderTarget = nullptr;
		Unique<VulkanRenderer> m_Renderer = nullptr;
		Unique<VulkanShader> m_VertexShader = nullptr;
		Unique<VulkanShader> m_FragmentShader = nullptr;
		Unique<VulkanGraphicsPipeline> m_Pipeline = nullptr;
		std::vector<Ref<Mesh>> m_DemoMeshes;

		ImTextureID m_SceneTextureID = 0;
		glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };
		glm::vec2 m_ViewportBounds[2]{};
		int32_t m_GizmoControlType = 1;
		bool m_IsHovered = false;
		bool m_IsFocused = false;
		bool m_IsImageHovered = false;
		bool m_IsActive = false;
		bool m_IsOpen = true;
		bool m_UseInitialPlacement = true;
		bool m_RequestWindowFocus = true;
		bool m_DemoShaderWarningIssued = false;
		bool m_DemoMeshWarningIssued = false;
	};

}
