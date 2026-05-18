#pragma once

#include "IAssetEditor.h"
#include "ui/ThumbnailCache.h"
#include "ui/UIAttributeUtil.h"

namespace Kita {

	class VulkanResourceFactory;

	class TextureAssetEditor : public IAssetEditor
	{
	public:
		TextureAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache, VulkanResourceFactory* resourceFactory);

		virtual AssetHandle GetAssetHandle() const override { return m_AssetHandle; }
		virtual AssetType GetAssetType() const override { return AssetType::Texture; }
		virtual const std::string& GetDisplayName() const override { return m_DisplayName; }
		virtual bool IsDirty() const override;
		virtual bool CanSave() const override { return m_TextureAsset != nullptr; }
		virtual void Save() override;
		virtual void Revert() override;
		virtual void OnImGuiRender() override;

	private:
		void DrawToolbar();
		void DrawPreview();
		void DrawDetails();
		bool IsWorkingCopyDirty() const;
		void ApplyWorkingCopyToAsset();
		void RefreshTextureResource();

	private:
		AssetHandle m_AssetHandle = InvalidAssetHandle;
		std::string m_DisplayName = "Texture";
		Ref<TextureAsset> m_TextureAsset = nullptr;
		TextureImportSettings m_WorkingCopy{};
		TextureImportSettings m_SavedCopy{};
		ThumbnailCache* m_ThumbnailCache = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
		UIAttributeUtil::TableStyle m_TableStyle = UIAttributeUtil::CreateDefaultTableStyle();
	};

}
