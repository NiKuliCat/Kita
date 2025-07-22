#include "pch.h"
#include <Engine.h>
class EditorApp : public Kita::Application
{
public:
	EditorApp(const Kita::ApplicationDescriptor& createApplication)
		:Application(createApplication) {
	}
	~EditorApp() {}
};

Kita::Application* Kita::CreateApplication(int argc, char** argv)
{
	ApplicationDescriptor createDescription;
	createDescription.Name = "Kita Renderer";
	createDescription.Width = 1920;
	createDescription.Height = 1080;
	return new EditorApp(createDescription);
}