#pragma once
#include "core/Core.h"
#include "asset/Asset.h"

//=============================================================================================
//		本层负责使用数据层创建运行时渲染所需资源 asset data > > > render runtime object
//		目前包括 material  shader  texture
//=============================================================================================

namespace Kita {

	class AssetManager;
	class VulkanContext;
	class VulkanGeometry;
	class VulkanMaterial;
	class VulkanShader;
	class VulkanTexture;

	class VulkanResourceFactory
	{
	public:
		struct ShaderBundle
		{
			Ref<VulkanShader> VertexShader = nullptr;
			Ref<VulkanShader> FragmentShader = nullptr;

			bool IsValid() const
			{
				return VertexShader && FragmentShader;
			}
		};

	public:
		VulkanResourceFactory(VulkanContext& context, AssetManager& assetManager);

		ShaderBundle GetOrCreateShaderBundle(AssetHandle handle);
		Ref<VulkanTexture> GetOrCreateTexture(AssetHandle handle);

		Ref<VulkanMaterial> CreateMaterial(AssetHandle handle);
		Ref<VulkanMaterial> CreateMaterial(const MaterialAsset& materialAsset);

		void ApplyMaterial(const MaterialAsset& materialAsset, VulkanMaterial& outMaterial);
		void RefreshMaterial(AssetHandle handle);
		void RefreshMaterialFrameResources(AssetHandle handle, uint32_t frameIndex);

		std::vector<Ref<VulkanGeometry>> GetOrCreateGeometries(AssetHandle handle);
		std::vector<Ref<VulkanGeometry>> GetOrCreateGeometries(const MeshAsset& meshAsset);

		void InvalidateShader(AssetHandle shaderHandle);
		void InvalidateTexture(AssetHandle textureHandle);
		void InvalidateMaterial(AssetHandle materialHandle);
		void InvalidateMesh(AssetHandle meshHandle);
		void Clear();


	private:
		Ref<VulkanShader> BuildShader(
			const ShaderAsset& shaderAsset,
			const ShaderStageBinary& stageBinary,
			VkShaderStageFlagBits stage,
			const char* suffix);



	private:
		VulkanContext& m_Context;
		AssetManager& m_AssetManager;

		std::unordered_map<AssetHandle, ShaderBundle> m_ShaderCache;
		std::unordered_map<AssetHandle, Ref<VulkanTexture>> m_TextureCache;
		std::unordered_map<AssetHandle, Ref<VulkanMaterial>> m_MaterialCache;
		std::unordered_map<AssetHandle, std::vector<Ref<VulkanGeometry>>> m_GeometryCache;
	};
}
