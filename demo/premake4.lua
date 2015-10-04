solution "microprofile"
   configurations { "Debug", "Release" }
   location "build/" 
   -- A project defines one build target
   project "ui"
      kind "WindowedApp"
      language "C++"
      files { "ui/*.h", "ui/*.cpp", "glew/*.c", "../microprofile.h", "../microprofileui.h", "../microprofile_html.h", "../src/microprofile.html"}
      includedirs {"sdl2/include/", "glew/", ".." }       
      defines {"GLEW_STATIC;_CRT_SECURE_NO_WARNINGS"}       
      libdirs {"sdl2/VisualC/SDL/Win32/Release/"}
      links {"SDL2"}
      debugdir "."
      defines {"MICROPROFILE_UI=1", "MICROPROFILE_WEBSERVER=1"}
      --remove comment below to auto generate microprofile_html.h. requires embed (in src/) to be built
      --prebuildcommands { "../embed.bat" }

      configuration "windows"
         links { "opengl32", "glu32", "winmm", "dxguid", "Ws2_32"}
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols", "StaticRuntime" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols", "StaticRuntime" }
   
	project "noui"
		kind "ConsoleApp"
   		language "C++"
   		defines {"_CRT_SECURE_NO_WARNINGS"}
   		defines {"MICROPROFILE_GPU_TIMERS=0;MICROPROFILE_WEBSERVER=1"}
   		includedirs {".." }
   		links{"winmm", "Ws2_32"}       
   		files { "noui/*.h", "noui/*.cpp", "../microprofile.h", "../microprofile_html.h", "../src/microprofile.html"}
   		debugdir "."
   		--remove comment below to auto generate microprofile_html.h. requires embed (in src/) to be built
		   --prebuildcommands { "../embed.bat" }

      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols", "StaticRuntime" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols", "StaticRuntime" }

   project "noui_d3d11"
      kind "WindowedApp"
      language "C++"
      files { "noui_d3d11/*.h", "noui_d3d11/*.cpp", "../microprofile.h", "../microprofile_html.h", "../src/microprofile.html"}
      includedirs {".." }       
      defines {"MICROPROFILE_GPU_TIMERS_D3D11=1;_CRT_SECURE_NO_WARNINGS"}       
      libdirs {}
      links {}
      debugdir "."
      defines {}
      --remove comment below to auto generate microprofile_html.h. requires embed (in src/) to be built
      --prebuildcommands { "../embed.bat" }
      postbuildcommands { "copy ..\\noui_d3d11\\Tutorial02*.* .." }

      configuration "windows"
         links {"dxguid", "Ws2_32", "d3d11","d3dcompiler", "winmm"}
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }


   project "noui_d3d12"
      kind "WindowedApp"
      language "C++"
      files { "noui_d3d12/*.h", "noui_d3d12/*.cpp", "../microprofile.h", "../microprofile_html.h", "../src/microprofile.html"}
      includedirs {".." }       
      defines {"_CRT_SECURE_NO_WARNINGS"}       
      libdirs {}
      links {}
      debugdir "."
      defines {}
      --remove comment below to auto generate microprofile_html.h. requires embed (in src/) to be built
      --prebuildcommands { "../embed.bat" }
      --postbuildcommands { "copy ..\\noui_d3d11\\Tutorial02*.* .." }

      configuration "windows"
         links {"dxguid", "Ws2_32", "d3d12","d3dcompiler","dxgi", "winmm"}
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
