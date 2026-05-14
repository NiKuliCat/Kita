#include "renderer_pch.h"
#include "TextureAssetEditor.h"
#include "AssetEditorUtils.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		constexpr float textureToolbarHeight = 36.0f;
		constexpr float textureBodyHorizontalPadding = 12.0f;
		const ImVec4 textureContentBgColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		const ImVec4 textureBarBgColor = ImVec4(0.13f, 0.13f, 0.13f, 1.0f);
	}

	TextureAssetEditor::TextureAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache)
		: m_AssetHandle(handle)
		, m_ThumbnailCache(thumbnailCache)
	{
		m_TextureAsset = AssetManager::GetInstance().GetTextureAsset(handle);
		m_DisplayName = GetAssetEditorDisplayName(handle);
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##TextureEditorToolbar", ImVec2(0.0f, textureToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::SetCursorPos(ImVec2(10.0f, 6.0f));

		ImGui::BeginDisabled();
		ImGui::Button("Save");
		ImGui::SameLine();
		ImGui::Button("Revert");
		ImGui::EndDisabled();

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
		ImGui::Text("Width: %u", m_TextureAsset->TexRawData.Width);
		ImGui::Text("Height: %u", m_TextureAsset->TexRawData.Height);
		ImGui::Text("Channels: %u", m_TextureAsset->TexRawData.Channels);
		ImGui::EndChild();
	}

}
