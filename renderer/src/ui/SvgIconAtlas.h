#pragma once
#include <EngineCore.h>
#include <EngineRender.h>

#include "imgui.h"

namespace Kita {

	class SvgIconAtlas
	{
	public:
		struct IconHandle
		{
			ImTextureID TextureID = 0;
			ImVec2 UV0 = ImVec2(0.0f, 0.0f);
			ImVec2 UV1 = ImVec2(1.0f, 1.0f);
			uint32_t Width = 0;
			uint32_t Height = 0;

			bool IsValid() const
			{
				return TextureID != 0;
			}
		};

	public:
		SvgIconAtlas() = default;
		~SvgIconAtlas();

		SvgIconAtlas(const SvgIconAtlas&) = delete;
		SvgIconAtlas& operator=(const SvgIconAtlas&) = delete;

		bool Load(const std::filesystem::path& atlasJsonPath);
		void Clear();

		IconHandle FindIcon(std::string_view iconName) const;
		bool IsLoaded() const { return !m_Pages.empty() && !m_Icons.empty(); }

	private:
		struct AtlasPage
		{
			Ref<VulkanTexture> Texture = nullptr;
			ImTextureID TextureID = 0;
			uint32_t Width = 0;
			uint32_t Height = 0;
		};

		struct AtlasIcon
		{
			size_t PageIndex = 0;
			ImVec2 UV0 = ImVec2(0.0f, 0.0f);
			ImVec2 UV1 = ImVec2(1.0f, 1.0f);
			uint32_t Width = 0;
			uint32_t Height = 0;
		};

	private:
		bool LoadPageTexture(const std::filesystem::path& imagePath, AtlasPage& outPage);

	private:
		std::vector<AtlasPage> m_Pages;
		std::unordered_map<std::string, AtlasIcon> m_Icons;
	};

}
