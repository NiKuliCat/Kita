#pragma once

#include "entt.hpp"
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

	private:
		void RenderSceneEditor();
	private:
		std::string m_Name = "New Scene";
		entt::registry m_Registry;



	};
}

