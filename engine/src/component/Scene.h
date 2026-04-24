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

		void OnUpdate(float deltaTime);

		std::string& GetName() { return m_Name; }
		void LoadSkyCubemap(const CubemapFacePaths& faces);

	private:
		void RenderSceneEditor();
	private:
		std::string m_Name = "New Scene";
		entt::registry m_Registry;

		
		Ref<Texture> m_SkyCubemap = nullptr;
	};
}

