#include "RendererPch.h"
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
	createDescription.name = "Kita Renderer";
	createDescription.width = 1280;
	createDescription.height = 720;
	return new EditorApp(createDescription);
}