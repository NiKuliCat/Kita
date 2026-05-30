#include "kita_pch.h"
#include "RenderGraph.h"
#include "core/Core.h"
#include "core/Log.h"
namespace Kita {




	RenderGraphPass::RenderGraphPass(std::string name)
		:m_Name(std::move(name))
	{
	}

	bool RenderGraph::IsValidResource(RenderGraphResourceID id) const
	{
		return id != InvalidRenderGraphResourceID && id < m_Resources.size();
	}

	bool RenderGraph::Validate() const
	{
		for (const RenderGraphResource& resource : m_Resources)
		{
			KITA_CORE_ASSERT(!resource.Name.empty(), "RenderGraph resource name is empty");
			KITA_CORE_ASSERT(resource.Type != RenderGraphResourceType::Unknown, "RenderGraph resource type is unknown");

			if (resource.Type == RenderGraphResourceType::ImportedRenderTarget)
			{
				KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");
				KITA_CORE_ASSERT(resource.Imported, "RenderGraph imported render target flag is false");
			}

			if (resource.Type == RenderGraphResourceType::TransientTexture)
			{
				const RenderGraphTextureDesc& desc = resource.TextureDesc;
				KITA_CORE_ASSERT(!desc.Name.empty(), "RenderGraph transient texture name is empty");
				KITA_CORE_ASSERT(desc.Width > 0 && desc.Height > 0, "RenderGraph transient texture size is invalid");
				KITA_CORE_ASSERT(desc.Format != VK_FORMAT_UNDEFINED, "RenderGraph transient texture format is undefined");
				KITA_CORE_ASSERT(desc.Usage != 0, "RenderGraph transient texture usage is empty");
			}
		}

		for (const RenderGraphPass& pass : m_Passes)
		{
			KITA_CORE_ASSERT(!pass.GetName().empty(), "RenderGraph pass name is empty");
			KITA_CORE_ASSERT(pass.HasExecuteCallback(), "RenderGraph pass has no execute callback");

			for (RenderGraphResourceID id : pass.GetReads())
				KITA_CORE_ASSERT(IsValidResource(id), "RenderGraph pass reads invalid resource");

			for (RenderGraphResourceID id : pass.GetWrites())
				KITA_CORE_ASSERT(IsValidResource(id), "RenderGraph pass writes invalid resource");
		}

		return true;
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
		m_CompileResult = {};
	}

	RenderGraphResourceID RenderGraph::ImportRenderTarget(const std::string& name, VulkanRenderTarget& rt)
	{ 
		RenderGraphResource res{};
		res.Name = name;
		res.Type = RenderGraphResourceType::ImportedRenderTarget;
		res.RT = &rt;
		res.Imported = true;

		m_CompileResult = {};

		m_Resources.push_back(res);
		return static_cast<RenderGraphResourceID>(m_Resources.size() - 1);

	}

	RenderGraphResourceID RenderGraph::CreateTexture(const RenderGraphTextureDesc& desc)
	{
		KITA_CORE_ASSERT(!desc.Name.empty(), "RenderGraph transient texture name is empty");
		KITA_CORE_ASSERT(desc.Width > 0 && desc.Height > 0, "RenderGraph transient texture size is invalid");
		KITA_CORE_ASSERT(desc.Format != VK_FORMAT_UNDEFINED, "RenderGraph transient texture format is undefined");
		KITA_CORE_ASSERT(desc.Usage != 0, "RenderGraph transient texture usage is empty");

		RenderGraphResource res{};
		res.Name = desc.Name;
		res.Type = RenderGraphResourceType::TransientTexture;
		res.TextureDesc = desc;

		// Transient texture 当前只是逻辑资源，不绑定外部 RenderTarget。
		res.RT = nullptr;
		res.Imported = false;

		m_CompileResult = {};
		m_Resources.push_back(std::move(res));

		return static_cast<RenderGraphResourceID>(m_Resources.size() - 1);
	}

	const RenderGraphResource& RenderGraph::GetResource(RenderGraphResourceID id) const
	{
		KITA_CORE_ASSERT(IsValidResource(id), "RenderGraph resource id is invalid");
		return m_Resources[id];
	}

	void RenderGraph::Dump() const
	{
		KITA_CORE_TRACE("RenderGraph dump: {} resources, {} passes", m_Resources.size(), m_Passes.size());

		for (uint32_t passIndex = 0; passIndex < static_cast<uint32_t>(m_Passes.size()); ++passIndex)
		{
			const RenderGraphPass& pass = m_Passes[passIndex];

			KITA_CORE_TRACE("  Pass[{}] {}", passIndex, pass.GetName());

			for (RenderGraphResourceID read : pass.GetReads())
			{
				if (IsValidResource(read))
					KITA_CORE_TRACE("    Read  [{}] {}", read, m_Resources[read].Name);
			}

			for (RenderGraphResourceID write : pass.GetWrites())
			{
				if (IsValidResource(write))
					KITA_CORE_TRACE("    Write [{}] {}", write, m_Resources[write].Name);
			}
		}
	}

	RenderGraphPass& RenderGraph::AddPass(const std::string& name)
	{
		m_CompileResult = {};

		m_Passes.emplace_back(name);
		return m_Passes.back();
	}


	const RenderGraphCompileResult& RenderGraph::Compile()
	{
		m_CompileResult = {};

		if (!Validate())
			return m_CompileResult;

		std::unordered_set<RenderGraphResourceID> writtenResources;
		std::unordered_set<RenderGraphResourceID> usedTransientResources;

		for (RenderGraphResourceID i = 0; i < static_cast<RenderGraphResourceID>(m_Resources.size()); ++i)
		{
			if (m_Resources[i].Imported)
				writtenResources.insert(i);
		}

		m_CompileResult.Valid = true;
		m_CompileResult.Passes.reserve(m_Passes.size());

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_Passes.size()); ++i)
		{
			const RenderGraphPass& pass = m_Passes[i];

			for (RenderGraphResourceID read : pass.GetReads())
			{
				KITA_CORE_ASSERT(
					writtenResources.find(read) != writtenResources.end(),
					"RenderGraph pass reads a resource before it is written");
			}

			RenderGraphCompiledPass compiledPass{};
			compiledPass.PassIndex = i;
			compiledPass.Reads = pass.GetReads();
			compiledPass.Writes = pass.GetWrites();

			m_CompileResult.Passes.push_back(std::move(compiledPass));

			for (RenderGraphResourceID write : pass.GetWrites())
			{
				writtenResources.insert(write);

				if (m_Resources[write].Type == RenderGraphResourceType::TransientTexture)
					usedTransientResources.insert(write);
			}
		}

		for (RenderGraphResourceID i = 0; i < static_cast<RenderGraphResourceID>(m_Resources.size()); ++i)
		{
			if (m_Resources[i].Type == RenderGraphResourceType::TransientTexture)
			{
				KITA_CORE_ASSERT(
					usedTransientResources.find(i) != usedTransientResources.end(),
					"RenderGraph transient texture is never written");
			}
		}

		return m_CompileResult;
	}

	void RenderGraph::Execute(VulkanContext& context, VkCommandBuffer commandBuffer)
	{


		const RenderGraphCompileResult& compileResult = Compile();
		if (!compileResult.Valid)
			return;

		if (m_DebugDumpEnabled)
			Dump();

		RenderGraphContext graphContext(context, commandBuffer, m_Resources);

		for (const RenderGraphCompiledPass& compiledPass : compileResult.Passes)
		{
			KITA_CORE_ASSERT(
				compiledPass.PassIndex < m_Passes.size(),
				"RenderGraph compiled pass index is invalid");

			m_Passes[compiledPass.PassIndex].Execute(graphContext);
		}
	}

}