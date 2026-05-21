#include "kita_pch.h"
#include "IBLGenerator.h"

#include "asset/AssetManager.h"
#include "core/Log.h"
#include "render/VulkanBuffer.h"

#include <array>
#include <cmath>

namespace Kita {

	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;
		constexpr uint32_t kCubeFaceCount = 6;
		constexpr uint32_t kIrradiancePhiSamples = 32;
		constexpr uint32_t kIrradianceThetaSamples = 16;
		constexpr uint32_t kPrefilterSampleCount = 256;
		constexpr uint32_t kBrdfSampleCount = 512;

		struct Float4
		{
			float R = 0.0f;
			float G = 0.0f;
			float B = 0.0f;
			float A = 1.0f;
		};

		struct PixelBuffer
		{
			std::vector<float> Data;

			void Resize(size_t pixelCount, uint32_t componentCount)
			{
				Data.assign(pixelCount * componentCount, 0.0f);
			}
		};

		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}

		float RadicalInverse_VdC(uint32_t bits)
		{
			bits = (bits << 16u) | (bits >> 16u);
			bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
			bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
			bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
			bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
			return static_cast<float>(bits) * 2.3283064365386963e-10f;
		}

		glm::vec2 Hammersley(uint32_t index, uint32_t count)
		{
			return glm::vec2(
				static_cast<float>(index) / static_cast<float>(count),
				RadicalInverse_VdC(index));
		}

		glm::vec3 ImportanceSampleGGX(const glm::vec2& xi, const glm::vec3& normal, float roughness)
		{
			const float a = roughness * roughness;
			const float phi = 2.0f * kPi * xi.x;
			const float cosTheta = std::sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
			const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

			glm::vec3 halfVector{};
			halfVector.x = std::cos(phi) * sinTheta;
			halfVector.y = std::sin(phi) * sinTheta;
			halfVector.z = cosTheta;

			const glm::vec3 up = std::abs(normal.z) < 0.999f
				? glm::vec3(0.0f, 0.0f, 1.0f)
				: glm::vec3(1.0f, 0.0f, 0.0f);
			const glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
			const glm::vec3 bitangent = glm::cross(normal, tangent);

			return glm::normalize(tangent * halfVector.x + bitangent * halfVector.y + normal * halfVector.z);
		}

		float GeometrySchlickGGX(float ndotV, float roughness)
		{
			const float a = roughness;
			const float k = (a * a) * 0.5f;
			const float denom = ndotV * (1.0f - k) + k;
			return ndotV / std::max(denom, 1e-5f);
		}

		float GeometrySmith(float ndotV, float ndotL, float roughness)
		{
			const float ggx1 = GeometrySchlickGGX(ndotV, roughness);
			const float ggx2 = GeometrySchlickGGX(ndotL, roughness);
			return ggx1 * ggx2;
		}

		glm::vec2 IntegrateBrdf(float ndotV, float roughness)
		{
			const glm::vec3 view(
				std::sqrt(std::max(0.0f, 1.0f - ndotV * ndotV)),
				0.0f,
				ndotV);
			const glm::vec3 normal(0.0f, 0.0f, 1.0f);

			float scale = 0.0f;
			float bias = 0.0f;
			for (uint32_t i = 0; i < kBrdfSampleCount; ++i)
			{
				const glm::vec2 xi = Hammersley(i, kBrdfSampleCount);
				const glm::vec3 halfVector = ImportanceSampleGGX(xi, normal, roughness);
				const glm::vec3 light = glm::normalize(2.0f * glm::dot(view, halfVector) * halfVector - view);

				const float ndotL = std::max(light.z, 0.0f);
				const float ndotH = std::max(halfVector.z, 0.0f);
				const float vdotH = std::max(glm::dot(view, halfVector), 0.0f);

				if (ndotL > 0.0f)
				{
					const float visibility = GeometrySmith(ndotV, ndotL, roughness);
					const float gVis = (visibility * vdotH) / std::max(ndotH * ndotV, 1e-5f);
					const float fc = std::pow(1.0f - vdotH, 5.0f);

					scale += (1.0f - fc) * gVis;
					bias += fc * gVis;
				}
			}

			return glm::vec2(scale, bias) / static_cast<float>(kBrdfSampleCount);
		}

		glm::vec3 RotateDirectionY(const glm::vec3& direction, float radians)
		{
			if (std::abs(radians) <= 1e-6f)
				return direction;

			const float c = std::cos(radians);
			const float s = std::sin(radians);
			return glm::normalize(glm::vec3(
				c * direction.x + s * direction.z,
				direction.y,
				-s * direction.x + c * direction.z));
		}

		glm::vec3 GetCubeFaceDirection(uint32_t faceIndex, float u, float v)
		{
			switch (faceIndex)
			{
			case 0: return glm::normalize(glm::vec3(1.0f, -v, -u));
			case 1: return glm::normalize(glm::vec3(-1.0f, -v, u));
			case 2: return glm::normalize(glm::vec3(u, 1.0f, v));
			case 3: return glm::normalize(glm::vec3(u, -1.0f, -v));
			case 4: return glm::normalize(glm::vec3(u, -v, 1.0f));
			case 5: return glm::normalize(glm::vec3(-u, -v, -1.0f));
			default: return glm::vec3(0.0f, 0.0f, 1.0f);
			}
		}

		uint32_t GetBytesPerPixel(TexturePixelFormat format)
		{
			switch (format)
			{
			case TexturePixelFormat::R8G8B8A8: return 4;
			case TexturePixelFormat::R16G16B16A16_Float: return 8;
			case TexturePixelFormat::R32G32B32A32_Float: return 16;
			default: return 0;
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
			default:
				return {};
			}
		}

		Float4 SampleEquirectangular(const TexturePrimitiveData& data, const glm::vec3& direction)
		{
			const glm::vec3 dir = glm::normalize(direction);
			const float u = std::atan2(dir.z, dir.x) / (2.0f * kPi) + 0.5f;
			const float v = std::acos(std::clamp(dir.y, -1.0f, 1.0f)) / kPi;

			const float wrappedU = u - std::floor(u);
			const float clampedV = std::clamp(v, 0.0f, 1.0f);
			const uint32_t x = std::min(static_cast<uint32_t>(wrappedU * static_cast<float>(data.Width)), data.Width - 1);
			const uint32_t y = std::min(static_cast<uint32_t>(clampedV * static_cast<float>(data.Height)), data.Height - 1);
			return LoadPixelAsFloat4(data, x, y);
		}

		Float4 SampleCubePixels(
			const std::array<PixelBuffer, kCubeFaceCount>& cubeFaces,
			uint32_t faceSize,
			const glm::vec3& direction)
		{
			const glm::vec3 absDir = glm::abs(direction);
			uint32_t faceIndex = 0;
			float uc = 0.0f;
			float vc = 0.0f;

			if (absDir.x >= absDir.y && absDir.x >= absDir.z)
			{
				if (direction.x > 0.0f)
				{
					faceIndex = 0;
					uc = -direction.z / absDir.x;
					vc = -direction.y / absDir.x;
				}
				else
				{
					faceIndex = 1;
					uc = direction.z / absDir.x;
					vc = -direction.y / absDir.x;
				}
			}
			else if (absDir.y >= absDir.x && absDir.y >= absDir.z)
			{
				if (direction.y > 0.0f)
				{
					faceIndex = 2;
					uc = direction.x / absDir.y;
					vc = direction.z / absDir.y;
				}
				else
				{
					faceIndex = 3;
					uc = direction.x / absDir.y;
					vc = -direction.z / absDir.y;
				}
			}
			else
			{
				if (direction.z > 0.0f)
				{
					faceIndex = 4;
					uc = direction.x / absDir.z;
					vc = -direction.y / absDir.z;
				}
				else
				{
					faceIndex = 5;
					uc = -direction.x / absDir.z;
					vc = -direction.y / absDir.z;
				}
			}

			const float u = (uc + 1.0f) * 0.5f;
			const float v = (vc + 1.0f) * 0.5f;
			const uint32_t x = std::min(static_cast<uint32_t>(u * static_cast<float>(faceSize)), faceSize - 1);
			const uint32_t y = std::min(static_cast<uint32_t>(v * static_cast<float>(faceSize)), faceSize - 1);
			const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(faceSize) + static_cast<size_t>(x)) * 4;

			const std::vector<float>& data = cubeFaces[faceIndex].Data;
			return { data[pixelIndex + 0], data[pixelIndex + 1], data[pixelIndex + 2], data[pixelIndex + 3] };
		}

		Float4 SampleEnvironment(
			const TexturePrimitiveData& sourceData,
			EnvironmentSourceType sourceType,
			const std::array<PixelBuffer, kCubeFaceCount>& sourceCubeFaces,
			uint32_t sourceCubeFaceSize,
			const glm::vec3& direction,
			float rotationY)
		{
			const glm::vec3 rotatedDirection = RotateDirectionY(direction, rotationY);
			if (sourceType == EnvironmentSourceType::Cubemap)
				return SampleCubePixels(sourceCubeFaces, sourceCubeFaceSize, rotatedDirection);

			return SampleEquirectangular(sourceData, rotatedDirection);
		}

		bool BuildSourceCubeFaces(
			const TexturePrimitiveData& sourceData,
			EnvironmentSourceType sourceType,
			uint32_t faceSize,
			float rotationY,
			std::array<PixelBuffer, kCubeFaceCount>& outCubeFaces)
		{
			if (faceSize == 0)
				return false;

			if (sourceType == EnvironmentSourceType::Cubemap)
			{
				KITA_CORE_WARN("IBLGenerator: cubemap source sampling is not implemented yet in asset raw data path, falling back to equirectangular interpretation.");
			}

			const uint32_t sourceCubeFaceSize = std::max(1u, std::min(sourceData.Width, sourceData.Height));
			std::array<PixelBuffer, kCubeFaceCount> emptySourceCube{};

			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				outCubeFaces[faceIndex].Resize(static_cast<size_t>(faceSize) * static_cast<size_t>(faceSize), 4);
				std::vector<float>& facePixels = outCubeFaces[faceIndex].Data;

				for (uint32_t y = 0; y < faceSize; ++y)
				{
					for (uint32_t x = 0; x < faceSize; ++x)
					{
						const float u = ((static_cast<float>(x) + 0.5f) / static_cast<float>(faceSize)) * 2.0f - 1.0f;
						const float v = ((static_cast<float>(y) + 0.5f) / static_cast<float>(faceSize)) * 2.0f - 1.0f;
						const glm::vec3 direction = GetCubeFaceDirection(faceIndex, u, v);
						const Float4 sample = SampleEnvironment(
							sourceData,
							EnvironmentSourceType::Equirectangular,
							emptySourceCube,
							sourceCubeFaceSize,
							direction,
							rotationY);

						const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(faceSize) + static_cast<size_t>(x)) * 4;
						facePixels[pixelIndex + 0] = sample.R;
						facePixels[pixelIndex + 1] = sample.G;
						facePixels[pixelIndex + 2] = sample.B;
						facePixels[pixelIndex + 3] = sample.A;
					}
				}
			}

			return true;
		}

		glm::vec3 ToVec3(const Float4& sample)
		{
			return glm::vec3(sample.R, sample.G, sample.B);
		}

		void BuildIrradianceCube(
			const std::array<PixelBuffer, kCubeFaceCount>& environmentCubeFaces,
			uint32_t environmentCubeSize,
			uint32_t irradianceSize,
			float intensity,
			std::array<PixelBuffer, kCubeFaceCount>& outIrradianceFaces)
		{
			// 这里先采用 CPU 半球积分，优先把结果链路跑通；
			// 后续如需提速，可以再替换成 compute / raster 预计算 pass。
			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				outIrradianceFaces[faceIndex].Resize(static_cast<size_t>(irradianceSize) * static_cast<size_t>(irradianceSize), 4);
				std::vector<float>& dst = outIrradianceFaces[faceIndex].Data;

				for (uint32_t y = 0; y < irradianceSize; ++y)
				{
					for (uint32_t x = 0; x < irradianceSize; ++x)
					{
						const float u = ((static_cast<float>(x) + 0.5f) / static_cast<float>(irradianceSize)) * 2.0f - 1.0f;
						const float v = ((static_cast<float>(y) + 0.5f) / static_cast<float>(irradianceSize)) * 2.0f - 1.0f;
						const glm::vec3 normal = GetCubeFaceDirection(faceIndex, u, v);

						const glm::vec3 up = std::abs(normal.y) < 0.999f
							? glm::vec3(0.0f, 1.0f, 0.0f)
							: glm::vec3(1.0f, 0.0f, 0.0f);
						const glm::vec3 tangent = glm::normalize(glm::cross(up, normal));
						const glm::vec3 bitangent = glm::cross(normal, tangent);

						glm::vec3 irradiance(0.0f);
						float sampleWeight = 0.0f;
						for (uint32_t phiIndex = 0; phiIndex < kIrradiancePhiSamples; ++phiIndex)
						{
							const float phi = (static_cast<float>(phiIndex) + 0.5f) / static_cast<float>(kIrradiancePhiSamples) * 2.0f * kPi;
							for (uint32_t thetaIndex = 0; thetaIndex < kIrradianceThetaSamples; ++thetaIndex)
							{
								const float theta = (static_cast<float>(thetaIndex) + 0.5f) / static_cast<float>(kIrradianceThetaSamples) * 0.5f * kPi;
								const float sinTheta = std::sin(theta);
								const float cosTheta = std::cos(theta);

								const glm::vec3 localDir(
									sinTheta * std::cos(phi),
									cosTheta,
									sinTheta * std::sin(phi));
								const glm::vec3 sampleDir = glm::normalize(
									tangent * localDir.x +
									normal * localDir.y +
									bitangent * localDir.z);

								const glm::vec3 env = ToVec3(SampleCubePixels(environmentCubeFaces, environmentCubeSize, sampleDir));
								const float weight = cosTheta * sinTheta;
								irradiance += env * weight;
								sampleWeight += weight;
							}
						}

						if (sampleWeight > 0.0f)
							irradiance *= (kPi / sampleWeight);

						irradiance *= intensity;
						const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(irradianceSize) + static_cast<size_t>(x)) * 4;
						dst[pixelIndex + 0] = irradiance.r;
						dst[pixelIndex + 1] = irradiance.g;
						dst[pixelIndex + 2] = irradiance.b;
						dst[pixelIndex + 3] = 1.0f;
					}
				}
			}
		}

		void BuildPrefilterCube(
			const std::array<PixelBuffer, kCubeFaceCount>& environmentCubeFaces,
			uint32_t environmentCubeSize,
			uint32_t prefilterSize,
			float intensity,
			std::vector<std::array<PixelBuffer, kCubeFaceCount>>& outMipFaces)
		{
			// split-sum 的 specular 预滤波贴图，按 roughness 映射到不同 mip。
			const uint32_t mipCount = std::max(1u, static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(prefilterSize)))) + 1u);
			outMipFaces.resize(mipCount);

			for (uint32_t mip = 0; mip < mipCount; ++mip)
			{
				const uint32_t mipSize = std::max(1u, prefilterSize >> mip);
				const float roughness = mipCount > 1
					? static_cast<float>(mip) / static_cast<float>(mipCount - 1)
					: 0.0f;

				for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
				{
					outMipFaces[mip][faceIndex].Resize(static_cast<size_t>(mipSize) * static_cast<size_t>(mipSize), 4);
					std::vector<float>& dst = outMipFaces[mip][faceIndex].Data;

					for (uint32_t y = 0; y < mipSize; ++y)
					{
						for (uint32_t x = 0; x < mipSize; ++x)
						{
							const float u = ((static_cast<float>(x) + 0.5f) / static_cast<float>(mipSize)) * 2.0f - 1.0f;
							const float v = ((static_cast<float>(y) + 0.5f) / static_cast<float>(mipSize)) * 2.0f - 1.0f;
							const glm::vec3 normal = GetCubeFaceDirection(faceIndex, u, v);
							const glm::vec3 view = normal;

							glm::vec3 prefiltered(0.0f);
							float totalWeight = 0.0f;
							for (uint32_t sampleIndex = 0; sampleIndex < kPrefilterSampleCount; ++sampleIndex)
							{
								const glm::vec2 xi = Hammersley(sampleIndex, kPrefilterSampleCount);
								const glm::vec3 halfVector = ImportanceSampleGGX(xi, normal, std::max(roughness, 0.02f));
								const glm::vec3 light = glm::normalize(2.0f * glm::dot(view, halfVector) * halfVector - view);
								const float ndotL = std::max(glm::dot(normal, light), 0.0f);
								if (ndotL <= 0.0f)
									continue;

								const glm::vec3 env = ToVec3(SampleCubePixels(environmentCubeFaces, environmentCubeSize, light));
								prefiltered += env * ndotL;
								totalWeight += ndotL;
							}

							if (totalWeight > 0.0f)
								prefiltered /= totalWeight;

							prefiltered *= intensity;
							const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(mipSize) + static_cast<size_t>(x)) * 4;
							dst[pixelIndex + 0] = prefiltered.r;
							dst[pixelIndex + 1] = prefiltered.g;
							dst[pixelIndex + 2] = prefiltered.b;
							dst[pixelIndex + 3] = 1.0f;
						}
					}
				}
			}
		}

		void BuildBrdfLut(uint32_t size, PixelBuffer& outPixels)
		{
			outPixels.Resize(static_cast<size_t>(size) * static_cast<size_t>(size), 2);
			for (uint32_t y = 0; y < size; ++y)
			{
				for (uint32_t x = 0; x < size; ++x)
				{
					const float ndotV = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
					const float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
					const glm::vec2 integrated = IntegrateBrdf(ndotV, roughness);
					const size_t pixelIndex = (static_cast<size_t>(y) * static_cast<size_t>(size) + static_cast<size_t>(x)) * 2;
					outPixels.Data[pixelIndex + 0] = integrated.x;
					outPixels.Data[pixelIndex + 1] = integrated.y;
				}
			}
		}

		std::vector<uint8_t> ConvertFloatPixelsToRgba16f(const std::vector<float>& src, uint32_t componentCount)
		{
			KITA_CORE_ASSERT(componentCount <= 4, "RGBA16F conversion supports up to 4 components");

			const size_t pixelCount = src.size() / componentCount;
			std::vector<uint8_t> bytes(pixelCount * 8);
			uint16_t* dst = reinterpret_cast<uint16_t*>(bytes.data());

			for (size_t i = 0; i < pixelCount; ++i)
			{
				const size_t base = i * componentCount;
				dst[i * 4 + 0] = FloatToHalf(src[base + 0]);
				dst[i * 4 + 1] = FloatToHalf(componentCount > 1 ? src[base + 1] : 0.0f);
				dst[i * 4 + 2] = FloatToHalf(componentCount > 2 ? src[base + 2] : 0.0f);
				dst[i * 4 + 3] = FloatToHalf(componentCount > 3 ? src[base + 3] : 1.0f);
			}

			return bytes;
		}

		std::vector<uint8_t> ConvertFloatPixelsToRg16f(const std::vector<float>& src)
		{
			const size_t pixelCount = src.size() / 2;
			std::vector<uint8_t> bytes(pixelCount * 4);
			uint16_t* dst = reinterpret_cast<uint16_t*>(bytes.data());

			for (size_t i = 0; i < pixelCount; ++i)
			{
				dst[i * 2 + 0] = FloatToHalf(src[i * 2 + 0]);
				dst[i * 2 + 1] = FloatToHalf(src[i * 2 + 1]);
			}

			return bytes;
		}

		VkCommandBuffer BeginSingleTimeCommands(VulkanContext& context)
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = context.GetCommandPool();
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VKCheck(
				vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer),
				"IBLGenerator: failed to allocate single-time command buffer");

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			VKCheck(
				vkBeginCommandBuffer(commandBuffer, &beginInfo),
				"IBLGenerator: failed to begin single-time command buffer");
			return commandBuffer;
		}

		void EndSingleTimeCommands(VulkanContext& context, VkCommandBuffer commandBuffer)
		{
			VKCheck(
				vkEndCommandBuffer(commandBuffer),
				"IBLGenerator: failed to end single-time command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VKCheck(
				vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
				"IBLGenerator: failed to submit single-time command buffer");
			VKCheck(
				vkQueueWaitIdle(context.GetGraphicsQueue()),
				"IBLGenerator: failed to wait for graphics queue idle");
			vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
		}

		Ref<VulkanTexture> CreateRuntimeCubeTexture(
			VulkanContext& context,
			const std::string& name,
			uint32_t size,
			VkFormat format,
			bool enableMipmaps)
		{
			VulkanTexture::CreateInfo createInfo{};
			createInfo.Name = name;
			createInfo.Type = TextureType::TextureCube;
			createInfo.Width = size;
			createInfo.Height = size;
			createInfo.Format = format;
			createInfo.Filter = VK_FILTER_LINEAR;
			createInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.EnableMipmaps = enableMipmaps;
			createInfo.MipLevels = enableMipmaps
				? std::max(1u, static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(size)))) + 1u)
				: 1u;
			return CreateRef<VulkanTexture>(context, createInfo);
		}

		Ref<VulkanTexture> CreateRuntimeTexture2D(
			VulkanContext& context,
			const std::string& name,
			uint32_t width,
			uint32_t height,
			VkFormat format)
		{
			VulkanTexture::CreateInfo createInfo{};
			createInfo.Name = name;
			createInfo.Type = TextureType::Texture2D;
			createInfo.Width = width;
			createInfo.Height = height;
			createInfo.Format = format;
			createInfo.Filter = VK_FILTER_LINEAR;
			createInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.EnableMipmaps = false;
			createInfo.MipLevels = 1;
			return CreateRef<VulkanTexture>(context, createInfo);
		}

		bool UploadCubeMipPixels(
			VulkanContext& context,
			VulkanTexture& texture,
			const std::array<PixelBuffer, kCubeFaceCount>& facePixels,
			uint32_t size,
			uint32_t mipLevel)
		{
			// 立方体纹理需要按当前 mip 的 6 个 face 一次性上传，
			// 避免被 VulkanImage 的整图 layout 跟踪逻辑干扰。
			VulkanImage& image = texture.GetImage();
			const uint32_t mipWidth = std::max(1u, size >> mipLevel);
			const uint32_t mipHeight = std::max(1u, size >> mipLevel);
			const size_t facePixelCount = static_cast<size_t>(mipWidth) * static_cast<size_t>(mipHeight);
			const size_t faceByteCount = facePixelCount * 8;
			const VkDeviceSize totalByteCount = static_cast<VkDeviceSize>(faceByteCount * kCubeFaceCount);

			std::vector<uint8_t> mergedBytes(static_cast<size_t>(totalByteCount));
			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				const std::vector<uint8_t> bytes = ConvertFloatPixelsToRgba16f(facePixels[faceIndex].Data, 4);
				std::memcpy(
					mergedBytes.data() + static_cast<size_t>(faceIndex) * faceByteCount,
					bytes.data(),
					faceByteCount);
			}

			VulkanBuffer::CreateInfo stagingInfo{};
			stagingInfo.Name = texture.GetName() + "_MipUpload_" + std::to_string(mipLevel);
			stagingInfo.Size = totalByteCount;
			stagingInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingInfo.InitialData = mergedBytes.data();
			stagingInfo.InitialDataSize = totalByteCount;

			VulkanBuffer stagingBuffer(context, stagingInfo);
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);

			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				image.GetCurrentLayout(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				mipLevel,
				1,
				0,
				kCubeFaceCount);

			std::array<VkBufferImageCopy, kCubeFaceCount> copyRegions{};
			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				VkBufferImageCopy& region = copyRegions[faceIndex];
				region.bufferOffset = static_cast<VkDeviceSize>(faceIndex) * static_cast<VkDeviceSize>(faceByteCount);
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = mipLevel;
				region.imageSubresource.baseArrayLayer = faceIndex;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { mipWidth, mipHeight, 1 };
			}

			vkCmdCopyBufferToImage(
				commandBuffer,
				stagingBuffer.GetHandle(),
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				kCubeFaceCount,
				copyRegions.data());

			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				mipLevel,
				1,
				0,
				kCubeFaceCount);
			EndSingleTimeCommands(context, commandBuffer);
			return true;
		}

		bool UploadBrdfPixels(
			VulkanContext& context,
			VulkanTexture& texture,
			const PixelBuffer& pixels)
		{
			const std::vector<uint8_t> bytes = ConvertFloatPixelsToRg16f(pixels.Data);
			VulkanImage& image = texture.GetImage();

			VulkanBuffer::CreateInfo stagingInfo{};
			stagingInfo.Name = texture.GetName() + "_UploadStaging";
			stagingInfo.Size = static_cast<VkDeviceSize>(bytes.size());
			stagingInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			stagingInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			stagingInfo.InitialData = bytes.data();
			stagingInfo.InitialDataSize = static_cast<VkDeviceSize>(bytes.size());

			VulkanBuffer stagingBuffer(context, stagingInfo);
			VkCommandBuffer commandBuffer = BeginSingleTimeCommands(context);

			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				image.GetCurrentLayout(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,
				1,
				0,
				1);

			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { texture.GetWidth(), texture.GetHeight(), 1 };

			vkCmdCopyBufferToImage(
				commandBuffer,
				stagingBuffer.GetHandle(),
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region);

			VulkanImage::TransitionImageLayout(
				commandBuffer,
				image.GetHandle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,
				1,
				0,
				1);
			EndSingleTimeCommands(context, commandBuffer);
			return true;
		}

		IBLPreviewImage BuildPreviewImageFromPixelBuffer(
			const PixelBuffer& pixels,
			uint32_t width,
			uint32_t height,
			uint32_t componentCount)
		{
			IBLPreviewImage preview{};
			preview.Width = width;
			preview.Height = height;
			preview.ComponentCount = componentCount;
			preview.Pixels = pixels.Data;
			return preview;
		}
	}

	IBLGenerator::IBLGenerator(VulkanContext& context)
		: m_Context(&context)
	{
	}

	Ref<ImageBasedLighting> IBLGenerator::Build(const SceneRenderSettings& environment)
	{
		m_LastPreviewData.reset();

		if (!m_Context)
		{
			KITA_CORE_ERROR("IBLGenerator: VulkanContext is null.");
			return nullptr;
		}

		if (!Asset::IsValidHandle(environment.EnvironmentSourceTexHandle))
		{
			KITA_CORE_WARN("IBLGenerator: environment source handle is invalid.");
			return nullptr;
		}

		Ref<TextureAsset> sourceAsset = AssetManager::GetInstance().GetTextureAsset(environment.EnvironmentSourceTexHandle);
		if (!sourceAsset || !sourceAsset->IsValidSource())
		{
			KITA_CORE_WARN("IBLGenerator: failed to load source texture asset for handle {}.", environment.EnvironmentSourceTexHandle);
			return nullptr;
		}

		m_BuildSettings.EnvironmentCubeSize = environment.EnvironmentCubeSize;
		m_BuildSettings.IrradianceCubeSize = environment.IrradianceCubeSize;
		m_BuildSettings.PrefilterCubeSize = environment.PrefilterCubeSize;
		m_BuildSettings.DfgLutSize = environment.DfgLutSize;

		std::array<PixelBuffer, kCubeFaceCount> environmentFaces{};
		if (!BuildSourceCubeFaces(
			sourceAsset->TexRawData,
			environment.SourceType,
			m_BuildSettings.EnvironmentCubeSize,
			environment.RotationY,
			environmentFaces))
		{
			KITA_CORE_ERROR("IBLGenerator: failed to build environment cubemap faces.");
			return nullptr;
		}

		std::array<PixelBuffer, kCubeFaceCount> irradianceFaces{};
		BuildIrradianceCube(
			environmentFaces,
			m_BuildSettings.EnvironmentCubeSize,
			m_BuildSettings.IrradianceCubeSize,
			environment.Intensity,
			irradianceFaces);

		std::vector<std::array<PixelBuffer, kCubeFaceCount>> prefilterMipFaces;
		BuildPrefilterCube(
			environmentFaces,
			m_BuildSettings.EnvironmentCubeSize,
			m_BuildSettings.PrefilterCubeSize,
			environment.Intensity,
			prefilterMipFaces);

		PixelBuffer brdfPixels{};
		BuildBrdfLut(m_BuildSettings.DfgLutSize, brdfPixels);

		m_LastPreviewData = CreateUnique<IBLPreviewData>();
		for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
		{
			m_LastPreviewData->Environment.Faces[faceIndex] = BuildPreviewImageFromPixelBuffer(
				environmentFaces[faceIndex],
				m_BuildSettings.EnvironmentCubeSize,
				m_BuildSettings.EnvironmentCubeSize,
				4);
			m_LastPreviewData->Irradiance.Faces[faceIndex] = BuildPreviewImageFromPixelBuffer(
				irradianceFaces[faceIndex],
				m_BuildSettings.IrradianceCubeSize,
				m_BuildSettings.IrradianceCubeSize,
				4);
		}

		m_LastPreviewData->PrefilterMipChain.resize(prefilterMipFaces.size());
		for (uint32_t mipLevel = 0; mipLevel < static_cast<uint32_t>(prefilterMipFaces.size()); ++mipLevel)
		{
			const uint32_t mipSize = std::max(1u, m_BuildSettings.PrefilterCubeSize >> mipLevel);
			for (uint32_t faceIndex = 0; faceIndex < kCubeFaceCount; ++faceIndex)
			{
				m_LastPreviewData->PrefilterMipChain[mipLevel].Faces[faceIndex] = BuildPreviewImageFromPixelBuffer(
					prefilterMipFaces[mipLevel][faceIndex],
					mipSize,
					mipSize,
					4);
			}
		}

		m_LastPreviewData->BrdfLut = BuildPreviewImageFromPixelBuffer(
			brdfPixels,
			m_BuildSettings.DfgLutSize,
			m_BuildSettings.DfgLutSize,
			2);

		Ref<ImageBasedLighting> ibl = CreateRef<ImageBasedLighting>();
		ibl->EnvironmentCube = CreateRuntimeCubeTexture(
			*m_Context,
			"SceneEnvironmentCube",
			m_BuildSettings.EnvironmentCubeSize,
			m_BuildSettings.CubeFormat,
			false);
		ibl->IrradianceCube = CreateRuntimeCubeTexture(
			*m_Context,
			"SceneIrradianceCube",
			m_BuildSettings.IrradianceCubeSize,
			m_BuildSettings.CubeFormat,
			false);
		ibl->PrefilteredSpecularCube = CreateRuntimeCubeTexture(
			*m_Context,
			"ScenePrefilterCube",
			m_BuildSettings.PrefilterCubeSize,
			m_BuildSettings.CubeFormat,
			true);
		ibl->BrdfLut = CreateRuntimeTexture2D(
			*m_Context,
			"SceneBrdfLut",
			m_BuildSettings.DfgLutSize,
			m_BuildSettings.DfgLutSize,
			m_BuildSettings.DfgFormat);

		if (!ibl->EnvironmentCube || !ibl->IrradianceCube || !ibl->PrefilteredSpecularCube || !ibl->BrdfLut)
		{
			KITA_CORE_ERROR("IBLGenerator: failed to allocate one or more runtime IBL textures.");
			return nullptr;
		}

		UploadCubeMipPixels(*m_Context, *ibl->EnvironmentCube, environmentFaces, m_BuildSettings.EnvironmentCubeSize, 0);
		UploadCubeMipPixels(*m_Context, *ibl->IrradianceCube, irradianceFaces, m_BuildSettings.IrradianceCubeSize, 0);

		for (uint32_t mipLevel = 0; mipLevel < static_cast<uint32_t>(prefilterMipFaces.size()); ++mipLevel)
		{
			UploadCubeMipPixels(
				*m_Context,
				*ibl->PrefilteredSpecularCube,
				prefilterMipFaces[mipLevel],
				m_BuildSettings.PrefilterCubeSize,
				mipLevel);
		}

		UploadBrdfPixels(*m_Context, *ibl->BrdfLut, brdfPixels);

		KITA_CORE_INFO(
			"IBLGenerator: built IBL resources from handle {}. env={} irr={} prefilter={} dfg={}",
			environment.EnvironmentSourceTexHandle,
			m_BuildSettings.EnvironmentCubeSize,
			m_BuildSettings.IrradianceCubeSize,
			m_BuildSettings.PrefilterCubeSize,
			m_BuildSettings.DfgLutSize);

		return ibl;
	}

}
