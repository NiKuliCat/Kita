#include "kita_pch.h"
#include "Scene.h"
#include "Object.h"
namespace Kita {

	Scene::~Scene()
	{
	}
	Object Scene::CreateObject(const std::string& name)
	{
		Object obj = { m_Registry.create(), this, name };

		return obj;
	}
	void Scene::DestroyObject(Object object)
	{
		m_Registry.destroy(object);
	}
}

