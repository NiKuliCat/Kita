#pragma once
#include "RenderDataStruct.h"
#include "RenderContext.h"
#include "SceneBindings.h"
#include "render/VulkanRenderTarget.h"
namespace Kita {

	class IRenderPass
	{
	public:
		virtual ~IRenderPass() = default;


		virtual const RenderPassDesc& GetDesc() const = 0;
		virtual void Execute(RenderPassContext& context) = 0;
	};


	class RenderPassBase : public IRenderPass
	{
	public:
		explicit RenderPassBase(RenderPassDesc desc);
		virtual ~RenderPassBase() = default;

		const RenderPassDesc& GetDesc() const override { return m_Desc; }

	protected:
		void ValidateRenderTarget(const VulkanRenderTarget& renderTarget) const;

		void BeginPass(
			RenderPassContext& context,
			const RenderPassBeginInfo& beginInfo) const;

		void EndPass(
			RenderPassContext& context,
			const RenderPassBeginInfo& beginInfo) const;

	protected:
		RenderPassDesc m_Desc;
	};


	class SceneRenderPassBase : public RenderPassBase
	{
	public:
		SceneRenderPassBase(SceneBindings& sceneBindings, RenderPassDesc desc);
		virtual ~SceneRenderPassBase() = default;

		void SetSceneData(const ScenePassData& sceneData) { m_SceneData = sceneData; }

	protected:
		void UpdateSceneBindings(RenderPassContext& context);

		SceneBindings& GetSceneBindings() { return *m_SceneBindings; }
		const SceneBindings& GetSceneBindings() const { return *m_SceneBindings; }

		const ScenePassData& GetSceneData() const { return m_SceneData; }

	private:
		SceneBindings* m_SceneBindings = nullptr;
		ScenePassData m_SceneData{};
	};


}