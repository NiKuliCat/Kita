#include "renderer_pch.h"
#include "SvgIconAtlas.h"

#include "core/Application.h"
#include "third_party/stb_image/stb_image.h"

#include <backends/imgui_impl_vulkan.h>
#include <nlohmann/json.hpp>

namespace Kita {

	SvgIconAtlas::~SvgIconAtlas()
	{
		Clear();
	}

	bool SvgIconAtlas::Load(const std::filesystem::path& atlasJsonPath)
	{
		Clear();

		if (atlasJsonPath.empty() || !std::filesystem::exists(atlasJsonPath))
		{
			KITA_CORE_WARN("SvgIconAtlas: atlas json not found: {}", atlasJsonPath.string());
			return false;
		}

		std::ifstream input(atlasJsonPath);
		if (!input.is_open())
		{
			KITA_CORE_WARN("SvgIconAtlas: failed to open atlas json: {}", atlasJsonPath.string());
			return false;
		}

		nlohmann::json root;
		input >> root;

		if (!root.contains("pages") || !root.at("pages").is_array())
		{
			KITA_CORE_WARN("SvgIconAtlas: invalid atlas json, missing pages array");
			return false;
		}

		if (!root.contains("icons") || !root.at("icons").is_object())
		{
			KITA_CORE_WARN("SvgIconAtlas: invalid atlas json, missing icons object");
			return false;
		}

		const std::filesystem::path atlasDir = atlasJsonPath.parent_path();

		for (const auto& pageNode : root.at("pages"))
		{
			if (!pageNode.contains("file") || !pageNode.at("file").is_string())
			{
				continue;
			}

			AtlasPage page{};
			const std::filesystem::path imagePath = atlasDir / pageNode.at("file").get<std::string>();
			if (!LoadPageTexture(imagePath, page))
			{
				Clear();
				return false;
			}

			m_Pages.push_back(std::move(page));
		}

		for (auto it = root.at("icons").begin(); it != root.at("icons").end(); ++it)
		{
			const auto& iconNode = it.value();
			if (!iconNode.is_object())
			{
				continue;
			}

			const size_t pageIndex = iconNode.value("page", 0u);
			if (pageIndex >= m_Pages.size())
			{
				continue;
			}

			AtlasIcon icon{};
			icon.PageIndex = pageIndex;
			icon.UV0 = ImVec2(iconNode.value("u0", 0.0f), iconNode.value("v0", 0.0f));
			icon.UV1 = ImVec2(iconNode.value("u1", 1.0f), iconNode.value("v1", 1.0f));
			icon.Width = iconNode.value("w", 0u);
			icon.Height = iconNode.value("h", 0u);

			m_Icons.emplace(it.key(), icon);
		}

		if (m_Pages.empty() || m_Icons.empty())
		{
			Clear();
			KITA_CORE_WARN("SvgIconAtlas: atlas loaded no usable pages or icons: {}", atlasJsonPath.string());
			return false;
		}

		KITA_CORE_INFO("SvgIconAtlas loaded: {}", atlasJsonPath.string());
		return true;
	}

	void SvgIconAtlas::Clear()
	{
		if (!m_Pages.empty())
		{
			Application::Get().GetVulkanContext().WaitIdle();
		}

		for (auto& page : m_Pages)
		{
			if (page.TextureID)
			{
				ImGui_ImplVulkan_RemoveTexture(
					reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(page.TextureID)));
				page.TextureID = 0;
			}

			page.Texture = nullptr;
			page.Width = 0;
			page.Height = 0;
		}

		m_Pages.clear();
		m_Icons.clear();
	}

	SvgIconAtlas::IconHandle SvgIconAtlas::FindIcon(std::string_view iconName) const
	{
		const auto it = m_Icons.find(std::string(iconName));
		if (it == m_Icons.end())
		{
			return {};
		}

		const AtlasIcon& icon = it->second;
		if (icon.PageIndex >= m_Pages.size())
		{
			return {};
		}

		const AtlasPage& page = m_Pages[icon.PageIndex];
		return { page.TextureID, icon.UV0, icon.UV1, icon.Width, icon.Height };
	}

	bool SvgIconAtlas::LoadPageTexture(const std::filesystem::path& imagePath, AtlasPage& outPage)
	{
		if (!std::filesystem::exists(imagePath))
		{
			KITA_CORE_WARN("SvgIconAtlas: page image not found: {}", imagePath.string());
			return false;
		}

		int width = 0;
		int height = 0;
		int channels = 0;
		stbi_set_flip_vertically_on_load(0);

		stbi_uc* pixels = stbi_load(imagePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels)
		{
			KITA_CORE_WARN("SvgIconAtlas: failed to load page image: {}", imagePath.string());
			return false;
		}

		const size_t pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

		VulkanTexture::CreateInfo createInfo{};
		createInfo.Name = imagePath.stem().string();
		createInfo.Width = static_cast<uint32_t>(width);
		createInfo.Height = static_cast<uint32_t>(height);
		createInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		createInfo.CreateSampler = true;
		createInfo.Filter = VK_FILTER_LINEAR;
		createInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.PixelData = pixels;
		createInfo.PixelDataSize = pixelBytes;

		outPage.Texture = CreateRef<VulkanTexture>(Application::Get().GetVulkanContext(), createInfo);
		stbi_image_free(pixels);

		if (!outPage.Texture || !outPage.Texture->IsValid())
		{
			KITA_CORE_WARN("SvgIconAtlas: failed to create VulkanTexture: {}", imagePath.string());
			return false;
		}

		const VulkanImage& image = outPage.Texture->GetImage();
		outPage.TextureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
			ImGui_ImplVulkan_AddTexture(
				image.GetSampler(),
				image.GetView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));
		outPage.Width = static_cast<uint32_t>(width);
		outPage.Height = static_cast<uint32_t>(height);
		return outPage.TextureID != 0;
	}

}
