#pragma once
#include <Engine.h>

namespace Kita {

	class EditorLayer :public Layer{

	public:
		EditorLayer()
			:Layer("Editor Layer"){}


		virtual ~EditorLayer() = default;


		virtual void OnCreate()  override;
		virtual void OnUpdate(float daltaTime)  override;
		virtual void OnDestroy()  override;

		virtual void OnImGuiRender()  override;
		virtual void OnEvent(Event& event)  override;
	};
}