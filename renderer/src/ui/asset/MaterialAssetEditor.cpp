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
		constexpr float materialLabelColumnWidth = 130.0f;
		constexpr float materialResetColumnWidth = 40.0f;
		constexpr float materialCellInnerPaddingX = 12.0f;
		constexpr float materialValueRightInset = materialCellInnerPaddingX;
		constexpr float materialPropertyRowHeight = 30.0f;
		constexpr float materialTextureSlotRowHeight = 56.0f;
		constexpr float materialToolbarHeight = 36.0f;
		constexpr float materialBodyHorizontalPadding = 12.0f;
		const ImU32 materialEditorBorderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		const ImU32 materialEditorRowBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.16f, 0.16f, 0.16f, 1.0f));
		const ImVec4 materialContentBgColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		const ImVec4 materialBarBgColor = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);

		float GetMaterialLabelYOffset()
		{
			const float contentHeight = materialPropertyRowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
			return ImMax(0.0f, (contentHeight - ImGui::GetTextLineHeight()) * 0.5f);
		}

		float GetMaterialControlYOffset()
		{
			const float contentHeight = materialPropertyRowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
			return ImMax(0.0f, (contentHeight - ImGui::GetFrameHeight()) * 0.5f);
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

		m_SourceAsset->ShaderHandle = m_WorkingCopy.ShaderHandle;
		m_SourceAsset->AlbedoTextureHandle = m_WorkingCopy.AlbedoTextureHandle;
		m_SourceAsset->BaseColor = m_WorkingCopy.BaseColor;
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##MaterialEditorToolbar", ImVec2(0.0f, materialToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetCursorPos(ImVec2(10.0f, 6.0f));

		const bool canSaveNow = CanSave() && IsDirty();
		if (!canSaveNow)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Save"))
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
		if (ImGui::Button("Revert"))
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
		if (BeginPropertyTable("##MaterialPropertyTable"))
		{
			DrawAssetRow("Shader", "##MaterialShader", m_WorkingCopy.ShaderHandle, AssetType::Shader, m_SavedCopy.ShaderHandle);
			DrawTextureSlotRow("Albedo", 0, m_WorkingCopy.AlbedoTextureHandle, m_SavedCopy.AlbedoTextureHandle);
			DrawColorRow("Base Color", "##MaterialBaseColor", m_WorkingCopy.BaseColor, m_SavedCopy.BaseColor);
			EndPropertyTable();
		}
		ImGui::EndChild();
	}

	bool MaterialAssetEditor::BeginPropertyTable(const char* id)
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, materialEditorBorderColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, materialEditorBorderColor);
		ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(materialEditorBorderColor));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

		const float tableMinX = ImGui::GetWindowContentRegionMin().x;
		const float tableMaxX = ImGui::GetWindowContentRegionMax().x;
		ImGui::SetCursorPosX(tableMinX);

		const bool open = ImGui::BeginTable(id, 3, tableFlags, ImVec2(tableMaxX - tableMinX, 0.0f));
		if (!open)
		{
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(4);
			return false;
		}

		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, materialLabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, materialResetColumnWidth);
		return true;
	}

	void MaterialAssetEditor::EndPropertyTable()
	{
		ImGui::EndTable();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(4);
	}

	void MaterialAssetEditor::DrawAssetRow(const char* label, const char* comboId, AssetHandle& handle, AssetType type, AssetHandle resetValue)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, materialPropertyRowHeight);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, materialEditorRowBgColor);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialLabelYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialControlYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);

		const std::vector<AssetMetadata> assets = AssetManager::GetInstance().GetAssetsByType(type);
		const std::string displayName = GetAssetEditorDisplayName(handle);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - materialValueRightInset);
		if (ImGui::BeginCombo(comboId, displayName.c_str()))
		{
			const bool noneSelected = !Asset::IsValidHandle(handle);
			if (ImGui::Selectable("None", noneSelected))
			{
				handle = InvalidAssetHandle;
			}

			if (noneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const auto& metadata : assets)
			{
				const std::string optionLabel = metadata.relativePath.filename().string().empty()
					? metadata.relativePath.generic_string()
					: metadata.relativePath.filename().string();
				const bool isSelected = handle == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					handle = metadata.handle;
					SyncWorkingCopyToAssetData();
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor(4);

		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialControlYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);

		const bool canReset = handle != resetValue;
		ImGui::PushID(label);
		if (!canReset)
		{
			ImGui::BeginDisabled();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		if (ImGui::Button("R", ImVec2(materialResetColumnWidth - 8.0f, 0.0f)))
		{
			handle = resetValue;
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleVar();
		if (!canReset)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
	}

	void MaterialAssetEditor::DrawTextureSlotRow(const char* label, size_t slotIndex, AssetHandle& handle, AssetHandle resetValue)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, materialTextureSlotRowHeight);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, materialEditorRowBgColor);

		const float contentHeight = materialTextureSlotRowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - ImGui::GetTextLineHeight()) * 0.5f);
		const ImVec2 compactFramePadding(6.0f, 1.0f);
		const float compactFrameHeight = ImGui::GetFontSize() + compactFramePadding.y * 2.0f;
		const float comboYOffset = ImMax(0.0f, (contentHeight - compactFrameHeight) * 0.5f);
		const float resetYOffset = ImMax(0.0f, (contentHeight - ImGui::GetFrameHeight()) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + comboYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);

		ImGui::PushID(static_cast<int>(slotIndex));

		const std::vector<AssetMetadata> textureAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Texture);
		const float valueWidth = ImGui::GetContentRegionAvail().x - materialValueRightInset;
		const float tileSize = materialTextureSlotRowHeight - 10.0f;
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
			drawList->AddImage(
				thumbnail.TextureID,
				thumbRect.Min,
				thumbRect.Max,
				ImVec2(0.0f, 0.0f),
				ImVec2(1.0f, 1.0f));
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

		const std::string textureName = GetAssetEditorDisplayName(handle);
		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, compactFramePadding);
		ImGui::SetNextItemWidth(comboWidth);
		if (ImGui::BeginCombo("##TextureSelector", textureName.c_str()))
		{
			const bool noneSelected = !Asset::IsValidHandle(handle);
			if (ImGui::Selectable("None", noneSelected))
			{
				handle = InvalidAssetHandle;
				SyncWorkingCopyToAssetData();
			}

			if (noneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const auto& metadata : textureAssets)
			{
				const std::string optionLabel = metadata.relativePath.filename().string().empty()
					? metadata.relativePath.generic_string()
					: metadata.relativePath.filename().string();
				const bool isSelected = handle == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					handle = metadata.handle;
					SyncWorkingCopyToAssetData();
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleColor(4);
		ImGui::EndGroup();

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

		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + resetYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);

		const bool canReset = handle != resetValue;
		ImGui::PushID(label);
		if (!canReset)
		{
			ImGui::BeginDisabled();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		if (ImGui::Button("R", ImVec2(materialResetColumnWidth - 8.0f, 0.0f)))
		{
			handle = resetValue;
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleVar();
		if (!canReset)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
	}

	void MaterialAssetEditor::DrawColorRow(const char* label, const char* colorId, glm::vec4& value, const glm::vec4& resetValue)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, materialPropertyRowHeight);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, materialEditorRowBgColor);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialLabelYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialControlYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + materialCellInnerPaddingX);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - materialValueRightInset);
		ImGui::ColorEdit4(colorId, &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		if (ImGui::IsItemEdited())
		{
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleColor();

		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetMaterialControlYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);

		const bool canReset = value != resetValue;
		ImGui::PushID(label);
		if (!canReset)
		{
			ImGui::BeginDisabled();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		if (ImGui::Button("R", ImVec2(materialResetColumnWidth - 8.0f, 0.0f)))
		{
			value = resetValue;
			SyncWorkingCopyToAssetData();
		}
		ImGui::PopStyleVar();
		if (!canReset)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
	}

	bool MaterialAssetEditor::IsWorkingCopyDirty() const
	{
		return m_WorkingCopy.ShaderHandle != m_SavedCopy.ShaderHandle ||
			m_WorkingCopy.AlbedoTextureHandle != m_SavedCopy.AlbedoTextureHandle ||
			m_WorkingCopy.BaseColor != m_SavedCopy.BaseColor;
	}

	void MaterialAssetEditor::SyncWorkingCopyToAssetData()
	{
		if (!m_SourceAsset)
		{
			return;
		}

		m_SourceAsset->ShaderHandle = m_WorkingCopy.ShaderHandle;
		m_SourceAsset->AlbedoTextureHandle = m_WorkingCopy.AlbedoTextureHandle;
		m_SourceAsset->BaseColor = m_WorkingCopy.BaseColor;

		if (m_ResourceFactory && Asset::IsValidHandle(m_AssetHandle))
		{
			m_ResourceFactory->RefreshMaterial(m_AssetHandle);
		}
	}

}
