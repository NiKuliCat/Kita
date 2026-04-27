#include "kita_pch.h"
#include "Object.h"
#include "Transform.h"

namespace Kita {

    Object::Object(entt::entity entityHandle, Scene* scene, const std::string& name)
        :Object(entityHandle,scene,UUID(),name){
    }

    Object::Object(entt::entity entityHandle, Scene* scene, const UUID& uuid, const std::string& name)
        :m_EntityHandle(entityHandle), m_Scene(scene)
    {

        if (!HasComponent<IDComponent>()) {
            AddComponent<IDComponent>(uuid);
        }

        if (!HasComponent<Name>()) {
            AddComponent<Name>(name);
        }

        if (!HasComponent<ObjectType>())
        {
            AddComponent<ObjectType>(Type::StaticMesh);
        }
        if (!HasComponent<Transform>()) {
            AddComponent<Transform>();
        }
    }

    Object::Object(entt::entity entityHandle, Scene* scene)
        :Object(entityHandle,scene,"new object"){
    }
}
