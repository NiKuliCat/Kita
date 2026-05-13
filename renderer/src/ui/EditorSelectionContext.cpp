#include "renderer_pch.h"
#include "EditorSelectionContext.h"
namespace Kita {




	void EditorSelectionContext::Clear()
	{
		m_CurrentSelectionItemType = EditorSelectionItemType::None;
		m_CurrentSelectionItemHandle.m_SelectionObject = {};
		m_CurrentSelectionItemHandle.m_SelectedAssetHandle = InvalidAssetHandle;
	}

	void EditorSelectionContext::SetSelectionAsset(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			KITA_CLENT_ERROR("this asset handle is not valid : {0}", handle);
			return;
		}

		m_CurrentSelectionItemHandle.m_SelectedAssetHandle = handle;
	}

	void EditorSelectionContext::SetSelectionType(EditorSelectionItemType type)
	{
		m_CurrentSelectionItemType = type;
	}

	void EditorSelectionContext::SetSelctionObject(Object obj)
	{
		if (!obj)
		{
			KITA_CLENT_ERROR("this object is null");
			return;
		}

		m_CurrentSelectionItemHandle.m_SelectionObject = obj;
	}

}
