#pragma once

#include <vulkan/vulkan.h>


namespace Kita {

	class VulkanContext;
	class VulkanShader
	{
	public:
		
		struct CreateInfo
		{
			std::string Name;
			VkShaderStageFlagBits Stage = VK_SHADER_STAGE_VERTEX_BIT;
			std::string EntryPoint = "main";
			std::vector<uint8_t> Spirv;
		};

		VulkanShader(VulkanContext* context, const CreateInfo& createInfo);
		~VulkanShader();

		VulkanShader(const VulkanShader&) = delete;
		VulkanShader& operator=(const VulkanShader&) = delete;

		VulkanShader(VulkanShader&& other) noexcept;
		VulkanShader& operator=(VulkanShader&& other) noexcept;

		void Destroy();

		bool IsValid() const { return m_ShaderModule != VK_NULL_HANDLE; }
		
		const std::string& GetName() const { return m_Name; }
		const std::string& GetEntryPoint() const { return m_EntryPoint; }
		VkShaderStageFlagBits GetStage() const { return m_Stage; }
		VkShaderModule GetShaderModule() const { return m_ShaderModule; }

		VkPipelineShaderStageCreateInfo GetStageCreateInfo() const;

	private:
		void CreateShaderModule(const std::vector<uint8_t>& spirvBytes);
		static std::vector<uint32_t> ConvertToWords(const std::vector<uint8_t>& spirvBytes);

	private:
		VulkanContext* m_Context = nullptr;

		std::string m_Name;
		std::string m_EntryPoint = "main";
		VkShaderStageFlagBits m_Stage = VK_SHADER_STAGE_VERTEX_BIT;

		VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
		std::vector<uint32_t> m_SpirvWords;

	};

}