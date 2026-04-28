#pragma once
#include <Engine.h>

namespace Kita {

	struct ProjectDescriptor
	{
		std::string name;
		uint32_t version = 1;
		std::filesystem::path projectFilePath;
		std::filesystem::path projectRoot;
		std::filesystem::path contentRoot;
	};

	class Project
	{
	public:
		static bool Load(const std::filesystem::path& projectFilePath);
		static void Unload();
		static Ref<Project> GetActive();



		const ProjectDescriptor& GetDescriptor() const { return m_Descriptor; }

		const std::string& GetName() const { return m_Descriptor.name; }
		uint32_t GetVersion() const { return m_Descriptor.version; }

		const std::filesystem::path& GetProjectFilePath() const { return m_Descriptor.projectFilePath; }
		const std::filesystem::path& GetProjectDirectory() const { return m_Descriptor.projectRoot; }
		const std::filesystem::path& GetContentDirectory() const { return m_Descriptor.contentRoot; }

		std::filesystem::path GetContentPath(const std::filesystem::path& relativePath) const{ return m_Descriptor.contentRoot / relativePath; }

	private:
		explicit Project(ProjectDescriptor descriptor)
			: m_Descriptor(std::move(descriptor))
		{
		}

		static bool DeserializeProjectFile(const std::filesystem::path& projectFilePath, ProjectDescriptor& outDescriptor);

	private:
		ProjectDescriptor m_Descriptor;

	private:
		static Ref<Project> s_ActiveProject;
	};
}