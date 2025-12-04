#pragma once
#include "core/Core.h"

namespace Kita {

	struct TextureDescriptor
	{
		bool EnableMipMaps = true;
	};

	class Texture {

	public:
		virtual ~Texture() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetID() const = 0;

		virtual void Bind(uint32_t slot = 0) const = 0;

		static Ref<Texture> Create(const TextureDescriptor& descriptor,const std::string& path);
	};
}