#include "kita_pch.h"
#include "SceneSerializer.h"
#include "asset/AssetManager.h"

namespace
{
	Kita::CurveType DeserializeCurveType(const Kita::json& value)
	{
		const uint32_t curveTypeValue = value.get<uint32_t>();
		if (curveTypeValue == static_cast<uint32_t>(Kita::CurveType::Polyline))
		{
			return Kita::CurveType::Polyline;
		}

		return Kita::CurveType::BezierCubic;
	}

	Kita::BezierHandleMode DeserializeBezierHandleMode(const Kita::json& value)
	{
		const uint32_t handleModeValue = value.get<uint32_t>();
		if (handleModeValue == static_cast<uint32_t>(Kita::BezierHandleMode::Aligned))
		{
			return Kita::BezierHandleMode::Aligned;
		}

		if (handleModeValue == static_cast<uint32_t>(Kita::BezierHandleMode::Mirrored))
		{
			return Kita::BezierHandleMode::Mirrored;
		}

		return Kita::BezierHandleMode::Free;
	}
}

namespace Kita {



	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		:m_Scene(scene)
	{
	}

	bool SceneSerializer::Serialize()
	{
		return Serialize(m_Scene->GetFilePath());
	}

	bool SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		std::filesystem::path path(filepath);
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		json root;
		root["version"] = 1;
		root["scene"]["name"] = m_Scene->GetName();
		root["objects"] = json::array();
		auto view = m_Scene->GetRegistry().view<entt::entity>();
		for (auto entity : view)
		{
			Object object(entity, m_Scene.get());

			json objectJson;
			SerializeObject(objectJson, object);
			root["objects"].push_back(objectJson);
		}

		std::ofstream out(filepath);
		if (!out.is_open())
		{
			return false;
		}

		out << root.dump(4);
		m_Scene->SetFilePath(path);
		KITA_CLENT_DEBUG("{0} scene scuessfully saved to {1}", m_Scene->GetName(), filepath.string());
		return true;
	}

	Ref<Scene> SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream in(filepath);
		if (!in.is_open())
		{
			KITA_CORE_ERROR("Failed to open scene file: {0}", filepath.string());
			return nullptr;
		}
		json root;
		in >> root;

		KITA_CORE_ASSERT(root.is_object(), "Scene file root must be a json object.");
		KITA_CORE_ASSERT(root.contains("version"), "Scene file missing version.");
		KITA_CORE_ASSERT(root.at("version").get<uint32_t>() == 1, "version not is 1.0");
		KITA_CORE_ASSERT(root.contains("scene"), "Scene file missing scene node.");
		KITA_CORE_ASSERT(root.contains("objects"), "Scene file missing objects node.");

		m_Scene->Clear();


		if (root["scene"].contains("name"))
		{
			m_Scene->SetName(root["scene"]["name"].get<std::string>());
		}

		const auto& objects = root["objects"];
		KITA_CORE_ASSERT(objects.is_array(), "Scene objects must be a json array.");

		for (const auto& objectData : objects)
		{
			DeserializeObject(objectData);
		}

		return m_Scene;
	}

	void SceneSerializer::SerializeObject(json& objectJson, Object object)
	{
		objectJson["uuid"] = JsonUtils::SerializeUUID(object.GetUUID());
		objectJson["name"] = object.GetName();
		objectJson["transform"] = JsonUtils::SerializeTransform(object.GetComponent<Transform>());

		if (object.HasComponent<MeshRenderer>())
		{
			objectJson["meshRenderer"] = ComponentSerializer::SerializeMeshRenderer(object.GetComponent<MeshRenderer>());
		}

		if (object.HasComponent<LightComponent>())
		{
			objectJson["lightComponent"] = ComponentSerializer::SerializeLightComponent(object.GetComponent<LightComponent>());
		}

		if (object.HasComponent<LineRenderer>())
		{
			objectJson["lineRenderer"] = ComponentSerializer::SerializeLineRenderer(object.GetComponent<LineRenderer>());
		}
	}

	void SceneSerializer::DeserializeObject(const json& entityData)
	{
		UUID uuid = JsonUtils::DeserializeUUID(entityData.at("uuid"));
		std::string name = entityData.at("name").get<std::string>();

		Object object = m_Scene->CreateObjectWithUUID(uuid, name);
		Transform transform = JsonUtils::DeserializeTransform(entityData.at("transform"));
		object.GetComponent<Transform>() = transform;

		if (entityData.contains("meshRenderer"))
		{
			auto& meshRenderer = object.AddComponent<MeshRenderer>();
			meshRenderer = ComponentSerializer::DeserializeMeshRenderer(entityData.at("meshRenderer"));
		}

		if (entityData.contains("lightComponent"))
		{
			auto& lightComponent = object.AddComponent<LightComponent>();
			lightComponent = ComponentSerializer::DeserializeLightComponent(entityData.at("lightComponent"));
		}

		if (entityData.contains("lineRenderer"))
		{
			auto& lineRenderer = object.AddComponent<LineRenderer>();
			lineRenderer = ComponentSerializer::DeserializeLineRenderer(entityData.at("lineRenderer"));
		}
	}



	json ComponentSerializer::SerializeMeshRenderer(const MeshRenderer& meshRenderer)
	{
		json meshRendererJson;
		meshRendererJson["meshAsset"] =
			JsonUtils::SerializeAssetHandle(meshRenderer.GetMeshAssetHandle());

		meshRendererJson["materialAssets"] = json::array();
		const auto& materialHandles = meshRenderer.GetMaterialAssetHandles();
		for (const AssetHandle materialHandle : materialHandles)
		{
			meshRendererJson["materialAssets"].push_back(
				JsonUtils::SerializeAssetHandle(materialHandle));
		}

		return meshRendererJson;
	}

	MeshRenderer ComponentSerializer::DeserializeMeshRenderer(const json& meshRendererJson)
	{
		MeshRenderer meshRenderer;
		if (!meshRendererJson.is_object())
		{
			return meshRenderer;
		}

		auto& assetManager = AssetManager::GetInstance();

		AssetHandle meshAssetHandle = InvalidAssetHandle;
		if (meshRendererJson.contains("meshAsset"))
		{
			meshAssetHandle = JsonUtils::DeserializeAssetHandle(meshRendererJson.at("meshAsset"));
		}

		if (!Asset::IsValidHandle(meshAssetHandle))
		{
			return meshRenderer;
		}

		const AssetMetadata* meshMetadata = assetManager.GetMetadata(meshAssetHandle);
		if (!meshMetadata)
		{
			KITA_CORE_WARN("DeserializeMeshRenderer failed, mesh asset metadata not found: {}", meshAssetHandle);
			return meshRenderer;
		}

		meshRenderer.LoadMeshs(meshMetadata->relativePath.generic_string());

		if (!meshRendererJson.contains("materialAssets") || !meshRendererJson.at("materialAssets").is_array())
		{
			return meshRenderer;
		}

		const auto& materialArray = meshRendererJson.at("materialAssets");
		auto& materialHandles = meshRenderer.GetMaterialAssetHandles();

		const size_t count = std::min(materialHandles.size(), materialArray.size());
		for (size_t i = 0; i < count; ++i)
		{
			AssetHandle materialHandle = JsonUtils::DeserializeAssetHandle(materialArray[i]);
			if (!Asset::IsValidHandle(materialHandle) || !assetManager.HasHandle(materialHandle))
			{
				materialHandle = InvalidAssetHandle;
			}

			meshRenderer.SetMaterialAssetHandle(i, materialHandle);
			meshRenderer.SyncMaterial(i);
		}

		return meshRenderer;
	}

	json ComponentSerializer::SerializeLightComponent(const LightComponent& lightComponent)
	{
		json lightComponentJson;
		lightComponentJson["color"] = JsonUtils::SerializeVec4(lightComponent.color);
		lightComponentJson["intensity"] = lightComponent.intensity;

		return lightComponentJson;
	}

	LightComponent ComponentSerializer::DeserializeLightComponent(const json& lightComponentJson)
	{
		LightComponent light;
		light.color = JsonUtils::DeserializeVec4(lightComponentJson.at("color"));
		light.intensity = lightComponentJson.at("intensity").get<float>();

		return light;
	}

	json ComponentSerializer::SerializeLineRenderer(const LineRenderer& lineRenderer)
	{
		json lineRendererJson;
		lineRendererJson["curveType"] = static_cast<uint32_t>(lineRenderer.GetCurveType());
		lineRendererJson["lineWidth"] = lineRenderer.GetLineWidth();
		lineRendererJson["lineColor"] = JsonUtils::SerializeVec4(lineRenderer.GetLineColor());

		lineRendererJson["controlPoints"] = json::array();
		const auto& controlPoints = lineRenderer.GetControlPoints();
		for (const auto& point : controlPoints)
		{
			json pointJson;
			pointJson["position"] = JsonUtils::SerializeVec3(point.position);
			pointJson["color"] = JsonUtils::SerializeVec4(point.color);
			lineRendererJson["controlPoints"].push_back(pointJson);
		}

		lineRendererJson["handleModes"] = json::array();
		for (uint32_t i = 0; i < lineRenderer.GetControlPointCount(); i += 3)
		{
			lineRendererJson["handleModes"].push_back(
				static_cast<uint32_t>(lineRenderer.GetHandleModeForPoint(static_cast<int>(i))));
		}

		return lineRendererJson;
	}

	LineRenderer ComponentSerializer::DeserializeLineRenderer(const json& lineRendererJson)
	{
		LineRenderer lineRenderer;
		if (!lineRendererJson.is_object())
		{
			return lineRenderer;
		}

		if (lineRendererJson.contains("curveType"))
		{
			lineRenderer.SetCurveType(DeserializeCurveType(lineRendererJson.at("curveType")));
		}

		if (lineRendererJson.contains("lineWidth"))
		{
			lineRenderer.SetLineWidth(lineRendererJson.at("lineWidth").get<float>());
		}

		if (lineRendererJson.contains("lineColor"))
		{
			lineRenderer.SetLineColor(JsonUtils::DeserializeVec4(lineRendererJson.at("lineColor")));
		}

		std::vector<glm::vec3> serializedPositions;
		std::vector<glm::vec4> serializedColors;
		if (lineRendererJson.contains("controlPoints") && lineRendererJson.at("controlPoints").is_array())
		{
			const auto& controlPointsJson = lineRendererJson.at("controlPoints");
			const size_t pointCount = controlPointsJson.size();
			if (pointCount >= 4 && ((pointCount - 1) % 3 == 0))
			{
				while (lineRenderer.GetControlPointCount() < pointCount)
				{
					lineRenderer.AppendBezierSegment();
				}

				while (lineRenderer.GetControlPointCount() > pointCount)
				{
					lineRenderer.RemoveLastBezierSegment();
				}

				serializedPositions.resize(pointCount);
				serializedColors.resize(pointCount);
				const auto& runtimeControlPoints = lineRenderer.GetControlPoints();
				for (size_t i = 0; i < pointCount; ++i)
				{
					const auto& pointJson = controlPointsJson[i];
					serializedPositions[i] = pointJson.contains("position")
						? JsonUtils::DeserializeVec3(pointJson.at("position"))
						: runtimeControlPoints[i].position;
					serializedColors[i] = pointJson.contains("color")
						? JsonUtils::DeserializeVec4(pointJson.at("color"))
						: runtimeControlPoints[i].color;
				}
			}
			else if (pointCount > 0)
			{
				KITA_CORE_WARN("DeserializeLineRenderer skipped invalid control point count: {}", pointCount);
			}
		}

		if (lineRendererJson.contains("handleModes") && lineRendererJson.at("handleModes").is_array())
		{
			const auto& handleModesJson = lineRendererJson.at("handleModes");
			const size_t anchorCount = (static_cast<size_t>(lineRenderer.GetControlPointCount()) + 2) / 3;
			const size_t count = std::min(anchorCount, handleModesJson.size());
			for (size_t anchorIndex = 0; anchorIndex < count; ++anchorIndex)
			{
				lineRenderer.SetHandleModeForPoint(
					static_cast<int>(anchorIndex * 3),
					DeserializeBezierHandleMode(handleModesJson[anchorIndex]));
			}
		}

		if (!serializedPositions.empty())
		{
			auto& runtimeControlPoints = lineRenderer.GetControlPoints();
			const size_t count = std::min(runtimeControlPoints.size(), serializedPositions.size());
			for (size_t i = 0; i < count; ++i)
			{
				runtimeControlPoints[i].position = serializedPositions[i];
				runtimeControlPoints[i].color = serializedColors[i];
				runtimeControlPoints[i].id = static_cast<int>(i);
			}

			lineRenderer.SetCurveType(lineRenderer.GetCurveType());
		}

		return lineRenderer;
	}

}
