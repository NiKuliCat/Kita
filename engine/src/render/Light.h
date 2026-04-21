#pragma once
#include <glm/glm.hpp>


namespace Kita {

	struct LightData
	{
		glm::vec3 Position = { 0.0,0.0,0.0 };
		glm::vec3 Direction = { 0.0,1.0,0.0 };
		glm::vec4 Color = { 1.0,1.0,1.0,1.0};
		float   intensity = 1.0;
		glm::vec3 _Padding2 = { 0.0f, 0.0f, 0.0f };
	};

	struct DirectLightData
	{
		glm::vec4 Direction = { 0.0,1.0,0.0,1.0}; // xyz: dir    w: intensity 
		glm::vec4 Color = { 1.0,1.0,1.0,1.0 };
	};

	class Light {

	public:
		Light() = default;

		void SetDirection(glm::vec3& direction) { m_LightData.Direction = direction; }
		void SetColor(glm::vec4& color) { m_LightData.Color = color; }
		LightData& GetLightData() { return m_LightData; }
	private:
		LightData m_LightData;
	};

}
