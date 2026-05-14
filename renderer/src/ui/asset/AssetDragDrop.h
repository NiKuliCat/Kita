#pragma once

#include <EngineCore.h>

namespace Kita {

	struct AssetDragDropPayload
	{
		AssetHandle Handle = InvalidAssetHandle;
		AssetType Type = AssetType::None;
	};

	inline constexpr const char* kAssetDragDropPayloadType = "KITA_ASSET_PAYLOAD";

}
