#include "renderer_pch.h"
#include "AssetEditorManager.h"

#include "MaterialAssetEditor.h"
#include "MeshAssetEditor.h"
#include "TextureAssetEditor.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		std::string BuildWindowTitle(IAssetEditor& editor)
		{
			std::string title = editor.GetDisplayName();
			if (editor.IsDirty())
			{
				title += "*";
			}
			title += "###AssetEditor_";
			title += std::to_string(editor.GetAssetHandle());
			return title;
		}

	}

	bool AssetEditorManager::OpenEditor(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return false;
		}

		if (OpenEditorEntry* existingEditor = FindOpenEditorEntry(handle))
		{
			existingEditor->IsOpen = true;
			existingEditor->RequestFocus = true;
			m_ActiveAssetHandle = handle;
			return true;
		}

		const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle);
		if (!metadata)
		{
			return false;
		}

		Unique<IAssetEditor> editor = CreateEditor(*metadata);
		if (!editor)
		{
			return false;
		}

		m_ActiveAssetHandle = handle;
		OpenEditorEntry entry{};
		entry.Editor = std::move(editor);
		entry.IsOpen = true;
		entry.RequestFocus = true;
		entry.PendingInitialDock = true;
		if (m_OpenEditors.empty())
		{
			entry.OpenAsFloatingRoot = true;
		}
		else
		{
			entry.DockReferenceHandle = m_OpenEditors.back().Editor ? m_OpenEditors.back().Editor->GetAssetHandle() : InvalidAssetHandle;
		}
		m_OpenEditors.push_back(std::move(entry));
		return true;
	}

	void AssetEditorManager::OnImGuiRender()
	{
		std::vector<AssetHandle> editorsToClose;
		for (auto& entry : m_OpenEditors)
		{
			if (!entry.Editor || !entry.IsOpen)
			{
				continue;
			}

			if (entry.PendingInitialDock)
			{
				if (entry.OpenAsFloatingRoot)
				{
					SetupFloatingRootDock(entry);
				}
				else
				{
					SetupDockWithReference(entry);
				}
				entry.PendingInitialDock = false;
			}

			if (entry.RequestFocus)
			{
				ImGui::SetNextWindowFocus();
				entry.RequestFocus = false;
			}

			bool keepOpen = entry.IsOpen;
			const std::string title = BuildWindowTitle(*entry.Editor);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.07f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			if (ImGui::Begin(title.c_str(), &keepOpen))
			{
				m_ActiveAssetHandle = entry.Editor->GetAssetHandle();
				entry.Editor->OnImGuiRender();
			}
			ImGui::End();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();

			entry.IsOpen = keepOpen;
			if (!keepOpen)
			{
				editorsToClose.push_back(entry.Editor->GetAssetHandle());
			}
		}

		for (AssetHandle handle : editorsToClose)
		{
			CloseEditor(handle);
		}
	}

	Unique<IAssetEditor> AssetEditorManager::CreateEditor(const AssetMetadata& metadata) const
	{
		switch (metadata.type)
		{
		case AssetType::Texture:
			return CreateUnique<TextureAssetEditor>(metadata.handle, m_ThumbnailCache, m_ResourceFactory);
		case AssetType::Mesh:
			return CreateUnique<MeshAssetEditor>(metadata.handle);
		case AssetType::Material:
			return CreateUnique<MaterialAssetEditor>(metadata.handle, m_ThumbnailCache, m_ResourceFactory);
		default:
			return nullptr;
		}
	}

	IAssetEditor* AssetEditorManager::FindOpenEditor(AssetHandle handle) const
	{
		for (const auto& entry : m_OpenEditors)
		{
			if (entry.Editor && entry.Editor->GetAssetHandle() == handle)
			{
				return entry.Editor.get();
			}
		}

		return nullptr;
	}

	AssetEditorManager::OpenEditorEntry* AssetEditorManager::FindOpenEditorEntry(AssetHandle handle)
	{
		for (auto& entry : m_OpenEditors)
		{
			if (entry.Editor && entry.Editor->GetAssetHandle() == handle)
			{
				return &entry;
			}
		}

		return nullptr;
	}

	const AssetEditorManager::OpenEditorEntry* AssetEditorManager::FindOpenEditorEntry(AssetHandle handle) const
	{
		for (const auto& entry : m_OpenEditors)
		{
			if (entry.Editor && entry.Editor->GetAssetHandle() == handle)
			{
				return &entry;
			}
		}

		return nullptr;
	}

	void AssetEditorManager::CloseEditor(AssetHandle handle)
	{
		m_OpenEditors.erase(
			std::remove_if(
				m_OpenEditors.begin(),
				m_OpenEditors.end(),
				[handle](const OpenEditorEntry& entry)
				{
					return entry.Editor && entry.Editor->GetAssetHandle() == handle;
				}),
			m_OpenEditors.end());

		if (m_ActiveAssetHandle == handle)
		{
			m_ActiveAssetHandle = InvalidAssetHandle;
		}

		if (m_OpenEditors.empty() && m_FloatingDockRootId != 0)
		{
			ImGui::DockBuilderRemoveNode(m_FloatingDockRootId);
			m_FloatingDockRootId = 0;
		}
	}

	void AssetEditorManager::SetupFloatingRootDock(OpenEditorEntry& entry)
	{
		if (!entry.Editor)
		{
			return;
		}

		if (m_FloatingDockRootId != 0)
		{
			ImGui::DockBuilderRemoveNode(m_FloatingDockRootId);
			m_FloatingDockRootId = 0;
		}

		const std::string windowTitle = BuildWindowTitle(*entry.Editor);
		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		const ImVec2 workPos = mainViewport ? mainViewport->WorkPos : ImVec2(0.0f, 0.0f);
		const ImVec2 workSize = mainViewport ? mainViewport->WorkSize : ImVec2(1600.0f, 900.0f);

		const ImVec2 floatingSize(
			ImClamp(workSize.x * 0.62f, 880.0f, 1280.0f),
			ImClamp(workSize.y * 0.74f, 620.0f, 900.0f));
		const ImVec2 floatingPos(
			workPos.x + (workSize.x - floatingSize.x) * 0.5f,
			workPos.y + (workSize.y - floatingSize.y) * 0.5f);

		m_FloatingDockRootId = ImGui::DockBuilderAddNode(0);
		ImGui::DockBuilderSetNodePos(m_FloatingDockRootId, floatingPos);
		ImGui::DockBuilderSetNodeSize(m_FloatingDockRootId, floatingSize);
		ImGui::DockBuilderDockWindow(windowTitle.c_str(), m_FloatingDockRootId);
		ImGui::DockBuilderFinish(m_FloatingDockRootId);
	}

	void AssetEditorManager::SetupDockWithReference(OpenEditorEntry& entry)
	{
		if (!entry.Editor)
		{
			return;
		}

		const std::string windowTitle = BuildWindowTitle(*entry.Editor);
		ImGuiID targetDockId = 0;

		if (const OpenEditorEntry* referenceEntry = FindOpenEditorEntry(entry.DockReferenceHandle))
		{
			std::string referenceTitle = BuildWindowTitle(*referenceEntry->Editor);
			if (ImGuiWindow* referenceWindow = ImGui::FindWindowByName(referenceTitle.c_str()))
			{
				if (referenceWindow->DockId != 0)
				{
					targetDockId = referenceWindow->DockId;
					ImGui::DockBuilderDockWindow(windowTitle.c_str(), targetDockId);
				}
				else
				{
					targetDockId = m_FloatingDockRootId;
					if (targetDockId != 0)
					{
						ImGui::DockBuilderDockWindow(windowTitle.c_str(), targetDockId);
					}
				}
			}
		}

		if (targetDockId == 0)
		{
			if (m_FloatingDockRootId == 0)
			{
				entry.OpenAsFloatingRoot = true;
				SetupFloatingRootDock(entry);
				entry.OpenAsFloatingRoot = false;
				return;
			}

			ImGui::DockBuilderDockWindow(windowTitle.c_str(), m_FloatingDockRootId);
		}
	}

}
