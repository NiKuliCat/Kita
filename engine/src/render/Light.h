#pragma once
#include <glm/glm.hpp>


namespace Kita {


	struct LightData
	{
		glm::vec4 Position = { 0.0,0.0,0.0,1.0 };  // w : intensity;
		glm::vec4 Color = { 1.0,1.0,1.0,1.0};
		glm::vec3 Direction = { 0.0,1.0,0.0 };
	};

	struct DirectLightData
	{
		glm::vec4 Direction = { 0.0,1.0,0.0,1.0}; // xyz: dir    w: intensity 
		glm::vec4 Color = { 1.0,1.0,1.0,1.0 };
	};
}
