#pragma once

#include "IAssetEditor.h"
#include "ui/ThumbnailCache.h"
#include "ui/UIAttributeUtil.h"

namespace Kita {

	class VulkanResourceFactory;

	class MaterialAssetEditor : public IAssetEditor
	{
	public:
		MaterialAssetEditor(AssetHandle handle, ThumbnailCache* thumbnailCache, VulkanResourceFactory* resourceFactory);

		virtual AssetHandle GetAssetHandle() const override { return m_AssetHandle; }
		virtual AssetType GetAssetType() const override { return AssetType::Material; }
		virtual const std::string& GetDisplayName() const override { return m_DisplayName; }
		virtual bool IsDirty() const override;
		virtual bool CanSave() const override { return m_SourceAsset != nullptr; }
		virtual void Save() override;
		virtual void Revert() override;
		virtual void OnImGuiRender() override;

	private:
		void DrawToolbar();
		void DrawPreview();
		void DrawDetails();
		void DrawAssetRow(const char* label, const char* comboId, AssetHandle& handle, AssetType type, AssetHandle resetValue);
		void DrawTextureSlotRow(const char* label, size_t slotIndex, AssetHandle& handle, AssetHandle resetValue);
		void DrawColorRow(const char* label, const char* colorId, glm::vec4& value, const glm::vec4& resetValue);
		bool IsWorkingCopyDirty() const;
		void SyncWorkingCopyToAssetData();

	private:
		AssetHandle m_AssetHandle = InvalidAssetHandle;
		std::string m_DisplayName = "Material";
		std::filesystem::path m_AssetPath;
		Ref<MaterialAsset> m_SourceAsset = nullptr;
		MaterialAsset m_WorkingCopy{};
		MaterialAsset m_SavedCopy{};
		ThumbnailCache* m_ThumbnailCache = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
		UIAttributeUtil::TableStyle m_TableStyle = UIAttributeUtil::CreateDefaultTableStyle();
	};

}
