#pragma once

#include <EngineCore.h>

namespace Kita {

	class IAssetEditor
	{
	public:
		virtual ~IAssetEditor() = default;

		virtual AssetHandle GetAssetHandle() const = 0;
		virtual AssetType GetAssetType() const = 0;
		virtual const std::string& GetDisplayName() const = 0;

		virtual bool IsDirty() const = 0;
		virtual bool CanSave() const = 0;
		virtual void Save() = 0;
		virtual void Revert() = 0;

		virtual void OnImGuiRender() = 0;
	};

}
