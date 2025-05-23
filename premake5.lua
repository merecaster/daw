workspace "daw"
    configurations { "debug", "release" }
    filter "system:macosx"
        architecture "arm64"
    filter "not system:macosx"
        architecture "x86_64"
    language "C"

project "daw"
    kind "ConsoleApp"
    language "C"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "bin/obj/%{cfg.buildcfg}"
    files { 
        "src/main.c",
    }
    filter "system:macosx"
        files {
            "src/platform_macos.m"
        }
        links {
            "AudioToolbox.framework",
            "CoreAudio.framework",
            "Cocoa.framework",
            "QuartzCore.framework",
        }
    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "On"
