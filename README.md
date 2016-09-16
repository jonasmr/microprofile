# microprofile

Microprofile is a embeddable profiler in a few files, written in C++
Microprofile is mirrored on 
* github: https://github.com/jonasmr/microprofile.git
* bitbucket: https://bitbucket.org/jonasmeyer/microprofile-git.git

# Integration

[code]
#include "microprofile.h"
{
	MICROPROFILE_SCOPEI("group","timername", MP_YELLOW);
	... code to be timed
}
[/code]

And once each frame you should call
[code]
MicroProfileFlip(nullptr);
[/code]

The implementation of microprofile is in microprofile.cpp, which you must include in your project.

The nullpointer is the void* device context passed for gpu timers.

MicroProfile can be used without calling MicroProfileFlip, in which case you should call MicroProfileDumpFileImmediately, to save the profile data as .html/.csv file on disk.

# Gpu Timing
Gpu timers work like normal timers, except they are all put in a shared group, "GPU". To use them insert MICROPROFILE_SCOPEGPUI().
Gpu timers also supports multithreaded renderers - Please refer to demo_nouid3d12 for an example of how to use this.

# Counters

MicroProfile has support for tracking various counters, that will then be shown in the live & capture views. 

[code]
	MICROPROFILE_COUNTER_ADD("memory/main", 1000);
	MICROPROFILE_COUNTER_SET("fisk/geder/", 42);
[/code]

# Live View

To start microprofile in live mode, simply point chrome to host:1338/
The live view can be used to enable different microprofile groups, using the Control menu. If no groups are enabled, no code is timed. 
Press the capture button to generate a capture. By default the last 30 frames are capture.
The capture itself is a fully contained webpage, that can be saved and shared for further reference.

There are a few ways the webserver can be invoked
* host:1338/100 (capture 100 frames)
* host:1338/p/foo (start live view, load preset foo)
* host:1338/b/foo (start live view, load builtin preset foo)

Presets can be loaded and saved directly from the webbrowser. They are saved by the application in a file named mppresets.cfg.
builtin presets are presets, that are loaded from mppresets.builtin.cfg. They work like presets, except they only be loaded. This is intended to be used to do predefined, non-overwriteable configurations.

![Alt text](images/live.png?raw=true "Live screenshot")

#Capture view
Capturing in live view provides a complete view of what is recorded in the ring buffers over a set amount of frames. Please be aware if you capture to far behind, the markers you're are looking for might be overwritten, in which case there will be no markers. As the buffers are reused per thread, this is something that will happen on a per thread basis.


![Alt text](images/detailed.png?raw=true "Capture screenshot")

#Example code
* noframes: Using microprofile to profile an application without any concept of frames. dumps a capture at the end. No live view.
* noui: frame based implementation. with live view. No gpu timing
* noui_d3d11: frame based implementation, with live view and D3D11 gpu timers
* noui_d3d12: frame based implementation, with live view and D3D12 gpu timers
* noui_d3d12_multithreading: frame based implementation, with live view and D3D12 gpu timers, with gpu timings generated from multiple threads.
* ui: inengine ui, deprecated, no longer supported.


# Dependencies
Microprofile supports generating compressed captures using miniz(http:/code.google.com/miniz). 

# Resource usage
MicroProfile uses a few large allocations.
	* One global instance of struct MicroProfile
	* One Large Allocation per thread that its tracking (MICROPROFILE_PER_THREAD_BUFFER_SIZE, default 2mb)
	* One Large Allocation per gpu buffer used (MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE, default 128k)
	* one thread for sending
	* one thread for handling context switches, when enabled
	* 128kb for context switch data, when context switch tracing is enabled.
	* two small variable size buffers. should not be more than a few kb.
		** A state buffer for receiving websocket packets
		** A state buffer for loading/saving settings.
	* one small allocation for receiving packets from the sender

 To change how microprofile allocates memory, define these macros when compiling microprofile implementation
 	* MICROPROFILE_ALLOC
 	* MICROPROFILE_REALLOC
 	* MICROPROFILE_FREE


# Known Issues
There are a few minor known issues:

	* Currently the relative placement of gpu timings vs the cpu timings tend to slide a bit, and thus the relative position of GPU timestamps and CPU timestamp are not correct.
	* Chrome has an internal limit to how big a page it will cache. if you generate a capture file that is larger than 32mb, chrome will refuse to cache it. This causes the page to redownload when you save it, which is pretty catastrophic, since it will generate a capture. To mitigate this:
	** Use miniz. This drastically reduces the size of captures, to the point where its no longer a problem
	** If you still have issues, increase chromes disk cache size.

# License
Licensed using unlicense.org

# Screenshot
![Microprofile in action](https://pbs.twimg.com/media/BnvzublCEAA0Mqf.png:large)




// Howto:
// Call these functions from your code:
//  MicroProfileOnThreadCreate
//  MicroProfileMouseButton
//  MicroProfileMousePosition 
//  MicroProfileModKey
//  MicroProfileFlip  				<-- Call this once per frame
//  MicroProfileDraw  				<-- Call this once per frame
//  MicroProfileToggleDisplayMode 	<-- Bind to a key to toggle profiling
//  MicroProfileTogglePause			<-- Bind to a key to toggle pause
//
// Use these macros in your code in blocks you want to time:
//
// 	MICROPROFILE_DECLARE
// 	MICROPROFILE_DEFINE
// 	MICROPROFILE_DECLARE_GPU
// 	MICROPROFILE_DEFINE_GPU
// 	MICROPROFILE_SCOPE
// 	MICROPROFILE_SCOPEI
// 	MICROPROFILE_SCOPEGPU
// 	MICROPROFILE_SCOPEGPUI
//  MICROPROFILE_META
//
//
//	Usage:
//
//	{
//		MICROPROFILE_SCOPEI("GroupName", "TimerName", nColorRgb):
// 		..Code to be timed..
//  }
//
//	MICROPROFILE_DECLARE / MICROPROFILE_DEFINE allows defining groups in a shared place, to ensure sorting of the timers
//
//  (in global scope)
//  MICROPROFILE_DEFINE(g_ProfileFisk, "Fisk", "Skalle", nSomeColorRgb);
//
//  (in some other file)
//  MICROPROFILE_DECLARE(g_ProfileFisk);
//
//  void foo(){
//  	MICROPROFILE_SCOPE(g_ProfileFisk);
//  }
//
//  Once code is instrumented the gui is activeted by calling MicroProfileToggleDisplayMode or by clicking in the upper left corner of
//  the screen
//
// The following functions must be implemented before the profiler is usable
//  debug render:
// 		void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters);
// 		void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType = MicroProfileBoxTypeFlat);
// 		void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor);
//  Gpu time stamps: (See below for d3d/opengl helper)
// 		uint32_t MicroProfileGpuInsertTimeStamp(void*);
// 		uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
// 		uint64_t MicroProfileTicksPerSecondGpu();
//  threading:
//      const char* MicroProfileGetThreadName(char* pzName); Threadnames in detailed view
//
// Default implementations of Gpu timestamp functions:
// 		Opengl:
// 			in .c file where MICROPROFILE_IMPL is defined:
//   		#define MICROPROFILE_GPU_TIMERS_GL
// 			call MicroProfileGpuInitGL() on startup
//		D3D11:
// 			in .c file where MICROPROFILE_IMPL is defined:
//   		#define MICROPROFILE_GPU_TIMERS_D3D11
// 			call MICROPROFILE_GPU_TIMERS_D3D11(). Pass Device & ImmediateContext
//
// Limitations:
//  GPU timestamps can only be inserted from one thread.
//
//

