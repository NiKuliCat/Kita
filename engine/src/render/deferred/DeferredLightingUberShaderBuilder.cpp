#include "kita_pch.h"
#include "DeferredLightingUberShaderBuilder.h"

#include "asset/ShaderLabAsset.h"
#include "render/ShaderCompiler.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace Kita {

	namespace
	{
		constexpr uint32_t kDefaultLitShadingModelID = 0;
		constexpr uint32_t kUnlitShadingModelID = 1;
		constexpr uint32_t kCustomShadingModelStartID = 16;

		struct SurfaceShaderRecord
		{
			AssetMetadata Metadata{};
			Ref<ShaderLabAsset> Asset = nullptr;
		};

		struct CustomShadingModelRecord
		{
			std::string Name;
			std::filesystem::path Include;
			std::string EvaluateFunction;
			std::filesystem::path SourcePath;
		};

		bool HasCompiledPass(const ShaderLabAsset& shaderLabAsset, PassType passType)
		{
			for (const ShaderLabCompiledPass& compiledPass : shaderLabAsset.CompiledPasses)
			{
				if (compiledPass.Type == passType)
					return true;
			}

			return false;
		}

		bool IsBuiltInShadingModel(const std::string& shadingModelName)
		{
			return shadingModelName == "DefaultLit" || shadingModelName == "Unlit";
		}

		uint32_t ResolveBuiltInShadingModelID(const std::string& shadingModelName)
		{
			return shadingModelName == "Unlit"
				? kUnlitShadingModelID
				: kDefaultLitShadingModelID;
		}

		void AppendUniqueDirectory(
			std::vector<std::filesystem::path>& includeDirs,
			std::set<std::string>& includeDirKeys,
			const std::filesystem::path& directory)
		{
			if (directory.empty())
				return;

			const std::string key = directory.lexically_normal().generic_string();
			if (!includeDirKeys.insert(key).second)
				return;

			includeDirs.push_back(directory);
		}

		std::vector<SurfaceShaderRecord> CollectSurfaceShaders(
			AssetManager& assetManager,
			std::ostringstream& diagnostics)
		{
			std::vector<SurfaceShaderRecord> records;
			const std::vector<AssetMetadata> shaderLabAssets =
				assetManager.GetAssetsByType(AssetType::MaterialDefinition);

			for (const AssetMetadata& metadata : shaderLabAssets)
			{
				Ref<MaterialDefinitionAsset> shaderLabAsset = assetManager.GetMaterialDefinitionAsset(metadata.handle);
				if (!shaderLabAsset)
				{
					diagnostics << "[DeferredLightingUber] Failed to load ShaderLab asset '"
						<< metadata.relativePath.generic_string() << "'.\n";
					continue;
				}

				if (!shaderLabAsset->Desc || shaderLabAsset->Desc->Domain != MaterialDomain::Surface)
					continue;

				if (!shaderLabAsset->LightingRuntime || !shaderLabAsset->LightingRuntime->Enabled)
					continue;

				if (!HasCompiledPass(*shaderLabAsset, PassType::GBuffer))
					continue;

				records.push_back({ metadata, shaderLabAsset });
			}

			return records;
		}

		std::map<std::string, CustomShadingModelRecord> CollectCustomShadingModels(
			const std::vector<SurfaceShaderRecord>& surfaceShaders,
			std::ostringstream& diagnostics)
		{
			std::map<std::string, CustomShadingModelRecord> records;

			for (const SurfaceShaderRecord& surfaceShader : surfaceShaders)
			{
				if (!surfaceShader.Asset || !surfaceShader.Asset->LightingRuntime)
					continue;

				const ShaderLabLightingRuntimeDesc& lightingRuntime = *surfaceShader.Asset->LightingRuntime;
				if (IsBuiltInShadingModel(lightingRuntime.ShadingModelName))
					continue;

				CustomShadingModelRecord candidate{};
				candidate.Name = lightingRuntime.ShadingModelName;
				candidate.Include = lightingRuntime.LightingHookInclude;
				candidate.EvaluateFunction = lightingRuntime.LightingHookFunction;
				candidate.SourcePath = surfaceShader.Asset->SourcePath;

				auto [it, inserted] = records.emplace(candidate.Name, candidate);
				if (!inserted)
				{
					const bool sameHook =
						it->second.Include == candidate.Include &&
						it->second.EvaluateFunction == candidate.EvaluateFunction;
					if (!sameHook)
					{
						diagnostics << "[DeferredLightingUber] ShadingModel '"
							<< candidate.Name
							<< "' is declared by multiple ShaderLab assets with different hooks. "
							<< "Keeping '" << it->second.SourcePath.generic_string()
							<< "', ignoring '" << candidate.SourcePath.generic_string() << "'.\n";
					}
				}
			}

			return records;
		}

		void AssignShadingModelIDs(
			const std::vector<SurfaceShaderRecord>& surfaceShaders,
			const std::map<std::string, CustomShadingModelRecord>& customShadingModels)
		{
			std::map<std::string, uint32_t> customIDs;
			uint32_t nextID = kCustomShadingModelStartID;
			for (const auto& [name, _] : customShadingModels)
			{
				customIDs.emplace(name, nextID++);
			}

			for (const SurfaceShaderRecord& surfaceShader : surfaceShaders)
			{
				if (!surfaceShader.Asset || !surfaceShader.Asset->LightingRuntime)
					continue;

				ShaderLabLightingRuntimeDesc& lightingRuntime = *surfaceShader.Asset->LightingRuntime;
				if (IsBuiltInShadingModel(lightingRuntime.ShadingModelName))
				{
					lightingRuntime.ShadingModelID =
						ResolveBuiltInShadingModelID(lightingRuntime.ShadingModelName);
					continue;
				}

				const auto customIt = customIDs.find(lightingRuntime.ShadingModelName);
				if (customIt != customIDs.end())
				{
					lightingRuntime.ShadingModelID = customIt->second;
				}
			}
		}

		std::vector<std::filesystem::path> CollectIncludeDirectories(
			AssetManager& assetManager,
			const std::vector<SurfaceShaderRecord>& surfaceShaders,
			const std::map<std::string, CustomShadingModelRecord>& customShadingModels)
		{
			std::vector<std::filesystem::path> includeDirs;
			std::set<std::string> includeDirKeys;

			AppendUniqueDirectory(includeDirs, includeDirKeys, assetManager.GetAssetRoot());

			for (const SurfaceShaderRecord& surfaceShader : surfaceShaders)
			{
				if (surfaceShader.Asset)
				{
					AppendUniqueDirectory(
						includeDirs,
						includeDirKeys,
						surfaceShader.Asset->SourcePath.parent_path());
				}
			}

			for (const auto& [_, customModel] : customShadingModels)
			{
				if (customModel.Include.is_absolute())
				{
					AppendUniqueDirectory(
						includeDirs,
						includeDirKeys,
						customModel.Include.parent_path());
					continue;
				}

				AppendUniqueDirectory(
					includeDirs,
					includeDirKeys,
					customModel.SourcePath.parent_path());
			}

			return includeDirs;
		}

		std::string BuildUberShaderSource(
			const std::map<std::string, CustomShadingModelRecord>& customShadingModels)
		{
			std::ostringstream oss;
			oss << "import render_core;\n";
			oss << "import pbr_common;\n";
			oss << "import IBL;\n\n";

			oss << "struct VSOutput\n";
			oss << "{\n";
			oss << "    float4 positionCS : SV_Position;\n";
			oss << "    float2 uv : TEXCOORD0;\n";
			oss << "};\n\n";

			oss << "struct PSOutput\n";
			oss << "{\n";
			oss << "    float4 color : SV_Target0;\n";
			oss << "    float depth : SV_Depth;\n";
			oss << "};\n\n";

			oss << "[[vk::binding(0, 0)]]\n";
			oss << "ConstantBuffer<SceneCameraData> g_Camera;\n\n";
			oss << "[[vk::binding(1, 0)]]\n";
			oss << "ConstantBuffer<SceneDirectionalLightData> g_MainLight;\n\n";
			oss << "[[vk::binding(0, 1)]]\n";
			oss << "Sampler2D g_GBufferBaseColorOpacity;\n\n";
			oss << "[[vk::binding(1, 1)]]\n";
			oss << "Sampler2D g_GBufferNormal;\n\n";
			oss << "[[vk::binding(2, 1)]]\n";
			oss << "Sampler2D g_GBufferMaterial;\n\n";
			oss << "[[vk::binding(3, 1)]]\n";
			oss << "Sampler2D g_GBufferEmissive;\n\n";
			oss << "[[vk::binding(4, 1)]]\n";
			oss << "Sampler2D g_GBufferCustomData;\n\n";
			oss << "[[vk::binding(5, 1)]]\n";
			oss << "Sampler2D g_GBufferDepth;\n\n";

			oss << "struct KitaDeferredSurfaceData\n";
			oss << "{\n";
			oss << "    float3 BaseColor;\n";
			oss << "    float Opacity;\n";
			oss << "    float3 NormalWS;\n";
			oss << "    float Metallic;\n";
			oss << "    float Roughness;\n";
			oss << "    float AmbientOcclusion;\n";
			oss << "    uint ShadingModelID;\n";
			oss << "    float3 Emissive;\n";
			oss << "    float4 CustomData;\n";
			oss << "};\n\n";

			oss << "struct KitaDeferredLightingContext\n";
			oss << "{\n";
			oss << "    float2 UV;\n";
			oss << "    float Depth;\n";
			oss << "    float3 PositionWS;\n";
			oss << "    float3 ViewDirectionWS;\n";
			oss << "    float3 LightDirectionWS;\n";
			oss << "    float3 LightColor;\n";
			oss << "    float LightIntensity;\n";
			oss << "};\n\n";

			for (const auto& [_, customModel] : customShadingModels)
			{
				oss << "#include \"" << customModel.Include.generic_string() << "\"\n";
			}
			if (!customShadingModels.empty())
				oss << "\n";

			oss << "float2 FullscreenTrianglePosition(uint vertexID)\n";
			oss << "{\n";
			oss << "    if (vertexID == 0) return float2(-1.0, -1.0);\n";
			oss << "    if (vertexID == 1) return float2(-1.0, 3.0);\n";
			oss << "    return float2(3.0, -1.0);\n";
			oss << "}\n\n";

			oss << "[shader(\"vertex\")]\n";
			oss << "VSOutput VSMain(uint vertexID : SV_VertexID)\n";
			oss << "{\n";
			oss << "    VSOutput output;\n";
			oss << "    float2 clipXY = FullscreenTrianglePosition(vertexID);\n";
			oss << "    output.positionCS = float4(clipXY, 0.0, 1.0);\n";
			oss << "    output.uv = clipXY * float2(0.5, -0.5) + 0.5;\n";
			oss << "    return output;\n";
			oss << "}\n\n";

			oss << "float3 DecodeNormal(float3 encodedNormal)\n";
			oss << "{\n";
			oss << "    return normalize(encodedNormal * 2.0 - 1.0);\n";
			oss << "}\n\n";

			oss << "float3 ReconstructWorldPosition(float2 uv, float depth)\n";
			oss << "{\n";
			oss << "    float2 clipXY = uv * 2.0 - 1.0;\n";
			oss << "    clipXY.y = -clipXY.y;\n";
			oss << "    float4 clipPos = float4(clipXY, depth, 1.0);\n";
			oss << "    float4 positionWS = mul(g_Camera.Matrix_I_VP, clipPos);\n";
			oss << "    return positionWS.xyz / max(abs(positionWS.w), 1e-6);\n";
			oss << "}\n\n";

			oss << "KitaDeferredSurfaceData LoadSurface(float2 uv)\n";
			oss << "{\n";
			oss << "    KitaDeferredSurfaceData surface;\n";
			oss << "    float4 baseColorOpacity = g_GBufferBaseColorOpacity.Sample(uv);\n";
			oss << "    float4 normalSample = g_GBufferNormal.Sample(uv);\n";
			oss << "    float4 materialSample = g_GBufferMaterial.Sample(uv);\n";
			oss << "    float4 emissiveSample = g_GBufferEmissive.Sample(uv);\n";
			oss << "    float4 customDataSample = g_GBufferCustomData.Sample(uv);\n";
			oss << "    surface.BaseColor = baseColorOpacity.rgb;\n";
			oss << "    surface.Opacity = baseColorOpacity.a;\n";
			oss << "    surface.NormalWS = DecodeNormal(normalSample.rgb);\n";
			oss << "    surface.Metallic = saturate(materialSample.r);\n";
			oss << "    surface.Roughness = ClampRoughness(materialSample.g);\n";
			oss << "    surface.AmbientOcclusion = saturate(materialSample.b);\n";
			oss << "    surface.ShadingModelID = (uint)max(0.0, floor(materialSample.a + 0.5));\n";
			oss << "    surface.Emissive = emissiveSample.rgb;\n";
			oss << "    surface.CustomData = float4(emissiveSample.a, customDataSample.rgb);\n";
			oss << "    return surface;\n";
			oss << "}\n\n";

			oss << "KitaDeferredLightingContext BuildLightingContext(float2 uv, float depth, float3 normalWS)\n";
			oss << "{\n";
			oss << "    KitaDeferredLightingContext context;\n";
			oss << "    context.UV = uv;\n";
			oss << "    context.Depth = depth;\n";
			oss << "    context.PositionWS = ReconstructWorldPosition(uv, depth);\n";
			oss << "    context.LightDirectionWS = SafeNormalize(-g_MainLight.Direction.xyz, float3(0.0, 1.0, 0.0));\n";
			oss << "    context.ViewDirectionWS = SafeNormalize(g_Camera.CameraPosWS.xyz - context.PositionWS, float3(0.0, 0.0, 1.0));\n";
			oss << "    context.LightColor = g_MainLight.Color.rgb;\n";
			oss << "    context.LightIntensity = g_MainLight.Color.w;\n";
			oss << "    return context;\n";
			oss << "}\n\n";

			oss << "float3 EvaluateDefaultLit(KitaDeferredSurfaceData surface, KitaDeferredLightingContext context)\n";
			oss << "{\n";
			oss << "    float3 F0 = ComputeF0(surface.BaseColor, surface.Metallic);\n";
			oss << "    float3 directLighting = EvaluateDirectionalLightPBR(\n";
			oss << "        surface.BaseColor,\n";
			oss << "        surface.Metallic,\n";
			oss << "        surface.Roughness,\n";
			oss << "        surface.AmbientOcclusion,\n";
			oss << "        surface.NormalWS,\n";
			oss << "        context.ViewDirectionWS,\n";
			oss << "        context.LightDirectionWS,\n";
			oss << "        context.LightColor,\n";
			oss << "        context.LightIntensity);\n";
			oss << "    float3 F = FresnelSchlickRoughness(saturate(dot(surface.NormalWS, context.ViewDirectionWS)), F0, surface.Roughness);\n";
			oss << "    float3 kS = F;\n";
			oss << "    float3 kD = (1.0 - kS) * (1.0 - surface.Metallic);\n";
			oss << "    float3 diffuseIBL = EvaluateDiffuseIBL(surface.NormalWS, surface.BaseColor, surface.Metallic);\n";
			oss << "    float3 specularIBL = EvaluateSpecularIBL(surface.NormalWS, context.ViewDirectionWS, surface.Roughness, F0);\n";
			oss << "    float3 indirectLighting = (kD * diffuseIBL + specularIBL) * surface.AmbientOcclusion;\n";
			oss << "    return directLighting + indirectLighting + surface.Emissive;\n";
			oss << "}\n\n";

			oss << "float3 EvaluateUnlit(KitaDeferredSurfaceData surface, KitaDeferredLightingContext context)\n";
			oss << "{\n";
			oss << "    return surface.BaseColor + surface.Emissive + context.LightColor * 0.0;\n";
			oss << "}\n\n";

			oss << "float3 EvaluateDeferredSurface(KitaDeferredSurfaceData surface, KitaDeferredLightingContext context)\n";
			oss << "{\n";
			oss << "    switch (surface.ShadingModelID)\n";
			oss << "    {\n";
			oss << "    case " << kDefaultLitShadingModelID << ":\n";
			oss << "        return EvaluateDefaultLit(surface, context);\n";
			oss << "    case " << kUnlitShadingModelID << ":\n";
			oss << "        return EvaluateUnlit(surface, context);\n";
			uint32_t customIndex = 0;
			for (const auto& [name, customModel] : customShadingModels)
			{
				(void)name;
				const uint32_t shadingModelID = kCustomShadingModelStartID + customIndex++;
				oss << "    case " << shadingModelID << ":\n";
				oss << "        return " << customModel.EvaluateFunction << "(surface, context);\n";
			}
			oss << "    default:\n";
			oss << "        return EvaluateDefaultLit(surface, context);\n";
			oss << "    }\n";
			oss << "}\n\n";

			oss << "[shader(\"fragment\")]\n";
			oss << "PSOutput PSMain(VSOutput input)\n";
			oss << "{\n";
			oss << "    PSOutput output;\n";
			oss << "    float depth = g_GBufferDepth.Sample(input.uv).r;\n";
			oss << "    KitaDeferredSurfaceData surface = LoadSurface(input.uv);\n";
			oss << "    if (depth >= 1.0 || surface.Opacity <= 0.0)\n";
			oss << "    {\n";
			oss << "        output.color = float4(surface.Emissive, 1.0);\n";
			oss << "        output.depth = 1.0;\n";
			oss << "        return output;\n";
			oss << "    }\n";
			oss << "    KitaDeferredLightingContext context = BuildLightingContext(input.uv, depth, surface.NormalWS);\n";
			oss << "    output.color = float4(EvaluateDeferredSurface(surface, context), 1.0);\n";
			oss << "    output.depth = depth;\n";
			oss << "    return output;\n";
			oss << "}\n";

			return oss.str();
		}
	}

	DeferredLightingUberShaderBuildResult DeferredLightingUberShaderBuilder::BuildForProject(
		AssetManager& assetManager,
		VulkanResourceFactory& resourceFactory)
	{
		DeferredLightingUberShaderBuildResult result{};
		std::ostringstream diagnostics;

		const std::vector<SurfaceShaderRecord> surfaceShaders =
			CollectSurfaceShaders(assetManager, diagnostics);
		const std::map<std::string, CustomShadingModelRecord> customShadingModels =
			CollectCustomShadingModels(surfaceShaders, diagnostics);

		AssignShadingModelIDs(surfaceShaders, customShadingModels);

		result.SurfaceShaderCount = static_cast<uint32_t>(surfaceShaders.size());
		result.CustomShadingModelCount = static_cast<uint32_t>(customShadingModels.size());
		result.GeneratedSource = BuildUberShaderSource(customShadingModels);

		ShaderCompiler compiler;
		ShaderCompiler::CompileRequest vsRequest{};
		vsRequest.SourcePath = assetManager.GetAssetRoot() / "__generated__/DeferredLightingUber.slang";
		vsRequest.ModuleName = "DeferredLightingUberVS";
		vsRequest.EntryPointName = "VSMain";
		vsRequest.ShaderStage = ShaderCompiler::Stage::Vertex;

		ShaderCompiler::CompileRequest fsRequest = vsRequest;
		fsRequest.ModuleName = "DeferredLightingUberFS";
		fsRequest.EntryPointName = "PSMain";
		fsRequest.ShaderStage = ShaderCompiler::Stage::Fragment;

		const std::vector<std::filesystem::path> includeDirs =
			CollectIncludeDirectories(assetManager, surfaceShaders, customShadingModels);
		vsRequest.IncludeDirs = includeDirs;
		fsRequest.IncludeDirs = includeDirs;

		const ShaderCompiler::CompileResult vsResult =
			compiler.CompileToSpirvFromSource(vsRequest, result.GeneratedSource);
		const ShaderCompiler::CompileResult fsResult =
			compiler.CompileToSpirvFromSource(fsRequest, result.GeneratedSource);

		if (!vsResult.Success)
		{
			diagnostics << "[DeferredLightingUber][VS] " << vsResult.Diagnostics << "\n";
		}

		if (!fsResult.Success)
		{
			diagnostics << "[DeferredLightingUber][FS] " << fsResult.Diagnostics << "\n";
		}

		if (vsResult.Success && fsResult.Success)
		{
			ShaderStageBinary vertexStage{};
			vertexStage.EntryPoint = "main";
			vertexStage.Spirv = vsResult.Spirv;

			ShaderStageBinary fragmentStage{};
			fragmentStage.EntryPoint = "main";
			fragmentStage.Spirv = fsResult.Spirv;

			result.ShaderBundle = resourceFactory.BuildShaderBundleFromStageBinaries(
				"DeferredLightingUber",
				vertexStage,
				fragmentStage);
			result.Success = result.ShaderBundle.IsValid();

			if (!result.Success)
			{
				diagnostics << "[DeferredLightingUber] Failed to create Vulkan shader bundle from compiled SPIR-V.\n";
			}
		}

		result.Diagnostics = diagnostics.str();
		return result;
	}

}
