#pragma once
#include <Engine.h>
#include "SceneSelectionContext.h"

namespace Kita {

	class InspectorPanel
	{
	public:
		InspectorPanel() = default;
		InspectorPanel(const Ref<SceneSelectionContext>& selectionContext)
			:m_SelectionContext(selectionContext){ }

		void SetSelectionContext(const Ref<SceneSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }

		void OnImGuiRender();
	private:
		Object& GetSelectedObject()
		{
			static Object emptyObject;
			return m_SelectionContext ? m_SelectionContext->GetSelectedObject() : emptyObject;
		}
		PointData& GetSelectedPoint()
		{
			static PointData emptyPoint{};
			return m_SelectionContext ? m_SelectionContext->GetSelectedPoint() : emptyPoint;
		}
		void ClearSelectedPoint() { if (m_SelectionContext) m_SelectionContext->ClearSelectedPoint(); }

		static const char* ObjectTypeToString(Type type);
		static float GetInspectorContentHeight();
		static float GetInspectorLabelYOffset();
		static float GetInspectorControlYOffset();

		bool BeginPropertyTable(const char* id);
		void EndPropertyTable();
		void BeginPropertyRow(bool& isHighlight);
		void DrawPropertyLabelCell(const char* label);
		void PreparePropertyValueCell(float yOffset);

		void DrawInspectorPanel();
		void DrawSelectedObject(Object& selectedObject, PointData& selectedPoint);
		void DrawObjectInfoSection(Object& selectedObject);
		void DrawLineRendererProperties(LineRenderer& lineRenderer, PointData& selectedPoint);
		void DrawLineRendererControlPoints(LineRenderer& lineRenderer, PointData& selectedPoint);

		void DrawInfoRow(const char* label, const std::string& value, bool& isHighlight);
		void DrawVec3Row(const char* label, glm::vec3& value, bool& isHighlight, float speed = 0.05f);
		void DrawFloatRow(const char* label, float& value, bool& isHighlight, float speed = 0.05f, float minValue = 0.0f);
		void DrawColorRow(const char* label, glm::vec4& value, bool& isHighlight);
		void DrawCurveTypeRow(const char* label, CurveType& curveType, bool& isHighlight);
		void DrawPointCountRow(LineRenderer& lineRenderer, PointData& selectedPoint, bool& isHighlight);
		void DrawAnchorRow(const char* label, glm::vec3& value, BezierHandleMode& handleMode, bool& isHighlight);

		template<typename T, typename DrawFunc>
		void DrawComponentSection(Object& object, const char* label, DrawFunc&& drawFunc, bool enableRemove = true)
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

			ImVec2 contentAvailable = ImGui::GetContentRegionAvail();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), componentNodeFlags, label);
			ImGui::PopStyleVar();

			ImGui::SameLine(contentAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup(popupId.c_str());
			}

			bool remove = false;
			if (ImGui::BeginPopup(popupId.c_str()))
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
	private:
		Ref<SceneSelectionContext> m_SelectionContext = nullptr;
	};
}
