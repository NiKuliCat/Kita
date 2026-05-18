#pragma once

#include "IAssetEditor.h"

namespace Kita {

	class ThumbnailCache;
	class VulkanResourceFactory;

	class AssetEditorManager
	{
	public:
		AssetEditorManager() = default;

		void SetThumbnailCache(ThumbnailCache* thumbnailCache) { m_ThumbnailCache = thumbnailCache; }
		void SetResourceFactory(VulkanResourceFactory* resourceFactory) { m_ResourceFactory = resourceFactory; }
		void SetDockSpaceId(ImGuiID dockSpaceId) { m_DockSpaceId = dockSpaceId; }

		bool OpenEditor(AssetHandle handle);
		void OnImGuiRender();

	private:
		struct OpenEditorEntry
		{
			Unique<IAssetEditor> Editor = nullptr;
			bool IsOpen = true;
			bool RequestFocus = false;
			bool PendingInitialDock = true;
			bool OpenAsFloatingRoot = false;
			AssetHandle DockReferenceHandle = InvalidAssetHandle;
		};

		Unique<IAssetEditor> CreateEditor(const AssetMetadata& metadata) const;
		IAssetEditor* FindOpenEditor(AssetHandle handle) const;
		OpenEditorEntry* FindOpenEditorEntry(AssetHandle handle);
		const OpenEditorEntry* FindOpenEditorEntry(AssetHandle handle) const;
		void CloseEditor(AssetHandle handle);
		void DrawFloatingHostWindow();
		bool SetupFloatingRootDock(OpenEditorEntry& entry, ImGuiID& outDockId);
		bool SetupDockWithReference(OpenEditorEntry& entry, ImGuiID& outDockId);

	private:
		std::vector<OpenEditorEntry> m_OpenEditors;
		AssetHandle m_ActiveAssetHandle = InvalidAssetHandle;
		ImGuiID m_DockSpaceId = 0;
		ImGuiID m_FloatingDockRootId = 0;
		bool m_ShowFloatingHostWindow = false;
		ThumbnailCache* m_ThumbnailCache = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
	};

}
