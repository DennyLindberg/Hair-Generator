binaries_folder = "binaries/"
includes_folder = "include/"
source_folder   = "source/"                         
libs_folder     = "libs/"

workspace "Hair Generator"
    location("temp/") -- temporary files (sln, proj, obj, pdb, ilk, etc)
    language "C++"

    configurations { "Debug", "Release" }

    cppdialect "C++17"
    systemversion("latest")
    system      "windows"
    platforms { "win64" }
    defines   { "OS_WINDOWS" }        

    filter { "platforms:*64"} architecture "x64"

    entrypoint "mainCRTStartup"     -- force Windows-executables to use main instead of WinMain as entry point   
    symbolspath '$(TargetName).pdb'
    staticruntime "on"

    debugdir(binaries_folder)
    includedirs { includes_folder }
    libdirs     { libs_folder }
    links       { "opengl32", "SDL2" }
    flags       { "MultiProcessorCompile" }

    filter { "configurations:Debug" }
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"
        
    filter { "configurations:Release" }
        defines { "NDEBUG" }
        symbols "Off"        
        optimize "On"
        
    filter{}


project "Main Scene"
    kind "ConsoleApp"
    targetdir(binaries_folder)
    targetname("main")
    files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
    removefiles{ source_folder .. "main*.cpp"}
    files ({source_folder .. "main.cpp"})
    
project "Texture Only"
    kind "ConsoleApp"
    targetdir(binaries_folder)
    targetname("main_2d")
    files ({source_folder .. "**.h", source_folder .. "**.c", source_folder .. "**.cpp"})
    removefiles{ source_folder .. "main*.cpp"}
    files ({source_folder .. "main_2d.cpp"})
    