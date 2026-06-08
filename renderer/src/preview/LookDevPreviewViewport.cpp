#include "renderer_pch.h"
#include "LookDevPreviewViewport.h"

namespace Kita {

	LookDevPreviewViewport::LookDevPreviewViewport(PreviewSceneRenderer* previewRenderer)
		: m_PreviewRenderer(previewRenderer)
	{
	}

	LookDevPreviewViewport::~LookDevPreviewViewport()
	{
		if (m_Surface)
		{
			m_Surface->Destroy();
			m_Surface.reset();
		}
	}

	void LookDevPreviewViewport::SetMaterialDefinitionAsset(AssetHandle handle)
	{
		m_Mode = Asset::IsValidHandle(handle) ? Mode::MaterialDefinition : Mode::None;
		m_AssetHandle = handle;
		m_SurfaceName = "LookDevPreview_MaterialDefinition_" + std::to_string(handle);
	}

	void LookDevPreviewViewport::SetMaterialAsset(AssetHandle handle)
	{
		m_Mode = Asset::IsValidHandle(handle) ? Mode::Material : Mode::None;
		m_AssetHandle = handle;
		m_SurfaceName = "LookDevPreview_Material_" + std::to_string(handle);
	}

	void LookDevPreviewViewport::SetCubemapAsset(AssetHandle handle)
	{
		m_Mode = Asset::IsValidHandle(handle) ? Mode::Cubemap : Mode::None;
		m_AssetHandle = handle;
		m_SurfaceName = "LookDevPreview_Cubemap_" + std::to_string(handle);
	}

	void LookDevPreviewViewport::ClearMode()
	{
		m_Mode = Mode::None;
		m_AssetHandle = InvalidAssetHandle;
	}

	void LookDevPreviewViewport::OnRender()
	{
		if (!m_PreviewRenderer || !m_Surface || m_Mode == Mode::None || !Asset::IsValidHandle(m_AssetHandle))
		{
			return;
		}

		switch (m_Mode)
		{
		case Mode::MaterialDefinition:
			m_PreviewRenderer->RenderMaterialDefinitionPreview(m_AssetHandle, *m_Surface, m_CameraState);
			break;
		case Mode::Material:
			m_PreviewRenderer->RenderMaterialPreview(m_AssetHandle, *m_Surface, m_CameraState);
			break;
		case Mode::Cubemap:
			m_PreviewRenderer->RenderCubemapPreview(m_AssetHandle, *m_Surface, m_CameraState);
			break;
		case Mode::None:
		default:
			break;
		}
	}

	void LookDevPreviewViewport::Draw(const char* id, const ImVec2& size)
	{
		const uint32_t width = static_cast<uint32_t>(std::max(1.0f, std::round(size.x)));
		const uint32_t height = static_cast<uint32_t>(std::max(1.0f, std::round(size.y)));
		m_LastDrawSize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		EnsureSurface(width, height);

		if (m_Surface && m_Surface->GetTextureID())
		{
			ImGui::Image(m_Surface->GetTextureID(), ImVec2(static_cast<float>(width), static_cast<float>(height)));
		}
		else
		{
			ImGui::InvisibleButton(id, ImVec2(static_cast<float>(width), static_cast<float>(height)));
		}

		HandleInteraction();
	}

	bool LookDevPreviewViewport::HasValidOutput() const
	{
		return m_Surface && m_Surface->GetTextureID() != 0;
	}

	void LookDevPreviewViewport::EnsureSurface(uint32_t width, uint32_t height)
	{
		width = std::max(1u, width);
		height = std::max(1u, height);

		if (!m_Surface)
		{
			EditorViewportSurface::CreateInfo createInfo{};
			createInfo.Name = m_SurfaceName;
			createInfo.Width = width;
			createInfo.Height = height;
			m_Surface = CreateUnique<EditorViewportSurface>(Application::Get().GetVulkanContext(), createInfo);
			return;
		}

		m_Surface->EnsureSize(width, height);
	}

	void LookDevPreviewViewport::HandleInteraction()
	{
		if (!ImGui::IsItemHovered())
		{
			return;
		}

		const ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			m_CameraState.YawDegrees -= dragDelta.x * 0.12f;
			m_CameraState.PitchDegrees -= dragDelta.y * 0.12f;
			m_CameraState.PitchDegrees = std::clamp(m_CameraState.PitchDegrees, -80.0f, 80.0f);
			ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
		}

		const float mouseWheel = ImGui::GetIO().MouseWheel;
		if (std::abs(mouseWheel) > 0.0001f)
		{
			m_CameraState.Distance = std::clamp(m_CameraState.Distance - mouseWheel * 0.25f, 1.0f, 8.0f);
		}
	}

}
