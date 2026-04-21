#include "kita_pch.h"
#include "Object.h"
#include "Transform.h"

namespace Kita {

    Object::Object(entt::entity entityHandle, Scene* scene, const std::string& name)
        :m_EntityHandle(entityHandle), m_Scene(scene) {
        if(!HasComponent<Name>()) {
            AddComponent<Name>(name);
        }
        if(!HasComponent<Transform>()) {
            AddComponent<Transform>();
        }
    }

}
