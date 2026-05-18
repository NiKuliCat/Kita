#pragma once

#include <EngineCore.h>
#include "scene/ViewportCamera.h"
#include "ImGuizmo.h"
namespace Kita {
	
	enum class GizmoOperation : uint8_t
	{
		None = 0,
		Translate,
		Rotate,
		Scale
	};

	enum class GizmoSpace : uint8_t
	{
		Local = 0,
		World
	};

	struct EditorGizmoContext
	{
		Object SelectedObject{};
		ViewportCamera* Camera = nullptr;
		glm::vec2 ViewportBoundsMin{};
		glm::vec2 ViewportBoundsMax{};
		bool ViewportFocused = false;
		bool ViewportHovered = false;
		bool ViewportActive = false;
	};

	class EditorGizmoController
	{
	public:
		void OnEvent(Event& event, const EditorGizmoContext& context);
		void OnImGuiRender(const EditorGizmoContext& context);
		
		bool IsOver() const { return m_IsOver; }
		bool IsUsing() const { return m_IsUsing; }

		bool WantsCaptureMouse() const { return m_IsUsing; }

		GizmoOperation GetOperation() const { return m_Operation; }
		void SetOperation(GizmoOperation operation) { m_Operation = operation; }

		GizmoSpace GetSpace() const { return m_Space; }
		void SetSpace(GizmoSpace space) { m_Space = space; }

	private:
		bool OnKeyPressed(KeyPressedEvent& event, const EditorGizmoContext& context);
		void DrawManipulate(Object object, ViewportCamera& camera, const glm::vec2& boundsMin, const glm::vec2& boundsMax);
		static ImGuizmo::OPERATION ToImGuizmoOperation(GizmoOperation operation);
		static ImGuizmo::MODE ToImGuizmoMode(GizmoSpace space);

	private:
		GizmoOperation m_Operation = GizmoOperation::Translate;
		GizmoSpace m_Space = GizmoSpace::Local;
		bool m_IsOver = false;
		bool m_IsUsing = false;
	};
}
