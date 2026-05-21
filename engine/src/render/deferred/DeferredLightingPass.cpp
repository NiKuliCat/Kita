#include "kita_pch.h"
#include "DeferredLightingPass.h"

#include "render/VulkanContext.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanRenderTarget.h"

namespace Kita {

	namespace
	{
		Ref<VulkanTexture> CreateSolidColorCube(
			VulkanContext& context,
			const std::string& name,
			float r,
			float g,
			float b,
			float a)
		{
			VulkanTexture::CreateInfo createInfo{};
			createInfo.Name = name;
			createInfo.Type = TextureType::TextureCube;
			createInfo.Width = 1;
			createInfo.Height = 1;
			createInfo.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
			createInfo.Filter = VK_FILTER_LINEAR;
			createInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.EnableMipmaps = false;
			createInfo.MipLevels = 1;

			Ref<VulkanTexture> texture = CreateRef<VulkanTexture>(context, createInfo);
			if (!texture || !texture->IsValid())
				return nullptr;

			auto floatToHalf = [](float value) -> uint16_t
			{
				uint32_t bits = 0;
				std::memcpy(&bits, &value, sizeof(uint32_t));

				const uint32_t sign = (bits >> 16) & 0x8000u;
				int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
				uint32_t mantissa = bits & 0x007FFFFFu;

				if (exponent <= 0)
				{
					if (exponent < -10)
						return static_cast<uint16_t>(sign);

					mantissa = (mantissa | 0x00800000u) >> (1 - exponent);
					return static_cast<uint16_t>(sign | ((mantissa + 0x00001000u) >> 13));
				}

				if (exponent >= 31)
				{
					return static_cast<uint16_t>(sign | 0x7C00u);
				}

				return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exponent) << 10) | ((mantissa + 0x00001000u) >> 13));
			};

			std::array<uint16_t, 4> facePixel = {
				floatToHalf(r),
				floatToHalf(g),
				floatToHalf(b),
				floatToHalf(a)
			};

			std::vector<uint16_t> cubePixels(6 * 4, 0);
			for (size_t faceIndex = 0; faceIndex < 6; ++faceIndex)
			{
				std::memcpy(cubePixels.data() + faceIndex * 4, facePixel.data(), sizeof(uint16_t) * 4);
			}

			VulkanBuffer::CreateInfo stagingInfo{};
			stagingInfo.Name = name + "_UploadStaging";
			stagingInfo.Size = static_cast<VkDeviceSize>(cubePixels.size() * sizeof(uint16_t));
			stagingInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingInfo.InitialData = cubePixels.data();
			stagingInfo.InitialDataSize = stagingInfo.Size;

			VulkanBuffer stagingBuffer(context, stagingInfo);

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = context.GetCommandPool();
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			VulkanImage& image = texture->GetImage();
			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				image.GetCurrentLayout(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,
				1,
				0,
				6);

			std::array<VkBufferImageCopy, 6> copyRegions{};
			for (uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
			{
				VkBufferImageCopy& region = copyRegions[faceIndex];
				region.bufferOffset = static_cast<VkDeviceSize>(faceIndex) * sizeof(uint16_t) * 4;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = faceIndex;
				region.imageSubresource.layerCount = 1;
				region.imageExtent = { 1, 1, 1 };
			}

			vkCmdCopyBufferToImage(
				commandBuffer,
				stagingBuffer.GetHandle(),
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(copyRegions.size()),
				copyRegions.data());

			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,
				1,
				0,
				6);

			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(context.GetGraphicsQueue());
			vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);

			return texture;
		}

		Ref<VulkanTexture> CreateDefaultBrdfLut(VulkanContext& context, const std::string& name)
		{
			VulkanTexture::CreateInfo createInfo{};
			createInfo.Name = name;
			createInfo.Type = TextureType::Texture2D;
			createInfo.Width = 1;
			createInfo.Height = 1;
			createInfo.Format = VK_FORMAT_R16G16_SFLOAT;
			createInfo.Filter = VK_FILTER_LINEAR;
			createInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.EnableMipmaps = false;
			createInfo.MipLevels = 1;

			const std::array<uint16_t, 2> pixels = { 0u, 0u };
			createInfo.PixelData = pixels.data();
			createInfo.PixelDataSize = static_cast<VkDeviceSize>(pixels.size() * sizeof(uint16_t));
			return CreateRef<VulkanTexture>(context, createInfo);
		}
	}

	DeferredLightingPass::DeferredLightingPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		: FullscreenPassBase(sceneBindings, std::move(desc))
	{
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		Destroy();
	}

	void DeferredLightingPass::Init(VulkanContext& context, uint32_t framesInFlight)
	{
		InitDescriptorSets(context, framesInFlight);
		m_FallbackIrradianceCube = CreateSolidColorCube(context, "FallbackIrradianceCube", 0.0f, 0.0f, 0.0f, 1.0f);
		m_FallbackPrefilterCube = CreateSolidColorCube(context, "FallbackPrefilterCube", 0.0f, 0.0f, 0.0f, 1.0f);
		m_FallbackEnvironmentCube = CreateSolidColorCube(context, "FallbackEnvironmentCube", 0.0f, 0.0f, 0.0f, 1.0f);
		m_FallbackBrdfLut = CreateDefaultBrdfLut(context, "FallbackBrdfLut");
	}

	void DeferredLightingPass::Destroy()
	{
		for (auto& descriptorSet : m_DescriptorSets)
		{
			descriptorSet.Destroy();
		}
		m_DescriptorSets.clear();
		m_FallbackIrradianceCube.reset();
		m_FallbackPrefilterCube.reset();
		m_FallbackBrdfLut.reset();
		m_FallbackEnvironmentCube.reset();
		m_Context = nullptr;
		m_GBufferRenderTarget = nullptr;
	}

	void DeferredLightingPass::SetGBufferInput(const VulkanRenderTarget* gbufferRenderTarget)
	{
		m_GBufferRenderTarget = gbufferRenderTarget;
	}


	void DeferredLightingPass::UpdateFrameResources(uint32_t frameIndex)
	{
		UpdateDescriptorSet(frameIndex);
	}

	void DeferredLightingPass::BindAdditionalResources(
		RenderPassContext& context,
		VkCommandBuffer commandBuffer,
		const VulkanGraphicsPipeline& pipeline,
		uint32_t frameIndex)
	{
		(void)context;

		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].Bind(commandBuffer, pipeline.GetLayout(), 1);
	}

	void DeferredLightingPass::InitDescriptorSets(VulkanContext& context, uint32_t framesInFlight)
	{
		Destroy();

		m_Context = &context;
		const uint32_t frameCount = std::max(1u, framesInFlight);
		m_DescriptorSets.resize(frameCount);

		VulkanDescriptorSet::CreateInfo descInfo{};
		descInfo.Name = "DeferredLightingGBuffer_Set";
		descInfo.Bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // baseColor
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // normal
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // material
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // emissive
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // depth
			{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // irradiance cube
			{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // prefilter cube
			{ 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }, // brdf lut
			{ 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }  // environment cube(optional debug / fallback)
		};

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
			perFrameDescInfo.Name += "_" + std::to_string(i);
			m_DescriptorSets[i].Init(context, perFrameDescInfo);
		}
	}

	void DeferredLightingPass::UpdateDescriptorSet(uint32_t frameIndex)
	{
		if (!m_Context || !m_GBufferRenderTarget)
			return;
		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].WriteImageSampler(0, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(0));
		m_DescriptorSets[frameIndex].WriteImageSampler(1, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(1));
		m_DescriptorSets[frameIndex].WriteImageSampler(2, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(2));
		m_DescriptorSets[frameIndex].WriteImageSampler(3, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(3));
		m_DescriptorSets[frameIndex].WriteImageSampler(4, m_GBufferRenderTarget->GetDepthDescriptorInfo());

		const Ref<VulkanTexture>& irradianceTexture =
			(m_IBL && m_IBL->IsValid() && m_IBL->IrradianceCube)
			? m_IBL->IrradianceCube
			: m_FallbackIrradianceCube;
		const Ref<VulkanTexture>& prefilterTexture =
			(m_IBL && m_IBL->IsValid() && m_IBL->PrefilteredSpecularCube)
			? m_IBL->PrefilteredSpecularCube
			: m_FallbackPrefilterCube;
		const Ref<VulkanTexture>& brdfTexture =
			(m_IBL && m_IBL->IsValid() && m_IBL->BrdfLut)
			? m_IBL->BrdfLut
			: m_FallbackBrdfLut;
		const Ref<VulkanTexture>& environmentTexture =
			(m_IBL && m_IBL->IsValid() && m_IBL->EnvironmentCube)
			? m_IBL->EnvironmentCube
			: m_FallbackEnvironmentCube;

		KITA_CORE_ASSERT(irradianceTexture && irradianceTexture->IsValid(), "DeferredLightingPass requires a valid irradiance texture");
		KITA_CORE_ASSERT(prefilterTexture && prefilterTexture->IsValid(), "DeferredLightingPass requires a valid prefilter texture");
		KITA_CORE_ASSERT(brdfTexture && brdfTexture->IsValid(), "DeferredLightingPass requires a valid brdf lut texture");
		KITA_CORE_ASSERT(environmentTexture && environmentTexture->IsValid(), "DeferredLightingPass requires a valid environment texture");

		m_DescriptorSets[frameIndex].WriteImageSampler(5, irradianceTexture->GetDescriptorInfo());
		m_DescriptorSets[frameIndex].WriteImageSampler(6, prefilterTexture->GetDescriptorInfo());
		m_DescriptorSets[frameIndex].WriteImageSampler(7, brdfTexture->GetDescriptorInfo());
		m_DescriptorSets[frameIndex].WriteImageSampler(8, environmentTexture->GetDescriptorInfo());
	}

	RenderPassDesc MakeDeferredLightingPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "DeferredLightingPass";
		desc.Type = PassType::DeferredLighting;
		desc.Samples = renderTarget.GetCreateInfo().Samples;
		desc.UseDepthAttachment = renderTarget.HasDepthAttachment();

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i)
		{
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));
		}

		if (renderTarget.HasDepthAttachment())
		{
			desc.DepthFormat = renderTarget.GetDepthFormat();
		}

		return desc;
	}

}
