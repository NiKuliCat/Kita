project "Engine"
    kind "StaticLib"
    cppdialect "C++17"
    staticruntime "off"


    targetdir ("%{wks.location}/bin/" ..outputdir.. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" ..outputdir.. "/%{prj.name}")


    pchheader "kita_pch.h"
    pchsource "src/kita_pch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
        "third-party/glm/glm/**.hpp",
         "third-party/glm/glm/**.inl"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE"
    }

    includedirs
    {
        "src",
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
        "GLFW",
        "glad",
        "ImGui",
        "opengl32.lib"
    }


    filter "system:windows"
        systemversion "latest"
        defines
        {
            "PLATFORM_WINDOWS"
        }

    filter "configurations:Debug"
        defines "KITA_DEBUG"
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
        defines "KITA_RELEASE"
        runtime "Release"
        optimize "On"

        links
        {
            "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.lib"
        }
        postbuildcommands
        {
            '{COPY} "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}" '
        }

    filter "configurations:Dist"
        defines "KITA_DIST"
        runtime "Release"
        optimize "On"

        links
        {
            "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.lib"
        }
        postbuildcommands
        {
            '{COPY} "%{wks.location}/engine/third-party/assimp/bin/Release/assimp-vc143-mtd.dll" "%{cfg.targetdir}" '
        }
