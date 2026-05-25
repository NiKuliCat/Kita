#include "renderer_pch.h"
#include "SceneRenderSettingsPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		uint32_t CalculateSkyboxMipCount(AssetHandle textureHandle)
		{
			if (!Asset::IsValidHandle(textureHandle))
				return 1;

			Ref<TextureAsset> textureAsset = AssetManager::GetInstance().GetTextureAsset(textureHandle);
			if (!textureAsset || !textureAsset->TexRawData.IsValid())
				return 1;

			uint32_t faceSize = 1;
			if (textureAsset->IsCube())
			{
				const uint32_t widthBased = std::max(1u, textureAsset->TexRawData.Width / 4u);
				const uint32_t heightBased = std::max(1u, textureAsset->TexRawData.Height / 2u);
				faceSize = std::max(1u, std::min(widthBased, heightBased));
			}
			else
			{
				faceSize = std::max(1u, std::min(textureAsset->TexRawData.Width, textureAsset->TexRawData.Height));
			}

			if (!textureAsset->ImportSettings.GenerateMipmaps)
				return 1;

			return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(faceSize)))) + 1;
		}

		const char* GetEnvironmentSourceTypeLabel(EnvironmentSourceType sourceType)
		{
			switch (sourceType)
			{
			case EnvironmentSourceType::Equirectangular:
				return "Equirectangular";
			case EnvironmentSourceType::Cubemap:
				return "Cubemap";
			default:
				return "Unknown";
			}
		}
	}

	void SceneRenderSettingsPanel::OnImGuiRender()
	{
		if (!m_IsOpen)
			return;

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		if (!ImGui::Begin("Scene Render Settings", &m_IsOpen))
		{
			ImGui::End();
			return;
		}

		if (!m_Scene)
		{
			ImGui::TextDisabled("No active scene.");
			ImGui::End();
			return;
		}

		SceneRenderSettings& settings = m_Scene->GetRenderSettings();
		DrawSkyboxSection(settings);
		DrawEnvironmentSection(settings);

		ImGui::Spacing();
		if (ImGui::Button("Bake IBL"))
		{
			if (m_BakeCallback)
				m_BakeCallback();
		}

		ImGui::SameLine();
		ImGui::TextDisabled("%s", m_StatusText.empty() ? "Ready." : m_StatusText.c_str());

		ImGui::End();
	}

	void SceneRenderSettingsPanel::DrawSkyboxSection(SceneRenderSettings& settings)
	{
		if (!ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		const std::vector<AssetMetadata> textureAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Texture);

		if (UIAttributeUtil::BeginPropertyTable("##SceneRenderSettings_Skybox", m_TableStyle))
		{
			DrawSkyboxTextureRow(settings, textureAssets);
			DrawFloatRow("Intensity", settings.SkyboxIntensity, 0.05f, 0.0f, 32.0f, 1.0f);
			DrawFloatRow("Rotation Y", settings.SkyboxRotationY, 0.01f, -6.2831853f, 6.2831853f, 0.0f);
			const uint32_t mipCount = CalculateSkyboxMipCount(
				Asset::IsValidHandle(settings.SkyboxTextureHandle)
					? settings.SkyboxTextureHandle
					: settings.EnvironmentSourceTexHandle);
			const float maxMipLevel = mipCount > 0 ? static_cast<float>(mipCount - 1) : 0.0f;
			settings.SkyboxMipLevel = std::clamp(settings.SkyboxMipLevel, 0.0f, maxMipLevel);
			DrawFloatRow("Mip Level", settings.SkyboxMipLevel, 0.05f, 0.0f, maxMipLevel, 0.0f);
			UIAttributeUtil::EndPropertyTable();
		}
	}

	void SceneRenderSettingsPanel::DrawEnvironmentSection(SceneRenderSettings& settings)
	{
		if (!ImGui::CollapsingHeader("Environment IBL", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		const std::vector<AssetMetadata> textureAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Texture);

		if (UIAttributeUtil::BeginPropertyTable("##SceneRenderSettings_IBL", m_TableStyle))
		{
			DrawTextureSourceRow(settings, textureAssets);
			DrawSourceTypeRow(settings);
			DrawCubeSizeRow("Environment Cube", settings.EnvironmentCubeSize, 512, 16, 4096);
			DrawCubeSizeRow("Irradiance Cube", settings.IrradianceCubeSize, 32, 4, 512);
			DrawCubeSizeRow("Prefilter Cube", settings.PrefilterCubeSize, 128, 16, 2048);
			DrawCubeSizeRow("DFG LUT", settings.DfgLutSize, 256, 16, 2048);
			DrawFloatRow("Intensity", settings.Intensity, 0.05f, 0.0f, 32.0f, 1.0f);
			DrawFloatRow("Rotation Y", settings.RotationY, 0.01f, -6.2831853f, 6.2831853f, 0.0f);
			UIAttributeUtil::EndPropertyTable();
		}
	}

	void SceneRenderSettingsPanel::DrawSkyboxTextureRow(SceneRenderSettings& settings, const std::vector<AssetMetadata>& textureAssets)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell("Skybox Texture", m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(
			m_TableStyle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, m_TableStyle.PreviewTileSize));

		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float tileSize = m_TableStyle.PreviewTileSize;
		const float comboSpacing = 10.0f;
		const float comboWidth = ImMax(120.0f, valueWidth - tileSize - comboSpacing);
		const ImVec2 tileSizeVec(tileSize, tileSize);

		ImGui::PushID("SceneRenderSettings_SkyboxTexture");
		ImGui::InvisibleButton("##SkyboxTexturePreview", tileSizeVec);
		const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(68, 68, 68, 255));
		drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);
		drawList->AddText(
			ImVec2(thumbRect.Min.x + 7.0f, thumbRect.Min.y + 7.0f),
			IM_COL32(185, 185, 185, 255),
			"CUBE");

		ImGui::SameLine(0.0f, comboSpacing);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		UIAttributeUtil::DrawAssetCombo(
			"##SkyboxTextureSelector",
			settings.SkyboxTextureHandle,
			textureAssets,
			comboWidth,
			UIAttributeUtil::AssetLabelMode::FileName);
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(
			"SceneRenderSettings_SkyboxTexture",
			m_TableStyle,
			settings.SkyboxTextureHandle != InvalidAssetHandle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight)))
		{
			settings.SkyboxTextureHandle = InvalidAssetHandle;
		}
	}

	void SceneRenderSettingsPanel::DrawTextureSourceRow(SceneRenderSettings& settings, const std::vector<AssetMetadata>& textureAssets)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell("Source Texture", m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(
			m_TableStyle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, m_TableStyle.PreviewTileSize));

		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float tileSize = m_TableStyle.PreviewTileSize;
		const float comboSpacing = 10.0f;
		const float comboWidth = ImMax(120.0f, valueWidth - tileSize - comboSpacing);
		const ImVec2 tileSizeVec(tileSize, tileSize);

		ImGui::PushID("SceneRenderSettings_SourceTexture");
		ImGui::InvisibleButton("##SourceTexturePreview", tileSizeVec);
		const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(68, 68, 68, 255));
		drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);
		drawList->AddText(
			ImVec2(thumbRect.Min.x + 7.0f, thumbRect.Min.y + 7.0f),
			IM_COL32(185, 185, 185, 255),
			"HDR");

		ImGui::SameLine(0.0f, comboSpacing);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		UIAttributeUtil::DrawAssetCombo(
			"##SourceTextureSelector",
			settings.EnvironmentSourceTexHandle,
			textureAssets,
			comboWidth,
			UIAttributeUtil::AssetLabelMode::FileName);
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(
			"SceneRenderSettings_SourceTexture",
			m_TableStyle,
			settings.EnvironmentSourceTexHandle != InvalidAssetHandle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight)))
		{
			settings.EnvironmentSourceTexHandle = InvalidAssetHandle;
		}
	}

	void SceneRenderSettingsPanel::DrawSourceTypeRow(SceneRenderSettings& settings)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell("Source Type", m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;

		ImGui::PushID("SceneRenderSettings_SourceType");
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::SetNextItemWidth(valueWidth);
		if (ImGui::BeginCombo("##SourceType", GetEnvironmentSourceTypeLabel(settings.SourceType)))
		{
			const bool isEquirectangular = settings.SourceType == EnvironmentSourceType::Equirectangular;
			if (ImGui::Selectable("Equirectangular", isEquirectangular))
				settings.SourceType = EnvironmentSourceType::Equirectangular;
			if (isEquirectangular)
				ImGui::SetItemDefaultFocus();

			const bool isCubemap = settings.SourceType == EnvironmentSourceType::Cubemap;
			if (ImGui::Selectable("Cubemap", isCubemap))
				settings.SourceType = EnvironmentSourceType::Cubemap;
			if (isCubemap)
				ImGui::SetItemDefaultFocus();

			ImGui::EndCombo();
		}
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(
			"SceneRenderSettings_SourceType",
			m_TableStyle,
			settings.SourceType != EnvironmentSourceType::Equirectangular))
		{
			settings.SourceType = EnvironmentSourceType::Equirectangular;
		}
	}

	void SceneRenderSettingsPanel::DrawCubeSizeRow(
		const char* label,
		uint32_t& value,
		uint32_t defaultValue,
		uint32_t minValue,
		uint32_t maxValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		int currentValue = static_cast<int>(value);
		const int minInt = static_cast<int>(minValue);
		const int maxInt = static_cast<int>(maxValue);

		ImGui::PushID(label);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		if (ImGui::SliderInt("##Value", &currentValue, minInt, maxInt))
		{
			value = static_cast<uint32_t>(currentValue);
		}
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, value != defaultValue))
		{
			value = defaultValue;
		}
	}

	void SceneRenderSettingsPanel::DrawFloatRow(
		const char* label,
		float& value,
		float speed,
		float minValue,
		float maxValue,
		float defaultValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		ImGui::PushID(label);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		ImGui::DragFloat("##Value", &value, speed, minValue, maxValue, "%.3f");
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, value != defaultValue))
		{
			value = defaultValue;
		}
	}

}
