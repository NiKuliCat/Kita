#pragma once
#ifdef PLATFORM_WINDOWS
extern Kita::Application* Kita::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Kita::Log::Init();
	KITA_CORE_INFO(" Core Log system Init");
	KITA_INFO(" Client Log system Init");

	Kita::Application* app = Kita::CreateApplication(argc, argv);
	app->Run();
	delete app;
	return 0;
}
#endif