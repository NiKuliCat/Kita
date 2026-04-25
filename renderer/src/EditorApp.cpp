#include "renderer_pch.h"
#include <Engine.h>
#include <core/EntryPoint.h>
#include "EditorLayer.h"

namespace Kita {

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
		return new EditorApp(createDescription);
	}

}