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
        "_CRT_SECURE_NO_WARNINGS"
    }

    includedirs
    {
        "src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.glm}"
    }
    buildoptions
    {
        "/utf-8"
    }

    links
    {
        "GLFW",
        "glad",
        "ImGui"
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

    filter "configurations:Release"
        defines "KITA_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "KITA_DIST"
        runtime "Release"
        optimize "On"
