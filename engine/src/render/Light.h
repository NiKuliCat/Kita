#pragma once
#include <glm/glm.hpp>


namespace Kita {

	struct LightData
	{
		glm::vec3 Position = { 0.0,0.0,0.0 };
		float _Padding0 = 0.0f;
		glm::vec3 Direction = { 0.0,1.0,0.0 };
		float _Padding1 = 0.0f;
		glm::vec4 Color = { 1.0,1.0,1.0,1.0};
		float   intensity = 1.0;
		glm::vec3 _Padding2 = { 0.0f, 0.0f, 0.0f };
	};

	struct DirectLightData
	{
		glm::vec3 Direction = { 0.0,1.0,0.0};
		float _Padding0 = 0.0f;
		glm::vec4 Color = { 1.0,1.0,1.0,1.0 };
		float   intensity = 1.0;
		glm::vec3 _Padding1 = { 0.0f, 0.0f, 0.0f };
	};

	static_assert(sizeof(LightData) == 64, "LightData must match std140 layout");
	static_assert(sizeof(DirectLightData) == 48, "DirectLightData must match std140 layout");
	class Light {

	public:
		Light() = default;

		void SetDirection(glm::vec3& direction) { m_LightData.Direction = direction; }
		LightData& GetLightData() { return m_LightData; }
	private:
		LightData m_LightData;
	};

}
