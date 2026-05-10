#pragma once
#include "Engine.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanRenderTarget.h"
#include "render/VulkanRenderer.h"
#include "render/VulkanResourceFactory.h"
#include "scene/ViewportCamera.h"

namespace Kita {

	class EditorRenderer
	{
	public:
		EditorRenderer(VulkanContext& context, VulkanRenderTarget& rt, const Ref<Scene>& scene, ViewportCamera& camera);

		void Init();
		void OnDestroy();
		void Render(VulkanRenderTarget& rt);

	private:
		void InitRenderSceneData(VulkanRenderer::CameraUBO& cameraData, VulkanRenderer::DirectionLightUBO& mainLightData);
		void EnsureDefaultPipeline(VulkanRenderTarget& rt, VulkanGeometry& geometry, VulkanMaterial& material);

	private:
		VulkanContext* m_Context = nullptr;
		VulkanRenderer m_Renderer;
		VulkanResourceFactory m_VulkanResFactory;
		VulkanRenderTarget* m_RenderTarget = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		Unique<VulkanGraphicsPipeline> m_OpaquePipeline = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;
	};

}
