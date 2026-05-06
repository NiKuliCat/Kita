#pragma once

#include "entt.hpp"
#include "render/Texture.h"
#include "render/Light.h"
#include "core/UUID.h"
namespace Kita {

	class Object;
	class Scene
	{

	public:
		Scene() = default;
		Scene(const std::string& name)
			:m_Name(name) {}

		~Scene();

	public:
		entt::registry& GetRegistry() { return m_Registry; }
		Object CreateObject(const std::string& name);
		Object CreateObjectWithUUID(UUID uuid, const std::string& name);

		void DestroyObject(Object object);


		std::string& GetName() { return m_Name; }
		void SetName(const std::string& name ) { m_Name = name; }
		void LoadSkyCubemap(const CubemapFacePaths& faces);

		const std::filesystem::path& GetFilePath() const { return m_FilePath; }
		void SetFilePath(const std::filesystem::path& path) { m_FilePath = path; }
		bool HasFilePath() const { return !m_FilePath.empty(); }

		void Clear();

		void SimulateSceneEditor();
		void RenderSceneEditor();
		DirectLightData GetMainDirectLightData() const;
	private:
		std::string m_Name = "New Scene";
		entt::registry m_Registry;

		
		Ref<Texture> m_SkyCubemap = nullptr;

		std::filesystem::path m_FilePath;
	};
}

