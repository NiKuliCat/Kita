#pragma once
#ifdef PLATFORM_WINDOWS
extern Kita::Application* Kita::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Kita::Application* app = Kita::CreateApplication(argc, argv);
	app->Run();
	delete app;
	return 0;
}
#endif