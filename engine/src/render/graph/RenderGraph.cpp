#include "kita_pch.h"
#include "RenderGraph.h"
#include "core/Core.h"
#include "core/Log.h"
namespace Kita {

	namespace {

		const char* ToString(RenderGraphAccessType type)
		{
			switch (type)
			{
			case RenderGraphAccessType::SampledTexture: return "SampledTexture";
			case RenderGraphAccessType::StorageImage: return "StorageImage";
			case RenderGraphAccessType::ColorAttachment: return "ColorAttachment";
			case RenderGraphAccessType::DepthRead: return "DepthRead";
			case RenderGraphAccessType::DepthStencilAttachment: return "DepthStencilAttachment";
			case RenderGraphAccessType::TransferSrc: return "TransferSrc";
			case RenderGraphAccessType::TransferDst: return "TransferDst";
			case RenderGraphAccessType::Present: return "Present";
			default: return "Unknown";
			}
		}


		void AccumulateResourceAccessSummary(RenderGraphResourceAccessSummary& summary, RenderGraphAccessType accessType)
		{
			const VkImageUsageFlags usage = GetImageUsageForAccess(accessType);
			const VkImageLayout layout = GetImageLayoutForAccess(accessType);

			summary.Usage |= usage;

			if (summary.FirstLayout == VK_IMAGE_LAYOUT_UNDEFINED)
				summary.FirstLayout = layout;

			summary.LastLayout = layout;

			if (IsWriteAccess(accessType))
				summary.HasWrite = true;
			else
				summary.HasRead = true;
		}

		RenderGraphAttachmentRef MakeDefaultAttachmentRef(
			RenderGraphResourceID resource,
			RenderGraphAccessType accessType)
		{
			switch (accessType)
			{
			case RenderGraphAccessType::DepthRead:
			case RenderGraphAccessType::DepthStencilAttachment:
				return RenderGraphAttachmentRef::MakeDepth(resource);
			default:
				return RenderGraphAttachmentRef::MakeColor(resource);
			}
		}

	}


	VkImageUsageFlags GetImageUsageForAccess(RenderGraphAccessType type)
	{
		switch (type)
		{
		case RenderGraphAccessType::SampledTexture:
			return VK_IMAGE_USAGE_SAMPLED_BIT;

		case RenderGraphAccessType::StorageImage:
			return VK_IMAGE_USAGE_STORAGE_BIT;

		case RenderGraphAccessType::ColorAttachment:
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		case RenderGraphAccessType::DepthRead:
			return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		case RenderGraphAccessType::DepthStencilAttachment:
			return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		case RenderGraphAccessType::TransferSrc:
			return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		case RenderGraphAccessType::TransferDst:
			return VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		case RenderGraphAccessType::Present:
			// swapchain image 由外部创建，通常不通过 transient allocator 创建。
			return 0;

		default:
			return 0;
		}
	}

	VkImageLayout GetImageLayoutForAccess(RenderGraphAccessType type)
	{
		switch (type)
		{
		case RenderGraphAccessType::SampledTexture:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		case RenderGraphAccessType::StorageImage:
			return VK_IMAGE_LAYOUT_GENERAL;

		case RenderGraphAccessType::ColorAttachment:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		case RenderGraphAccessType::DepthRead:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		case RenderGraphAccessType::DepthStencilAttachment:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		case RenderGraphAccessType::TransferSrc:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		case RenderGraphAccessType::TransferDst:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		case RenderGraphAccessType::Present:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		default:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	bool IsWriteAccess(RenderGraphAccessType type)
	{
		switch (type)
		{
		case RenderGraphAccessType::StorageImage:
		case RenderGraphAccessType::ColorAttachment:
		case RenderGraphAccessType::DepthStencilAttachment:
		case RenderGraphAccessType::TransferDst:
			return true;

		default:
			return false;
		}
	}


	RenderGraphPass::RenderGraphPass(std::string name)
		:m_Name(std::move(name))
	{
	}


	RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceID resource)
	{
		return Read(resource, RenderGraphAccessType::SampledTexture);
	}

	RenderGraphPass& RenderGraphPass::Read(RenderGraphResourceID resource, RenderGraphAccessType access)
	{
		m_Reads.push_back(resource);
		m_ReadAccesses.push_back({ MakeDefaultAttachmentRef(resource, access), access });
		return *this;
	}

	RenderGraphPass& RenderGraphPass::Read(const RenderGraphAttachmentRef& attachment, RenderGraphAccessType access)
	{
		KITA_CORE_ASSERT(attachment.IsValid(), "RenderGraph read attachment is invalid");
		m_Reads.push_back(attachment.Resource);
		m_ReadAccesses.push_back({ attachment, access });
		return *this;
	}


	RenderGraphPass& RenderGraphPass::ReadTexture(RenderGraphResourceID resource)
	{
		return ReadTexture(resource, 0);
	}

	RenderGraphPass& RenderGraphPass::ReadTexture(RenderGraphResourceID resource, uint32_t attachmentIndex)
	{
		return ReadTexture(RenderGraphAttachmentRef::MakeColor(resource, attachmentIndex));
	}

	RenderGraphPass& RenderGraphPass::ReadTexture(const RenderGraphAttachmentRef& attachment)
	{
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Color,
			"RenderGraph sampled texture read requires a color attachment ref");
		return Read(attachment, RenderGraphAccessType::SampledTexture);
	}

	RenderGraphPass& RenderGraphPass::ReadDepth(RenderGraphResourceID resource)
	{
		return ReadDepth(RenderGraphAttachmentRef::MakeDepth(resource));
	}

	RenderGraphPass& RenderGraphPass::ReadDepth(const RenderGraphAttachmentRef& attachment)
	{
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Depth,
			"RenderGraph depth read requires a depth attachment ref");
		return Read(attachment, RenderGraphAccessType::DepthRead);
	}

	RenderGraphPass& RenderGraphPass::TransferRead(RenderGraphResourceID resource)
	{
		return TransferRead(RenderGraphAttachmentRef::MakeColor(resource));
	}

	RenderGraphPass& RenderGraphPass::TransferRead(const RenderGraphAttachmentRef& attachment)
	{
		return Read(attachment, RenderGraphAccessType::TransferSrc);
	}


	RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceID resource)
	{
		return Write(resource, RenderGraphAccessType::ColorAttachment);
	}

	RenderGraphPass& RenderGraphPass::Write(RenderGraphResourceID resource, RenderGraphAccessType access)
	{
		m_Writes.push_back(resource);
		m_WriteAccesses.push_back({ MakeDefaultAttachmentRef(resource, access), access });
		return *this;
	}

	RenderGraphPass& RenderGraphPass::Write(const RenderGraphAttachmentRef& attachment, RenderGraphAccessType access)
	{
		KITA_CORE_ASSERT(attachment.IsValid(), "RenderGraph write attachment is invalid");
		m_Writes.push_back(attachment.Resource);
		m_WriteAccesses.push_back({ attachment, access });
		return *this;
	}

	RenderGraphPass& RenderGraphPass::WriteColor(RenderGraphResourceID resource)
	{
		return WriteColor(resource, 0);
	}

	RenderGraphPass& RenderGraphPass::WriteColor(RenderGraphResourceID resource, uint32_t attachmentIndex)
	{
		return WriteColor(RenderGraphAttachmentRef::MakeColor(resource, attachmentIndex));
	}

	RenderGraphPass& RenderGraphPass::WriteColor(const RenderGraphAttachmentRef& attachment)
	{
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Color,
			"RenderGraph color write requires a color attachment ref");
		return Write(attachment, RenderGraphAccessType::ColorAttachment);
	}

	RenderGraphPass& RenderGraphPass::WriteDepth(RenderGraphResourceID resource)
	{
		return WriteDepth(RenderGraphAttachmentRef::MakeDepth(resource));
	}

	RenderGraphPass& RenderGraphPass::WriteDepth(const RenderGraphAttachmentRef& attachment)
	{
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Depth,
			"RenderGraph depth write requires a depth attachment ref");
		return Write(attachment, RenderGraphAccessType::DepthStencilAttachment);
	}

	RenderGraphPass& RenderGraphPass::WriteStorage(RenderGraphResourceID resource)
	{
		return WriteStorage(RenderGraphAttachmentRef::MakeColor(resource));
	}

	RenderGraphPass& RenderGraphPass::WriteStorage(const RenderGraphAttachmentRef& attachment)
	{
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Color,
			"RenderGraph storage write requires a color attachment ref");
		return Write(attachment, RenderGraphAccessType::StorageImage);
	}

	RenderGraphPass& RenderGraphPass::TransferWrite(RenderGraphResourceID resource)
	{
		return TransferWrite(RenderGraphAttachmentRef::MakeColor(resource));
	}

	RenderGraphPass& RenderGraphPass::TransferWrite(const RenderGraphAttachmentRef& attachment)
	{
		return Write(attachment, RenderGraphAccessType::TransferDst);
	}


	RenderGraphPass& RenderGraphPass::SetExecute(RenderGraphExecuteCallback callback)
	{
		m_Execute = std::move(callback);
		return *this;
	}

	void RenderGraphPass::Execute(RenderGraphContext& context) const
	{
		if (m_Execute)
			m_Execute(context);
	}




	void RenderGraph::Reset()
	{
		m_Resources.clear();
		m_Passes.clear();
		m_CompileResult = {};
		MarkDirty();
	}

	void RenderGraph::MarkDirty()
	{
		m_CompileDirty = true;
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


			KITA_CORE_ASSERT(
				pass.GetReads().size() == pass.GetReadAccesses().size(),
				"RenderGraph pass read access count mismatch");

			KITA_CORE_ASSERT(
				pass.GetWrites().size() == pass.GetWriteAccesses().size(),
				"RenderGraph pass write access count mismatch");

			for (const RenderGraphResourceAccess& access : pass.GetReadAccesses())
			{
				KITA_CORE_ASSERT(access.Attachment.IsValid(), "RenderGraph pass read attachment is invalid");
				KITA_CORE_ASSERT(IsValidResource(access.GetResource()), "RenderGraph pass reads invalid access resource");
				KITA_CORE_ASSERT(access.Type != RenderGraphAccessType::Unknown, "RenderGraph pass read access type is unknown");
			}

			for (const RenderGraphResourceAccess& access : pass.GetWriteAccesses())
			{
				KITA_CORE_ASSERT(access.Attachment.IsValid(), "RenderGraph pass write attachment is invalid");
				KITA_CORE_ASSERT(IsValidResource(access.GetResource()), "RenderGraph pass writes invalid access resource");
				KITA_CORE_ASSERT(access.Type != RenderGraphAccessType::Unknown, "RenderGraph pass write access type is unknown");
			}

		}

		return true;
	}

	RenderGraphResourceID RenderGraph::ImportRenderTarget(const std::string& name, VulkanRenderTarget& rt)
	{ 
		RenderGraphResource res{};
		res.Name = name;
		res.Type = RenderGraphResourceType::ImportedRenderTarget;
		res.RT = &rt;
		res.Imported = true;

		MarkDirty();

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

		MarkDirty();
		m_Resources.push_back(std::move(res));

		return static_cast<RenderGraphResourceID>(m_Resources.size() - 1);
	}

	const RenderGraphResource& RenderGraph::GetResource(RenderGraphResourceID id) const
	{
		KITA_CORE_ASSERT(IsValidResource(id), "RenderGraph resource id is invalid");
		return m_Resources[id];
	}

	VulkanImage* RenderGraph::GetTransientImage(RenderGraphResourceID id)
	{
		return m_Allocator.GetTransientImage(id);
	}

	const VulkanImage* RenderGraph::GetTransientImage(RenderGraphResourceID id) const
	{
		return m_Allocator.GetTransientImage(id);
	}

	void RenderGraph::Dump() const
	{
		KITA_CORE_TRACE("RenderGraph dump: {} resources, {} passes", m_Resources.size(), m_Passes.size());

		for (RenderGraphResourceID resourceIndex = 0; resourceIndex < static_cast<RenderGraphResourceID>(m_Resources.size()); ++resourceIndex)
		{
			const RenderGraphResource& resource = m_Resources[resourceIndex];

			if (resource.Type == RenderGraphResourceType::TransientTexture)
			{
				const RenderGraphTextureDesc& desc = resource.TextureDesc;

				KITA_CORE_TRACE(
					"  Resource[{}] {} TransientTexture {}x{} format={} usage={}",
					resourceIndex,
					resource.Name,
					desc.Width,
					desc.Height,
					static_cast<uint32_t>(desc.Format),
					static_cast<uint32_t>(desc.Usage));
			}
			else
			{
				KITA_CORE_TRACE(
					"  Resource[{}] {} ImportedRenderTarget",
					resourceIndex,
					resource.Name);
			}
		}

		for (uint32_t passIndex = 0; passIndex < static_cast<uint32_t>(m_Passes.size()); ++passIndex)
		{
			const RenderGraphPass& pass = m_Passes[passIndex];

			KITA_CORE_TRACE("  Pass[{}] {}", passIndex, pass.GetName());

			for (const RenderGraphResourceAccess& access : pass.GetReadAccesses())
			{
				if (IsValidResource(access.GetResource()))
				{
					KITA_CORE_TRACE(
						"    Read  [{}] {} access={} attachmentType={} attachmentIndex={}",
						access.GetResource(),
						m_Resources[access.GetResource()].Name,
						ToString(access.Type),
						access.Attachment.Type == RenderGraphAttachmentType::Depth ? "Depth" : "Color",
						access.Attachment.Index);
				}
			}

			for (const RenderGraphResourceAccess& access : pass.GetWriteAccesses())
			{
				if (IsValidResource(access.GetResource()))
				{
					KITA_CORE_TRACE(
						"    Write [{}] {} access={} attachmentType={} attachmentIndex={}",
						access.GetResource(),
						m_Resources[access.GetResource()].Name,
						ToString(access.Type),
						access.Attachment.Type == RenderGraphAttachmentType::Depth ? "Depth" : "Color",
						access.Attachment.Index);
				}
			}
		}

		if (m_CompileResult.Valid && m_CompileResult.ResourceLifetimes.size() == m_Resources.size())
		{
			KITA_CORE_TRACE("  Resource lifetimes:");

			for (RenderGraphResourceID resourceIndex = 0; resourceIndex < static_cast<RenderGraphResourceID>(m_Resources.size()); ++resourceIndex)
			{
				const RenderGraphResource& resource = m_Resources[resourceIndex];
				const RenderGraphResourceLifetime& lifetime = m_CompileResult.ResourceLifetimes[resourceIndex];

				KITA_CORE_TRACE(
					"    [{}] {} firstWrite={} lastRead={} lastAccess={}",
					resourceIndex,
					resource.Name,
					lifetime.FirstWritePass,
					lifetime.LastReadPass,
					lifetime.LastAccessPass);
			}
		}


		if (m_CompileResult.Valid && m_CompileResult.ResourceAccessSummaries.size() == m_Resources.size())
		{
			KITA_CORE_TRACE("  Resource access summaries:");

			for (RenderGraphResourceID resourceIndex = 0; resourceIndex < static_cast<RenderGraphResourceID>(m_Resources.size()); ++resourceIndex)
			{
				const RenderGraphResource& resource = m_Resources[resourceIndex];
				const RenderGraphResourceAccessSummary& summary =
					m_CompileResult.ResourceAccessSummaries[resourceIndex];

				KITA_CORE_TRACE(
					"    [{}] {} usage={} firstLayout={} lastLayout={} hasRead={} hasWrite={}",
					resourceIndex,
					resource.Name,
					static_cast<uint32_t>(summary.Usage),
					static_cast<uint32_t>(summary.FirstLayout),
					static_cast<uint32_t>(summary.LastLayout),
					summary.HasRead,
					summary.HasWrite);
				}
		}

		m_Allocator.Dump(m_Resources);
	}

	RenderGraphPass& RenderGraph::AddPass(const std::string& name)
	{
		MarkDirty();

		m_Passes.emplace_back(name);
		return m_Passes.back();
	}


	const RenderGraphCompileResult& RenderGraph::Compile()
	{
		if (!m_CompileDirty && m_CompileResult.Valid)
			return m_CompileResult;

		m_CompileResult = {};

		if (!Validate())
			return m_CompileResult;

		std::unordered_set<RenderGraphResourceID> writtenResources;
		m_CompileResult.Valid = true;
		m_CompileResult.Passes.reserve(m_Passes.size());
		m_CompileResult.ResourceLifetimes.resize(m_Resources.size());
		m_CompileResult.ResourceAccessSummaries.resize(m_Resources.size());


		for (RenderGraphResourceID i = 0; i < static_cast<RenderGraphResourceID>(m_Resources.size()); ++i)
		{
			if (m_Resources[i].Imported)
				writtenResources.insert(i);
		}

		for (uint32_t passIndex = 0; passIndex < static_cast<uint32_t>(m_Passes.size()); ++passIndex)
		{
			const RenderGraphPass& pass = m_Passes[passIndex];

			for (RenderGraphResourceID read : pass.GetReads())
			{
				KITA_CORE_ASSERT(
					writtenResources.find(read) != writtenResources.end(),
					"RenderGraph pass reads a resource before it is written");

				RenderGraphResourceLifetime& lifetime = m_CompileResult.ResourceLifetimes[read];
				lifetime.LastReadPass = passIndex;
				lifetime.LastAccessPass = passIndex;
			}

			RenderGraphCompiledPass compiledPass{};
			compiledPass.PassIndex = passIndex;
			compiledPass.ReadAccesses = pass.GetReadAccesses();
			compiledPass.WriteAccesses = pass.GetWriteAccesses();


			for (const RenderGraphResourceAccess& access : pass.GetReadAccesses())
			{
				RenderGraphResourceAccessSummary& summary =
					m_CompileResult.ResourceAccessSummaries[access.GetResource()];

				AccumulateResourceAccessSummary(summary, access.Type);
			}

			for (const RenderGraphResourceAccess& access : pass.GetWriteAccesses())
			{
				RenderGraphResourceAccessSummary& summary =
					m_CompileResult.ResourceAccessSummaries[access.GetResource()];

				AccumulateResourceAccessSummary(summary, access.Type);
			}


			m_CompileResult.Passes.push_back(std::move(compiledPass));

			for (RenderGraphResourceID write : pass.GetWrites())
			{
				RenderGraphResourceLifetime& lifetime = m_CompileResult.ResourceLifetimes[write];

				if (lifetime.FirstWritePass == InvalidRenderGraphPassIndex)
					lifetime.FirstWritePass = passIndex;

				lifetime.LastAccessPass = passIndex;
				writtenResources.insert(write);
			}
		}

		for (RenderGraphResourceID i = 0; i < static_cast<RenderGraphResourceID>(m_Resources.size()); ++i)
		{
			if (m_Resources[i].Type == RenderGraphResourceType::TransientTexture)
			{
				const RenderGraphResourceLifetime& lifetime = m_CompileResult.ResourceLifetimes[i];

				KITA_CORE_ASSERT(
					lifetime.FirstWritePass != InvalidRenderGraphPassIndex,
					"RenderGraph transient texture is never written");
			}
		}

		m_CompileDirty = false;
		return m_CompileResult;
	}

	void RenderGraph::Execute(VulkanContext& context, VkCommandBuffer commandBuffer)
	{


		const RenderGraphCompileResult& compileResult = Compile();
		if (!compileResult.Valid)
			return;

		if (m_DebugDumpEnabled)
			Dump();

		m_Allocator.Prepare(context, m_Resources, compileResult);

		RenderGraphContext graphContext(context, commandBuffer, m_Resources, m_Allocator);

		for (const RenderGraphCompiledPass& compiledPass : compileResult.Passes)
		{
			KITA_CORE_ASSERT(
				compiledPass.PassIndex < m_Passes.size(),
				"RenderGraph compiled pass index is invalid");

			m_Passes[compiledPass.PassIndex].Execute(graphContext);
		}
	}

}
