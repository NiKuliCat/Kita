#pragma once

#include <Engine.h>

namespace Kita {

	class EditorProjectBootstrap
	{
	public:
		static void Initialize();

		static std::filesystem::path GetEditorFontPath();
		static std::filesystem::path GetEditorIconFontPath();
		static std::filesystem::path GetViewportDemoMeshPath();
		static std::filesystem::path GetViewportDemoShaderPath();
	};

}
