#pragma once
#include <EngineCore.h>
#include "EditorSelectionContext.h"

namespace Kita {


	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene, const Ref<EditorSelectionContext>& selectionContext)
			:m_SceneContext(scene), m_SelectionContext(selectionContext) {}

		void SetContext(const Ref<Scene>& scene) { m_SceneContext = scene; }
		void SetSelectionContext(const Ref<EditorSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }



		void SetSelectedObject(Object obj);

		void OnImGuiRender();
		operator bool() const { return !(m_SceneContext == nullptr); }
	private:
		void DrawObjectNode(Object obj);
		void OnSlectedObjectChange(Object obj);

	private:
		Ref<Scene> m_SceneContext = nullptr;
		Ref<EditorSelectionContext> m_SelectionContext = nullptr;
	};

}
