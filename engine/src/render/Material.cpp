#include "kita_pch.h"
#include "Material.h"
#include "asset/AssetManager.h"

namespace Kita {

	Material::Material(const Ref<Shader>& shader, const Ref<Texture>& tex)
		:m_Shader(shader),m_AlbedoTex(tex)
	{
		
	}

	void Material::Bind()
	{
		if (!m_Shader)
			return;

		if (m_AlbedoTex)
		{
			m_AlbedoTex->Bind(3);
			m_Shader->SetInt("MainTex", 3);
		}

		m_Shader->SetColor("BaseColor", m_BaseColor);
	}

}
