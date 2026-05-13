#include "renderer_pch.h"
#include "EditorPickRegistry.h"

namespace Kita {

	EditorPickRegistry::EditorPickRegistry()
	{
		Clear();
	}

	void EditorPickRegistry::Clear()
	{
		m_Entries.clear();
		m_Entries.emplace_back();

		m_SceneObjectPickIds.clear();
		m_AssetPickIds.clear();
	}

	uint32_t EditorPickRegistry::RegisterSceneObject(const Object& object)
	{
		if (!object)
			return NullPickId;

		return RegisterSceneObject(UUID(object.GetUUID()));
	}

	uint32_t EditorPickRegistry::RegisterSceneObject(UUID uuid)
	{
		if (uuid == UUID(0))
			return NullPickId;

		const uint64_t uuidValue = static_cast<uint64_t>(uuid);
		const auto it = m_SceneObjectPickIds.find(uuidValue);
		if (it != m_SceneObjectPickIds.end())
			return it->second;

		EditorPickEntry entry{};
		entry.SelectionType = EditorSelectionItemType::SceneObject;
		entry.SceneObjectUUID = uuid;

		const uint32_t pickId = AllocateEntry(entry);
		m_SceneObjectPickIds.emplace(uuidValue, pickId);
		return pickId;
	}

	uint32_t EditorPickRegistry::RegisterAsset(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
			return NullPickId;

		const auto it = m_AssetPickIds.find(handle);
		if (it != m_AssetPickIds.end())
			return it->second;

		EditorPickEntry entry{};
		entry.SelectionType = EditorSelectionItemType::Asset;
		entry.SelectedAssetHandle = handle;

		const uint32_t pickId = AllocateEntry(entry);
		m_AssetPickIds.emplace(handle, pickId);
		return pickId;
	}

	uint32_t EditorPickRegistry::RegisterEntry(const EditorPickEntry& entry)
	{
		switch (entry.SelectionType)
		{
		case EditorSelectionItemType::SceneObject:
			return RegisterSceneObject(entry.SceneObjectUUID);

		case EditorSelectionItemType::Asset:
			return RegisterAsset(entry.SelectedAssetHandle);

		case EditorSelectionItemType::None:
		default:
			return NullPickId;
		}
	}

	bool EditorPickRegistry::Contains(uint32_t pickId) const
	{
		return pickId != NullPickId && pickId < m_Entries.size();
	}

	const EditorPickEntry* EditorPickRegistry::GetEntry(uint32_t pickId) const
	{
		if (!Contains(pickId))
			return nullptr;

		return &m_Entries[pickId];
	}

	bool EditorPickRegistry::TryGetEntry(uint32_t pickId, EditorPickEntry& outEntry) const
	{
		const EditorPickEntry* entry = GetEntry(pickId);
		if (!entry)
			return false;

		outEntry = *entry;
		return true;
	}

	uint32_t EditorPickRegistry::GetEntryCount() const
	{
		return m_Entries.empty() ? 0u : static_cast<uint32_t>(m_Entries.size() - 1);
	}

	uint32_t EditorPickRegistry::AllocateEntry(const EditorPickEntry& entry)
	{
		KITA_CORE_ASSERT(entry.SelectionType != EditorSelectionItemType::None, "EditorPickRegistry entry type is invalid");

		m_Entries.push_back(entry);
		return static_cast<uint32_t>(m_Entries.size() - 1);
	}

}
