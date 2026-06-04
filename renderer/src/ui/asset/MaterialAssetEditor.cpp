#include "renderer_pch.h"
#include "MaterialAssetEditor.h"

#include "AssetDragDrop.h"
#include "AssetEditorUtils.h"
#include "asset/AssetManager.h"
#include "file/Project.h"
#include "render/VulkanResourceFactory.h"
#include "serialize/MaterialSerializer.h"

#include "imgui.h"
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>
#include <unordered_set>

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

		using PropertyMap = std::unordered_map<std::string, MaterialPropertyValue>;

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

		template<typename T>
		MaterialPropertyValue MakePropertyValue(MaterialValueType valueType, const T& data)
		{
			MaterialPropertyValue value{};
			value.ValueType = valueType;
			value.Data = data;
			return value;
		}

		bool IsTextureProperty(MaterialValueType valueType)
		{
			return valueType == MaterialValueType::Texture2D ||
				valueType == MaterialValueType::TextureCube;
		}

		AssetHandle GetTextureHandle(const MaterialPropertyValue& value)
		{
			if (!std::holds_alternative<AssetHandle>(value.Data))
			{
				return InvalidAssetHandle;
			}

			return std::get<AssetHandle>(value.Data);
		}

		PropertyMap BuildLegacyPropertyValues(const MaterialAsset& materialAsset)
		{
			PropertyMap properties{};
			properties["_BaseColor"] = MakePropertyValue(MaterialValueType::Color, materialAsset.m_SurfaceParams.BaseColor);
			properties["_Emissive"] = MakePropertyValue(MaterialValueType::Float3, materialAsset.m_SurfaceParams.Emissive);
			properties["_Metallic"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.Metallic);
			properties["_Roughness"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.Roughness);
			properties["_AmbientOcclusion"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.AmbientOcclusion);
			properties["_Opacity"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.Opacity);
			properties["_NormalScale"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.NormalScale);
			properties["_AlphaCutoff"] = MakePropertyValue(MaterialValueType::Float, materialAsset.m_SurfaceParams.AlphaCutoff);
			properties["_Albedo"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.Albedo);
			properties["_Normal"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.Normal);
			properties["_MetallicRoughness"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.MetallicRoughness);
			properties["_AmbientOcclusionTexture"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.AmbientOcclusion);

			properties["_EmissionMask"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.Emissive);

			properties["_OpacityTexture"] = MakePropertyValue(MaterialValueType::Texture2D, materialAsset.m_Textures.Opacity);
			return properties;
		}

		const ShaderLabAssetDesc* GetShaderLabDesc(AssetHandle handle)
		{
			if (!Asset::IsValidHandle(handle))
			{
				return nullptr;
			}

			Ref<ShaderLabAsset> shaderLabAsset = AssetManager::GetInstance().GetShaderLabAsset(handle);
			if (!shaderLabAsset || !shaderLabAsset->Desc)
			{
				return nullptr;
			}

			return shaderLabAsset->Desc.get();
		}

		const MaterialPropertyValue* FindPropertyValue(const PropertyMap& properties, const std::string& name)
		{
			auto it = properties.find(name);
			if (it == properties.end())
			{
				return nullptr;
			}

			return &it->second;
		}

		bool ArePropertyValuesEqual(const MaterialPropertyValue& lhs, const MaterialPropertyValue& rhs)
		{
			if (lhs.ValueType != rhs.ValueType || lhs.Data.index() != rhs.Data.index())
			{
				return false;
			}

			switch (lhs.ValueType)
			{
			case MaterialValueType::Bool:
				return std::get<bool>(lhs.Data) == std::get<bool>(rhs.Data);
			case MaterialValueType::Int:
				return std::get<int32_t>(lhs.Data) == std::get<int32_t>(rhs.Data);
			case MaterialValueType::Float:
				return std::get<float>(lhs.Data) == std::get<float>(rhs.Data);
			case MaterialValueType::Float2:
				return glm::all(glm::equal(std::get<glm::vec2>(lhs.Data), std::get<glm::vec2>(rhs.Data)));
			case MaterialValueType::Float3:
				return glm::all(glm::equal(std::get<glm::vec3>(lhs.Data), std::get<glm::vec3>(rhs.Data)));
			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				return glm::all(glm::equal(std::get<glm::vec4>(lhs.Data), std::get<glm::vec4>(rhs.Data)));
			case MaterialValueType::Texture2D:
			case MaterialValueType::TextureCube:
				if (std::holds_alternative<AssetHandle>(lhs.Data))
				{
					return std::get<AssetHandle>(lhs.Data) == std::get<AssetHandle>(rhs.Data);
				}
				if (std::holds_alternative<std::string>(lhs.Data))
				{
					return std::get<std::string>(lhs.Data) == std::get<std::string>(rhs.Data);
				}
				return false;
			default:
				return false;
			}
		}

		bool ArePropertyMapsEqual(const PropertyMap& lhs, const PropertyMap& rhs)
		{
			if (lhs.size() != rhs.size())
			{
				return false;
			}

			for (const auto& [name, value] : lhs)
			{
				auto rhsIt = rhs.find(name);
				if (rhsIt == rhs.end() || !ArePropertyValuesEqual(value, rhsIt->second))
				{
					return false;
				}
			}

			return true;
		}

		void NormalizePropertyBlock(MaterialAsset& materialAsset, const ShaderLabAssetDesc& desc)
		{
			PropertyMap previousValues = materialAsset.PropertyBlock.Values;
			PropertyMap normalizedOrphans = materialAsset.PropertyBlock.OrphanValues;
			PropertyMap normalizedValues{};
			std::unordered_set<std::string> schemaNames;
			schemaNames.reserve(desc.Properties.size());

			for (const MaterialPropertyDesc& property : desc.Properties)
			{
				schemaNames.insert(property.Name);
			}

			for (const auto& [name, value] : previousValues)
			{
				if (schemaNames.find(name) == schemaNames.end())
				{
					normalizedOrphans[name] = value;
				}
			}

			for (const MaterialPropertyDesc& property : desc.Properties)
			{
				auto currentIt = previousValues.find(property.Name);
				auto orphanIt = normalizedOrphans.find(property.Name);

				if (currentIt != previousValues.end() && currentIt->second.ValueType == property.ValueType)
				{
					normalizedValues[property.Name] = currentIt->second;
					if (orphanIt != normalizedOrphans.end() && orphanIt->second.ValueType == property.ValueType)
					{
						normalizedOrphans.erase(orphanIt);
					}
					continue;
				}

				if (currentIt != previousValues.end())
				{
					normalizedOrphans[property.Name] = currentIt->second;
					orphanIt = normalizedOrphans.find(property.Name);
				}

				if (orphanIt != normalizedOrphans.end() && orphanIt->second.ValueType == property.ValueType)
				{
					normalizedValues[property.Name] = orphanIt->second;
					normalizedOrphans.erase(orphanIt);
					continue;
				}

				normalizedValues[property.Name] = property.DefaultValue;
			}

			materialAsset.PropertyBlock.Values = std::move(normalizedValues);
			materialAsset.PropertyBlock.OrphanValues = std::move(normalizedOrphans);
		}

		void RebuildLegacyPropertyBlockForAsset(MaterialAsset& materialAsset)
		{
			PropertyMap rebuiltValues = BuildLegacyPropertyValues(materialAsset);
			PropertyMap rebuiltOrphans = materialAsset.PropertyBlock.OrphanValues;

			for (const auto& [name, value] : materialAsset.PropertyBlock.Values)
			{
				if (rebuiltValues.find(name) == rebuiltValues.end())
				{
					rebuiltOrphans[name] = value;
				}
			}

			for (const auto& [name, value] : rebuiltValues)
			{
				auto orphanIt = rebuiltOrphans.find(name);
				if (orphanIt != rebuiltOrphans.end() && ArePropertyValuesEqual(orphanIt->second, value))
				{
					rebuiltOrphans.erase(orphanIt);
				}
			}

			materialAsset.PropertyBlock.Values = std::move(rebuiltValues);
			materialAsset.PropertyBlock.OrphanValues = std::move(rebuiltOrphans);
		}

		std::vector<std::string> CollectSortedPropertyNames(const PropertyMap& properties)
		{
			std::vector<std::string> names;
			names.reserve(properties.size());
			for (const auto& [name, _] : properties)
			{
				names.push_back(name);
			}

			std::sort(names.begin(), names.end());
			return names;
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

			if (const ShaderLabAssetDesc* desc = GetShaderLabDesc(m_WorkingCopy.ShaderLabHandle))
			{
				NormalizePropertyBlock(m_WorkingCopy, *desc);
				NormalizePropertyBlock(m_SavedCopy, *desc);
			}
			else if (!Asset::IsValidHandle(m_WorkingCopy.ShaderLabHandle))
			{
				RebuildLegacyPropertyBlockForAsset(m_WorkingCopy);
				RebuildLegacyPropertyBlockForAsset(m_SavedCopy);
			}
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

	void MaterialAssetEditor::OnUpdate()
	{
		ApplyPendingRuntimeChanges();
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
		const float previewSize = ImClamp(ImMin(available.x, available.y), 180.0f, 340.0f);
		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		const ImVec2 previewMin(
			cursor.x + ImMax(0.0f, (available.x - previewSize) * 0.5f),
			cursor.y + ImMax(0.0f, (available.y - previewSize) * 0.18f));
		const ImVec2 previewMax(previewMin.x + previewSize, previewMin.y + previewSize);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(previewMin, previewMax, IM_COL32(26, 26, 26, 255), 6.0f);
		drawList->AddRect(previewMin, previewMax, IM_COL32(12, 12, 12, 255), 6.0f, 0, 1.0f);

		const ThumbnailCache::ThumbnailHandle thumbnail =
			m_ThumbnailCache ? m_ThumbnailCache->GetOrCreate(m_AssetHandle, AssetType::Material, static_cast<uint32_t>(previewSize)) : ThumbnailCache::ThumbnailHandle{};
		if (thumbnail.IsValid())
		{
			drawList->AddImage(thumbnail.TextureID, previewMin, previewMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
		}
		else
		{
			const ImVec2 center((previewMin.x + previewMax.x) * 0.5f, (previewMin.y + previewMax.y) * 0.5f);
			const float radius = previewSize * 0.30f;
			drawList->AddCircleFilled(center, radius, IM_COL32(118, 118, 118, 255), 48);
			drawList->AddCircle(center, radius, IM_COL32(38, 38, 38, 255), 48, 2.0f);
			drawList->AddText(
				ImVec2(previewMin.x + 14.0f, previewMin.y + 12.0f),
				IM_COL32(168, 168, 168, 255),
				"Material preview unavailable");
		}

		ImGui::Dummy(ImVec2(available.x, previewSize + 24.0f));
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
			DrawAssetRow("ShaderLab", "##MaterialShaderLab", m_WorkingCopy.ShaderLabHandle, AssetType::ShaderLab, m_SavedCopy.ShaderLabHandle);
			DrawAssetRow("Shader", "##MaterialShader", m_WorkingCopy.ShaderHandle, AssetType::Shader, m_SavedCopy.ShaderHandle);

			if (const ShaderLabAssetDesc* desc = GetShaderLabDesc(m_WorkingCopy.ShaderLabHandle))
			{
				DrawShaderLabProperties(*desc);
			}
			else if (!Asset::IsValidHandle(m_WorkingCopy.ShaderLabHandle))
			{
				DrawLegacyProperties();
			}
			else
			{
				ImGui::TableNextRow(ImGuiTableRowFlags_None, materialPropertyRowHeight);
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("ShaderLab");
				ImGui::TableSetColumnIndex(1);
				ImGui::TextDisabled("Schema asset is unavailable.");
				ImGui::TableSetColumnIndex(2);
			}

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
		ImGui::ColorEdit4(
			colorId,
			&value.x,
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
		ImGui::ColorEdit3(
			colorId,
			&value.x,
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

	void MaterialAssetEditor::DrawShaderLabProperties(const ShaderLabAssetDesc& desc)
	{
		for (const MaterialPropertyDesc& property : desc.Properties)
		{
			auto valueIt = m_WorkingCopy.PropertyBlock.Values.find(property.Name);
			if (valueIt == m_WorkingCopy.PropertyBlock.Values.end())
			{
				continue;
			}

			const std::string label = property.DisplayName.empty() ? property.Name : property.DisplayName;
			const MaterialPropertyValue* resetValue = FindPropertyValue(m_SavedCopy.PropertyBlock.Values, property.Name);
			DrawMaterialPropertyRow(
				label,
				"##MaterialProperty_" + property.Name,
				valueIt->second,
				resetValue,
				&property.UIHint,
				false);
		}

		for (const std::string& orphanName : CollectSortedPropertyNames(m_WorkingCopy.PropertyBlock.OrphanValues))
		{
			auto orphanIt = m_WorkingCopy.PropertyBlock.OrphanValues.find(orphanName);
			if (orphanIt == m_WorkingCopy.PropertyBlock.OrphanValues.end())
			{
				continue;
			}

			const MaterialPropertyValue* resetValue = FindPropertyValue(m_SavedCopy.PropertyBlock.OrphanValues, orphanName);
			DrawMaterialPropertyRow(
				"[Orphan] " + orphanName,
				"##MaterialOrphan_" + orphanName,
				orphanIt->second,
				resetValue,
				nullptr,
				true);
		}
	}

	void MaterialAssetEditor::DrawLegacyProperties()
	{
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
	}

	void MaterialAssetEditor::DrawMaterialPropertyRow(
		const std::string& label,
		const std::string& valueId,
		MaterialPropertyValue& value,
		const MaterialPropertyValue* resetValue,
		const MaterialPropertyUIHint* uiHint,
		bool orphan)
	{
		(void)orphan;

		if (IsTextureProperty(value.ValueType))
		{
			UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
			UIAttributeUtil::DrawPropertyLabelCell(label.c_str(), m_TableStyle, m_TableStyle.PreviewRowHeight);

			const float contentHeight = UIAttributeUtil::GetContentHeight(m_TableStyle, m_TableStyle.PreviewRowHeight);
			const ImVec2 compactFramePadding(6.0f, 1.0f);
			const float compactFrameHeight = ImGui::GetFontSize() + compactFramePadding.y * 2.0f;
			const float comboYOffset = UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, compactFrameHeight);
			const float resetYOffset = UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, comboYOffset);

			const std::vector<AssetMetadata> textureAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Texture);
			const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
			const float tileSize = m_TableStyle.PreviewTileSize;
			const float thumbYOffset = ImMax(0.0f, (contentHeight - tileSize) * 0.5f);
			const float comboSpacing = 10.0f;
			const float comboWidth = ImMax(80.0f, valueWidth - tileSize - comboSpacing);
			const ImVec2 tileSizeVec(tileSize, tileSize);
			AssetHandle textureHandle = GetTextureHandle(value);

			ImGui::PushID(valueId.c_str());

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - comboYOffset + thumbYOffset);
			ImGui::InvisibleButton("##TextureThumbnail", tileSizeVec);
			const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(32, 32, 32, 255));
			drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);

			const ThumbnailCache::ThumbnailHandle thumbnail =
				m_ThumbnailCache ? m_ThumbnailCache->GetOrCreate(textureHandle, AssetType::Texture) : ThumbnailCache::ThumbnailHandle{};
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
					value.ValueType == MaterialValueType::TextureCube ? "CUBE" : "TEX");
			}

			ImGui::SameLine(0.0f, comboSpacing);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - thumbYOffset + comboYOffset);

			UIAttributeUtil::PushInputStyle(m_TableStyle);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, compactFramePadding);
			const bool changed = UIAttributeUtil::DrawAssetCombo(
				"##TextureSelector",
				textureHandle,
				textureAssets,
				comboWidth,
				UIAttributeUtil::AssetLabelMode::FileName);
			ImGui::PopStyleVar();
			UIAttributeUtil::PopInputStyle();

			if (changed)
			{
				value.Data = textureHandle;
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
							value.Data = assetPayload.Handle;
							SyncWorkingCopyToAssetData();
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::PopID();

			const bool canReset = resetValue != nullptr && !ArePropertyValuesEqual(value, *resetValue);
			if (resetValue)
			{
				if (UIAttributeUtil::DrawResetButtonCell(label.c_str(), m_TableStyle, canReset, resetYOffset))
				{
					value = *resetValue;
					SyncWorkingCopyToAssetData();
				}
			}
			else
			{
				UIAttributeUtil::DrawEmptyResetCell(m_TableStyle, m_TableStyle.PreviewRowHeight);
			}

			return;
		}

		UIAttributeUtil::BeginPropertyRow(m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label.c_str(), m_TableStyle, materialPropertyRowHeight);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight));

		const float inputWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		ImGui::SetNextItemWidth(inputWidth);

		bool changed = false;
		switch (value.ValueType)
		{
		case MaterialValueType::Bool:
		{
			bool boolValue = std::get<bool>(value.Data);
			changed = ImGui::Checkbox(valueId.c_str(), &boolValue);
			if (changed)
			{
				value.Data = boolValue;
			}
			break;
		}
		case MaterialValueType::Int:
		{
			int intValue = std::get<int32_t>(value.Data);
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			if (uiHint && uiHint->HasRange)
			{
				changed = ImGui::SliderInt(
					valueId.c_str(),
					&intValue,
					static_cast<int>(uiHint->MinValue),
					static_cast<int>(uiHint->MaxValue));
			}
			else
			{
				changed = ImGui::DragInt(valueId.c_str(), &intValue, 1.0f);
			}
			UIAttributeUtil::PopInputStyle();
			if (changed)
			{
				value.Data = static_cast<int32_t>(intValue);
			}
			break;
		}
		case MaterialValueType::Float:
		{
			float floatValue = std::get<float>(value.Data);
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			if (uiHint && uiHint->HasRange)
			{
				changed = ImGui::SliderFloat(valueId.c_str(), &floatValue, uiHint->MinValue, uiHint->MaxValue);
			}
			else
			{
				const float speed = uiHint ? uiHint->Step : 0.01f;
				changed = ImGui::DragFloat(valueId.c_str(), &floatValue, speed, -FLT_MAX, FLT_MAX, "%.3f");
			}
			UIAttributeUtil::PopInputStyle();
			if (changed)
			{
				value.Data = floatValue;
			}
			break;
		}
		case MaterialValueType::Float2:
		{
			glm::vec2 vecValue = std::get<glm::vec2>(value.Data);
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			changed = ImGui::DragFloat2(valueId.c_str(), &vecValue.x, uiHint ? uiHint->Step : 0.01f, -FLT_MAX, FLT_MAX, "%.3f");
			UIAttributeUtil::PopInputStyle();
			if (changed)
			{
				value.Data = vecValue;
			}
			break;
		}
		case MaterialValueType::Float3:
		{
			glm::vec3 vecValue = std::get<glm::vec3>(value.Data);
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			changed = ImGui::DragFloat3(valueId.c_str(), &vecValue.x, uiHint ? uiHint->Step : 0.01f, -FLT_MAX, FLT_MAX, "%.3f");
			UIAttributeUtil::PopInputStyle();
			if (changed)
			{
				value.Data = vecValue;
			}
			break;
		}
		case MaterialValueType::Float4:
		{
			glm::vec4 vecValue = std::get<glm::vec4>(value.Data);
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			changed = ImGui::DragFloat4(valueId.c_str(), &vecValue.x, uiHint ? uiHint->Step : 0.01f, -FLT_MAX, FLT_MAX, "%.3f");
			UIAttributeUtil::PopInputStyle();
			if (changed)
			{
				value.Data = vecValue;
			}
			break;
		}
		case MaterialValueType::Color:
		{
			glm::vec4 colorValue = std::get<glm::vec4>(value.Data);
			ImGuiColorEditFlags flags =
				ImGuiColorEditFlags_DisplayRGB |
				ImGuiColorEditFlags_Float |
				ImGuiColorEditFlags_AlphaBar;
			if (uiHint && uiHint->HDR)
			{
				flags |= ImGuiColorEditFlags_HDR;
			}

			ImGui::PushStyleColor(ImGuiCol_Border, m_TableStyle.InputBorderColor);
			changed = ImGui::ColorEdit4(valueId.c_str(), &colorValue.x, flags);
			ImGui::PopStyleColor();
			if (changed)
			{
				value.Data = colorValue;
			}
			break;
		}
		default:
			ImGui::TextDisabled("Unsupported property type");
			break;
		}

		if (changed)
		{
			SyncWorkingCopyToAssetData();
		}

		const bool canReset = resetValue != nullptr && !ArePropertyValuesEqual(value, *resetValue);
		if (resetValue)
		{
			if (UIAttributeUtil::DrawResetButtonCell(
				label.c_str(),
				m_TableStyle,
				canReset,
				UIAttributeUtil::GetControlYOffset(m_TableStyle, materialPropertyRowHeight)))
			{
				value = *resetValue;
				SyncWorkingCopyToAssetData();
			}
		}
		else
		{
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle, materialPropertyRowHeight);
		}
	}

	void MaterialAssetEditor::NormalizeWorkingCopyProperties()
	{
		if (const ShaderLabAssetDesc* desc = GetShaderLabDesc(m_WorkingCopy.ShaderLabHandle))
		{
			NormalizePropertyBlock(m_WorkingCopy, *desc);
			return;
		}

		if (!Asset::IsValidHandle(m_WorkingCopy.ShaderLabHandle))
		{
			RebuildLegacyPropertyBlock();
		}
	}

	void MaterialAssetEditor::RebuildLegacyPropertyBlock()
	{
		RebuildLegacyPropertyBlockForAsset(m_WorkingCopy);
	}

	void MaterialAssetEditor::ApplyPendingRuntimeChanges()
	{
		if (!m_RuntimeRefreshPending)
		{
			return;
		}

		m_RuntimeRefreshPending = false;

		if (m_ResourceFactory && Asset::IsValidHandle(m_AssetHandle))
		{
			Application::Get().GetVulkanContext().WaitIdle();
			m_ResourceFactory->RefreshMaterial(m_AssetHandle);
		}

		if (m_ThumbnailCache && Asset::IsValidHandle(m_AssetHandle))
		{
			m_ThumbnailCache->Invalidate(m_AssetHandle);
		}
	}

	bool MaterialAssetEditor::IsWorkingCopyDirty() const
	{
		return m_WorkingCopy.ShaderLabHandle != m_SavedCopy.ShaderLabHandle ||
			m_WorkingCopy.ShaderHandle != m_SavedCopy.ShaderHandle ||
			m_WorkingCopy.m_Textures.Albedo != m_SavedCopy.m_Textures.Albedo ||
			m_WorkingCopy.m_Textures.Normal != m_SavedCopy.m_Textures.Normal ||
			m_WorkingCopy.m_Textures.MetallicRoughness != m_SavedCopy.m_Textures.MetallicRoughness ||
			m_WorkingCopy.m_Textures.AmbientOcclusion != m_SavedCopy.m_Textures.AmbientOcclusion ||
			m_WorkingCopy.m_Textures.Emissive != m_SavedCopy.m_Textures.Emissive ||
			m_WorkingCopy.m_Textures.Opacity != m_SavedCopy.m_Textures.Opacity ||
			!glm::all(glm::equal(m_WorkingCopy.m_SurfaceParams.BaseColor, m_SavedCopy.m_SurfaceParams.BaseColor)) ||
			!glm::all(glm::equal(m_WorkingCopy.m_SurfaceParams.Emissive, m_SavedCopy.m_SurfaceParams.Emissive)) ||
			m_WorkingCopy.m_SurfaceParams.Metallic != m_SavedCopy.m_SurfaceParams.Metallic ||
			m_WorkingCopy.m_SurfaceParams.Roughness != m_SavedCopy.m_SurfaceParams.Roughness ||
			m_WorkingCopy.m_SurfaceParams.AmbientOcclusion != m_SavedCopy.m_SurfaceParams.AmbientOcclusion ||
			m_WorkingCopy.m_SurfaceParams.Opacity != m_SavedCopy.m_SurfaceParams.Opacity ||
			m_WorkingCopy.m_SurfaceParams.NormalScale != m_SavedCopy.m_SurfaceParams.NormalScale ||
			m_WorkingCopy.m_SurfaceParams.AlphaCutoff != m_SavedCopy.m_SurfaceParams.AlphaCutoff ||
			!ArePropertyMapsEqual(m_WorkingCopy.PropertyBlock.Values, m_SavedCopy.PropertyBlock.Values) ||
			!ArePropertyMapsEqual(m_WorkingCopy.PropertyBlock.OrphanValues, m_SavedCopy.PropertyBlock.OrphanValues);
	}

	void MaterialAssetEditor::SyncWorkingCopyToAssetData()
	{
		if (!m_SourceAsset)
		{
			return;
		}

		NormalizeWorkingCopyProperties();

		m_SourceAsset->ShaderLabHandle = m_WorkingCopy.ShaderLabHandle;
		m_SourceAsset->PropertyBlock = m_WorkingCopy.PropertyBlock;
		m_SourceAsset->ShaderHandle = m_WorkingCopy.ShaderHandle;
		m_SourceAsset->m_Textures = m_WorkingCopy.m_Textures;
		m_SourceAsset->m_SurfaceParams = m_WorkingCopy.m_SurfaceParams;
		m_RuntimeRefreshPending = true;
	}

}
