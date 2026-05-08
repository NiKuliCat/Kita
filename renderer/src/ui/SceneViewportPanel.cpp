#include "renderer_pch.h"
#include "SceneViewportPanel.h"

#include "imgui.h"
#include "ImGuizmo.h"
#include "project/EditorProjectBootstrap.h"
#include "render/font/FontManager.h"
#include "render/VulkanDescriptorSet.h"
#include "render/VulkanRenderCommand.h"
#include "render/VulkanRenderer.h"
#include <imgui_internal.h>

#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Kita {

	namespace
	{
		struct DemoShaderData
		{
			std::vector<uint8_t> VertexSpirv;
			std::vector<uint8_t> FragmentSpirv;
			std::string VertexEntryPoint = "VSMain";
			std::string FragmentEntryPoint = "PSMain";

			bool IsValid() const
			{
				return !VertexSpirv.empty() && !FragmentSpirv.empty();
			}
		};

		bool DrawViewportCloseButton(const ImRect& anchorRect, ImGuiID id)
		{
			(void)id;

			ImGuiContext& g = *GImGui;
			ImGuiStyle& style = ImGui::GetStyle();
			ImDrawList* drawList = ImGui::GetForegroundDrawList();
			const float buttonSize = g.FontSize;
			const ImVec2 buttonMin(
				anchorRect.Max.x - style.FramePadding.x - buttonSize,
				anchorRect.Min.y + (anchorRect.GetHeight() - buttonSize) * 0.5f);
			const ImVec2 buttonMax(buttonMin.x + buttonSize, buttonMin.y + buttonSize);
			const ImRect buttonRect(buttonMin, buttonMax);
			const bool hovered = ImGui::IsMouseHoveringRect(buttonRect.Min, buttonRect.Max, false);
			const bool pressed = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

			if (hovered)
				drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, ImGui::GetColorU32(ImGuiCol_ButtonHovered));

			const ImU32 iconColor = ImGui::GetColorU32(ImGuiCol_Text);
			unsigned int iconCodepoint = 0;
			ImTextCharFromUtf8(&iconCodepoint, ICON_FON_CANCEL, nullptr);
			const ImFontGlyph* glyph = g.Font->FindGlyphNoFallback(iconCodepoint);
			if (glyph)
			{
				const float glyphWidth = glyph->X1 - glyph->X0;
				const float glyphHeight = glyph->Y1 - glyph->Y0;
				const ImVec2 iconPos(
					buttonRect.Min.x + ImFloor((buttonRect.GetWidth() - glyphWidth) * 0.5f - glyph->X0),
					buttonRect.Min.y + ImFloor((buttonRect.GetHeight() - glyphHeight) * 0.5f - glyph->Y0));
				drawList->AddText(g.Font, g.FontSize, iconPos, iconColor, ICON_FON_CANCEL);
			}
			else
			{
				const ImVec2 iconSize = ImGui::CalcTextSize(ICON_FON_CANCEL);
				const ImVec2 iconPos(
					buttonRect.Min.x + ImFloor((buttonRect.GetWidth() - iconSize.x) * 0.5f),
					buttonRect.Min.y + ImFloor((buttonRect.GetHeight() - iconSize.y) * 0.5f));
				drawList->AddText(iconPos, iconColor, ICON_FON_CANCEL);
			}

			if (hovered)
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

			return pressed;
		}

		AssetHandle GetRequiredAssetHandle(const std::filesystem::path& assetPath)
		{
			if (assetPath.empty())
				return InvalidAssetHandle;

			return AssetManager::GetInstance().GetHandleByPath(assetPath.lexically_normal());
		}

		DemoShaderData TryLoadDemoShaderAsset()
		{
			DemoShaderData data{};
			auto& assetManager = AssetManager::GetInstance();
			const AssetHandle handle = GetRequiredAssetHandle(EditorProjectBootstrap::GetViewportDemoShaderPath());
			if (!Asset::IsValidHandle(handle))
				return data;

			Ref<ShaderAsset> shaderAsset = assetManager.GetShaderAsset(handle);
			if (!shaderAsset)
				return data;

			if (!shaderAsset->GetVertexStage().Valid() || !shaderAsset->GetFragmentStage().Valid())
				return data;

			data.VertexSpirv = shaderAsset->GetVertexStage().Spirv;
			data.FragmentSpirv = shaderAsset->GetFragmentStage().Spirv;
			data.VertexEntryPoint = shaderAsset->GetVertexStage().EntryPoint;
			data.FragmentEntryPoint = shaderAsset->GetFragmentStage().EntryPoint;
			return data;
		}

		std::vector<Ref<Mesh>> TryLoadDemoMeshesFromAsset()
		{
			auto& assetManager = AssetManager::GetInstance();
			const AssetHandle handle = GetRequiredAssetHandle(EditorProjectBootstrap::GetViewportDemoMeshPath());
			if (!Asset::IsValidHandle(handle))
				return {};

			Ref<MeshAsset> meshAsset = assetManager.GetMeshAsset(handle);
			if (!meshAsset)
				return {};

			const auto& subMeshes = meshAsset->GetSubMeshes();
			if (!subMeshes.empty())
				return subMeshes;

			return {};
		}
	}

	SceneViewportPanel::SceneViewportPanel(
		const Ref<Scene>& scene,
		const Ref<SceneSelectionContext>& selectionContext,
		std::string windowName)
		: m_WindowName(std::move(windowName)),
		m_ViewportCamera(CreateUnique<ViewportCamera>()),
		m_SceneContext(scene),
		m_SelectionContext(selectionContext)
	{
		m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;
		InitRenderResources();
	}

	SceneViewportPanel::~SceneViewportPanel()
	{
		if (m_SceneTextureID)
		{
			Application::Get().GetVulkanContext().WaitIdle();
			ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(m_SceneTextureID)));
			m_SceneTextureID = 0;
		}
	}

	void SceneViewportPanel::InitRenderResources()
	{
		VulkanContext& context = Application::Get().GetVulkanContext();

		VulkanRenderTarget::CreateInfo renderTargetInfo{};
		renderTargetInfo.Name = m_WindowName + "_RenderTarget";
		renderTargetInfo.Width = static_cast<uint32_t>(m_ViewportSize.x);
		renderTargetInfo.Height = static_cast<uint32_t>(m_ViewportSize.y);
		renderTargetInfo.Samples = VK_SAMPLE_COUNT_1_BIT;

		VulkanRenderTarget::ColorAttachmentDesc colorAttachment{};
		colorAttachment.Name = m_WindowName + "_Color";
		colorAttachment.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.CreateSampler = true;
		colorAttachment.CreateResolveImage = false;
		colorAttachment.Filter = VK_FILTER_LINEAR;
		colorAttachment.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		renderTargetInfo.ColorAttachments.push_back(colorAttachment);

		renderTargetInfo.DepthAttachment.Enabled = true;
		renderTargetInfo.DepthAttachment.Name = m_WindowName + "_Depth";
		renderTargetInfo.DepthAttachment.Format = VK_FORMAT_D32_SFLOAT;
		renderTargetInfo.DepthAttachment.CreateSampler = false;

		m_Renderer = CreateUnique<VulkanRenderer>(context);
		m_RenderTarget = CreateUnique<VulkanRenderTarget>(context, renderTargetInfo);
		RecreateViewportTexture();
	}

	bool SceneViewportPanel::EnsureDemoRenderResources()
	{
		if (m_Pipeline && !m_DemoMeshes.empty())
			return true;

		VulkanContext& context = Application::Get().GetVulkanContext();

		if (m_DemoMeshes.empty())
		{
			m_DemoMeshes = TryLoadDemoMeshesFromAsset();
			if (m_DemoMeshes.empty())
			{
				if (!m_DemoMeshWarningIssued)
				{
					KITA_CORE_WARN(
						"SceneViewportPanel missing required mesh asset: {}. Viewport will render clear color only.",
						EditorProjectBootstrap::GetViewportDemoMeshPath().string());
					m_DemoMeshWarningIssued = true;
				}
				return false;
			}

			for (const Ref<Mesh>& mesh : m_DemoMeshes)
			{
				if (!mesh)
					continue;

				mesh->CreateVulkanGeometry(context);
				KITA_CORE_ASSERT(mesh->GetVulkanGeometry(), "Failed to create editor viewport Vulkan geometry");
			}
		}

		if (!m_VertexShader || !m_FragmentShader)
		{
			DemoShaderData shaderData = TryLoadDemoShaderAsset();
			if (!shaderData.IsValid())
			{
				if (!m_DemoShaderWarningIssued)
				{
					KITA_CORE_WARN(
						"SceneViewportPanel missing required shader asset: {}. Viewport will render clear color only.",
						EditorProjectBootstrap::GetViewportDemoShaderPath().string());
					m_DemoShaderWarningIssued = true;
				}
				return false;
			}

			VulkanShader::CreateInfo vertexShaderInfo{};
			vertexShaderInfo.Name = m_WindowName + "_VertexShader";
			vertexShaderInfo.Stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexShaderInfo.EntryPoint = shaderData.VertexEntryPoint;
			vertexShaderInfo.Spirv = shaderData.VertexSpirv;
			m_VertexShader = CreateUnique<VulkanShader>(&context, vertexShaderInfo);

			VulkanShader::CreateInfo fragmentShaderInfo{};
			fragmentShaderInfo.Name = m_WindowName + "_FragmentShader";
			fragmentShaderInfo.Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentShaderInfo.EntryPoint = shaderData.FragmentEntryPoint;
			fragmentShaderInfo.Spirv = shaderData.FragmentSpirv;
			m_FragmentShader = CreateUnique<VulkanShader>(&context, fragmentShaderInfo);
		}

		if (!m_VertexShader || !m_FragmentShader)
			return false;

		VulkanGraphicsPipeline::CreateInfo pipelineInfo{};
		pipelineInfo.Name = m_WindowName + "_Pipeline";
		pipelineInfo.VertexShader = m_VertexShader.get();
		pipelineInfo.FragmentShader = m_FragmentShader.get();
		pipelineInfo.Geometry = m_DemoMeshes.front()->GetVulkanGeometry().get();
		pipelineInfo.ColorFormat = m_RenderTarget->GetColorFormat(0);
		pipelineInfo.DepthFormat = m_RenderTarget->GetDepthFormat();
		pipelineInfo.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInfo.CullMode = VK_CULL_MODE_NONE;
		pipelineInfo.EnableDepthTest = true;
		pipelineInfo.EnableDepthWrite = true;
		pipelineInfo.EnableBlending = false;
		pipelineInfo.DescriptorSetLayout = m_Renderer->GetCameraDescriptorSet().GetLayout();

		m_Pipeline = CreateUnique<VulkanGraphicsPipeline>(context, pipelineInfo);
		return m_Pipeline != nullptr;
	}

	void SceneViewportPanel::Simulate(float daltaTime)
	{
		if (!m_ViewportCamera || !m_IsActive)
			return;

		m_ViewportCamera->OnUpdate(daltaTime);
	}

	void SceneViewportPanel::OnImGuiRender()
	{
		if (!m_SelectionContext || !m_SceneContext || !m_IsOpen)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		if (m_UseInitialPlacement)
		{
			const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
			const ImGuiID windowId = ImHashStr(m_WindowName.c_str());
			const float offsetX = 48.0f * static_cast<float>(windowId % 4u);
			const float offsetY = 40.0f * static_cast<float>((windowId / 4u) % 4u);

			ImGui::SetNextWindowDockID(0, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(
				ImVec2(mainViewport->WorkPos.x + 72.0f + offsetX, mainViewport->WorkPos.y + 72.0f + offsetY),
				ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(720.0f, 460.0f), ImGuiCond_FirstUseEver);
		}

		if (m_RequestWindowFocus)
			ImGui::SetNextWindowFocus();

		ImGui::Begin(m_WindowName.c_str());
		m_UseInitialPlacement = false;
		m_RequestWindowFocus = false;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (!window->DockIsActive || (window->DockNode && window->DockNode->VisibleWindow == window))
		{
			ImRect closeButtonAnchorRect{};
			if (window->DockIsActive && window->DockNode && window->DockNode->TabBar)
				closeButtonAnchorRect = window->DockNode->TabBar->BarRect;
			else
				closeButtonAnchorRect = window->TitleBarRect();

			if (DrawViewportCloseButton(closeButtonAnchorRect, window->GetID("#VIEWPORT_CLOSE")))
			{
				m_IsOpen = false;
				m_IsHovered = false;
				m_IsFocused = false;
				m_IsImageHovered = false;
				ImGui::End();
				ImGui::PopStyleVar();
				return;
			}
		}

		m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		if (viewportSize.x > 1.0f && viewportSize.y > 1.0f)
		{
			m_ViewportSize = { viewportSize.x, viewportSize.y };
			m_ViewportCamera->SetViewport(m_ViewportSize.x, m_ViewportSize.y);
		}

		const auto viewportRegionMin = ImGui::GetWindowContentRegionMin();
		const auto viewportRegionMax = ImGui::GetWindowContentRegionMax();
		const auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportRegionMin.x + viewportOffset.x, viewportRegionMin.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportRegionMax.x + viewportOffset.x, viewportRegionMax.y + viewportOffset.y };

		if (m_SceneTextureID)
		{
			ImGui::Image(
				m_SceneTextureID,
				ImVec2(m_ViewportSize.x, m_ViewportSize.y),
				ImVec2(0.0f, 1.0f),
				ImVec2(1.0f, 0.0f));
			m_IsImageHovered = ImGui::IsItemHovered();
		}
		else
		{
			ImGui::TextUnformatted("Viewport texture unavailable.");
			m_IsImageHovered = false;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewportPanel::OnEvent(Event& event)
	{
		if (!m_ViewportCamera)
			return;

		bool allowInput = false;
		if (event.IsInCategory(EventCategory::EventMouse))
			allowInput = m_IsActive && m_IsImageHovered;
		else if (event.IsInCategory(EventCategory::EventKeyboard))
			allowInput = m_IsActive && (m_IsFocused || m_IsImageHovered);
		else
			allowInput = m_IsActive && (m_IsImageHovered || m_IsFocused);

		if (!allowInput)
			return;

		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<KeyPressedEvent>(BIND_EVENT_FUNC(SceneViewportPanel::OnKeyPressed));
		dispatcher.Dispatcher<MouseButtonPressedEvent>(BIND_EVENT_FUNC(SceneViewportPanel::OnMouseButtonPressed));
		m_ViewportCamera->OnEvent(event);
	}

	void SceneViewportPanel::Render()
	{
		if (!m_RenderTarget)
			return;

		EnsureDemoRenderResources();
		ResizeRenderTargetIfNeeded();

		VkCommandBuffer commandBuffer = Application::Get().GetVulkanContext().GetCurrentCommandBuffer();
		if (commandBuffer == VK_NULL_HANDLE)
			return;

		RenderDemoMeshToTarget(commandBuffer);
	}

	void SceneViewportPanel::ResizeRenderTargetIfNeeded()
	{
		if (!m_RenderTarget)
			return;

		const uint32_t width = std::max(1u, static_cast<uint32_t>(m_ViewportSize.x));
		const uint32_t height = std::max(1u, static_cast<uint32_t>(m_ViewportSize.y));

		if (width == m_RenderTarget->GetWidth() && height == m_RenderTarget->GetHeight())
			return;

		VulkanContext& context = Application::Get().GetVulkanContext();
		context.WaitIdle();

		if (m_SceneTextureID)
		{
			ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(m_SceneTextureID)));
			m_SceneTextureID = 0;
		}

		m_RenderTarget->Resize(width, height);
		RecreateViewportTexture();
	}

	void SceneViewportPanel::RecreateViewportTexture()
	{
		if (m_SceneTextureID)
		{
			ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(m_SceneTextureID)));
			m_SceneTextureID = 0;
		}

		if (!m_RenderTarget)
			return;

		const VulkanImage& sampledImage = m_RenderTarget->GetSampledColorAttachment(0);
		KITA_CORE_ASSERT(sampledImage.HasSampler(), "Viewport sampled image has no sampler");

		m_SceneTextureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(ImGui_ImplVulkan_AddTexture(
			sampledImage.GetSampler(),
			sampledImage.GetView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));
	}

	void SceneViewportPanel::RenderDemoMeshToTarget(VkCommandBuffer commandBuffer)
	{
		if (!m_RenderTarget || !m_Renderer || !m_ViewportCamera)
			return;

		VulkanRenderer::CameraUBO cameraData{};
		cameraData.Matrix_V = m_ViewportCamera->GetViewMatrix();
		cameraData.Matrix_P = m_ViewportCamera->GetProjectionMatrix();
		cameraData.Matrix_VP = m_ViewportCamera->GetViewProjectionMatrix();
		cameraData.Matrix_I_V = glm::inverse(cameraData.Matrix_V);
		cameraData.Matrix_I_P = glm::inverse(cameraData.Matrix_P);
		cameraData.Matrix_I_VP = glm::inverse(cameraData.Matrix_VP);
		cameraData.CameraPosWS = glm::vec4(m_ViewportCamera->GetPosition(), 1.0f);

		const glm::vec4 clearColor(0.03f, 0.032f, 0.034f, 1.0f);

		m_Renderer->BeginScene(commandBuffer, *m_RenderTarget, cameraData, clearColor);

		if (m_Pipeline && !m_DemoMeshes.empty())
		{
			for (const Ref<Mesh>& mesh : m_DemoMeshes)
			{
				if (!mesh || !mesh->GetVulkanGeometry())
					continue;

				VulkanRenderer::ObjectData objectData{};
				objectData.Matrix_M = glm::mat4(1.0f);
				objectData.Matrix_I_M = glm::inverse(objectData.Matrix_M);

				m_Renderer->SubmitMesh(commandBuffer, *m_Pipeline, *mesh->GetVulkanGeometry(), objectData);
			}
		}

		m_Renderer->EndScene(commandBuffer, *m_RenderTarget);
	}

	bool SceneViewportPanel::OnKeyPressed(KeyPressedEvent& event)
	{
		if (event.IsRepeat())
			return false;

		switch (event.GetKeyCode())
		{
		case Key::W:
			m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			m_GizmoControlType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			m_GizmoControlType = ImGuizmo::OPERATION::SCALE;
			break;
		}

		return false;
	}

	bool SceneViewportPanel::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (m_IsActive && event.GetMouseButton() == Mouse::Button0 && IsMouseInsideImageBounds())
			TryPickObject();

		return false;
	}

	bool SceneViewportPanel::IsMouseInsideImageBounds() const
	{
		auto [mx, my] = ImGui::GetMousePos();
		return mx >= m_ViewportBounds[0].x && mx < m_ViewportBounds[1].x
			&& my >= m_ViewportBounds[0].y && my < m_ViewportBounds[1].y;
	}

	void SceneViewportPanel::TryPickObject()
	{
		// Legacy picking depends on the removed framebuffer/id-buffer path.
		// Keep selection untouched until a Vulkan picking pass is introduced.
	}

}
