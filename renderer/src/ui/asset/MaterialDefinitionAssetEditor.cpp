#include "renderer_pch.h"
#include "MaterialDefinitionAssetEditor.h"

#include "AssetEditorUtils.h"
#include "asset/AssetManager.h"
#include "file/Project.h"
#include "render/VulkanResourceFactory.h"
#include "render/shaderLab/ShaderLabCompiler.h"
#include "render/shaderLab/ShaderLabParser.h"
#include "ui/ThumbnailCache.h"

#include "imgui.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace Kita {

	namespace
	{
		static int InputTextResizeCallback(ImGuiInputTextCallbackData* data)
		{
			if (data->EventFlag != ImGuiInputTextFlags_CallbackResize)
			{
				return 0;
			}

			std::string* text = static_cast<std::string*>(data->UserData);
			if (!text)
			{
				return 0;
			}

			text->resize(static_cast<size_t>(data->BufTextLen));
			data->Buf = text->data();
			return 0;
		}

		static bool InputTextMultilineStdString(
			const char* label,
			std::string& text,
			const ImVec2& size,
			ImGuiInputTextFlags flags = 0)
		{
			flags |= ImGuiInputTextFlags_CallbackResize;
			return ImGui::InputTextMultiline(
				label,
				text.data(),
				text.capacity() + 1,
				size,
				flags,
				InputTextResizeCallback,
				&text);
		}

		bool ReadTextFile(const std::filesystem::path& path, std::string& outText)
		{
			std::ifstream input(path, std::ios::binary);
			if (!input.is_open())
			{
				return false;
			}

			std::stringstream stream;
			stream << input.rdbuf();
			outText = stream.str();
			return true;
		}
	}

	MaterialDefinitionAssetEditor::MaterialDefinitionAssetEditor(
		AssetHandle handle,
		ThumbnailCache* thumbnailCache,
		VulkanResourceFactory* resourceFactory,
		PreviewSceneRenderer* previewSceneRenderer)
		: m_AssetHandle(handle)
		, m_ThumbnailCache(thumbnailCache)
		, m_ResourceFactory(resourceFactory)
		, m_PreviewSceneRenderer(previewSceneRenderer)
		, m_LookDevPreview(previewSceneRenderer)
	{
		m_SourceAsset = AssetManager::GetInstance().GetMaterialDefinitionAsset(handle);
		m_DisplayName = GetAssetEditorDisplayName(handle);
		m_LookDevPreview.SetMaterialDefinitionAsset(handle);

		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
		{
			const Ref<Project> project = Project::GetActive();
			if (project)
			{
				m_AssetPath = project->GetAssetRootDirectory() / metadata->relativePath;
			}
		}

		ReloadSourceFromDisk();
		ApplyWorkingSourceToRuntime(false);
	}

	bool MaterialDefinitionAssetEditor::IsDirty() const
	{
		return m_SourceText != m_SavedSourceText;
	}

	void MaterialDefinitionAssetEditor::Save()
	{
		if (m_AssetPath.empty())
		{
			return;
		}

		std::ofstream output(m_AssetPath, std::ios::binary | std::ios::trunc);
		if (!output.is_open())
		{
			m_Diagnostics = "Failed to open material file for write.";
			return;
		}

		output << m_SourceText;
		output.close();
		m_SavedSourceText = m_SourceText;

		if (m_RecompilePending)
		{
			ApplyWorkingSourceToRuntime(true);
		}
	}

	void MaterialDefinitionAssetEditor::Revert()
	{
		ReloadSourceFromDisk();
		ApplyWorkingSourceToRuntime(false);
	}

	void MaterialDefinitionAssetEditor::OnUpdate()
	{
		if (m_RecompilePending)
		{
			ApplyWorkingSourceToRuntime(false);
		}
	}

	void MaterialDefinitionAssetEditor::OnRender()
	{
		m_LookDevPreview.OnRender();
	}

	void MaterialDefinitionAssetEditor::OnImGuiRender()
	{
		DrawToolbar();

		if (ImGui::BeginTable("##MaterialDefinitionEditorLayout", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.38f);
			ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch, 0.62f);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			DrawSummaryPanel();

			ImGui::TableSetColumnIndex(1);
			DrawSourcePanel();

			ImGui::EndTable();
		}
	}

	bool MaterialDefinitionAssetEditor::ReloadSourceFromDisk()
	{
		if (m_AssetPath.empty())
		{
			return false;
		}

		if (!ReadTextFile(m_AssetPath, m_SourceText))
		{
			m_Diagnostics = "Failed to read material source file.";
			return false;
		}

		m_SavedSourceText = m_SourceText;
		m_RecompilePending = true;
		return true;
	}

	bool MaterialDefinitionAssetEditor::ApplyWorkingSourceToRuntime(bool logFailures)
	{
		m_RecompilePending = false;

		const MaterialParseResult parseResult = MaterialDefinitionParser::ParseText(m_SourceText, m_AssetPath);
		if (!parseResult.Success)
		{
			m_CompileSucceeded = false;
			m_Diagnostics = parseResult.Error.Message;
			return false;
		}

		MaterialCompiler compiler{};
		const MaterialCompileResult compileResult = compiler.CompileAsset(parseResult.Asset, m_AssetPath);
		m_CompileSucceeded = compileResult.Success;
		m_Diagnostics = compileResult.Diagnostics.empty()
			? (compileResult.Success ? "Compiled successfully." : "Compile failed.")
			: compileResult.Diagnostics;

		if (!compileResult.Success)
		{
			if (logFailures)
			{
				KITA_CORE_WARN("Material compile failed: {}", m_Diagnostics);
			}
			return false;
		}

		if (m_SourceAsset)
		{
			m_SourceAsset->SourcePath = m_AssetPath;
			m_SourceAsset->Desc = std::make_shared<MaterialDefinitionDesc>(compileResult.SourceAsset);
			m_SourceAsset->RuntimeLayout = std::make_shared<MaterialRuntimeLayout>(compileResult.MaterialLayout);
			m_SourceAsset->LightingRuntime = std::make_shared<MaterialLightingRuntimeDesc>(compileResult.LightingRuntime);
			m_SourceAsset->CompiledPasses = compileResult.Passes;
		}

		if (m_ResourceFactory)
		{
			Application::Get().GetVulkanContext().WaitIdle();
			m_ResourceFactory->InvalidateMaterialDefinition(m_AssetHandle);
		}

		if (m_ThumbnailCache)
		{
			m_ThumbnailCache->Invalidate(m_AssetHandle);
		}

		if (m_PreviewSceneRenderer)
		{
			m_PreviewSceneRenderer->Clear();
		}

		return true;
	}

	void MaterialDefinitionAssetEditor::DrawToolbar()
	{
		ImGui::BeginChild("##MaterialDefinitionToolbar", ImVec2(0.0f, 44.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Material");
		ImGui::SameLine();
		ImGui::TextDisabled("%s", m_DisplayName.c_str());

		const float buttonWidth = 74.0f;
		ImGui::SameLine(std::max(0.0f, ImGui::GetContentRegionAvail().x - buttonWidth * 2.0f - 12.0f));
		if (ImGui::Button("Save", ImVec2(buttonWidth, 0.0f)))
		{
			Save();
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert", ImVec2(buttonWidth, 0.0f)))
		{
			Revert();
		}
		ImGui::EndChild();
	}

	void MaterialDefinitionAssetEditor::DrawSummaryPanel()
	{
		ImGui::BeginChild("##MaterialDefinitionSummary", ImVec2(0.0f, 0.0f), true);
		ImGui::TextUnformatted("Preview");
		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float previewSize = std::clamp(std::min(available.x, available.y * 0.48f), 180.0f, 320.0f);
		m_LookDevPreview.Draw("##MaterialDefinitionLookDev", ImVec2(previewSize, previewSize));

		ImGui::Dummy(ImVec2(0.0f, 12.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		if (m_SourceAsset && m_SourceAsset->Desc)
		{
			ImGui::Text("Domain: %s", MaterialDomainToString(m_SourceAsset->Desc->Domain));
			if (m_SourceAsset->Desc->Domain == MaterialDomain::Surface)
			{
				const std::string shadingModel = m_SourceAsset->Desc->Lighting.ShadingModel.empty()
					? "DefaultLit"
					: m_SourceAsset->Desc->Lighting.ShadingModel;
				ImGui::Text("Shading Model: %s", shadingModel.c_str());
			}
			ImGui::Text("Pass Count: %u", static_cast<uint32_t>(m_SourceAsset->Desc->Passes.size()));
		}
		else
		{
			ImGui::TextDisabled("Material metadata unavailable.");
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::TextColored(
			m_CompileSucceeded ? ImVec4(0.55f, 0.82f, 0.56f, 1.0f) : ImVec4(0.92f, 0.54f, 0.38f, 1.0f),
			"%s",
			m_CompileSucceeded ? "Compile Status: OK" : "Compile Status: Error");
		ImGui::Spacing();
		ImGui::TextWrapped("%s", m_Diagnostics.empty() ? "No diagnostics." : m_Diagnostics.c_str());
		ImGui::EndChild();
	}

	void MaterialDefinitionAssetEditor::DrawSourcePanel()
	{
		ImGui::BeginChild("##MaterialDefinitionSource", ImVec2(0.0f, 0.0f), true);
		ImGui::TextUnformatted("Source");
		ImGui::Dummy(ImVec2(0.0f, 8.0f));

		const ImVec2 editorSize = ImGui::GetContentRegionAvail();
		if (InputTextMultilineStdString(
			"##MaterialSourceText",
			m_SourceText,
			editorSize,
			ImGuiInputTextFlags_AllowTabInput))
		{
			m_RecompilePending = true;
		}

		ImGui::EndChild();
	}

}
