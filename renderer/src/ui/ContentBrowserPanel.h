#pragma once
#include <Engine.h>

#include <array>

namespace Kita {

	class ContentBrowserPanel {

	public:
		ContentBrowserPanel() = default;
		ContentBrowserPanel(const std::filesystem::path& contentPath);


		void SetContentRoot(const std::filesystem::path& contentRoot);
		void SetToolbarHeight(float toolbarHeight);
		void OnImGuiRender();


	private:
		void DrawToolbar();
		void DrawDirectoryTree(const std::filesystem::path& directory);
		void DrawDirectoryContents();
		void DrawBreadcrumb();
		std::string GetDisplayName(const std::filesystem::path& path) const;

	private:
		std::filesystem::path m_ContentRoot;
		std::filesystem::path m_SelectedDirectory;
		std::array<char, 256> m_SearchBuffer{};
		float m_ToolbarHeight = 24.0f;
	};
}
