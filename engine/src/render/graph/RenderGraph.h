#pragma once
#include "RenderGraphAllocator.h"
#include "RenderGraphCompiled.h"
#include"RenderGraphResource.h"
#include "RenderGraphContext.h"
namespace Kita {


	class VulkanContext;
	class VulkanImage;
	class VulkanRenderTarget;

	using RenderGraphExecuteCallback = std::function<void(RenderGraphContext&)>;

	class RenderGraphPass
	{
	public:
		explicit RenderGraphPass(std::string name);

		RenderGraphPass& Read(RenderGraphResourceID resource);
		RenderGraphPass& Read(RenderGraphResourceID resource, RenderGraphAccessType access);
		RenderGraphPass& Read(const RenderGraphAttachmentRef& attachment, RenderGraphAccessType access);
		RenderGraphPass& ReadTexture(RenderGraphResourceID resource);
		RenderGraphPass& ReadTexture(RenderGraphResourceID resource, uint32_t attachmentIndex);
		RenderGraphPass& ReadTexture(const RenderGraphAttachmentRef& attachment);
		RenderGraphPass& ReadDepth(RenderGraphResourceID resource);
		RenderGraphPass& ReadDepth(const RenderGraphAttachmentRef& attachment);
		RenderGraphPass& TransferRead(RenderGraphResourceID resource);
		RenderGraphPass& TransferRead(const RenderGraphAttachmentRef& attachment);

		RenderGraphPass& Write(RenderGraphResourceID resource);
		RenderGraphPass& Write(RenderGraphResourceID resource, RenderGraphAccessType access);
		RenderGraphPass& Write(const RenderGraphAttachmentRef& attachment, RenderGraphAccessType access);
		RenderGraphPass& WriteColor(RenderGraphResourceID resource);
		RenderGraphPass& WriteColor(RenderGraphResourceID resource, uint32_t attachmentIndex);
		RenderGraphPass& WriteColor(const RenderGraphAttachmentRef& attachment);
		RenderGraphPass& WriteDepth(RenderGraphResourceID resource);
		RenderGraphPass& WriteDepth(const RenderGraphAttachmentRef& attachment);
		RenderGraphPass& WriteStorage(RenderGraphResourceID resource);
		RenderGraphPass& WriteStorage(const RenderGraphAttachmentRef& attachment);
		RenderGraphPass& TransferWrite(RenderGraphResourceID resource);
		RenderGraphPass& TransferWrite(const RenderGraphAttachmentRef& attachment);

		RenderGraphPass& SetExecute(RenderGraphExecuteCallback callback);

		const std::vector<RenderGraphResourceID>& GetReads() const { return m_Reads; }
		const std::vector<RenderGraphResourceID>& GetWrites() const { return m_Writes; }

		const std::vector<RenderGraphResourceAccess>& GetReadAccesses() const { return m_ReadAccesses; }
		const std::vector<RenderGraphResourceAccess>& GetWriteAccesses() const { return m_WriteAccesses; }


		const std::string& GetName() const { return m_Name; }
		bool HasExecuteCallback() const { return static_cast<bool>(m_Execute); }

		void Execute(RenderGraphContext& context) const;
	private:
		std::string m_Name;
		std::vector<RenderGraphResourceID> m_Reads;
		std::vector<RenderGraphResourceID> m_Writes;

		std::vector<RenderGraphResourceAccess> m_ReadAccesses;
		std::vector<RenderGraphResourceAccess> m_WriteAccesses;

		RenderGraphExecuteCallback m_Execute;
	};



	class RenderGraph
	{
	public:
		void Reset();

		RenderGraphResourceID ImportRenderTarget(const std::string& name, VulkanRenderTarget& rt);
		RenderGraphPass& AddPass(const std::string& name);

		bool Validate() const;
		const RenderGraphCompileResult& Compile();
		void Execute(VulkanContext& context, VkCommandBuffer commandBuffer);

		// 创建一个 RenderGraph 内部声明的临时纹理。
		// 当前阶段只保存描述，不创建 Vulkan image。
		RenderGraphResourceID CreateTexture(const RenderGraphTextureDesc& desc);

		const RenderGraphResource& GetResource(RenderGraphResourceID id) const;
		VulkanImage* GetTransientImage(RenderGraphResourceID id);
		const VulkanImage* GetTransientImage(RenderGraphResourceID id) const;
		void Dump() const;
		void SetDebugDumpEnabled(bool enabled) { m_DebugDumpEnabled = enabled; }
		bool IsDebugDumpEnabled() const { return m_DebugDumpEnabled; }

	private:
		bool IsValidResource(RenderGraphResourceID id) const;

		// 标记 graph 结构发生变化，需要重新 Compile。
		// 当前 dirty 机制服务于“每帧 Reset + Build + Execute”的构建方式：
		// Reset / ImportRenderTarget / CreateTexture / AddPass 会标记 dirty。
		//
		// 注意：AddPass 返回 RenderGraphPass& 后，继续链式调用
		// ReadTexture / WriteColor / SetExecute 等修改不会再次通知 RenderGraph。
		// 当前每帧重建 graph，因此不会出问题。
		// 后续如果要支持长期 graph 增量修改，需要引入 PassBuilder 或 pass revision。
		void MarkDirty();

	private:
		std::vector<RenderGraphResource> m_Resources;
		std::vector<RenderGraphPass> m_Passes;
		RenderGraphCompileResult m_CompileResult;
		RenderGraphAllocator m_Allocator;
		bool m_CompileDirty = true;
		bool m_DebugDumpEnabled = false;
	};
}
