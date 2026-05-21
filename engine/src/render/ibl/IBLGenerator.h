#pragma once

#include <array>

#include <vulkan/vulkan.h>

#include "core/Core.h"
#include "render/VulkanContext.h"
#include "render/VulkanTexture.h"
#include "render/scene/SceneRenderConfig.h"

namespace Kita {

	class ImageBasedLighting
	{
	public:
		// 运行时 IBL 资源集合，供后续场景光照阶段直接采样。
		Ref<VulkanTexture> EnvironmentCube;
		Ref<VulkanTexture> IrradianceCube;
		Ref<VulkanTexture> PrefilteredSpecularCube;
		Ref<VulkanTexture> BrdfLut;

		bool IsValid() const
		{
			return EnvironmentCube && EnvironmentCube->IsValid()
				&& IrradianceCube && IrradianceCube->IsValid()
				&& PrefilteredSpecularCube && PrefilteredSpecularCube->IsValid()
				&& BrdfLut && BrdfLut->IsValid();
		}
	};

	struct IBLBuildSettings
	{
		uint32_t EnvironmentCubeSize = 512;
		uint32_t IrradianceCubeSize = 32;
		uint32_t PrefilterCubeSize = 128;
		uint32_t DfgLutSize = 256;
		VkFormat CubeFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		VkFormat DfgFormat = VK_FORMAT_R16G16_SFLOAT;
	};

	struct IBLPreviewImage
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t ComponentCount = 0;
		std::vector<float> Pixels;

		bool IsValid() const
		{
			return Width > 0
				&& Height > 0
				&& ComponentCount > 0
				&& Pixels.size() == static_cast<size_t>(Width) * static_cast<size_t>(Height) * static_cast<size_t>(ComponentCount);
		}
	};

	struct IBLPreviewCubemap
	{
		std::array<IBLPreviewImage, 6> Faces{};
	};

	struct IBLPreviewData
	{
		IBLPreviewCubemap Environment;
		IBLPreviewCubemap Irradiance;
		std::vector<IBLPreviewCubemap> PrefilterMipChain;
		IBLPreviewImage BrdfLut;

		bool IsValid() const
		{
			return BrdfLut.IsValid();
		}
	};

	class IBLGenerator
	{
	public:
		explicit IBLGenerator(VulkanContext& context);

		// 根据场景环境配置生成一套完整的 IBL 运行时纹理。
		Ref<ImageBasedLighting> Build(const SceneRenderSettings& environment);

		// 返回最近一次 Bake 的 CPU 预览数据，供 editor 可视化面板使用。
		const IBLPreviewData* GetLastPreviewData() const { return m_LastPreviewData.get(); }

	private:
		VulkanContext* m_Context = nullptr;
		IBLBuildSettings m_BuildSettings{};
		Unique<IBLPreviewData> m_LastPreviewData = nullptr;
	};

}
