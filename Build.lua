workspace "Kita"
    architecture "x86_64"
    startproject "Renderer"
    configurations { "Debug", "Release", "Dist" }
    flags {"MultiProcessorCompile" }

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    
    IncludeDir = {}
    IncludeDir["GLFW"] = "%{wks.location}/engine/third-party/glfw/include"
    IncludeDir["glad"] = "%{wks.location}/engine/third-party/glad/include"
    IncludeDir["ImGui"] = "%{wks.location}/engine/third-party/imgui"
    IncludeDir["spdlog"] = "%{wks.location}/engine/third-party/spdlog/include"
    -- IncludeDir["glm"] = "%{wks.location}/engine/third-party/glm"


    group "Dependencies"
        include "engine/third-party/glfw"
        include "engine/third-party/glad"
        include "engine/third-party/imgui"
    group "" 


    include "engine/engine_core_build.lua"
    include "renderer/renderer_build.lua"