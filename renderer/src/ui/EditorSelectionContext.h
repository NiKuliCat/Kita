#pragma once
#include <EngineCore.h>
namespace Kita {

	enum class EditorSelectionItemType : uint8_t
	{
		None = 0,
		SceneObject,
		Asset
	};

	struct EditorSelectionItemHandle
	{
		Object m_SelectionObject = {};
		AssetHandle m_SelectedAssetHandle = InvalidAssetHandle;
	};

	class EditorSelectionContext
	{
	public:
		EditorSelectionContext() = default;

		void Clear();
		void SetSelectionAsset(AssetHandle handle);
		void SetSelectionType(EditorSelectionItemType type);
		void SetSelctionObject(Object obj);

		EditorSelectionItemType GetSelectionType() const { return m_CurrentSelectionItemType; }

		EditorSelectionItemHandle& GetSelectionItemHandle() { return m_CurrentSelectionItemHandle; }
		const EditorSelectionItemHandle& GetSelectionItemHandle() const { return m_CurrentSelectionItemHandle; }
	private:
		EditorSelectionItemType m_CurrentSelectionItemType = EditorSelectionItemType::None;
		EditorSelectionItemHandle m_CurrentSelectionItemHandle = {};
	};

}
