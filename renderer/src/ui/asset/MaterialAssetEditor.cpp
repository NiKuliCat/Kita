#include "renderer_pch.h"
#include "MaterialAssetEditor.h"

#include "AssetDragDrop.h"
#include "AssetEditorUtils.h"
#include "file/Project.h"
#include "render/VulkanResourceFactory.h"

#include "imgui.h"
#include <imgui_internal.h>
#include "serialize/MaterialSerializer.h"

namespace Kita {

	namespace
	{
		constexpr float materialPropertyRowHeight = 30.0f;
		constexpr float materialToolbarHeight = 48.0f;
		constexpr float materialBodyHorizontalPadding = 12.0f;
		constexpr float kMaterialThumbCheckerCellSize = 8.0f;
		const ImVec4 materialContentBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 materialBarBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 materialHeaderAccentColor = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);

		void DrawCheckerboard(
			ImDrawList* drawList,
			const ImVec2& min,
			const ImVec2& max,
			float cellSize,
			ImU32 colorA,
			ImU32 colorB)
		{
			if (!drawList || cellSize <= 0.0f || max.x <= min.x || max.y <= min.y)
			{
				return;
			}

			for (float y = min.y; y < max.y; y += cellSize)
			{
				for (float x = min.x; x < max.x; x += cellSize)
				{
					const int cellX = static_cast<int>((x - min.x) / cellSize);
					const int cellY = static_cast<int>((y - min.y) / cellSize);
					const ImU32 color = ((cellX + cellY) & 1) == 0 ? colorA : colorB;
					drawList->AddRectFilled(
						ImVec2(x, y),
						ImVec2(ImMin(x + cellSize, max.x), ImMin(y + cellSize, max.y)),
						color);
				}
			}
		}
	}

	MaterialAssetEditor::MaterialAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache, VulkanResourceFactory* resourceFactory)
		: m_AssetHandle(handle)
		, m_ThumbnailCache(thumbnailCache)
		, m_ResourceFactory(resourceFactory)
	{
		m_SourceAsset = AssetManager::GetInstance().GetMaterialAsset(handle);
		m_DisplayName = GetAssetEditorDisplayName(handle);
		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
		{
			const Ref<Project> project = Project::GetActive();
			if (project)
			{
				m_AssetPath = project->GetAssetRootDirectory() / metadata->relativePath;
			}
		}

		if (m_SourceAsset)
		{
			m_WorkingCopy = *m_SourceAsset;
			m_SavedCopy = *m_SourceAsset;
		}

		m_TableStyle.PreviewRowHeight = 80.0f;
		m_TableStyle.PreviewTileSize = 65.0f;
	}

	bool MaterialAssetEditor::IsDirty() const
	{
		return IsWorkingCopyDirty();
	}

	void MaterialAssetEditor::Save()
	{
		if (!m_SourceAsset || m_AssetPath.empty())
		{
			return;
		}

		SyncWorkingCopyToAssetData();

		if (MaterialSerializer::Serialize(m_AssetPath, *m_SourceAsset))
		{
			m_SavedCopy = *m_SourceAsset;
		}
	}

	void MaterialAssetEditor::Revert()
	{
		m_WorkingCopy = m_SavedCopy;
		SyncWorkingCopyToAssetData();
	}

	void MaterialAssetEditor::OnImGuiRender()
	{
		DrawToolbar();

		const float bodyHeight = ImMax(0.0f, ImGui::GetContentRegionAvail().y);
		const ImGuiTableFlags layoutFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, materialContentBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(materialBodyHorizontalPadding, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		if (ImGui::BeginChild("##MaterialEditorBody", ImVec2(0.0f, bodyHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			if (ImGui::BeginTable("##MaterialEditorLayout", 2, layoutFlags, ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.70f);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.30f);
				ImGui::TableNextRow(ImGuiTableRowFlags_None, bodyHeight);

				ImGui::TableSetColumnIndex(0);
				DrawPreview();

				ImGui::TableSetColumnIndex(1);
				DrawDetails();

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	void MaterialAssetEditor::DrawToolbar()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, materialBarBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##MaterialEditorToolbar", ImVec2(0.0f, materialToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 headerMin = ImGui::GetWindowPos();
		const ImVec2 headerMax(headerMin.x + ImGui::GetWindowSize().x, headerMin.y + ImGui::GetWindowSize().y);
		drawList->AddRectFilled(headerMin, ImVec2(headerMax.x, headerMin.y + 2.0f), ImGui::ColorConvertFloat4ToU32(materialHeaderAccentColor));

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Material");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", m_DisplayName.c_str());

		const float buttonWidth = 74.0f;
		ImGui::SameLine(ImMax(0.0f, ImGui::GetContentRegionAvail().x - buttonWidth * 2.0f - 12.0f));

		const bool canSaveNow = CanSave() && IsDirty();
		if (!canSaveNow)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Save", ImVec2(buttonWidth, 0.0f)))
		{
			Save();
		}
		if (!canSaveNow)
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		const bool canRevertNow = IsDirty();
		if (!canRevertNow)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Revert", ImVec2(buttonWidth, 0.0f)))
		{
			Revert();
		}
		if (!canRevertNow)
		{
			ImGui::EndDisabled();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	void MaterialAssetEditor::DrawPreview()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 12.0f));
		ImGui::BeginChild("##MaterialPreviewPane", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImGui::TextUnformatted("Preview");
		ImGui::Dummy(ImVec2(0.0f, 10.0f));

		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float previewSize = ImMin(available.x, available.y);
		const float clampedPreviewSize = ImClamp(previewSize, 180.0f, 340.0f);
		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		const ImVec2 previewMin(
			cursor.x + ImMax(0.0f, (available.x - clampedPreviewSize) * 0.5f),
			cursor.y + ImMax(0.0f, (available.y - clampedPreviewSize) * 0.18f));
		const ImVec2 previewMax(previewMin.x + clampedPreviewSize, previewMin.y + clampedPreviewSize);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(previewMin, previewMax, IM_COL32(26, 26, 26, 255), 6.0f);
		drawList->AddRect(previewMin, previewMax, IM_COL32(12, 12, 12, 255), 6.0f, 0, 1.0f);

		const ImVec2 center((previewMin.x + previewMax.x) * 0.5f, (previewMin.y + previewMax.y) * 0.5f);
		const float radius = clampedPreviewSize * 0.30f;
		drawList->AddCircleFilled(center, radius, IM_COL32(118, 118, 118, 255), 48);
		drawList->AddCircle(center, radius, IM_COL32(38, 38, 38, 255), 48, 2.0f);
		drawList->AddCircleFilled(
			ImVec2(center.x - radius * 0.28f, center.y - radius * 0.28f),
			radius * 0.42f,
			IM_COL32(210, 210, 210, 55),
			32);
		drawList->AddText(
			ImVec2(previewMin.x + 14.0f, previewMin.y + 12.0f),
			IM_COL32(168, 168, 168, 255),
			"TODO: Material sphere viewport");

		ImGui::Dummy(ImVec2(available.x, clampedPreviewSize + 24.0f));
		ImGui::EndChild();
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
	}

	void MaterialAssetEditor::DrawDetails()
	{
		if (!m_SourceAsset)
		{
			ImGui::TextUnformatted("Material asset is unavailable.");
			return;
		}

		ImGui::BeginChild("##MaterialDetailsPane", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		if (UIAttributeUtil::BeginPropertyTable("##MaterialPropertyTable", m_TableStyle))
		{
			DrawAssetRow("Shader", "##MaterialShader", m_WorkingCopy.ShaderHandle, AssetType::Shader, m_SavedCopy.ShaderHandle);
			DrawTextureSlotRow("Albedo", 0, m_WorkingCopy.m_Textures.Albedo, m_SavedCopy.m_Textures.Albedo);
			DrawTextureSlotRow("Normal", 1, m_WorkingCopy.m_Textures.Normal, m_SavedCopy.m_Textures.Normal);
			DrawTextureSlotRow("Metal/Rough", 2, m_WorkingCopy.m_Textures.MetallicRoughness, m_SavedCopy.m_Textures.MetallicRoughness);
			DrawTextureSlotRow("AO", 3, m_WorkingCopy.m_Textures.AmbientOcclusion, m_SavedCopy.m_Textures.AmbientOcclusion);
			DrawTextureSlotRow("Emissive", 4, m_WorkingCopy.m_Textures.Emissive, m_SavedCopy.m_Textures.Emissive);
			DrawTextureSlotRow("Opacity", 5, m_WorkingCopy.m_Textures.Opacity, m_SavedCopy.m_Textures.Opacity);
			DrawColorRow("Base Color", "##MaterialBaseColor", m_WorkingCopy.m_SurfaceParams.BaseColor, m_SavedCopy.m_SurfaceParams.BaseColor);
			DrawColorRow("Emissive", "##MaterialEmissive", m_WorkingCopy.m_SurfaceParams.Emissive, m_SavedCopy.m_SurfaceParams.Emissive);
			DrawFloatRow("Metallic", "##MaterialMetallic", m_WorkingCopy.m_SurfaceParams.Metallic, m_SavedCopy.m_SurfaceParams.Metallic, 0.01f, 0.0f, 1.0f);
			DrawFloatRow("Roughness", "##MaterialRoughness", m_WorkingCopy.m_SurfaceParams.Roughness, m_SavedCopy.m_SurfaceParams.Roughness, 0.01f, 0.0f, 1.0f);
			DrawFloatRow("AO Strength", "##MaterialAO", m_WorkingCopy.m_SurfaceParams.AmbientOcclusion, m_SavedCopy.m_SurfaceParams.AmbientOcclusion, 0.01f, 0.0f, 1.0f);
			DrawFloatRow("Opacity", "##MaterialOpacity", m_WorkingCopy.m_SurfaceParams.Opacity, m_SavedCopy.m_SurfaceParams.Opacity, 0.01f, 0.0f, 1.0f);
			DrawFloatRow("Normal Scale", "##MaterialNormalScale", m_WorkingCopy.m_SurfaceParams.NormalScale, m_SavedCopy.m_SurfaceParams.NormalScale, 0.01f, 0.0f, 8.0f);
			DrawFloatRow("Alpha Cutoff", "##MaterialAlphaCutoff", m_WorkingCopy.m_SurfaceParams.AlphaCutoff, m_SavedCopy.m_SurfaceParams.AlphaCutoff, 0.01f, 0.0f, 1.0f);
			UIAttributeUtil::EndPropertyTable();
		}
		ImGui::EndChild();
	}

	void MaterialAssetEditor::DrawAssetRow(const char* label, const char* comboId, AssetHandle& handle, AssetType type, AssetHandle resetValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight));

		const std::vector<AssetMetadata> assets = AssetManager::GetInstance().GetAssetsByType(type);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		const bool changed = UIAttributeUtil::DrawAssetCombo(
			comboId,
			handle,
			assets,
			ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset,
			UIAttributeUtil::AssetLabelMode::FileName);
		UIAttributeUtil::PopInputStyle();

		if (changed)
		{
			SyncWorkingCopyToAssetData();
		}

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			handle != resetValue,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight)))
		{
			handle = resetValue;
			SyncWorkingCopyToAssetData();
		}
	}

	void MaterialAssetEditor::DrawTextureSlotRow(const char* label, size_t slotIndex, AssetHandle& handle, AssetHandle resetValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle, m_TableStyle.PreviewRowHeight);

		const float contentHeight = UIAttributeUtil::GetContentHeight(m_TableStyle, m_TableStyle.PreviewRowHeight);
		const ImVec2 compactFramePadding(6.0f, 1.0f);
		const float compactFrameHeight = ImGui::GetFontSize() + compactFramePadding.y * 2.0f;
		const float comboYOffset = UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, compactFrameHeight);
		const float resetYOffset = UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, comboYOffset);

		ImGui::PushID(static_cast<int>(slotIndex));

		const std::vector<AssetMetadata> textureAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Texture);
		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float tileSize = m_TableStyle.PreviewTileSize;
		const float thumbYOffset = ImMax(0.0f, (contentHeight - tileSize) * 0.5f);
		const float comboSpacing = 10.0f;
		const float comboWidth = ImMax(80.0f, valueWidth - tileSize - comboSpacing);
		const ImVec2 tileSizeVec(tileSize, tileSize);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - comboYOffset + thumbYOffset);
		ImGui::InvisibleButton("##TextureThumbnail", tileSizeVec);
		const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(32, 32, 32, 255));
		drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);

		const ThumbnailCache::ThumbnailHandle thumbnail =
			m_ThumbnailCache ? m_ThumbnailCache->GetOrCreate(handle, AssetType::Texture) : ThumbnailCache::ThumbnailHandle{};
		if (thumbnail.IsValid())
		{
			DrawCheckerboard(
				drawList,
				thumbRect.Min,
				thumbRect.Max,
				kMaterialThumbCheckerCellSize,
				IM_COL32(74, 74, 74, 255),
				IM_COL32(108, 108, 108, 255));
			drawList->AddImage(thumbnail.TextureID, thumbRect.Min, thumbRect.Max, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
		}
		else
		{
			drawList->AddLine(
				ImVec2(thumbRect.Min.x + 6.0f, thumbRect.Max.y - 6.0f),
				ImVec2(thumbRect.Max.x - 6.0f, thumbRect.Min.y + 6.0f),
				IM_COL32(105, 105, 105, 255),
				1.0f);
			drawList->AddText(
				ImVec2(thumbRect.Min.x + 6.0f, thumbRect.Min.y + 7.0f),
				IM_COL32(180, 180, 180, 255),
				"TEX");
		}

		ImGui::SameLine(0.0f, comboSpacing);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - thumbYOffset + comboYOffset);

		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, compactFramePadding);
		const bool changed = UIAttributeUtil::DrawAssetCombo(
			"##TextureSelector",
			handle,
			textureAssets,
			comboWidth,
			UIAttributeUtil::AssetLabelMode::FileName);
		ImGui::PopStyleVar();
		UIAttributeUtil::PopInputStyle();

		if (changed)
		{
			SyncWorkingCopyToAssetData();
		}

		const ImRect slotRect(
			thumbRect.Min,
			ImVec2(ImGui::GetItemRectMax().x, ImMax(thumbRect.Max.y, ImGui::GetItemRectMax().y)));
		if (ImGui::BeginDragDropTargetCustom(slotRect, ImGui::GetID("##TextureSlotDropTarget")))
		{
			drawList->AddRect(slotRect.Min, slotRect.Max, IM_COL32(90, 140, 220, 255), 0.0f, 0, 2.0f);
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetDragDropPayloadType))
			{
				if (payload->DataSize == sizeof(AssetDragDropPayload))
				{
					const AssetDragDropPayload& assetPayload = *static_cast<const AssetDragDropPayload*>(payload->Data);
					if (assetPayload.Type == AssetType::Texture && Asset::IsValidHandle(assetPayload.Handle))
					{
						handle = assetPayload.Handle;
						SyncWorkingCopyToAssetData();
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, handle != resetValue, resetYOffset))
		{
			handle = resetValue;
			SyncWorkingCopyToAssetData();
		}
	}

	void MaterialAssetEditor::DrawColorRow(const char* label, const char* colorId, glm::vec4& value, const glm::vec4& resetValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight));

		ImGui::PushStyleColor(ImGuiCol_Border, m_TableStyle.InputBorderColor);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		ImGui::ColorEdit4(colorId, &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		if (ImGui::IsItemEdited())
		{
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleColor();

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			value != resetValue,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight)))
		{
			value = resetValue;
			SyncWorkingCopyToAssetData();
		}
	}

	void MaterialAssetEditor::DrawColorRow(const char* label, const char* colorId, glm::vec3& value, const glm::vec3& resetValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight));

		ImGui::PushStyleColor(ImGuiCol_Border, m_TableStyle.InputBorderColor);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		ImGui::ColorEdit3(colorId, &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float);
		if (ImGui::IsItemEdited())
		{
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleColor();

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			value != resetValue,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight)))
		{
			value = resetValue;
			SyncWorkingCopyToAssetData();
		}
	}

	void MaterialAssetEditor::DrawFloatRow(
		const char* label,
		const char* valueId,
		float& value,
		float resetValue,
		float speed,
		float minValue,
		float maxValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight));

		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		if (ImGui::DragFloat(valueId, &value, speed, minValue, maxValue, "%.3f"))
		{
			SyncWorkingCopyToAssetData();
		}
		UIAttributeUtil::PopInputStyle();

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			value != resetValue,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight)))
		{
			value = resetValue;
			SyncWorkingCopyToAssetData();
		}
	}

	bool MaterialAssetEditor::IsWorkingCopyDirty() const
	{
		return m_WorkingCopy.ShaderHandle != m_SavedCopy.ShaderHandle ||
			m_WorkingCopy.m_Textures.Albedo != m_SavedCopy.m_Textures.Albedo ||
			m_WorkingCopy.m_Textures.Normal != m_SavedCopy.m_Textures.Normal ||
			m_WorkingCopy.m_Textures.MetallicRoughness != m_SavedCopy.m_Textures.MetallicRoughness ||
			m_WorkingCopy.m_Textures.AmbientOcclusion != m_SavedCopy.m_Textures.AmbientOcclusion ||
			m_WorkingCopy.m_Textures.Emissive != m_SavedCopy.m_Textures.Emissive ||
			m_WorkingCopy.m_Textures.Opacity != m_SavedCopy.m_Textures.Opacity ||
			m_WorkingCopy.m_SurfaceParams.BaseColor != m_SavedCopy.m_SurfaceParams.BaseColor ||
			m_WorkingCopy.m_SurfaceParams.Emissive != m_SavedCopy.m_SurfaceParams.Emissive ||
			m_WorkingCopy.m_SurfaceParams.Metallic != m_SavedCopy.m_SurfaceParams.Metallic ||
			m_WorkingCopy.m_SurfaceParams.Roughness != m_SavedCopy.m_SurfaceParams.Roughness ||
			m_WorkingCopy.m_SurfaceParams.AmbientOcclusion != m_SavedCopy.m_SurfaceParams.AmbientOcclusion ||
			m_WorkingCopy.m_SurfaceParams.Opacity != m_SavedCopy.m_SurfaceParams.Opacity ||
			m_WorkingCopy.m_SurfaceParams.NormalScale != m_SavedCopy.m_SurfaceParams.NormalScale ||
			m_WorkingCopy.m_SurfaceParams.AlphaCutoff != m_SavedCopy.m_SurfaceParams.AlphaCutoff;
	}

	void MaterialAssetEditor::SyncWorkingCopyToAssetData()
	{
		if (!m_SourceAsset)
		{
			return;
		}

		m_SourceAsset->ShaderHandle = m_WorkingCopy.ShaderHandle;
		m_SourceAsset->m_Textures.Albedo = m_WorkingCopy.m_Textures.Albedo;
		m_SourceAsset->m_Textures.Normal = m_WorkingCopy.m_Textures.Normal;
		m_SourceAsset->m_Textures.MetallicRoughness = m_WorkingCopy.m_Textures.MetallicRoughness;
		m_SourceAsset->m_Textures.AmbientOcclusion = m_WorkingCopy.m_Textures.AmbientOcclusion;
		m_SourceAsset->m_Textures.Emissive = m_WorkingCopy.m_Textures.Emissive;
		m_SourceAsset->m_Textures.Opacity = m_WorkingCopy.m_Textures.Opacity;
		m_SourceAsset->m_SurfaceParams.BaseColor = m_WorkingCopy.m_SurfaceParams.BaseColor;
		m_SourceAsset->m_SurfaceParams.Emissive = m_WorkingCopy.m_SurfaceParams.Emissive;
		m_SourceAsset->m_SurfaceParams.Metallic = m_WorkingCopy.m_SurfaceParams.Metallic;
		m_SourceAsset->m_SurfaceParams.Roughness = m_WorkingCopy.m_SurfaceParams.Roughness;
		m_SourceAsset->m_SurfaceParams.AmbientOcclusion = m_WorkingCopy.m_SurfaceParams.AmbientOcclusion;
		m_SourceAsset->m_SurfaceParams.Opacity = m_WorkingCopy.m_SurfaceParams.Opacity;
		m_SourceAsset->m_SurfaceParams.NormalScale = m_WorkingCopy.m_SurfaceParams.NormalScale;
		m_SourceAsset->m_SurfaceParams.AlphaCutoff = m_WorkingCopy.m_SurfaceParams.AlphaCutoff;

		if (m_ResourceFactory && Asset::IsValidHandle(m_AssetHandle))
		{
			m_ResourceFactory->RefreshMaterial(m_AssetHandle);
		}
	}

}
