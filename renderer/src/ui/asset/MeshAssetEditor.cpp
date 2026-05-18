#include "renderer_pch.h"
#include "MeshAssetEditor.h"
#include "AssetEditorUtils.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		constexpr float meshToolbarHeight = 48.0f;
		constexpr float meshBodyHorizontalPadding = 12.0f;
		const ImVec4 meshContentBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 meshBarBgColor = ImVec4(0.12f, 0.12f, 0.13f, 1.0f);
		const ImVec4 meshHeaderAccentColor = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
	}

	MeshAssetEditor::MeshAssetEditor(AssetHandle handle)
		: m_AssetHandle(handle)
	{
		m_MeshAsset = AssetManager::GetInstance().GetMeshAsset(handle);
		m_DisplayName = GetAssetEditorDisplayName(handle);
	}

	void MeshAssetEditor::OnImGuiRender()
	{
		DrawToolbar();

		const ImGuiTableFlags layoutFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_NoSavedSettings;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, meshContentBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(meshBodyHorizontalPadding, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		if (ImGui::BeginChild("##MeshEditorBody", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			if (ImGui::BeginTable("##MeshEditorLayout", 2, layoutFlags, ImVec2(0.0f, 0.0f)))
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

	void MeshAssetEditor::DrawToolbar()
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, meshBarBgColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 8.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::BeginChild("##MeshEditorToolbar", ImVec2(0.0f, meshToolbarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 headerMin = ImGui::GetWindowPos();
		const ImVec2 headerMax(headerMin.x + ImGui::GetWindowSize().x, headerMin.y + ImGui::GetWindowSize().y);
		drawList->AddRectFilled(headerMin, ImVec2(headerMax.x, headerMin.y + 2.0f), ImGui::ColorConvertFloat4ToU32(meshHeaderAccentColor));

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Mesh");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", m_DisplayName.c_str());

		const float buttonWidth = 74.0f;
		ImGui::SameLine(ImMax(0.0f, ImGui::GetContentRegionAvail().x - buttonWidth * 2.0f - 12.0f));

		ImGui::BeginDisabled();
		ImGui::Button("Save", ImVec2(buttonWidth, 0.0f));
		ImGui::SameLine();
		ImGui::Button("Revert", ImVec2(buttonWidth, 0.0f));
		ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor();
	}

	void MeshAssetEditor::DrawPreview()
	{
		ImGui::BeginChild("##MeshPreviewPane", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::TextUnformatted("TODO: Mesh viewport preview");
		ImGui::Separator();
		ImGui::TextUnformatted("This MVP shows mesh metadata first.");
		ImGui::EndChild();
	}

	void MeshAssetEditor::DrawDetails()
	{
		if (!m_MeshAsset)
		{
			ImGui::TextUnformatted("Mesh asset is unavailable.");
			return;
		}

		ImGui::BeginChild("##MeshDetailsPane", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		size_t totalVertices = 0;
		size_t totalIndices = 0;
		for (const auto& primitive : m_MeshAsset->MeshRawData)
		{
			totalVertices += primitive.Vertices.size();
			totalIndices += primitive.Indices.size();
		}

		if (UIAttributeUtil::BeginPropertyTable("##MeshPropertyTable", m_TableStyle))
		{
			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("SubMeshes", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%zu", m_MeshAsset->MeshRawData.size());
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Vertices", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%zu", totalVertices);
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::BeginPropertyRow(m_TableStyle);
			UIAttributeUtil::DrawPropertyLabelCell("Indices", m_TableStyle);
			UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
			ImGui::Text("%zu", totalIndices);
			UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);

			UIAttributeUtil::EndPropertyTable();
		}

		ImGui::Spacing();
		for (size_t i = 0; i < m_MeshAsset->MeshRawData.size(); ++i)
		{
			const auto& primitive = m_MeshAsset->MeshRawData[i];
			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_DefaultOpen, "SubMesh %zu", i))
			{
				ImGui::Text("Vertices: %zu", primitive.Vertices.size());
				ImGui::Text("Indices: %zu", primitive.Indices.size());
				ImGui::TreePop();
			}
		}

		ImGui::EndChild();
	}

}
