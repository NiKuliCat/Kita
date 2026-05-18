#include "renderer_pch.h"
#include "EditorGizmoController.h"

namespace Kita {

	const float  GizmoSize = 0.2f;
	const bool EnableAxisFlip = false;

	void EditorGizmoController::OnEvent(Event& event, const EditorGizmoContext& context)
	{
		if (!context.ViewportActive || !context.ViewportFocused)
			return;

		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<KeyPressedEvent>([this, &context](KeyPressedEvent& e) { return OnKeyPressed(e, context); });
	}

	void EditorGizmoController::OnImGuiRender(const EditorGizmoContext& context)
	{
		m_IsOver = false;
		m_IsUsing = false;

		if (!context.ViewportActive || !context.Camera)
			return;

		Object selectedObject = context.SelectedObject;
		if (!selectedObject || !selectedObject.HasComponent<Transform>())
			return;

		if (m_Operation == GizmoOperation::None)
			return;

		DrawManipulate(
			selectedObject,
			*context.Camera,
			context.ViewportBoundsMin,
			context.ViewportBoundsMax);
	}

	bool EditorGizmoController::OnKeyPressed(KeyPressedEvent& event, const EditorGizmoContext& context)
	{
		(void)context;
		if (event.IsRepeat())
			return false;

		switch (event.GetKeyCode())
		{
		case Key::Q:
			m_Operation = GizmoOperation::None;
			return true;
		case Key::W:
			m_Operation = GizmoOperation::Translate;
			return true;
		case Key::E:
			m_Operation = GizmoOperation::Rotate;
			return true;
		case Key::R:
			m_Operation = GizmoOperation::Scale;
			return true;
		default:
			return false;
		}

		return false;
	}

	void EditorGizmoController::DrawManipulate(Object object, ViewportCamera& camera, const glm::vec2& boundsMin, const glm::vec2& boundsMax)
	{
		Transform& transform = object.GetComponent<Transform>();

		glm::mat4 transformMatrix = transform.GetTransformMatrix();
		const glm::mat4& view = camera.GetViewMatrix();
		const glm::mat4& projection = camera.GetProjectionMatrix();

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(
			boundsMin.x,
			boundsMin.y,
			boundsMax.x - boundsMin.x,
			boundsMax.y - boundsMin.y);

		ImGuizmo::SetGizmoSizeClipSpace(GizmoSize);
		ImGuizmo::AllowAxisFlip(EnableAxisFlip);

		ImGuizmo::Manipulate(
			glm::value_ptr(view),
			glm::value_ptr(projection),
			ToImGuizmoOperation(m_Operation),
			ToImGuizmoMode(m_Space),
			glm::value_ptr(transformMatrix));

		m_IsOver = ImGuizmo::IsOver();
		m_IsUsing = ImGuizmo::IsUsing();

		if (!m_IsUsing)
			return;

		glm::vec3 translation{};
		glm::vec3 rotation{};
		glm::vec3 scale{};
		if (!Transform::DecomposeTransformMatrix(transformMatrix, translation, rotation, scale))
			return;

		transform.SetPosition(translation);
		transform.SetScale(scale);
		transform.SetRotation(glm::degrees(rotation));
	}

	ImGuizmo::OPERATION EditorGizmoController::ToImGuizmoOperation(GizmoOperation operation)
	{
		switch (operation)
		{
		case GizmoOperation::Translate:
			return ImGuizmo::TRANSLATE;
		case GizmoOperation::Rotate:
			return ImGuizmo::ROTATE;
		case GizmoOperation::Scale:
			return ImGuizmo::SCALE;
		case GizmoOperation::None:
		default:
			return ImGuizmo::TRANSLATE;
		}
	}

	ImGuizmo::MODE EditorGizmoController::ToImGuizmoMode(GizmoSpace space)
	{
		switch (space)
		{
		case GizmoSpace::World:
			return ImGuizmo::WORLD;
		case GizmoSpace::Local:
		default:
			return ImGuizmo::LOCAL;
		}
	}

}
