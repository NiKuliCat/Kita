#include "renderer_pch.h"
#include "Project.h"

namespace Kita {
	Ref<Project> Project::s_ActiveProject = nullptr;


	static bool TryReadVersion(const json& value, uint32_t& outVersion)
	{
		if (value.is_number_unsigned())
		{
			outVersion = value.get<uint32_t>();
			return true;
		}

		if (value.is_number_integer())
		{
			const int version = value.get<int>();
			if (version < 0)
				return false;

			outVersion = static_cast<uint32_t>(version);
			return true;
		}

		if (value.is_string())
		{
			outVersion = static_cast<uint32_t>(std::stoul(value.get<std::string>()));
			return true;
		}

		return false;
	}

	bool Project::Load(const std::filesystem::path& projectFilePath)
	{
		ProjectDescriptor desc;
		if (!DeserializeProjectFile(projectFilePath, desc))
		{
			return false;
		}

		s_ActiveProject = Ref<Project>(new Project(std::move(desc)));

		KITA_CORE_INFO("Active project loaded: {0}", s_ActiveProject->GetName());
		KITA_CORE_INFO("Project root: {0}", s_ActiveProject->GetProjectDirectory().string());
		KITA_CORE_INFO("Content root: {0}", s_ActiveProject->GetContentDirectory().string());

		return true;
	}
	void Project::Unload()
	{
		s_ActiveProject.reset();
	}
	Ref<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}
	bool Project::DeserializeProjectFile(const std::filesystem::path& projectFilePath, ProjectDescriptor& outDescriptor)
	{
		std::ifstream in(projectFilePath);

		if (!in.is_open())
		{
			KITA_CORE_ERROR("Failed to open project file: {0}", projectFilePath.string());
			return false;
		}

		json root;
		in >> root;

		if (!root.is_object())
		{
			KITA_CORE_ERROR("Project file root must be a json object: {0}", projectFilePath.string());
			return false;
		}


		if (!root.contains("name") || !root.at("name").is_string())
		{
			KITA_CORE_ERROR("Project file missing valid 'name': {0}", projectFilePath.string());
			return false;
		}

		if (!root.contains("version"))
		{
			KITA_CORE_ERROR("Project file missing 'version': {0}", projectFilePath.string());
			return false;
		}

		if (!root.contains("contentRoot") || !root.at("contentRoot").is_string())
		{
			KITA_CORE_ERROR("Project file missing valid 'contentRoot': {0}", projectFilePath.string());
			return false;
		}

		uint32_t version = 1;
		if (!TryReadVersion(root.at("version"), version))
		{
			KITA_CORE_ERROR("Project file has invalid 'version': {0}", projectFilePath.string());
			return false;
		}

		const std::filesystem::path canonicalProjectFile = std::filesystem::weakly_canonical(projectFilePath);
		const std::filesystem::path projectRoot = canonicalProjectFile.parent_path();
		const std::filesystem::path contentRoot = projectRoot / root.at("contentRoot").get<std::string>();

		outDescriptor.name = root.at("name").get<std::string>();
		outDescriptor.version = version;
		outDescriptor.projectFilePath = canonicalProjectFile;
		outDescriptor.projectRoot = projectRoot;
		outDescriptor.contentRoot = contentRoot;
		return true;
	}
}