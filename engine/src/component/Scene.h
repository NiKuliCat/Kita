#pragma once

#include "entt.hpp"
namespace Kita {


	class Scene
	{

	public:
		Scene() = default;
		Scene(const std::string& name)
			:m_Name(name) {}

		~Scene();


	private:
		std::string m_Name = "New Scene";
		entt::registry m_Registry;

	};
}

