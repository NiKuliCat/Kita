#pragma once

#include "ui/EditorSelectionContext.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Kita {

	struct EditorPickEntry
	{
		EditorSelectionItemType SelectionType = EditorSelectionItemType::None;
		UUID SceneObjectUUID = UUID(0);
		AssetHandle SelectedAssetHandle = InvalidAssetHandle;
	};

	class EditorPickRegistry
	{
	public:
		static constexpr uint32_t NullPickId = 0;

	public:
		EditorPickRegistry();

		void Clear();

		uint32_t RegisterSceneObject(const Object& object);
		uint32_t RegisterSceneObject(UUID uuid);
		uint32_t RegisterAsset(AssetHandle handle);
		uint32_t RegisterEntry(const EditorPickEntry& entry);

		bool Contains(uint32_t pickId) const;
		const EditorPickEntry* GetEntry(uint32_t pickId) const;
		bool TryGetEntry(uint32_t pickId, EditorPickEntry& outEntry) const;

		uint32_t GetEntryCount() const;

	private:
		uint32_t AllocateEntry(const EditorPickEntry& entry);

	private:
		std::vector<EditorPickEntry> m_Entries;
		std::unordered_map<uint64_t, uint32_t> m_SceneObjectPickIds;
		std::unordered_map<AssetHandle, uint32_t> m_AssetPickIds;
	};

}
