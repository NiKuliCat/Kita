#pragma once

#include "IAssetEditor.h"
#include "ui/ThumbnailCache.h"

namespace Kita {

	class TextureAssetEditor : public IAssetEditor
	{
	public:
		TextureAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache);

		virtual AssetHandle GetAssetHandle() const override { return m_AssetHandle; }
		virtual AssetType GetAssetType() const override { return AssetType::Texture; }
		virtual const std::string& GetDisplayName() const override { return m_DisplayName; }
		virtual bool IsDirty() const override { return false; }
		virtual bool CanSave() const override { return false; }
		virtual void Save() override {}
		virtual void Revert() override {}
		virtual void OnImGuiRender() override;

	private:
		void DrawToolbar();
		void DrawPreview();
		void DrawDetails();

	private:
		AssetHandle m_AssetHandle = InvalidAssetHandle;
		std::string m_DisplayName = "Texture";
		Ref<TextureAsset> m_TextureAsset = nullptr;
		ThumbnailCache* m_ThumbnailCache = nullptr;
	};

}
