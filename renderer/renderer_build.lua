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
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.assimp}"
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
      
       links
        {
            "%{wks.location}/engine/third-party/assimp/bin/Debug/assimp-vc143-mtd.lib"
        }
        postbuildcommands
        {
            '{COPY} "%{wks.location}/engine/third-party/assimp/bin/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}" '
        }

   filter "configurations:Release"
      defines { "KITA_RELEASE" }
      runtime "Release"
      optimize "On"
      symbols "On"

        links
        {
            "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.lib"
        }
        postbuildcommands
        {
            '{COPY} "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}" '
        }

   filter "configurations:Dist"
      defines { "KITA_DIST" }
      runtime "Release"
      optimize "On"
      symbols "Off"

        links
        {
            "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.lib"
        }
        postbuildcommands
        {
            '{COPY} "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}" '
        }