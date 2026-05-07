#include "kita_pch.h"
#include "ShaderCompiler.h"

#include "core/Log.h"

#include <fstream>
#include <sstream>

namespace Kita {

ShaderCompiler::ShaderCompiler()
{
	EnsureGlobalSession();
}

void ShaderCompiler::EnsureGlobalSession()
{
	if (m_GlobalSession)
		return;

	slang::IGlobalSession* session = nullptr;
	createGlobalSession(&session);
	m_GlobalSession.attach(session);
}

std::string ShaderCompiler::ReadTextFile(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	KITA_CORE_ASSERT(file.is_open(), "Failed to open shader file: {0}", path.string());

	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string text = buffer.str();
	if (text.size() >= 3 &&
		static_cast<unsigned char>(text[0]) == 0xEF &&
		static_cast<unsigned char>(text[1]) == 0xBB &&
		static_cast<unsigned char>(text[2]) == 0xBF)
	{
		text.erase(0, 3);
	}
	return text;
}

bool ShaderCompiler::WriteBinaryFile(const std::filesystem::path& path, const void* data, size_t size)
{
	std::error_code ec;
	std::filesystem::create_directories(path.parent_path(), ec);

	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		KITA_CORE_ERROR("Failed to write shader binary: {0}", path.string());
		return false;
	}

	file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
	return file.good();
}

std::string ShaderCompiler::GetDiagnosticsString(slang::IBlob* diagnostics)
{
	if (!diagnostics)
		return {};

	const char* text = static_cast<const char*>(diagnostics->getBufferPointer());
	size_t size = diagnostics->getBufferSize();
	if (!text || size == 0)
		return {};

	return std::string(text, text + size);
}

void ShaderCompiler::LogDiagnostics(slang::IBlob* diagnostics)
{
	std::string message = GetDiagnosticsString(diagnostics);
	if (message.empty())
		return;

	std::istringstream stream(message);
	std::string line;
	while (std::getline(stream, line))
	{
		if (!line.empty())
			KITA_CORE_ERROR("[slang] {0}", line);
	}
}

void ShaderCompiler::AppendMacro(std::vector<slang::PreprocessorMacroDesc>& macros, const std::string& name, const std::string& value)
{
	slang::PreprocessorMacroDesc macro{};
	macro.name = name.c_str();
	macro.value = value.empty() ? nullptr : value.c_str();
	macros.push_back(macro);
}

SlangStage ShaderCompiler::ToSlangStage(Stage stage)
{
	switch (stage)
	{
		case Stage::Vertex:          return SLANG_STAGE_VERTEX;
		case Stage::Fragment:        return SLANG_STAGE_FRAGMENT;
		case Stage::Compute:         return SLANG_STAGE_COMPUTE;
		case Stage::Geometry:        return SLANG_STAGE_GEOMETRY;
		case Stage::TessControl:     return SLANG_STAGE_HULL;
		case Stage::TessEvaluation:  return SLANG_STAGE_DOMAIN;
		default:                     return SLANG_STAGE_VERTEX;
	}
}

ShaderCompiler::CompileResult ShaderCompiler::CompileToSpirv(const CompileRequest& request)
{
	std::string source = ReadTextFile(request.SourcePath);
	if (source.empty())
	{
		CompileResult result{};
		result.Diagnostics = "shader source is empty";
		return result;
	}

	return CompileToSpirvInternal(request, source);
}

ShaderCompiler::CompileResult ShaderCompiler::CompileToSpirvFromSource(const CompileRequest& request, const std::string& source)
{
	if (source.empty())
	{
		CompileResult result{};
		result.Diagnostics = "shader source is empty";
		return result;
	}

	return CompileToSpirvInternal(request, source);
}

ShaderCompiler::CompileResult ShaderCompiler::CompileToSpirvInternal(const CompileRequest& request, const std::string& source)
{
	EnsureGlobalSession();

	CompileResult result{};

	slang::SessionDesc sessionDesc{};
	slang::TargetDesc targetDesc{};
	targetDesc.format = SLANG_SPIRV;
	targetDesc.profile = m_GlobalSession->findProfile(request.Profile.c_str());

	std::array<slang::CompilerOptionEntry, 2> optionEntries{};
	uint32_t optionCount = 0;

	optionEntries[optionCount].name = slang::CompilerOptionName::EmitSpirvDirectly;
	optionEntries[optionCount].value = { slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr };
	++optionCount;

	if (request.EmitDebugInfo)
	{
		optionEntries[optionCount].name = slang::CompilerOptionName::DebugInformation;
		optionEntries[optionCount].value = { slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_STANDARD, 0, nullptr, nullptr };
		++optionCount;
	}

	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	sessionDesc.compilerOptionEntries = optionEntries.data();
	sessionDesc.compilerOptionEntryCount = optionCount;

	std::vector<slang::PreprocessorMacroDesc> macros;
	macros.reserve(request.Defines.size());
	for (const auto& [key, value] : request.Defines)
	{
		AppendMacro(macros, key, value);
	}
	sessionDesc.preprocessorMacros = macros.empty() ? nullptr : macros.data();
	sessionDesc.preprocessorMacroCount = static_cast<uint32_t>(macros.size());

	std::vector<std::string> includeStorage;
	std::vector<const char*> includeDirs;
	includeStorage.reserve(request.IncludeDirs.size());
	includeDirs.reserve(request.IncludeDirs.size());
	for (const auto& includeDir : request.IncludeDirs)
	{
		includeStorage.push_back(includeDir.string());
		includeDirs.push_back(nullptr);
	}

	for (size_t i = 0; i < includeStorage.size(); ++i)
		includeDirs[i] = includeStorage[i].c_str();

	sessionDesc.searchPaths = includeDirs.empty() ? nullptr : includeDirs.data();
	sessionDesc.searchPathCount = static_cast<uint32_t>(includeDirs.size());

	Slang::ComPtr<slang::ISession> session;
	SlangResult createSessionResult = m_GlobalSession->createSession(sessionDesc, session.writeRef());
	if (SLANG_FAILED(createSessionResult))
	{
		result.Diagnostics = "failed to create slang session";
		return result;
	}

	Slang::ComPtr<slang::IModule> module;
	Slang::ComPtr<slang::IBlob> diagnostics;
	const std::string sourcePathString = request.SourcePath.empty()
		? request.ModuleName
		: request.SourcePath.string();
	module = session->loadModuleFromSourceString(
		request.ModuleName.c_str(),
		sourcePathString.c_str(),
		source.c_str(),
		diagnostics.writeRef());
	LogDiagnostics(diagnostics);
	if (!module)
	{
		result.Diagnostics = GetDiagnosticsString(diagnostics);
		if (result.Diagnostics.empty())
			result.Diagnostics = "failed to load slang module";
		return result;
	}

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	diagnostics = nullptr;
	module->findEntryPointByName(request.EntryPointName.c_str(), entryPoint.writeRef());
	if (!entryPoint)
	{
		if (SLANG_FAILED(module->findAndCheckEntryPoint(
			request.EntryPointName.c_str(),
			ToSlangStage(request.ShaderStage),
			entryPoint.writeRef(),
			diagnostics.writeRef())))
		{
			LogDiagnostics(diagnostics);
			result.Diagnostics = GetDiagnosticsString(diagnostics);
			if (result.Diagnostics.empty())
				result.Diagnostics = "failed to find entry point";
			return result;
		}
	}

	Slang::ComPtr<slang::IComponentType> program;
	slang::IComponentType* components[] = { module, entryPoint };
	diagnostics = nullptr;
	if (SLANG_FAILED(session->createCompositeComponentType(
		components,
		2,
		program.writeRef(),
		diagnostics.writeRef())))
	{
		LogDiagnostics(diagnostics);
		result.Diagnostics = GetDiagnosticsString(diagnostics);
		if (result.Diagnostics.empty())
			result.Diagnostics = "failed to create composite component type";
		return result;
	}

	Slang::ComPtr<slang::IComponentType> linked;
	diagnostics = nullptr;
	if (SLANG_FAILED(program->link(linked.writeRef(), diagnostics.writeRef())))
	{
		LogDiagnostics(diagnostics);
		result.Diagnostics = GetDiagnosticsString(diagnostics);
		if (result.Diagnostics.empty())
			result.Diagnostics = "failed to link slang program";
		return result;
	}
	program = linked;

	Slang::ComPtr<slang::IBlob> spirvBlob;
	diagnostics = nullptr;
	if (SLANG_FAILED(program->getEntryPointCode(0, 0, spirvBlob.writeRef(), diagnostics.writeRef())))
	{
		LogDiagnostics(diagnostics);
		result.Diagnostics = GetDiagnosticsString(diagnostics);
		if (result.Diagnostics.empty())
			result.Diagnostics = "failed to get spirv code";
		return result;
	}

	const uint8_t* data = reinterpret_cast<const uint8_t*>(spirvBlob->getBufferPointer());
	size_t size = spirvBlob->getBufferSize();
	result.Spirv.assign(data, data + size);
	result.Success = true;

	if (diagnostics)
		LogDiagnostics(diagnostics);

	return result;
}

}
