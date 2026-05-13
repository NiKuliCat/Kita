#include "kita_pch.h"
#include "VulkanShader.h"
#include "VulkanContext.h"
#include "core/Log.h"
namespace Kita {
	namespace
	{
		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}
	}

	VulkanShader::VulkanShader(VulkanContext* context, const CreateInfo& createInfo)
		: m_Context(context),
		m_Name(createInfo.Name),
		m_EntryPoint(createInfo.EntryPoint.empty() ? "main" : createInfo.EntryPoint),
		m_Stage(createInfo.Stage)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanShader context is null");
		KITA_CORE_ASSERT(!createInfo.Spirv.empty(), "VulkanShader spirv is empty");

		CreateShaderModule(createInfo.Spirv);
	}

	VulkanShader::~VulkanShader()
	{
		Destroy();
	}

	VulkanShader::VulkanShader(VulkanShader&& other) noexcept
		: m_Context(other.m_Context),
		m_Name(std::move(other.m_Name)),
		m_EntryPoint(std::move(other.m_EntryPoint)),
		m_Stage(other.m_Stage),
		m_ShaderModule(other.m_ShaderModule),
		m_SpirvWords(std::move(other.m_SpirvWords))
	{
		other.m_Context = nullptr;
		other.m_ShaderModule = VK_NULL_HANDLE;
	}

	VulkanShader& VulkanShader::operator=(VulkanShader&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		m_Context = other.m_Context;
		m_Name = std::move(other.m_Name);
		m_EntryPoint = std::move(other.m_EntryPoint);
		m_Stage = other.m_Stage;
		m_ShaderModule = other.m_ShaderModule;
		m_SpirvWords = std::move(other.m_SpirvWords);

		other.m_Context = nullptr;
		other.m_ShaderModule = VK_NULL_HANDLE;

		return *this;
	}


	void VulkanShader::Destroy()
	{
		if (!m_Context)
			return;

		VkDevice device = m_Context->GetDevice();
		if (device != VK_NULL_HANDLE && m_ShaderModule != VkShaderModule{})
		{
			vkDestroyShaderModule(device, m_ShaderModule, nullptr);
			m_ShaderModule = VK_NULL_HANDLE;
		}

		m_SpirvWords.clear();
	}

	VkPipelineShaderStageCreateInfo VulkanShader::GetStageCreateInfo() const
	{
		KITA_CORE_ASSERT(m_ShaderModule != VkShaderModule{}, "VulkanShader module is invalid");

		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = m_Stage;
		stageInfo.module = m_ShaderModule;
		stageInfo.pName = m_EntryPoint.c_str();
		return stageInfo;
	}

	void VulkanShader::CreateShaderModule(const std::vector<uint8_t>& spirvBytes)
	{
		m_SpirvWords = ConvertToWords(spirvBytes);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = m_SpirvWords.size() * sizeof(uint32_t);
		createInfo.pCode = m_SpirvWords.data();

		VKCheck(
			vkCreateShaderModule(m_Context->GetDevice(), &createInfo, nullptr, &m_ShaderModule),
			"Failed to create Vulkan shader module");

		KITA_CORE_INFO("Created VulkanShader '{0}'", m_Name);
	}

	std::vector<uint32_t> VulkanShader::ConvertToWords(const std::vector<uint8_t>& spirvBytes)
	{
		KITA_CORE_ASSERT(!spirvBytes.empty(), "SPIR-V bytecode is empty");
		KITA_CORE_ASSERT((spirvBytes.size() % sizeof(uint32_t)) == 0, "SPIR-V bytecode size is not 4-byte aligned");

		std::vector<uint32_t> words(spirvBytes.size() / sizeof(uint32_t));
		std::memcpy(words.data(), spirvBytes.data(), spirvBytes.size());
		return words;
	}
}
