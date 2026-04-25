#pragma once

#include "entt.hpp"
#include "render/Texture.h"
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

		void DestroyObject(Object object);


		std::string& GetName() { return m_Name; }
		void LoadSkyCubemap(const CubemapFacePaths& faces);


		void SimulateSceneEditor();
		void RenderSceneEditor();
	private:
		std::string m_Name = "New Scene";
		entt::registry m_Registry;

		
		Ref<Texture> m_SkyCubemap = nullptr;
	};
}

