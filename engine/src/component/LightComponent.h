#pragma once
#include "render/Light.h"

namespace Kita{

	struct LightComponent
	{
		LightComponent() = default;
		glm::vec4 color		= glm::vec4(1.0f);
		float intensity		= 1.0f;
	};
}