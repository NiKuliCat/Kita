#include "kita_pch.h"

#include "Mesh.h"
#include "core/Log.h"
namespace Kita
{
    Mesh::Mesh(const std::string& filepath)
    {

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            KITA_CORE_ERROR("assimp import error : {0}", import.GetErrorString());
            return;
        }
    }
}