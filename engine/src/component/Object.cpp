#include "kita_pch.h"
#include "Object.h"


namespace Kita {

    Object::Object(entt::entity entityHandle, Scene* scene, const std::string& name)
        :m_EntityHandle(entityHandle), m_Scene(scene) {
        AddComponent<Name>(name);
    }

}
