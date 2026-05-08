#pragma once
#include "Engine.h"
namespace Kita {

	class EditorRenderer
	{
	public:
		EditorRenderer() = default;
		EditorRenderer(const Ref<Scene>& scene);

		void Init();
		void OnDestroy();

		void Render();

	private:
		Ref<Scene> m_SceneContext = nullptr;
	};

}