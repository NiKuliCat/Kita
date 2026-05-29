#include "kita_pch.h"
#include "RenderGraph.h"

namespace Kita {




	RenderGraphPass::RenderGraphPass(std::string name)
		:m_Name(std::move(name))
	{
	}

	void RenderGraphPass::Execute(RenderGraphContext& context) const
	{
		if (m_Execute)
			m_Execute(context);
	}
	RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceID resource)
	{
		m_Reads.push_back(resource);
		return *this;
	}
	RenderGraphPass& RenderGraphPass::SetExecute(RenderGraphExecuteCallback callback)
	{
		m_Execute = std::move(callback);
		return *this;
	}
	RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceID resource)
	{
		m_Writes.push_back(resource);
		return *this;
	}
	void RenderGraph::Reset()
	{
		m_Resources.clear();
		m_Passes.clear();
	}

	RenderGraphResourceID RenderGraph::ImportRenderTarget(const std::string& name, VulkanRenderTarget& rt)
	{ 
		RenderGraphResource res{};
		res.Name = name;
		res.Type = RenderGraphResourceType::ImportedRenderTarget;
		res.RT = &rt;

		m_Resources.push_back(res);
		return static_cast<RenderGraphResourceID>(m_Resources.size() - 1);
	}

	RenderGraphPass& RenderGraph::AddPass(const std::string& name)
	{
		m_Passes.emplace_back(name);
		return m_Passes.back();
	}

	void RenderGraph::Execute(VulkanContext& context, VkCommandBuffer commandBuffer)
	{
		RenderGraphContext graphContext(context, commandBuffer, m_Resources);

		for (const RenderGraphPass& pass : m_Passes)
			pass.Execute(graphContext);
	}

}