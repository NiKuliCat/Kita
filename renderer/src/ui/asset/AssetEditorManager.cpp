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
		constexpr char kFloatingHostWindowName[] = "Asset Editors###AssetEditorsHost";
		constexpr char kFloatingDockSpaceName[] = "AssetEditorsFloatingDockspace";
		constexpr ImVec2 kDefaultAssetEditorWindowSize(1080.0f, 720.0f);
		constexpr ImVec2 kMinAssetEditorWindowSize(720.0f, 480.0f);
		constexpr ImVec2 kFloatingHostPadding(6.0f, 6.0f);
		const ImVec4 kFloatingHostBgColor = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
		const ImVec4 kFloatingHostTitleColor = ImVec4(0.07f, 0.07f, 0.08f, 1.0f);
		const ImVec4 kFloatingHostTitleActiveColor = ImVec4(0.08f, 0.08f, 0.09f, 1.0f);
		const ImVec4 kFloatingHostBorderColor = ImVec4(0.04f, 0.04f, 0.05f, 1.0f);
		const ImVec4 kFloatingHostTabColor = ImVec4(0.13f, 0.13f, 0.14f, 1.0f);
		const ImVec4 kFloatingHostTabActiveColor = ImVec4(0.19f, 0.19f, 0.20f, 1.0f);
		const ImVec4 kFloatingHostTabHoveredColor = ImVec4(0.16f, 0.16f, 0.18f, 1.0f);

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

		std::string BuildTabContextMenuId(AssetHandle handle)
		{
			return "AssetEditorTabContextMenu##" + std::to_string(handle);
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
			m_RequestHostWindowFocus = true;
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
		m_RequestHostWindowFocus = true;
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
		DrawFloatingHostWindow();

		std::vector<AssetHandle> editorsToClose;
		const auto enqueueClose = [&editorsToClose](AssetHandle handle)
		{
			if (!Asset::IsValidHandle(handle))
			{
				return;
			}

			if (std::find(editorsToClose.begin(), editorsToClose.end(), handle) == editorsToClose.end())
			{
				editorsToClose.push_back(handle);
			}
		};
		for (auto& entry : m_OpenEditors)
		{
			if (!entry.Editor || !entry.IsOpen)
			{
				continue;
			}

			ImGuiID initialDockId = 0;
			if (entry.PendingInitialDock)
			{
				if (entry.OpenAsFloatingRoot)
				{
					if (!SetupFloatingRootDock(entry, initialDockId))
					{
						continue;
					}
				}
				else
				{
					if (!SetupDockWithReference(entry, initialDockId))
					{
						continue;
					}
				}
			}

			if (entry.RequestFocus)
			{
				ImGui::SetNextWindowFocus();
				entry.RequestFocus = false;
			}

			const std::string title = BuildWindowTitle(*entry.Editor);
			if (initialDockId != 0)
			{
				ImGui::SetNextWindowDockID(initialDockId, ImGuiCond_Always);
				entry.PendingInitialDock = false;
			}
			ImGui::SetNextWindowSize(kDefaultAssetEditorWindowSize, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSizeConstraints(kMinAssetEditorWindowSize, ImVec2(FLT_MAX, FLT_MAX));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.07f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
			{
				ImGuiWindow* window = ImGui::GetCurrentWindow();
				const ImRect titleBarRect = window->TitleBarRect();
				const std::string contextMenuId = BuildTabContextMenuId(entry.Editor->GetAssetHandle());
				if (ImGui::IsMouseHoveringRect(titleBarRect.Min, titleBarRect.Max, false) &&
					ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup(contextMenuId.c_str());
				}

				if (ImGui::BeginPopup(contextMenuId.c_str()))
				{
					if (ImGui::MenuItem("Close"))
					{
						enqueueClose(entry.Editor->GetAssetHandle());
					}

					if (ImGui::MenuItem("Close Others"))
					{
						const AssetHandle currentHandle = entry.Editor->GetAssetHandle();
						for (const auto& otherEntry : m_OpenEditors)
						{
							if (!otherEntry.Editor)
							{
								continue;
							}

							const AssetHandle otherHandle = otherEntry.Editor->GetAssetHandle();
							if (otherHandle != currentHandle)
							{
								enqueueClose(otherHandle);
							}
						}
					}

					if (ImGui::MenuItem("Close All"))
					{
						for (const auto& otherEntry : m_OpenEditors)
						{
							if (otherEntry.Editor)
							{
								enqueueClose(otherEntry.Editor->GetAssetHandle());
							}
						}
					}
					ImGui::EndPopup();
				}

				m_ActiveAssetHandle = entry.Editor->GetAssetHandle();
				entry.Editor->OnImGuiRender();
			}
			ImGui::End();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor();
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
			m_ShowFloatingHostWindow = false;
		}
	}

	void AssetEditorManager::DrawFloatingHostWindow()
	{
		if (m_OpenEditors.empty())
		{
			m_ShowFloatingHostWindow = false;
			return;
		}

		bool hasFloatingEditor = false;
		for (const auto& entry : m_OpenEditors)
		{
			if (entry.IsOpen)
			{
				hasFloatingEditor = true;
				break;
			}
		}

		if (!hasFloatingEditor)
		{
			m_ShowFloatingHostWindow = false;
			return;
		}

		const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		const ImVec2 workPos = mainViewport ? mainViewport->WorkPos : ImVec2(0.0f, 0.0f);
		const ImVec2 workSize = mainViewport ? mainViewport->WorkSize : ImVec2(1600.0f, 900.0f);
		const ImVec2 floatingSize(
			ImClamp(workSize.x * 0.62f, 880.0f, 1280.0f),
			ImClamp(workSize.y * 0.74f, 620.0f, 900.0f));
		const ImVec2 floatingPos(
			workPos.x + (workSize.x - floatingSize.x) * 0.5f,
			workPos.y + (workSize.y - floatingSize.y) * 0.5f);

		m_ShowFloatingHostWindow = true;
		ImGui::SetNextWindowPos(floatingPos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(floatingSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(kMinAssetEditorWindowSize, ImVec2(FLT_MAX, FLT_MAX));
		if (m_RequestHostWindowFocus)
		{
			ImGui::SetNextWindowFocus();
		}

		ImGuiWindowFlags hostFlags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoCollapse;

		ImGui::PushStyleColor(ImGuiCol_WindowBg, kFloatingHostBgColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBg, kFloatingHostTitleColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, kFloatingHostTitleActiveColor);
		ImGui::PushStyleColor(ImGuiCol_Border, kFloatingHostBorderColor);
		ImGui::PushStyleColor(ImGuiCol_Tab, kFloatingHostTabColor);
		ImGui::PushStyleColor(ImGuiCol_TabActive, kFloatingHostTabActiveColor);
		ImGui::PushStyleColor(ImGuiCol_TabHovered, kFloatingHostTabHoveredColor);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, kFloatingHostPadding);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		if (ImGui::Begin(kFloatingHostWindowName, &m_ShowFloatingHostWindow, hostFlags))
		{
			const ImGuiID dockspaceId = ImGui::GetID(kFloatingDockSpaceName);
			m_FloatingDockRootId = dockspaceId;
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_AutoHideTabBar);
		}
		ImGui::End();
		m_RequestHostWindowFocus = false;
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(7);

		if (!m_ShowFloatingHostWindow)
		{
			for (auto& entry : m_OpenEditors)
			{
				entry.IsOpen = false;
			}

			if (m_FloatingDockRootId != 0)
			{
				ImGui::DockBuilderRemoveNode(m_FloatingDockRootId);
				m_FloatingDockRootId = 0;
			}
			m_RequestHostWindowFocus = false;
		}
	}

	bool AssetEditorManager::SetupFloatingRootDock(OpenEditorEntry& entry, ImGuiID& outDockId)
	{
		outDockId = 0;
		if (!entry.Editor)
		{
			return false;
		}

		m_ShowFloatingHostWindow = true;
		if (m_FloatingDockRootId == 0)
		{
			return false;
		}

		outDockId = m_FloatingDockRootId;
		return true;
	}

	bool AssetEditorManager::SetupDockWithReference(OpenEditorEntry& entry, ImGuiID& outDockId)
	{
		outDockId = 0;
		if (!entry.Editor)
		{
			return false;
		}

		ImGuiID targetDockId = 0;

		if (const OpenEditorEntry* referenceEntry = FindOpenEditorEntry(entry.DockReferenceHandle))
		{
			std::string referenceTitle = BuildWindowTitle(*referenceEntry->Editor);
			if (ImGuiWindow* referenceWindow = ImGui::FindWindowByName(referenceTitle.c_str()))
			{
				if (referenceWindow->DockId != 0)
				{
					targetDockId = referenceWindow->DockId;
				}
				else
				{
					targetDockId = m_FloatingDockRootId;
				}
			}
		}

		if (targetDockId == 0)
		{
			if (m_FloatingDockRootId == 0)
			{
				entry.OpenAsFloatingRoot = true;
				const bool dockReady = SetupFloatingRootDock(entry, targetDockId);
				entry.OpenAsFloatingRoot = false;
				if (!dockReady)
				{
					return false;
				}
			}
			else
			{
				targetDockId = m_FloatingDockRootId;
			}
		}

		outDockId = targetDockId;
		return outDockId != 0;
	}

}
