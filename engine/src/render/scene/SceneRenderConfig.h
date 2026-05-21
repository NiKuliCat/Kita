#pragma once
#include "asset/Asset.h"

namespace Kita {


	enum class EnvironmentSourceType
	{
		Equirectangular,
		Cubemap
	};


	struct SceneRenderSettings
	{
		AssetHandle  EnvironmentSourceTexHandle = InvalidAssetHandle;
		EnvironmentSourceType SourceType = EnvironmentSourceType::Equirectangular;



		uint32_t EnvironmentCubeSize = 512;
		uint32_t IrradianceCubeSize = 32;
		uint32_t PrefilterCubeSize = 128;
		uint32_t DfgLutSize = 256;
		float Intensity = 1.0f;
		float RotationY = 0.0f;
	};

}
