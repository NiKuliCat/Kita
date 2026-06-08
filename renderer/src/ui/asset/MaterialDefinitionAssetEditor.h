#pragma once

#include "IAssetEditor.h"
#include "preview/LookDevPreviewViewport.h"

namespace Kita {

	class PreviewSceneRenderer;
	class VulkanResourceFactory;
	class ThumbnailCache;
	struct MaterialDefinitionAsset;

	// 文本母材质编辑器：负责源码编辑、编译诊断和实时 LookDev 预览。
	class MaterialDefinitionAssetEditor : public IAssetEditor
	{
	public:
		MaterialDefinitionAssetEditor(
			AssetHandle handle,
			ThumbnailCache* thumbnailCache,
			VulkanResourceFactory* resourceFactory,
			PreviewSceneRenderer* previewSceneRenderer);

		virtual AssetHandle GetAssetHandle() const override { return m_AssetHandle; }
		virtual AssetType GetAssetType() const override { return AssetType::MaterialDefinition; }
		virtual const std::string& GetDisplayName() const override { return m_DisplayName; }
		virtual bool IsDirty() const override;
		virtual bool CanSave() const override { return !m_AssetPath.empty(); }
		virtual void Save() override;
		virtual void Revert() override;
		virtual void OnUpdate() override;
		virtual void OnRender() override;
		virtual void OnImGuiRender() override;

	private:
		bool ReloadSourceFromDisk();
		bool ApplyWorkingSourceToRuntime(bool logFailures);
		void DrawToolbar();
		void DrawSummaryPanel();
		void DrawSourcePanel();

	private:
		AssetHandle m_AssetHandle = InvalidAssetHandle;
		std::string m_DisplayName = "Material";
		std::filesystem::path m_AssetPath;
		Ref<MaterialDefinitionAsset> m_SourceAsset = nullptr;
		ThumbnailCache* m_ThumbnailCache = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
		PreviewSceneRenderer* m_PreviewSceneRenderer = nullptr;
		LookDevPreviewViewport m_LookDevPreview;
		std::string m_SourceText;
		std::string m_SavedSourceText;
		std::string m_Diagnostics;
		bool m_CompileSucceeded = false;
		bool m_RecompilePending = false;
	};

}
