#pragma once
#include <glm/glm.hpp>


namespace Kita {

	struct LightData
	{
		glm::vec3 Position = { 0.0,0.0,0.0 };
		glm::vec3 Direction = { 0.0,1.0,0.0 };
		glm::vec4 Color = { 1.0,1.0,1.0,1.0};
		float   intensity = 1.0;
	};

	struct DirectLightData
	{
		glm::vec3 Direction = { 0.0,1.0,0.0};
		glm::vec4 Color = { 1.0,1.0,1.0,1.0 };
		float   intensity = 1.0;
	};
	class Light {

	public:
		Light() = default;

		void SetDirection(glm::vec3& direction) { m_LightData.Direction = direction; }
		LightData& GetLightData() { return m_LightData; }
	private:
		LightData m_LightData;
	};

}