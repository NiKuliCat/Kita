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
        "src/**.cpp"
    }



    includedirs
    {
        "src",
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
        defines "PTA_DEBUG"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines "PTA_RELEASE"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines "PTA_DIST"
        runtime "Release"
        optimize "On"
