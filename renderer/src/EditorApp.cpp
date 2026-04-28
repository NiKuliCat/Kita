#include "renderer_pch.h"
#include <Engine.h>
#include <core/EntryPoint.h>

#include "EditorLayer.h"         
#include "file/Project.h"
namespace Kita {

	const std::string applicationName = "Kita Engine";
	const uint32_t   defaultWidth = 1920;
	const uint32_t   defaultHeight = 1080;


	static bool HasProjectExtension(const std::filesystem::path& path)
	{
		return path.has_extension() && path.extension() == ".kitaproj";
	}

	static std::optional<std::filesystem::path> ParseProjectPathFromArgs(int argc, char** argv)
	{
		for (int i = 1; i < argc; ++i)
		{
			std::string_view arg = argv[i];

			if (arg == "--project")
			{
				if (i + 1 >= argc)
				{
					KITA_CORE_ERROR("Missing value after --project");
					return std::nullopt;
				}

				return std::filesystem::path(argv[i + 1]);
			}

			std::filesystem::path possiblePath = argv[i];
			if (HasProjectExtension(possiblePath))
				return possiblePath;
		}

		return std::nullopt;
	}


	class EditorApp : public Application
	{
	public:
		EditorApp(const ApplicationDescriptor& createApplication)
			:Application(createApplication) {

			PushLayer(new EditorLayer());
		}
		~EditorApp() {}
	};

	Application* CreateApplication(int argc, char** argv)
	{
		ApplicationDescriptor createDescription;
		createDescription.name = "Kita Engine";
		createDescription.width = 1920;
		createDescription.height = 1080;

		const std::optional<std::filesystem::path> projectArg = ParseProjectPathFromArgs(argc, argv);
		if (projectArg.has_value())
		{
			if (!Project::Load(projectArg.value()))
			{
				KITA_CORE_ERROR("Failed to load project: {0}", projectArg->string());
				return nullptr;
			}

			KITA_CORE_INFO("Project loaded successfully.");
			std::filesystem::current_path(Project::GetActive()->GetProjectDirectory());

			createDescription.name = Project::GetActive()->GetName() + " - " + applicationName;
		}
		else
		{
			KITA_CORE_INFO("No project file specified. Starting editor without project context.");
		}

		return new EditorApp(createDescription);
	}

}