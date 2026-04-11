local function find_first_match(patterns)
    for _, pattern in ipairs(patterns) do
        local matches = os.matchfiles(pattern)
        table.sort(matches)

        if #matches > 0 then
            return matches[1]
        end
    end

    return nil
end

local function workspace_path(file_path)
    return "%{wks.location}/" .. file_path:gsub("\\", "/")
end

local function resolve_assimp_binary(config_name)
    local bin_dir = path.join("engine", "third-party", "assimp", "bin", config_name)
    local lib_patterns
    local dll_patterns

    if config_name == "Debug" then
        lib_patterns = {
            path.join(bin_dir, "assimp*-mtd.lib"),
            path.join(bin_dir, "assimp*d.lib"),
            path.join(bin_dir, "assimp*.lib")
        }
        dll_patterns = {
            path.join(bin_dir, "assimp*-mtd.dll"),
            path.join(bin_dir, "assimp*d.dll"),
            path.join(bin_dir, "assimp*.dll")
        }
    else
        lib_patterns = {
            path.join(bin_dir, "assimp*-mt.lib"),
            path.join(bin_dir, "assimp.lib"),
            path.join(bin_dir, "assimp*.lib")
        }
        dll_patterns = {
            path.join(bin_dir, "assimp*-mt.dll"),
            path.join(bin_dir, "assimp.dll"),
            path.join(bin_dir, "assimp*.dll")
        }
    end

    return {
        lib = find_first_match(lib_patterns),
        dll = find_first_match(dll_patterns)
    }
end

AssimpBinary = {
    Debug = resolve_assimp_binary("Debug"),
    Release = resolve_assimp_binary("Release")
}

function configure_assimp(config_name, should_link)
    local assimp = AssimpBinary[config_name]

    if assimp and assimp.lib then
        defines { "KITA_HAS_ASSIMP=1" }

        if should_link then
            links { workspace_path(assimp.lib) }

            if assimp.dll then
                postbuildcommands
                {
                    string.format('{COPY} "%s" "%%{cfg.targetdir}" ', workspace_path(assimp.dll))
                }
            end
        end
    else
        defines { "KITA_HAS_ASSIMP=0" }
    end
end

workspace "Kita"
    architecture "x86_64"
    startproject "Renderer"
    configurations { "Debug", "Release", "Dist" }
    multiprocessorcompile "On"

    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

    
    IncludeDir = {}
    IncludeDir["GLFW"] = "%{wks.location}/engine/third-party/glfw/include"
    IncludeDir["glad"] = "%{wks.location}/engine/third-party/glad/include"
    IncludeDir["ImGui"] = "%{wks.location}/engine/third-party/imgui"
    IncludeDir["spdlog"] = "%{wks.location}/engine/third-party/spdlog/include"
    IncludeDir["glm"] = "%{wks.location}/engine/third-party/glm"
    IncludeDir["assimp"] = "%{wks.location}/engine/third-party/assimp/include"



    group "Dependencies"
        include "engine/third-party/glfw"
        include "engine/third-party/glad"
        include "engine/third-party/imgui"
    group "" 


    include "engine/engine_core_build.lua"
    include "renderer/renderer_build.lua"
