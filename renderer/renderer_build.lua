project "Renderer"
    kind "ConsoleApp"
    language "c++"
    cppdialect "C++17"
    staticruntime "off"


    targetdir ("%{wks.location}/bin/" ..outputdir.. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" ..outputdir.. "/%{prj.name}")


    pchheader "renderer_pch.h"
    pchsource "src/renderer_pch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/engine/src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.spdlog}"
    }
    buildoptions
    {
        "/utf-8"
    }

    links
    {
        "Engine"
    }


    filter "system:windows"
      systemversion "latest"
      defines { "PLATFORM_WINDOWS" }

   filter "configurations:Debug"
      defines { "KITA_DEBUG" }
      runtime "Debug"
      symbols "On"

   filter "configurations:Release"
      defines { "KITA_RELEASE" }
      runtime "Release"
      optimize "On"
      symbols "On"

   filter "configurations:Dist"
      defines { "KITA_DIST" }
      runtime "Release"
      optimize "On"
      symbols "Off"