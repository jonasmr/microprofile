solution "microprofile"
   configurations { "Debug", "Release" }
   location "build/" 
   -- A project defines one build target
   project "microprofile"
      kind "WindowedApp"
      language "C++"
      files { "*.h", "*.cpp", "glew/*.c"}
      includedirs {"sdl2/include/", "glew/", ".." } 
      
      defines {"GLEW_STATIC;_CRT_SECURE_NO_WARNINGS"} 
      
      libdirs {"sdl2/VisualC/SDL/Win32/Release/"}

      links {"SDL2"}

      debugdir "."

      configuration "windows"
         links { "opengl32", "glu32", "winmm", "dxguid"}

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols", "StaticRuntime" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols", "StaticRuntime" }