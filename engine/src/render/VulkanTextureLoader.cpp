#include "kita_pch.h"
#include "VulkanTextureLoader.h"

#include "core/Log.h"
#include "render/VulkanContext.h"
#include "render/VulkanBuffer.h"
#include "render/VulkanImage.h"
#include "render/VulkanTexture.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Kita {

	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;
		constexpr uint32_t kCubeFaceCount = 6;

		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}

		struct Float4
		{
			float R = 0.0f;
			float G = 0.0f;
			float B = 0.0f;
			float A = 1.0f;
		};

		VkFormat GetVkFormat(const TextureAsset& texAsset)
		{
			switch (texAsset.TexRawData.Format)
			{
			case TexturePixelFormat::R8G8B8A8:
				return texAsset.ImportSettings.ColorSpace == TextureColorSpace::SRGB
					? VK_FORMAT_R8G8B8A8_SRGB
					: VK_FORMAT_R8G8B8A8_UNORM;

			case TexturePixelFormat::R16G16B16A16_Float:
				return VK_FORMAT_R16G16B16A16_SFLOAT;

			case TexturePixelFormat::R32G32B32A32_Float:
				return VK_FORMAT_R32G32B32A32_SFLOAT;

			case TexturePixelFormat::Unknown:
			default:
				return VK_FORMAT_UNDEFINED;
			}
		}

		uint32_t GetBytesPerPixel(TexturePixelFormat format)
		{
			switch (format)
			{
			case TexturePixelFormat::R8G8B8A8:
				return 4;

			case TexturePixelFormat::R16G16B16A16_Float:
				return 8;

			case TexturePixelFormat::R32G32B32A32_Float:
				return 16;

			case TexturePixelFormat::Unknown:
			default:
				return 0;
			}
		}

		float HalfToFloat(uint16_t value)
		{
			const uint32_t sign = (static_cast<uint32_t>(value & 0x8000u)) << 16;
			uint32_t exponent = (value >> 10) & 0x1Fu;
			uint32_t mantissa = value & 0x03FFu;

			uint32_t resultBits = 0;
			if (exponent == 0)
			{
				if (mantissa == 0)
				{
					resultBits = sign;
				}
				else
				{
					exponent = 1;
					while ((mantissa & 0x0400u) == 0)
					{
						mantissa <<= 1;
						--exponent;
					}

					mantissa &= 0x03FFu;
					const uint32_t floatExponent = exponent + (127u - 15u);
					resultBits = sign | (floatExponent << 23) | (mantissa << 13);
				}
			}
			else if (exponent == 0x1Fu)
			{
				resultBits = sign | 0x7F800000u | (mantissa << 13);
			}
			else
			{
				const uint32_t floatExponent = exponent + (127u - 15u);
				resultBits = sign | (floatExponent << 23) | (mantissa << 13);
			}

			float result = 0.0f;
			std::memcpy(&result, &resultBits, sizeof(float));
			return result;
		}

		uint8_t FloatToUnorm8(float value)
		{
			const float clamped = std::clamp(value, 0.0f, 1.0f);
			return static_cast<uint8_t>(std::round(clamped * 255.0f));
		}

		uint16_t FloatToHalf(float value)
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
		}

		Float4 LoadPixelAsFloat4(const TexturePrimitiveData& data, uint32_t x, uint32_t y)
		{
			KITA_CORE_ASSERT(data.Width > 0 && data.Height > 0, "Texture source dimensions must be valid");

			x = std::min(x, data.Width - 1);
			y = std::min(y, data.Height - 1);

			const size_t pixelIndex = static_cast<size_t>(y) * static_cast<size_t>(data.Width) + static_cast<size_t>(x);

			switch (data.Format)
			{
			case TexturePixelFormat::R8G8B8A8:
			{
				const size_t offset = pixelIndex * 4;
				return {
					data.Pixels[offset + 0] / 255.0f,
					data.Pixels[offset + 1] / 255.0f,
					data.Pixels[offset + 2] / 255.0f,
					data.Pixels[offset + 3] / 255.0f
				};
			}

			case TexturePixelFormat::R16G16B16A16_Float:
			{
				const size_t offset = pixelIndex * 8;
				const uint16_t* halfValues = reinterpret_cast<const uint16_t*>(data.Pixels.data() + offset);
				return {
					HalfToFloat(halfValues[0]),
					HalfToFloat(halfValues[1]),
					HalfToFloat(halfValues[2]),
					HalfToFloat(halfValues[3])
				};
			}

			case TexturePixelFormat::R32G32B32A32_Float:
			{
				const size_t offset = pixelIndex * 16;
				const float* floatValues = reinterpret_cast<const float*>(data.Pixels.data() + offset);
				return {
					floatValues[0],
					floatValues[1],
					floatValues[2],
					floatValues[3]
				};
			}

			case TexturePixelFormat::Unknown:
			default:
				return {};
			}
		}

		Float4 SamplePanoramaNearest(const TexturePrimitiveData& data, const glm::vec3& direction)
		{
			const glm::vec3 dir = glm::normalize(direction);
			const float u = std::atan2(dir.z, dir.x) / (2.0f * kPi) + 0.5f;
			const float v = std::acos(std::clamp(dir.y, -1.0f, 1.0f)) / kPi;

			const float wrappedU = u - std::floor(u);
			const float clampedV = std::clamp(v, 0.0f, 1.0f);

			const uint32_t x = std::min(
				static_cast<uint32_t>(wrappedU * static_cast<float>(data.Width)),
				data.Width - 1);
			const uint32_t y = std::min(
				static_cast<uint32_t>(clampedV * static_cast<float>(data.Height)),
				data.Height - 1);

			return LoadPixelAsFloat4(data, x, y);
		}

		glm::vec3 GetCubeFaceDirection(uint32_t faceIndex, float u, float v)
		{
			switch (faceIndex)
			{
			case 0: return glm::normalize(glm::vec3(1.0f, -v, -u));   // +X
			case 1: return glm::normalize(glm::vec3(-1.0f, -v, u));   // -X
			case 2: return glm::normalize(glm::vec3(u, 1.0f, v));     // +Y
			case 3: return glm::normalize(glm::vec3(u, -1.0f, -v));   // -Y
			case 4: return glm::normalize(glm::vec3(u, -v, 1.0f));    // +Z
			case 5: return glm::normalize(glm::vec3(-u, -v, -1.0f));  // -Z
			default: return glm::vec3(0.0f, 0.0f, 1.0f);
			}
		}

		uint32_t ComputeCubeFaceSize(const TexturePrimitiveData& data)
		{
			const uint32_t widthBased = std::max(1u, data.Width / 4u);
			const uint32_t heightBased = std::max(1u, data.Height / 2u);
			return std::max(1u, std::min(widthBased, heightBased));
		}

		bool BuildCubeFacesFromPanorama(
			const TextureAsset& texAsset,
			uint32_t faceSize,
			std::array<std::vector<uint8_t>, kCubeFaceCount>& outFaceData)
		{
			const TexturePrimitiveData& source = texAsset.TexRawData;
			const uint32_t bytesPerPixel = GetBytesPerPixel(source.Format);
			if (bytesPerPixel == 0 || !source.IsValid())
			{
				KITA_CORE_ERROR("VulkanTextureLoader: invalid source texture data for cubemap conversion: {}", texAsset.SourcePath.string());
				return false;
			}

			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				std::vector<uint8_t>& facePixels = outFaceData[faceIndex];
				facePixels.resize(static_cast<size_t>(faceSize) * static_cast<size_t>(faceSize) * bytesPerPixel);

				for (uint32_t y = 0; y < faceSize; ++y)
				{
					for (uint32_t x = 0; x < faceSize; ++x)
					{
						const float nx = ((static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) * 2.0f - 1.0f;
						const float ny = ((static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) * 2.0f - 1.0f;

						const glm::vec3 direction = GetCubeFaceDirection(faceIndex, nx, ny);
						const Float4 sampled = SamplePanoramaNearest(source, direction);
						const size_t pixelIndex = static_cast<size_t>(y) * static_cast<size_t>(faceSize) + static_cast<size_t>(x);

						switch (source.Format)
						{
						case TexturePixelFormat::R8G8B8A8:
						{
							const size_t offset = pixelIndex * 4;
							facePixels[offset + 0] = FloatToUnorm8(sampled.R);
							facePixels[offset + 1] = FloatToUnorm8(sampled.G);
							facePixels[offset + 2] = FloatToUnorm8(sampled.B);
							facePixels[offset + 3] = FloatToUnorm8(sampled.A);
							break;
						}

						case TexturePixelFormat::R16G16B16A16_Float:
						{
							const size_t offset = pixelIndex * 8;
							uint16_t* dest = reinterpret_cast<uint16_t*>(facePixels.data() + offset);
							dest[0] = FloatToHalf(sampled.R);
							dest[1] = FloatToHalf(sampled.G);
							dest[2] = FloatToHalf(sampled.B);
							dest[3] = FloatToHalf(sampled.A);
							break;
						}

						case TexturePixelFormat::R32G32B32A32_Float:
						{
							const size_t offset = pixelIndex * 16;
							float* dest = reinterpret_cast<float*>(facePixels.data() + offset);
							dest[0] = sampled.R;
							dest[1] = sampled.G;
							dest[2] = sampled.B;
							dest[3] = sampled.A;
							break;
						}

						case TexturePixelFormat::Unknown:
						default:
							KITA_CORE_ERROR("VulkanTextureLoader: unsupported cubemap source pixel format for '{}'", texAsset.SourcePath.string());
							return false;
						}
					}
				}
			}

			return true;
		}

		Ref<VulkanTexture> CreateTexture2D(VulkanContext& context, const TextureAsset& texAsset)
		{
			const TexturePrimitiveData& data = texAsset.TexRawData;
			const VkFormat format = GetVkFormat(texAsset);
			if (format == VK_FORMAT_UNDEFINED)
			{
				KITA_CORE_ERROR("VulkanTextureLoader: unsupported texture format for '{}'", texAsset.SourcePath.string());
				return nullptr;
			}

			const VkDeviceSize imageSize =
				static_cast<VkDeviceSize>(data.Width) *
				static_cast<VkDeviceSize>(data.Height) *
				static_cast<VkDeviceSize>(GetBytesPerPixel(data.Format));

			VulkanTexture::CreateInfo textureInfo{};
			textureInfo.Name = texAsset.SourcePath.filename().string();
			textureInfo.Type = TextureType::Texture2D;
			textureInfo.Width = static_cast<uint32_t>(data.Width);
			textureInfo.Height = static_cast<uint32_t>(data.Height);
			textureInfo.Format = format;
			textureInfo.CreateSampler = true;
			textureInfo.Filter = VK_FILTER_LINEAR;
			textureInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			textureInfo.MaxAnisotropy = 1.0f;
			textureInfo.EnableMipmaps = false;
			textureInfo.MipLevels = 1;
			textureInfo.PixelData = data.Pixels.data();
			textureInfo.PixelDataSize = imageSize;

			try
			{
				return CreateRef<VulkanTexture>(context, textureInfo);
			}
			catch (const std::exception& e)
			{
				KITA_CORE_ERROR(
					"VulkanTextureLoader: failed to create 2D VulkanTexture from '{0}': {1}",
					texAsset.SourcePath.string(),
					e.what());
				return nullptr;
			}
		}

		Ref<VulkanTexture> CreateTextureCubeFromPanorama(VulkanContext& context, const TextureAsset& texAsset)
		{
			const TexturePrimitiveData& data = texAsset.TexRawData;
			const VkFormat format = GetVkFormat(texAsset);
			if (format == VK_FORMAT_UNDEFINED)
			{
				KITA_CORE_ERROR("VulkanTextureLoader: unsupported cubemap source format for '{}'", texAsset.SourcePath.string());
				return nullptr;
			}

			const uint32_t faceSize = ComputeCubeFaceSize(data);
			if (faceSize == 0)
			{
				KITA_CORE_ERROR("VulkanTextureLoader: failed to compute cubemap face size for '{}'", texAsset.SourcePath.string());
				return nullptr;
			}

			std::array<std::vector<uint8_t>, kCubeFaceCount> faceData{};
			if (!BuildCubeFacesFromPanorama(texAsset, faceSize, faceData))
			{
				return nullptr;
			}

			VulkanTexture::CreateInfo textureInfo{};
			textureInfo.Name = texAsset.SourcePath.filename().string();
			textureInfo.Type = TextureType::TextureCube;
			textureInfo.Width = faceSize;
			textureInfo.Height = faceSize;
			textureInfo.Format = format;
			textureInfo.CreateSampler = true;
			textureInfo.Filter = VK_FILTER_LINEAR;
			textureInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			textureInfo.MaxAnisotropy = 1.0f;
			textureInfo.EnableMipmaps = false;
			textureInfo.MipLevels = 1;

			Ref<VulkanTexture> texture = nullptr;
			try
			{
				texture = CreateRef<VulkanTexture>(context, textureInfo);
			}
			catch (const std::exception& e)
			{
				KITA_CORE_ERROR(
					"VulkanTextureLoader: failed to create cubemap VulkanTexture from '{0}': {1}",
					texAsset.SourcePath.string(),
					e.what());
				return nullptr;
			}

			VulkanImage& image = texture->GetImage();
			const uint32_t bytesPerPixel = GetBytesPerPixel(data.Format);
			const VkDeviceSize faceByteSize =
				static_cast<VkDeviceSize>(faceSize) *
				static_cast<VkDeviceSize>(faceSize) *
				static_cast<VkDeviceSize>(bytesPerPixel);
			const VkDeviceSize totalByteSize = faceByteSize * static_cast<VkDeviceSize>(kCubeFaceCount);

			VulkanBuffer::CreateInfo stagingInfo{};
			stagingInfo.Name = textureInfo.Name + "_CubeUploadStaging";
			stagingInfo.Size = totalByteSize;
			stagingInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

			VulkanBuffer stagingBuffer(context, stagingInfo);
			uint8_t* mapped = static_cast<uint8_t*>(stagingBuffer.Map(totalByteSize, 0));
			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				std::memcpy(
					mapped + static_cast<size_t>(faceIndex * faceByteSize),
					faceData[faceIndex].data(),
					faceData[faceIndex].size());
			}
			stagingBuffer.Unmap();

			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = context.GetCommandPool();
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			try
			{
				VKCheck(
					vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer),
					"VulkanTextureLoader: failed to allocate cubemap upload command buffer");

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				VKCheck(
					vkBeginCommandBuffer(commandBuffer, &beginInfo),
					"VulkanTextureLoader: failed to begin cubemap upload command buffer");

				image.TransitionLayout(
					commandBuffer,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1,
					0, kCubeFaceCount);

				std::array<VkBufferImageCopy, kCubeFaceCount> copyRegions{};
				for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
				{
					VkBufferImageCopy& region = copyRegions[faceIndex];
					region.bufferOffset = faceByteSize * static_cast<VkDeviceSize>(faceIndex);
					region.bufferRowLength = 0;
					region.bufferImageHeight = 0;
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.mipLevel = 0;
					region.imageSubresource.baseArrayLayer = faceIndex;
					region.imageSubresource.layerCount = 1;
					region.imageOffset = { 0, 0, 0 };
					region.imageExtent = { faceSize, faceSize, 1 };
				}

				vkCmdCopyBufferToImage(
					commandBuffer,
					stagingBuffer.GetHandle(),
					image.GetHandle(),
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					kCubeFaceCount,
					copyRegions.data());

				image.TransitionLayout(
					commandBuffer,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_ASPECT_COLOR_BIT,
					0, 1,
					0, kCubeFaceCount);

				VKCheck(
					vkEndCommandBuffer(commandBuffer),
					"VulkanTextureLoader: failed to end cubemap upload command buffer");

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffer;

				VKCheck(
					vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
					"VulkanTextureLoader: failed to submit cubemap upload command buffer");
				VKCheck(
					vkQueueWaitIdle(context.GetGraphicsQueue()),
					"VulkanTextureLoader: failed to wait for cubemap upload completion");

				vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
			}
			catch (const std::exception&)
			{
				if (commandBuffer != VK_NULL_HANDLE)
				{
					vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
				}
				return nullptr;
			}

			return texture;
		}
	}

	Ref<VulkanTexture> VulkanTextureLoader::LoadTexture(
		VulkanContext& context,
		const TextureAsset& texAsset)
	{
		if (!texAsset.IsValidSource())
		{
			KITA_CORE_ERROR("VulkanTextureLoader: texture asset has invalid source data: {}", texAsset.SourcePath.string());
			return nullptr;
		}

		if (texAsset.IsCube())
		{
			return CreateTextureCubeFromPanorama(context, texAsset);
		}

		return CreateTexture2D(context, texAsset);
	}

}
