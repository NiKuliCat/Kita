#pragma once

#include "Core.h"
namespace Kita {

	class Input
	{
	public:

		static bool IsKeyPressed(int keycode);
		static bool IsMouseButtonPressed(int mouseButton);
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}