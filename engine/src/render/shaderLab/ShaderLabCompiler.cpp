#include "kita_pch.h"
#include "ShaderLabCompiler.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace Kita {

	namespace {

		static uint32_t AlignUp(uint32_t value, uint32_t alignment)
		{
			if (alignment == 0)
				return value;
			const uint32_t remainder = value % alignment;
			return remainder == 0 ? value : (value + alignment - remainder);
		}

		static bool IsTextureProperty(MaterialValueType type)
		{
			return type == MaterialValueType::Texture2D || type == MaterialValueType::TextureCube;
		}

		// 按 std140 风格给材质 UBO 做稳定布局。
		// 第三步不依赖 Slang 反射，先让 CPU 和生成出来的 shader 声明共用同一套规则。
		static uint32_t GetStd140Alignment(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Bool:
			case MaterialValueType::Int:
			case MaterialValueType::Float:
				return 4;

			case MaterialValueType::Float2:
				return 8;

			case MaterialValueType::Float3:
			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				return 16;

			default:
				return 4;
			}
		}

		static uint32_t GetStd140Size(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Bool:
			case MaterialValueType::Int:
			case MaterialValueType::Float:
				return 4;

			case MaterialValueType::Float2:
				return 8;

				// vec3 在 std140 中也按 16 字节槽位处理，便于 CPU/GPU 对齐一致。
			case MaterialValueType::Float3:
			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				return 16;

			default:
				return 0;
			}
		}

		static std::string SanitizeIdentifier(const std::string& text)
		{
			std::string result;
			result.reserve(text.size());

			for (char c : text)
			{
				if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_')
				{
					result.push_back(c);
				}
				else
				{
					result.push_back('_');
				}
			}

			if (result.empty())
				result = "ShaderLabSymbol";

			if (std::isdigit(static_cast<unsigned char>(result[0])) != 0)
				result.insert(result.begin(), '_');

			return result;
		}

		static std::string ToModuleSafeName(const std::string& shaderName, const std::string& passName, const char* suffix)
		{
			return SanitizeIdentifier(shaderName) + "_" + SanitizeIdentifier(passName) + "_" + suffix;
		}

		static std::string ToSlangUniformFieldType(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Bool:   return "int";
			case MaterialValueType::Int:    return "int";
			case MaterialValueType::Float:  return "float";
			case MaterialValueType::Float2: return "float2";
			case MaterialValueType::Float3: return "float3";
			case MaterialValueType::Float4: return "float4";
			case MaterialValueType::Color:  return "float4";
			default:                        return {};
			}
		}

		static std::string ToSlangResourceType(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Texture2D:   return "Sampler2D";
			case MaterialValueType::TextureCube: return "SamplerCube";
			default:                             return {};
			}
		}

		static std::string BuildBoolAccessorMacro(const std::string& propertyName)
		{
			return "#define " + propertyName + " (g_MaterialParams." + propertyName + " != 0)";
		}

		static std::string BuildDirectAccessorMacro(const std::string& propertyName)
		{
			return "#define " + propertyName + " (g_MaterialParams." + propertyName + ")";
		}

		// ShaderLab 当前走的是 Slang/HLSL 风格 ConstantBuffer，
		// CPU 侧也要按 cbuffer 的 16-byte register packing 规则排布。
		static uint32_t GetConstantBufferPackedSize(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Bool:
			case MaterialValueType::Int:
			case MaterialValueType::Float:
				return 4;

			case MaterialValueType::Float2:
				return 8;

			case MaterialValueType::Float3:
				return 12;

			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				return 16;

			default:
				return 0;
			}
		}

		static bool HasPassType(const std::vector<MaterialPassDesc>& passes, PassType passType)
		{
			for (const MaterialPassDesc& pass : passes)
			{
				if (pass.Type == passType)
					return true;
			}

			return false;
		}

		static bool IsBuiltInShadingModel(const std::string& shadingModel)
		{
			return shadingModel == "DefaultLit" || shadingModel == "Unlit";
		}

	}

	MaterialRuntimeLayout MaterialCompiler::BuildMaterialLayout(
		const std::vector<MaterialPropertyDesc>& properties,
		bool includeSystemFields)
	{
		MaterialRuntimeLayout layout{};

		uint32_t uniformOffset = 0;
		uint32_t nextResourceBinding = 1;
		const auto appendUniformMember = [&](const std::string& name, MaterialValueType valueType)
		{
			const uint32_t size = GetConstantBufferPackedSize(valueType);
			if (size == 0)
				return;

			const uint32_t registerOffset = uniformOffset % 16;
			if (registerOffset + size > 16)
			{
				uniformOffset = AlignUp(uniformOffset, 16);
			}

			MaterialUniformMember member{};
			member.Name = name;
			member.ValueType = valueType;
			member.Offset = uniformOffset;
			member.Size = size;
			layout.UniformMembers.push_back(std::move(member));

			uniformOffset += size;
		};

		for (const MaterialPropertyDesc& property : properties)
		{
			if (IsTextureProperty(property.ValueType))
			{
				MaterialResourceBinding resource{};
				resource.Name = property.Name;
				resource.ValueType = property.ValueType;
				resource.Set = layout.DescriptorSet;
				resource.Binding = nextResourceBinding++;
				layout.ResourceBindings.push_back(std::move(resource));
				continue;
			}

			const uint32_t size = GetConstantBufferPackedSize(property.ValueType);
			if (size == 0)
				continue;

			appendUniformMember(property.Name, property.ValueType);
		}

		if (includeSystemFields)
		{
			appendUniformMember(std::string(MaterialSystemFieldShadingModelID), MaterialValueType::Int);
			appendUniformMember(std::string(MaterialSystemFieldCustomDataCount), MaterialValueType::Int);
		}

		layout.UniformBufferSize = AlignUp(uniformOffset, 16);
		return layout;
	}

	MaterialLightingRuntimeDesc MaterialCompiler::BuildLightingRuntime(
		const MaterialLightingDesc& lightingDesc)
	{
		ShaderLabLightingRuntimeDesc runtimeDesc{};
		runtimeDesc.Enabled = lightingDesc.Enabled;
		runtimeDesc.ShadingModelName = lightingDesc.ShadingModel.empty()
			? "DefaultLit"
			: lightingDesc.ShadingModel;
		runtimeDesc.CustomDataCount = std::min(lightingDesc.CustomDataCount, MaterialMaxCustomDataCount);
		runtimeDesc.LightingHookInclude = lightingDesc.Include;
		runtimeDesc.LightingHookFunction = lightingDesc.Evaluate;

		if (runtimeDesc.ShadingModelName == "Unlit")
			runtimeDesc.ShadingModelID = 1;
		else
			runtimeDesc.ShadingModelID = 0;

		return runtimeDesc;
	}

	std::string MaterialCompiler::BuildMaterialPrelude(
		const std::vector<MaterialPropertyDesc>& properties,
		const MaterialRuntimeLayout& materialLayout)
	{
		// 对于 deferred lighting / tonemap 这类 fullscreen pass，
		// 可能完全不依赖材质属性，而是自行声明 set=1 资源。
		// 这时不要再强行注入 Material cbuffer，避免和用户 shader 的 binding 冲突。
		if (materialLayout.UniformMembers.empty() && materialLayout.ResourceBindings.empty())
		{
			return {};
		}

		std::ostringstream oss;
		oss << "// =============================================\n";
		oss << "// ShaderLab generated material prelude\n";
		oss << "// 说明：\n";
		oss << "// 1. set=1, binding=0 固定为材质 UBO\n";
		oss << "// 2. 纹理属性按声明顺序分配到 set=1, binding=1...\n";
		oss << "// 3. 用户 shader 直接使用属性名，如 _BaseColor / _Albedo\n";
		oss << "// =============================================\n\n";

		oss << "struct KitaMaterialParams\n";
		oss << "{\n";
		for (const MaterialPropertyDesc& property : properties)
		{
			if (IsTextureProperty(property.ValueType))
				continue;

			const std::string fieldType = ToSlangUniformFieldType(property.ValueType);
			if (fieldType.empty())
				continue;

			oss << "    " << fieldType << " " << property.Name << ";\n";
		}
		oss << "    int " << MaterialSystemFieldShadingModelID << ";\n";
		oss << "    int " << MaterialSystemFieldCustomDataCount << ";\n";
		oss << "};\n\n";

		oss << "[[vk::binding(" << materialLayout.UniformBinding << ", " << materialLayout.DescriptorSet << ")]]\n";
		oss << "ConstantBuffer<KitaMaterialParams> g_MaterialParams;\n\n";

		for (const MaterialPropertyDesc& property : properties)
		{
			if (IsTextureProperty(property.ValueType))
			{
				const MaterialResourceBinding* binding = materialLayout.FindResourceBinding(property.Name);
				if (!binding)
					continue;

				oss << "[[vk::binding(" << binding->Binding << ", " << binding->Set << ")]]\n";
				oss << ToSlangResourceType(property.ValueType) << " " << property.Name << ";\n\n";
				continue;
			}

			if (property.ValueType == MaterialValueType::Bool)
			{
				oss << BuildBoolAccessorMacro(property.Name) << "\n";
			}
			else
			{
				oss << BuildDirectAccessorMacro(property.Name) << "\n";
			}
		}

		oss << "#define KITA_SHADING_MODEL_ID (g_MaterialParams." << MaterialSystemFieldShadingModelID << ")\n";
		oss << "#define KITA_CUSTOM_DATA_COUNT (g_MaterialParams." << MaterialSystemFieldCustomDataCount << ")\n";
		oss << "\n";
		return oss.str();
	}

	std::string MaterialCompiler::BuildLightingPrelude(
		const MaterialLightingDesc& lightingDesc)
	{
		if (!lightingDesc.Enabled)
		{
			return {};
		}

		std::ostringstream oss;
		oss << "// ShaderLab lighting metadata\n";
		oss << "#define KITA_LIGHTING_SHADING_MODEL \"" << lightingDesc.ShadingModel << "\"\n";
		oss << "#define KITA_LIGHTING_CUSTOM_DATA_COUNT " << lightingDesc.CustomDataCount << "\n\n";
		return oss.str();
	}

	std::string MaterialCompiler::ReadUserSource(const std::filesystem::path& path)
	{
		return ShaderCompiler::ReadTextFile(path);
	}

	std::string MaterialCompiler::GenerateWrappedSource(
		const MaterialDefinitionDesc& assetDesc,
		const MaterialPassDesc& passDesc,
		const MaterialRuntimeLayout& materialLayout,
		const std::string& userSource)
	{
		std::ostringstream oss;
		oss << "// Material Asset: " << assetDesc.MaterialName << "\n";
		oss << "// Pass: " << passDesc.Name << "\n";
		oss << "// Source: " << passDesc.Program.Source.string() << "\n\n";
		oss << BuildMaterialPrelude(assetDesc.Properties, materialLayout);
		oss << BuildLightingPrelude(assetDesc.Lighting);
		oss << "// ---------------- User Slang Source ----------------\n";
		oss << userSource << "\n";
		return oss.str();
	}

	ShaderCompiler::CompileRequest MaterialCompiler::BuildCompileRequest(
		const MaterialDefinitionDesc& assetDesc,
		const MaterialPassDesc& passDesc,
		const std::filesystem::path& materialPath,
		ShaderCompiler::Stage stage)
	{
		ShaderCompiler::CompileRequest request{};
		request.SourcePath = passDesc.Program.Source;
		request.ModuleName = ToModuleSafeName(
			assetDesc.MaterialName.empty() ? "Material" : assetDesc.MaterialName,
			passDesc.Name.empty() ? "Pass" : passDesc.Name,
			stage == ShaderCompiler::Stage::Vertex ? "vs" : "fs");
		request.EntryPointName =
			stage == ShaderCompiler::Stage::Vertex
			? passDesc.Program.VertexEntry
			: passDesc.Program.FragmentEntry;
		request.ShaderStage = stage;
		request.EmitDebugInfo = true;
		request.Optimize = false;

		// 保留用户源码目录，支持其原有 include/import 习惯。
		if (!passDesc.Program.Source.empty())
			request.IncludeDirs.push_back(passDesc.Program.Source.parent_path());

		// 同时补上 ShaderLab 文件自身目录，便于以后支持相对共享 include。
		if (!materialPath.empty())
			request.IncludeDirs.push_back(materialPath.parent_path());

		return request;
	}

	MaterialCompileResult MaterialCompiler::CompileAsset(
		const MaterialDefinitionDesc& assetDesc,
		const std::filesystem::path& materialPath) const
	{
		MaterialCompileResult result{};
		MaterialDefinitionDesc resolvedAssetDesc = assetDesc;
		std::ostringstream diagnostics;

		const bool hasGBufferPass = HasPassType(resolvedAssetDesc.Passes, PassType::GBuffer);
		const bool inferredSurfaceMaterial =
			resolvedAssetDesc.Domain == MaterialDomain::Surface ||
			resolvedAssetDesc.Lighting.Enabled ||
			hasGBufferPass;
		if (resolvedAssetDesc.UsesLegacyShaderKeyword)
		{
			diagnostics << "[Material] Legacy root keyword 'Shader' is deprecated. Use 'Material'.\n";
		}

		if (resolvedAssetDesc.Domain == MaterialDomain::Surface && !resolvedAssetDesc.Lighting.Enabled)
		{
			resolvedAssetDesc.Lighting.Enabled = true;
			resolvedAssetDesc.Lighting.ShadingModel = "DefaultLit";
			resolvedAssetDesc.Lighting.CustomDataCount = 0;
			diagnostics << "[Material] Surface material missing Lighting block, fallback to DefaultLit.\n";
		}

		if (resolvedAssetDesc.Domain != MaterialDomain::Surface && resolvedAssetDesc.Lighting.Enabled)
		{
			result.Success = false;
			result.Diagnostics = "Only Surface materials can declare a Lighting block.";
			return result;
		}

		if (resolvedAssetDesc.Domain == MaterialDomain::Surface || inferredSurfaceMaterial)
		{
			if (!hasGBufferPass)
			{
				result.Success = false;
				result.Diagnostics = "Surface material must contain a GBuffer pass.";
				return result;
			}

			if (resolvedAssetDesc.Lighting.ShadingModel.empty())
			{
				resolvedAssetDesc.Lighting.ShadingModel = "DefaultLit";
			}

			if (resolvedAssetDesc.Lighting.CustomDataCount > MaterialMaxCustomDataCount)
			{
				result.Success = false;
				result.Diagnostics = "Material Lighting.CustomDataCount exceeds the supported limit of 4.";
				return result;
			}

			if (!IsBuiltInShadingModel(resolvedAssetDesc.Lighting.ShadingModel) &&
				(resolvedAssetDesc.Lighting.Include.empty() || resolvedAssetDesc.Lighting.Evaluate.empty()))
			{
				result.Success = false;
				result.Diagnostics = "Custom shading model requires both Lighting.Include and Lighting.Evaluate.";
				return result;
			}
		}

		result.SourceAsset = resolvedAssetDesc;
		result.MaterialLayout = BuildMaterialLayout(
			resolvedAssetDesc.Properties,
			resolvedAssetDesc.Lighting.Enabled);
		result.LightingRuntime = BuildLightingRuntime(resolvedAssetDesc.Lighting);

		if (resolvedAssetDesc.Passes.empty())
		{
			result.Success = false;
			result.Diagnostics = "Material asset has no passes.";
			return result;
		}

		ShaderCompiler compiler;

		for (const MaterialPassDesc& passDesc : resolvedAssetDesc.Passes)
		{
			if ((resolvedAssetDesc.Domain == MaterialDomain::Surface || inferredSurfaceMaterial) &&
				passDesc.Type == PassType::DeferredLighting)
			{
				diagnostics << "[Pass " << passDesc.Name << "] DeferredLighting pass is ignored for Surface materials.\n";
				continue;
			}

			MaterialCompiledPass compiledPass{};
			compiledPass.Name = passDesc.Name;
			compiledPass.Type = passDesc.Type;
			compiledPass.RenderState = passDesc.RenderState;
			compiledPass.RenderGraph = passDesc.RenderGraph;
			compiledPass.Program = passDesc.Program;

			const std::string userSource = ReadUserSource(passDesc.Program.Source);
			if (userSource.empty())
			{
				diagnostics << "[Pass " << passDesc.Name << "] Failed to read source: "
					<< passDesc.Program.Source.string() << "\n";
				result.Success = false;
				continue;
			}

			compiledPass.WrappedSource = GenerateWrappedSource(
				resolvedAssetDesc,
				passDesc,
				result.MaterialLayout,
				userSource);

			const ShaderCompiler::CompileRequest vsRequest =
				BuildCompileRequest(resolvedAssetDesc, passDesc, materialPath, ShaderCompiler::Stage::Vertex);
			const ShaderCompiler::CompileRequest fsRequest =
				BuildCompileRequest(resolvedAssetDesc, passDesc, materialPath, ShaderCompiler::Stage::Fragment);

			const ShaderCompiler::CompileResult vsResult =
				compiler.CompileToSpirvFromSource(vsRequest, compiledPass.WrappedSource);
			const ShaderCompiler::CompileResult fsResult =
				compiler.CompileToSpirvFromSource(fsRequest, compiledPass.WrappedSource);

			if (!vsResult.Success)
			{
				diagnostics << "[Pass " << passDesc.Name << "][VS] "
					<< vsResult.Diagnostics << "\n";
			}
			else
			{
				compiledPass.VertexStage.EntryPoint = passDesc.Program.VertexEntry;
				compiledPass.VertexStage.EntryPoint = "main";
				compiledPass.VertexStage.Spirv = vsResult.Spirv;
			}

			if (!fsResult.Success)
			{
				diagnostics << "[Pass " << passDesc.Name << "][FS] "
					<< fsResult.Diagnostics << "\n";
			}
			else
			{
				compiledPass.FragmentStage.EntryPoint = passDesc.Program.FragmentEntry;
				compiledPass.FragmentStage.EntryPoint = "main";
				compiledPass.FragmentStage.Spirv = fsResult.Spirv;
			}

			const bool passSucceeded =
				compiledPass.VertexStage.Valid() &&
				compiledPass.FragmentStage.Valid();

			if (!passSucceeded)
			{
				result.Success = false;
				continue;
			}

			result.Passes.push_back(std::move(compiledPass));
		}

		result.Diagnostics = diagnostics.str();
		const size_t expectedPassCount = std::count_if(
			resolvedAssetDesc.Passes.begin(),
			resolvedAssetDesc.Passes.end(),
			[&](const MaterialPassDesc& pass)
			{
				return !((resolvedAssetDesc.Domain == MaterialDomain::Surface || inferredSurfaceMaterial) &&
					pass.Type == PassType::DeferredLighting);
			});
		result.Success = !result.Passes.empty() && result.Passes.size() == expectedPassCount;
		return result;
	}

}
