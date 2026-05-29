#pragma once

#include"RenderGraphResource.h"
#include "RenderGraphContext.h"
namespace Kita {


	class VulkanContext;
	class VulkanRenderTarget;

	using RenderGraphExecuteCallback = std::function<void(RenderGraphContext&)>;

	class RenderGraphPass
	{
	public:
		explicit RenderGraphPass(std::string name);
		RenderGraphPass& Read(RenderGraphResourceID resource);
		RenderGraphPass& Write(RenderGraphResourceID resource);
		RenderGraphPass& SetExecute(RenderGraphExecuteCallback callback);
		const std::string& GetName() const { return m_Name; }
		void Execute(RenderGraphContext& context) const;
	private:
		std::string m_Name;
		std::vector<RenderGraphResourceID> m_Reads;
		std::vector<RenderGraphResourceID> m_Writes;
		RenderGraphExecuteCallback m_Execute;
	};



	class RenderGraph
	{
	public:
		void Reset();

		RenderGraphResourceID ImportRenderTarget(const std::string& name, VulkanRenderTarget& rt);
		RenderGraphPass& AddPass(const std::string& name);
		void Execute(VulkanContext& context, VkCommandBuffer commandBuffer);

	private:
		std::vector<RenderGraphResource> m_Resources;
		std::vector<RenderGraphPass> m_Passes;
	};
}