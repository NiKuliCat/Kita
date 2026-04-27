#include "kita_pch.h"
#include "JsonUtils.h"
#include "core/Core.h"
#include "core/Log.h"
namespace Kita {
	json JsonUtils::SerializeVec3(const glm::vec3& value)
	{
		return json::array({ value.x, value.y, value.z });
	}

	glm::vec3 JsonUtils::DeserializeVec3(const json& value)
	{
		KITA_CORE_ASSERT(value.is_array() && value.size() == 3, "DeserializeVec3 expects an array of 3 elements.");

		return glm::vec3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
	}

	json JsonUtils::SerializeVec4(const glm::vec4& value)
	{
		return json::array({ value.x, value.y, value.z, value.w });
	}

	glm::vec4 JsonUtils::DeserializeVec4(const json& value)
	{
		KITA_CORE_ASSERT(value.is_array() && value.size() == 4, "DeserializeVec4 expects an array of 4 elements.");

		return glm::vec4(value[0].get<float>(), value[1].get<float>(), value[2].get<float>(), value[3].get<float>());
	}

	json JsonUtils::SerializeUUID(const UUID& uuid)
	{
		return std::to_string((uint64_t)uuid);
	}

	UUID JsonUtils::DeserializeUUID(const json& value)
	{
		KITA_CORE_ASSERT(value.is_string() || value.is_number_unsigned(), "DeserializeUUID expects a string or unsigned integer");

		if (value.is_string())
		{
			return UUID(std::stoull(value.get<std::string>()));
		}

		return UUID(value.get<uint64_t>());
	}
	json JsonUtils::SerializeTransform(const Transform& value)
	{
		json transformJson;
		transformJson["position"] = SerializeVec3(value.GetPosition());
		transformJson["rotation"] = SerializeVec3(value.GetRotation());
		transformJson["scale"] = SerializeVec3(value.GetScale());
		return transformJson;
	}
	Transform JsonUtils::DeserializeTransform(const json& value)
	{
		KITA_CORE_ASSERT(value.is_object(), "DeserializeTransform expects a json object.");

		Transform transform;
		transform.SetPosition(DeserializeVec3(value.at("position")));
		transform.SetRotation(DeserializeVec3(value.at("rotation")));
		transform.SetScale(DeserializeVec3(value.at("scale")));
		return transform;
	}
}