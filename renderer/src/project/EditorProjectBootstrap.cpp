#include "renderer_pch.h"
#include "EditorProjectBootstrap.h"

#include "file/Project.h"

namespace Kita {

	namespace
	{
		std::filesystem::path GetPackagesDirectory()
		{
			const Ref<Project> project = Project::GetActive();
			if (!project)
				return {};

			return project->GetPackagesDirectory();
		}

		bool HasPathPrefix(const std::filesystem::path& path, const std::string& prefix)
		{
			const std::string pathString = path.generic_string();
			return pathString.size() >= prefix.size()
				&& pathString.compare(0, prefix.size(), prefix) == 0;
		}

		void PreloadPackageAssetsByType(AssetType type, const std::string& prefix)
		{
			auto& assetManager = AssetManager::GetInstance();
			const auto assets = assetManager.GetAssetsByType(type);
			for (const auto& metadata : assets)
			{
				if (!HasPathPrefix(metadata.relativePath, prefix))
					continue;

				assetManager.LoadAsset(metadata.handle);
			}
		}
	}

	void EditorProjectBootstrap::Initialize()
	{
		const Ref<Project> project = Project::GetActive();
		if (!project)
			return;

		PreloadPackageAssetsByType(AssetType::Shader, "packages/render/");
		PreloadPackageAssetsByType(AssetType::Mesh, "packages/render/");
		PreloadPackageAssetsByType(AssetType::Texture, "packages/render/");
	}

	std::filesystem::path EditorProjectBootstrap::GetEditorFontPath()
	{
		const std::filesystem::path packagesDirectory = GetPackagesDirectory();
		if (packagesDirectory.empty())
			return {};

		return packagesDirectory / "editor" / "fonts" / "Poppins" / "Poppins-Regular.ttf";
	}

	std::filesystem::path EditorProjectBootstrap::GetEditorIconFontPath()
	{
		const std::filesystem::path packagesDirectory = GetPackagesDirectory();
		if (packagesDirectory.empty())
			return {};

		return packagesDirectory / "editor" / "icons" / "fontello-fe918da2" / "fontello-fe918da2" / "fontello.ttf";
	}

	std::filesystem::path EditorProjectBootstrap::GetViewportDemoMeshPath()
	{
		const std::filesystem::path packagesDirectory = GetPackagesDirectory();
		if (packagesDirectory.empty())
			return {};

		return packagesDirectory / "render" / "models" / "Sphere.fbx";
	}

	std::filesystem::path EditorProjectBootstrap::GetViewportDemoShaderPath()
	{
		const std::filesystem::path packagesDirectory = GetPackagesDirectory();
		if (packagesDirectory.empty())
			return {};

		return packagesDirectory / "render" / "shaders" / "DemoMesh.slang";
	}

}
