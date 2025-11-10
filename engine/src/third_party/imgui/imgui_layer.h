#pragma once
#include "core/Layer.h"

namespace Kita {

	class ImGuiLayer : public Layer{
	public:

		ImGuiLayer();
		~ImGuiLayer();


		virtual void OnCreate() override;
		virtual void OnDestroy() override;
		virtual void OnUpdate(float daltaTime) override;

		virtual void OnImGuiRender() override;



		void SetDarkThemeSpace();
		void Begin();
		void End();


	private:
		float m_Time = 0.0f;
	};
}