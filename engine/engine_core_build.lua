project "Engine"
    kind "StaticLib"
    language "c++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" ..outputdir.. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" ..outputdir.. "/%{prj.name}")

    pchheader "Kitapch.h"
    pchsource "src/Kitapch.cpp"

    files
    {
        "src/**.h", 
        "src/**.cpp",
        "third-party/glm/glm/**.hpp",
        "third-party/glm/glm/**.inl"
    }
    

    includedirs
    {
        "src",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.glm}"
    }

    links
    {
        "GLFW",
        "ImGui",
        "Glad"
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
