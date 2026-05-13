#pragma once
#include "RenderDataStruct.h"
#include "render/VulkanDescriptorSet.h"
#include "render/VulkanUniformBuffer.h"
namespace Kita {
	class VulkanContext;

	class SceneBindings
	{
	public:
		SceneBindings() = default;
		~SceneBindings();


		void Init(VulkanContext& context, uint32_t  framesInFlight);

		void Destroy();

		void Update(uint32_t frameIndex, const SceneCameraData& camera, const SceneDirectionalLightData& mainLight);

		const VulkanDescriptorSet& GetDescriptorSet(uint32_t frameIndex) const;

		bool IsValid() const { return m_Initialized; }


	private:
		VulkanContext* m_Context = nullptr;

		bool  m_Initialized = false;


		std::vector<VulkanUniformBuffer> m_CameraUBOs;
		std::vector<VulkanUniformBuffer> m_MainLightUBOs;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

}