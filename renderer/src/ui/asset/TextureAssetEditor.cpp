#include "renderer_pch.h"
#include "TextureAssetEditor.h"

#include "AssetEditorUtils.h"
#include "render/VulkanResourceFactory.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		constexpr float textureToolbarHeight = 48.0f;
		constexpr float textureBodyHorizontalPadding = 12.0f;
		const ImVec4 textureContentBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 textureBarBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 textureHeaderAccentColor = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);

		int TextureShapeToComboIndex(TextureShape shape)
		{
			switch (shape)
			{
			case TextureShape::TextureCube: return 1;
			case TextureShape::Texture2D:
			default: return 0;
			}
		}

		TextureShape ComboIndexToTextureShape(int index)
		{
			return index == 1 ? TextureShape::TextureCube : TextureShape::Texture2D;
		}

		const char* GetTextureShapeLabel(TextureShape shape)
		{
			switch (shape)
			{
			case TextureShape::TextureCube: return "Cube";
			case TextureShape::Texture2D:
			default: return "2D";
			}
		}

		int TextureColorSpaceToComboIndex(TextureColorSpace colorSpace)
		{
			switch (colorSpace)
			{
			case TextureColorSpace::Linear: return 1;
			case TextureColorSpace::SRGB:
			default: return 0;
			}
		}

		TextureColorSpace ComboIndexToTextureColorSpace(int index)
		{
			return index == 1 ? TextureColorSpace::Linear : TextureColorSpace::SRGB;
		}

		const char* GetTextureColorSpaceLabel(TextureColorSpace colorSpace)
		{
			switch (colorSpace)
			{
			case TextureColorSpace::Linear: return "Linear";
			case TextureColorSpace::SRGB:
			default: return "sRGB";
			}
		}
	}

	TextureAssetEditor::TextureAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache, VulkanResourceFactory* resourceFactory)
		: m_AssetHandle(handle)
		, m_ThumbnailCache(thumbnailCache)
		, m_ResourceFactory(resourceFactory)
	{
		m_TextureAsset = AssetManager::GetInstance().GetTextureAsset(handle);
		m_DisplayName = GetAssetEditorDisplayName(handle);

		if (m_TextureAsset)
		{
			m_WorkingCopy = m_TextureAsset->ImportSettings;
			m_SavedCopy = m_TextureAsset->ImportSettings;
		}
	}

	bool TextureAssetEditor::IsDirty() const
	{
		return IsWorkingCopyDirty();
	}

	void TextureAssetEditor::Save()
	{
		if (!m_TextureAsset)
		{
			return;
		}

		if (!AssetManager::GetInstance().SaveTextureImportSettings(m_AssetHandle, m_WorkingCopy))
		{
			KITA_CORE_WARN("TextureAssetEditor: failed to save texture import settings for handle={}", m_AssetHandle);
			return;
		}

		m_SavedCopy = m_WorkingCopy;
		ApplyWorkingCopyToAsset();
		RefreshTextureResource();
	}

	void TextureAssetEditor::Revert()
	{
		m_WorkingCopy = m_SavedCopy;
		ApplyWorkingCopyToAsset();
		RefreshTextureResource();
	}

	void TextureAssetEditor::OnImGuiRender()
	{
		DrawToolbar();

		const ImGuiTableFlags layoutFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_NoSavedSettings;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, textureContentBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(textureBodyHorizontalPadding, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		if (ImGui::BeginChild("##TextureEditorBody", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			if (ImGui::BeginTable("##TextureEditorLayout", 2, layoutFlags, ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.70f);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.30f);
				ImGui::TableNextRow();

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

	void TextureAssetEditor::DrawToolbar()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, textureBarBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##TextureEditorToolbar", ImVec2(0.0f, textureToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 headerMin = ImGui::GetWindowPos();
		const ImVec2 headerMax(headerMin.x + ImGui::GetWindowSize().x, headerMin.y + ImGui::GetWindowSize().y);
		drawList->AddRectFilled(headerMin, ImVec2(headerMax.x, headerMin.y + 2.0f), ImGui::ColorConvertFloat4ToU32(textureHeaderAccentColor));

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Texture");
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

	void TextureAssetEditor::DrawPreview()
	{
		if (!m_TextureAsset)
		{
			ImGui::TextUnformatted("Texture asset is unavailable.");
			return;
		}

		ImGui::BeginChild("##TexturePreviewPane", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (m_WorkingCopy.Shape == TextureShape::TextureCube)
		{
			ImGui::TextUnformatted("Cubemap preview is not available yet.");
			ImGui::TextDisabled("Save to rebuild the runtime cubemap.");
			ImGui::EndChild();
			return;
		}

		const ThumbnailCache::ThumbnailHandle thumbnail =
			m_ThumbnailCache ? m_ThumbnailCache->GetOrCreate(m_AssetHandle, AssetType::Texture) : ThumbnailCache::ThumbnailHandle{};

		if (!thumbnail.IsValid())
		{
			ImGui::TextUnformatted("Texture preview is unavailable.");
			ImGui::EndChild();
			return;
		}

		const float availWidth = ImGui::GetContentRegionAvail().x;
		const float availHeight = ImGui::GetContentRegionAvail().y;
		const float scale = std::min(availWidth / static_cast<float>(thumbnail.Width), availHeight / static_cast<float>(thumbnail.Height));
		const ImVec2 previewSize(
			static_cast<float>(thumbnail.Width) * std::min(1.0f, scale),
			static_cast<float>(thumbnail.Height) * std::min(1.0f, scale));
		ImGui::SetCursorPosX(std::max(0.0f, (availWidth - previewSize.x) * 0.5f));
		ImGui::Image(thumbnail.TextureID, previewSize);
		ImGui::EndChild();
	}

	void TextureAssetEditor::DrawDetails()
	{
		if (!m_TextureAsset)
		{
			ImGui::TextUnformatted("Texture asset is unavailable.");
			return;
		}

		ImGui::BeginChild("##TextureDetailsPane", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		if (UIAttributeUtil::BeginPropertyTable("##TexturePropertyTable", m_TableStyle))
		{
			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Width", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%u", m_TextureAsset->TexRawData.Width);
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Height", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%u", m_TextureAsset->TexRawData.Height);
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Channels", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%u", m_TextureAsset->TexRawData.Channels);
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Format", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::TextUnformatted(m_TextureAsset->IsCube() ? "Cubemap Source" : "Texture2D Source");
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Shape", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			if (ImGui::BeginCombo("##TextureShape", GetTextureShapeLabel(m_WorkingCopy.Shape)))
			{
				for (int index = 0; index < 2; ++index)
				{
					const TextureShape shape = ComboIndexToTextureShape(index);
					const bool selected = TextureShapeToComboIndex(m_WorkingCopy.Shape) == index;
					if (ImGui::Selectable(GetTextureShapeLabel(shape), selected))
					{
						m_WorkingCopy.Shape = shape;
					}

					if (selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			UIAttributeUtil::PopInputStyle();
			if (UIAttributeUtil::DrawResetButtonCell("TextureShape", m_TableStyle, m_WorkingCopy.Shape != m_SavedCopy.Shape))
			{
				m_WorkingCopy.Shape = m_SavedCopy.Shape;
			}

			const bool colorSpaceForcedLinear = m_TextureAsset->IsHDRSource();
			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Color Space", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));
			if (colorSpaceForcedLinear)
			{
				ImGui::BeginDisabled();
			}
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			if (ImGui::BeginCombo("##TextureColorSpace", GetTextureColorSpaceLabel(m_WorkingCopy.ColorSpace)))
			{
				for (int index = 0; index < 2; ++index)
				{
					const TextureColorSpace colorSpace = ComboIndexToTextureColorSpace(index);
					const bool selected = TextureColorSpaceToComboIndex(m_WorkingCopy.ColorSpace) == index;
					if (ImGui::Selectable(GetTextureColorSpaceLabel(colorSpace), selected))
					{
						m_WorkingCopy.ColorSpace = colorSpace;
					}

					if (selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
			UIAttributeUtil::PopInputStyle();
			if (colorSpaceForcedLinear)
			{
				ImGui::EndDisabled();
			}
			if (UIAttributeUtil::DrawResetButtonCell("TextureColorSpace", m_TableStyle, m_WorkingCopy.ColorSpace != m_SavedCopy.ColorSpace && !colorSpaceForcedLinear))
			{
				m_WorkingCopy.ColorSpace = m_SavedCopy.ColorSpace;
			}

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Mipmaps", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));
			bool generateMipmaps = m_WorkingCopy.GenerateMipmaps;
			if (ImGui::Checkbox("##TextureGenerateMipmaps", &generateMipmaps))
			{
				m_WorkingCopy.GenerateMipmaps = generateMipmaps;
			}
			if (UIAttributeUtil::DrawResetButtonCell("TextureMipmaps", m_TableStyle, m_WorkingCopy.GenerateMipmaps != m_SavedCopy.GenerateMipmaps))
			{
				m_WorkingCopy.GenerateMipmaps = m_SavedCopy.GenerateMipmaps;
			}

			UIAttributeUtil::EndPropertyTable();
		}

		if (m_TextureAsset->IsHDRSource())
		{
			ImGui::Spacing();
			ImGui::TextDisabled("HDR source textures are forced to Linear.");
		}

		ImGui::EndChild();
	}

	bool TextureAssetEditor::IsWorkingCopyDirty() const
	{
		return m_WorkingCopy.Shape != m_SavedCopy.Shape ||
			m_WorkingCopy.ColorSpace != m_SavedCopy.ColorSpace ||
			m_WorkingCopy.GenerateMipmaps != m_SavedCopy.GenerateMipmaps;
	}

	void TextureAssetEditor::ApplyWorkingCopyToAsset()
	{
		if (!m_TextureAsset)
		{
			return;
		}

		m_TextureAsset->ImportSettings = m_WorkingCopy;
		if (m_TextureAsset->IsHDRSource())
		{
			m_TextureAsset->ImportSettings.ColorSpace = TextureColorSpace::Linear;
			m_WorkingCopy.ColorSpace = TextureColorSpace::Linear;
		}
	}

	void TextureAssetEditor::RefreshTextureResource()
	{
		if (!Asset::IsValidHandle(m_AssetHandle))
		{
			return;
		}

		if (m_ThumbnailCache)
		{
			m_ThumbnailCache->Invalidate(m_AssetHandle);
		}

		if (!m_ResourceFactory)
		{
			return;
		}

		m_ResourceFactory->InvalidateTexture(m_AssetHandle);
		Ref<VulkanTexture> rebuiltTexture = m_ResourceFactory->GetOrCreateTexture(m_AssetHandle);

		if (!rebuiltTexture)
		{
			KITA_CORE_WARN("TextureAssetEditor: failed to rebuild runtime texture for handle={}", m_AssetHandle);
			return;
		}

		const std::vector<AssetMetadata> materialAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Material);
		for (const AssetMetadata& metadata : materialAssets)
		{
			Ref<MaterialAsset> materialAsset = AssetManager::GetInstance().GetMaterialAsset(metadata.handle);
			if (!materialAsset)
			{
				continue;
			}

			if (materialAsset->AlbedoTextureHandle == m_AssetHandle)
			{
				if (rebuiltTexture->GetType() == TextureType::TextureCube)
				{
					KITA_CORE_WARN(
						"TextureAssetEditor: texture handle={} is now a cubemap and will be skipped for material handle={}",
						m_AssetHandle,
						metadata.handle);
					continue;
				}

				m_ResourceFactory->RefreshMaterial(metadata.handle);
			}
		}
	}

}
