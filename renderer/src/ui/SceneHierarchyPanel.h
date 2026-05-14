#pragma once
#include <EngineCore.h>
#include "EditorSelectionContext.h"
#include "SvgIconAtlas.h"

namespace Kita {


	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene, const Ref<EditorSelectionContext>& selectionContext)
			:m_SceneContext(scene), m_SelectionContext(selectionContext) {}

		void SetContext(const Ref<Scene>& scene) { m_SceneContext = scene; }
		void SetSelectionContext(const Ref<EditorSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }
		void SetIconAtlas(SvgIconAtlas* iconAtlas) { m_IconAtlas = iconAtlas; }



		void SetSelectedObject(Object obj);

		void OnImGuiRender();
		operator bool() const { return !(m_SceneContext == nullptr); }
	private:
		static const char* ObjectTypeToString(Type type);

		void DrawToolbar();
		void DrawObjectList();
		void DrawAtlasIcon(const SvgIconAtlas::IconHandle& icon, float size, const ImVec4& tint = ImVec4(1, 1, 1, 1));
		void DrawObjectNode(Object obj);
		void OnSlectedObjectChange(Object obj);

	private:
		Ref<Scene> m_SceneContext = nullptr;
		Ref<EditorSelectionContext> m_SelectionContext = nullptr;
		std::array<char, 128> m_SearchBuffer{};
		int m_FilterCategoryIndex = 0;
		entt::entity m_PendingDeleteObject = entt::null;
		SvgIconAtlas* m_IconAtlas = nullptr;
	};

}
