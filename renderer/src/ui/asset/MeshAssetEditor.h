#pragma once

#include "IAssetEditor.h"

namespace Kita {

	class MeshAssetEditor : public IAssetEditor
	{
	public:
		explicit MeshAssetEditor(AssetHandle handle);

		virtual AssetHandle GetAssetHandle() const override { return m_AssetHandle; }
		virtual AssetType GetAssetType() const override { return AssetType::Mesh; }
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
		std::string m_DisplayName = "Mesh";
		Ref<MeshAsset> m_MeshAsset = nullptr;
	};

}
