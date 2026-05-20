#pragma once

#include <string_view>

namespace Kita {

	struct GBufferMaterialBindings
	{
		static constexpr uint32_t AlbedoTexture = 0;
		static constexpr uint32_t ParamsBuffer = 1;
		static constexpr uint32_t NormalTexture = 2;
		static constexpr uint32_t MetallicRoughnessTexture = 3;
		static constexpr uint32_t AmbientOcclusionTexture = 4;
		static constexpr uint32_t EmissiveTexture = 5;
		static constexpr uint32_t OpacityTexture = 6;
	};

	struct GBufferTargets
	{
		static constexpr uint32_t BaseColor = 0;
		static constexpr uint32_t Normal = 1;
		static constexpr uint32_t Material = 2;
		static constexpr uint32_t Emissive = 3;
	};

	inline constexpr std::string_view GetGBufferMaterialBindingComment()
	{
		return
			"set=1 material bindings: "
			"0=Albedo, 1=MaterialParamsUBO, 2=Normal, 3=MetallicRoughness, "
			"4=AmbientOcclusion, 5=Emissive, 6=Opacity";
	}

	inline constexpr std::string_view GetGBufferTargetLayoutComment()
	{
		return
			"GBuffer MRT layout: "
			"RT0=BaseColor+Opacity, RT1=NormalWS/EncodedNormal, "
			"RT2=Metallic/Roughness/AO/Flags, RT3=Emissive";
	}

}
