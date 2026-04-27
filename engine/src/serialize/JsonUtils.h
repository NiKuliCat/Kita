#pragma once


#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <core/UUID.h>
#include "component/Transform.h"
namespace Kita {

	using json = nlohmann::ordered_json;

	class JsonUtils
	{

	public:

		static json SerializeVec3(const glm::vec3& value);
		static glm::vec3 DeserializeVec3(const json& value);

		static json SerializeVec4(const glm::vec4& value);
		static glm::vec4 DeserializeVec4(const json& value);


		static json SerializeUUID(const UUID& uuid);
		static UUID DeserializeUUID(const json& value);

		static json SerializeTransform(const Transform& value);
		static Transform DeserializeTransform(const json& value);
	};
}