#pragma once
#include <EngineCore.h>
#include "EditorSelectionContext.h"
namespace Kita {

	class InspectorPanel
	{
	public:
		InspectorPanel() = default;
		InspectorPanel(const Ref<EditorSelectionContext>& selectionContext)
			:m_SelectionContext(selectionContext){ }

		void SetSelectionContext(const Ref<EditorSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }
		void SetOpenAssetCallback(std::function<void(AssetHandle)> callback) { m_OpenAssetCallback = std::move(callback); }

		void OnImGuiRender();
	private:

		void DrawItemByType(EditorSelectionItemType type);

		static float GetInspectorContentHeight();
		static float GetInspectorLabelYOffset();
		static float GetInspectorControlYOffset();

		bool BeginPropertyTable(const char* id);
		void EndPropertyTable();
		void BeginPropertyRow(bool& isHighlight);
		void DrawPropertyLabelCell(const char* label);
		void PreparePropertyValueCell(float yOffset);
		void PreparePropertyResetCell(float yOffset);
		void DrawEmptyResetCell();
		bool DrawResetButtonCell(const char* id, bool enabled = true);

		void DrawInspectorPanel();
		void DrawSelectedObject(Object& selectedObject);
		void DrawSelectedAsset(AssetHandle handle);
		void DrawComponentOverview(Object& selectedObject);
		void DrawAddComponentMenu(Object& selectedObject);

		void DrawTransformProperties(Transform& transform);
		void DrawStaticMeshProperties(MeshRenderer& meshRenderer);
		void DrawStaticMeshSlotRow(
			const char* label,
			AssetHandle& meshHandle,
			const std::vector<AssetMetadata>& meshAssets,
			bool& isHighlight,
			AssetHandle defaultValue = InvalidAssetHandle);
		void DrawMeshRendererProperties(MeshRenderer& meshRenderer);
		void DrawLightComponentProperties(LightComponent& lightComponent);
		void DrawMaterialSlotRow(
			const char* label,
			size_t slotIndex,
			AssetHandle& slotMaterialHandle,
			AssetHandle defaultMaterialHandle,
			const std::vector<AssetMetadata>& materialAssets,
			bool& isHighlight);
		void OpenMaterialEditor(AssetHandle materialHandle);
		static std::string GetAssetDisplayName(AssetHandle handle);
		static std::string GetAssetPathLabel(AssetHandle handle);

		void DrawInfoRow(const char* label, const std::string& value, bool& isHighlight);
		void DrawVec3Row(const char* label, glm::vec3& value, bool& isHighlight, float speed = 0.05f, const glm::vec3& defaultValue = glm::vec3(0.0f));
		void DrawFloatRow(const char* label, float& value, bool& isHighlight, float speed = 0.05f, float minValue = 0.0f, float maxValue = 0.0f, float defaultValue = 0.0f);
		void DrawColorRow(const char* label, glm::vec4& value, bool& isHighlight, const glm::vec4& defaultValue = glm::vec4(1.0f));
		void DrawAssetSelectionRow(const char* label, AssetHandle& value, const std::vector<AssetMetadata>& assets, bool& isHighlight, AssetHandle defaultValue = InvalidAssetHandle);

		template<typename T, typename DrawFunc>
		void DrawComponentSection(Object& object, const char* label, DrawFunc&& drawFunc, bool enableRemove = true, bool showOptionsButton = true)
		{
			if (!object.HasComponent<T>())
			{
				return;
			}

			auto& component = object.GetComponent<T>();
			const ImGuiTreeNodeFlags componentNodeFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_AllowOverlap |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_FramePadding;
			const std::string popupId = std::string(label) + "##ComponentMoreOperation";

			const float lineHeight = 30.0f;
			const float contentMinX = ImGui::GetWindowContentRegionMin().x;
			const float contentMaxX = ImGui::GetWindowContentRegionMax().x;
			const float headerTextPaddingY = (lineHeight - ImGui::GetTextLineHeight()) * 0.5f;

			ImGui::SetCursorPosX(contentMinX);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, headerTextPaddingY });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
			bool open = ImGui::TreeNodeEx(label, componentNodeFlags);
			const ImVec2 headerMin = ImGui::GetItemRectMin();
			const ImVec2 headerMax = ImGui::GetItemRectMax();
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(2);

			if (showOptionsButton)
			{
				ImGui::SetCursorScreenPos(ImVec2(headerMax.x - lineHeight, headerMin.y));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.0f, 0.0f });
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.27f, 0.27f, 0.27f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.27f, 0.27f, 0.27f, 1.0f));
				if (ImGui::Button("...", ImVec2{ lineHeight, lineHeight }))
				{
					ImGui::OpenPopup(popupId.c_str());
				}
				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar(2);
				ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMax.y));
			}
			else
			{
				ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMax.y));
			}

			bool remove = false;
			if (showOptionsButton && ImGui::BeginPopup(popupId.c_str()))
			{
				if (ImGui::MenuItem("Remove Component", nullptr, false, enableRemove))
				{
					remove = true;
				}
				ImGui::EndPopup();
			}

			if (open)
			{
				drawFunc(component);
				ImGui::TreePop();
			}

			if (remove)
			{
				object.RemoveComponent<T>();
			}
		}

		template<typename DrawFunc>
		void DrawComponentPropertyTable(const char* id, DrawFunc&& drawFunc)
		{
			const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
			ImGui::Unindent(treeIndent);

			if (BeginPropertyTable(id))
			{
				bool isHighlight = false;
				std::forward<DrawFunc>(drawFunc)(isHighlight);
				EndPropertyTable();
			}

			ImGui::Indent(treeIndent);
		}
	private:
		Ref<EditorSelectionContext> m_SelectionContext = nullptr;
		std::function<void(AssetHandle)> m_OpenAssetCallback = nullptr;
	};
}
