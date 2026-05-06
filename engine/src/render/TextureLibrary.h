#pragma once
#include "Texture.h"
#include "core/Log.h"

namespace Kita {

	class TextureLibrary
	{
	public:
		static TextureLibrary& GetInstance()
		{
			static TextureLibrary instance;
			return instance;
		}

		TextureLibrary(const TextureLibrary&) = delete;
		TextureLibrary& operator=(const TextureLibrary&) = delete;

		Ref<Texture> Load(const std::string& path)
		{
			if (Exists(path))
			{
				return m_Textures[path];
			}

			TextureDescriptor desc{};
			Ref<Texture> texture = Texture::Create(desc, path);
			if (texture)
			{
				m_Textures[path] = texture;
			}

			return texture;
		}

		Ref<Texture> Get(const std::string& path)
		{
			if (Exists(path))
			{
				return m_Textures[path];
			}

			KITA_CORE_ERROR("the texture: {0} don't exists in TextureLibrary : {0}", path);
			return nullptr;
		}

		bool Exists(const std::string& path) const
		{
			return m_Textures.find(path) != m_Textures.end();
		}

	private:
		TextureLibrary() = default;

	private:
		std::unordered_map<std::string, Ref<Texture>> m_Textures;
	};
}
