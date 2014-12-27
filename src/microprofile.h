#pragma once
// This is free and unencumbered software released into the public domain.
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// For more information, please refer to <http://unlicense.org/>
//
// ***********************************************************************
//
//
//
//
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
//  Gpu time stamps:
// 		uint32_t MicroProfileGpuInsertTimeStamp();
// 		uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
// 		uint64_t MicroProfileTicksPerSecondGpu();
//  threading:
//      const char* MicroProfileGetThreadName(); Threadnames in detailed view


#ifndef MICROPROFILE_ENABLED
#define MICROPROFILE_ENABLED 1
#endif


#if 0 == MICROPROFILE_ENABLED

#define MICROPROFILE_DECLARE(var)
#define MICROPROFILE_DEFINE(var, group, name, color)
#define MICROPROFILE_DECLARE_GPU(var)
#define MICROPROFILE_DEFINE_GPU(var, group, name, color)
#define MICROPROFILE_SCOPE(var) do{}while(0)
#define MICROPROFILE_SCOPEI(group, name, color) do{}while(0)
#define MICROPROFILE_SCOPEGPU(var) do{}while(0)
#define MICROPROFILE_SCOPEGPUI(group, name, color) do{}while(0)
#define MICROPROFILE_META_CPU(name, count)
#define MICROPROFILE_META_GPU(name, count)
#define MICROPROFILE_FORCEENABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEENABLEGPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLEGPUGROUP(s) do{} while(0)

#define MicroProfileGetTime(group, name) 0.f
#define MicroProfileOnThreadCreate(foo) do{}while(0)
#define MicroProfileFlip() do{}while(0)
#define MicroProfileSetAggregateFrames(a) do{}while(0)
#define MicroProfileGetAggregateFrames() 0
#define MicroProfileGetCurrentAggregateFrames() 0
#define MicroProfileTogglePause() do{}while(0)
#define MicroProfileToggleAllGroups() do{} while(0)
#define MicroProfileDumpTimers() do{}while(0)
#define MicroProfileShutdown() do{}while(0)
#define MicroProfileSetForceEnable(a) do{} while(0)
#define MicroProfileGetForceEnable() false
#define MicroProfileSetEnableAllGroups(a) do{} while(0)
#define MicroProfileGetEnableAllGroups() false
#define MicroProfileSetForceMetaCounters(a)
#define MicroProfileGetForceMetaCounters() 0
#define MicroProfileDumpHtml(c) do{} while(0)

#else

#include <stdint.h>
#include <string.h>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#define MICROPROFILE_IOS
#endif

#define MP_TICK() mach_absolute_time()
inline int64_t MicroProfileTicksPerSecondCpu()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		mach_timebase_info_data_t sTimebaseInfo;	
		mach_timebase_info(&sTimebaseInfo);
		nTicksPerSecond = 1000000000ll * sTimebaseInfo.denom / sTimebaseInfo.numer;
	}
	return nTicksPerSecond;
}

#define MP_BREAK() __builtin_trap()
#define MP_THREAD_LOCAL __thread
#define MP_STRCASECMP strcasecmp
#define MP_GETCURRENTTHREADID() (uint64_t)pthread_self()
typedef uint64_t ThreadIdType;

#elif defined(_WIN32)
int64_t MicroProfileTicksPerSecondCpu();
int64_t MicroProfileGetTick();
#define MP_TICK() MicroProfileGetTick()
#define MP_BREAK() __debugbreak()
#define MP_THREAD_LOCAL __declspec(thread)
#define MP_STRCASECMP _stricmp
#define MP_GETCURRENTTHREADID() GetCurrentThreadId()
typedef uint32_t ThreadIdType;

#elif defined(__linux__)
#include <unistd.h>
#include <time.h>
inline int64_t MicroProfileTicksPerSecondCpu()
{
	return 1000000000ll;
}

inline int64_t MicroProfileGetTick()
{
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return 1000000000ll * ts.tv_sec + ts.tv_nsec;
}
#define MP_TICK() MicroProfileGetTick()
#define MP_BREAK() __builtin_trap()
#define MP_THREAD_LOCAL __thread
#define MP_STRCASECMP strcasecmp
#define MP_GETCURRENTTHREADID() (uint64_t)pthread_self()
typedef uint64_t ThreadIdType;
#endif

#ifndef MP_GETCURRENTTHREADID 
#define MP_GETCURRENTTHREADID() 0
typedef uint32_t ThreadIdType;
#endif

#ifndef MICROPROFILE_API
#define MICROPROFILE_API
#endif

#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)
#define MICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu)
#define MICROPROFILE_DECLARE_GPU(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE_GPU(var, group, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeGpu)
#define MICROPROFILE_TOKEN_PASTE0(a, b) a ## b
#define MICROPROFILE_TOKEN_PASTE(a, b)  MICROPROFILE_TOKEN_PASTE0(a,b)
#define MICROPROFILE_SCOPE(var) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPE_TOKEN(token) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token)
#define MICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu); MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_SCOPEGPU(var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEGPUI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_META_CPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeCpu)
#define MICROPROFILE_META_GPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeGpu)


#ifndef MICROPROFILE_USE_THREAD_NAME_CALLBACK
#define MICROPROFILE_USE_THREAD_NAME_CALLBACK 0
#endif

#ifndef MICROPROFILE_DRAWCURSOR
#define MICROPROFILE_DRAWCURSOR 0
#endif

#ifndef MICROPROFILE_DETAILED_BAR_NAMES
#define MICROPROFILE_DETAILED_BAR_NAMES 1
#endif

#ifndef MICROPROFILE_GPU_FRAME_DELAY
#define MICROPROFILE_GPU_FRAME_DELAY 3 //must be > 0
#endif

#ifndef MICROPROFILE_PER_THREAD_BUFFER_SIZE
#define MICROPROFILE_PER_THREAD_BUFFER_SIZE (2048<<10)
#endif

#ifndef MICROPROFILE_MAX_FRAME_HISTORY
#define MICROPROFILE_MAX_FRAME_HISTORY 512
#endif

#ifndef MICROPROFILE_PRINTF
#define MICROPROFILE_PRINTF printf
#endif

#ifndef MICROPROFILE_META_MAX
#define MICROPROFILE_META_MAX 8
#endif

#ifndef MICROPROFILE_WEBSERVER_PORT
#define MICROPROFILE_WEBSERVER_PORT 1338
#endif

#ifndef MICROPROFILE_WEBSERVER
#define MICROPROFILE_WEBSERVER 1
#endif

#ifndef MICROPROFILE_WEBSERVER_MAXFRAMES
#define MICROPROFILE_WEBSERVER_MAXFRAMES 30
#endif

#ifndef MICROPROFILE_GPU_TIMERS
#define MICROPROFILE_GPU_TIMERS 1
#endif

#ifndef MICROPROFILE_NAME_MAX_LEN
#define MICROPROFILE_NAME_MAX_LEN 64
#endif

#define MICROPROFILE_FORCEENABLECPUGROUP(s) MicroProfileForceEnableGroup(s, MicroProfileTokenTypeCpu)
#define MICROPROFILE_FORCEDISABLECPUGROUP(s) MicroProfileForceDisableGroup(s, MicroProfileTokenTypeCpu)
#define MICROPROFILE_FORCEENABLEGPUGROUP(s) MicroProfileForceEnableGroup(s, MicroProfileTokenTypeGpu)
#define MICROPROFILE_FORCEDISABLEGPUGROUP(s) MicroProfileForceDisableGroup(s, MicroProfileTokenTypeGpu)

#define MICROPROFILE_INVALID_TICK ((uint64_t)-1)
#define MICROPROFILE_GROUP_MASK_ALL 0xffffffffffff


typedef uint64_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;

#define MICROPROFILE_INVALID_TOKEN (uint64_t)-1

enum MicroProfileTokenType
{
	MicroProfileTokenTypeCpu,
	MicroProfileTokenTypeGpu,
};
enum MicroProfileBoxType
{
	MicroProfileBoxTypeBar,
	MicroProfileBoxTypeFlat,
};

struct MicroProfileState
{
	uint32_t nDisplay;
	uint32_t nMenuAllGroups;
	uint64_t nMenuActiveGroup;
	uint32_t nMenuAllThreads;
	uint32_t nAggregateFlip;
	uint32_t nBars;
	float fReferenceTime;
};


MICROPROFILE_API void MicroProfileInit();
MICROPROFILE_API void MicroProfileShutdown();
MICROPROFILE_API MicroProfileToken MicroProfileFindToken(const char* sGroup, const char* sName);
MICROPROFILE_API MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token = MicroProfileTokenTypeCpu);
MICROPROFILE_API MicroProfileToken MicroProfileGetMetaToken(const char* pName);
MICROPROFILE_API void MicroProfileMetaUpdate(MicroProfileToken, int nCount, MicroProfileTokenType eTokenType);
MICROPROFILE_API uint64_t MicroProfileEnter(MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileLeave(MicroProfileToken nToken, uint64_t nTick);
MICROPROFILE_API uint64_t MicroProfileGpuEnter(MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileGpuLeave(MicroProfileToken nToken, uint64_t nTick);
inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t){ return (t&0xffff); }
inline uint64_t MicroProfileGetGroupMask(MicroProfileToken t){ return ((t>>16)&MICROPROFILE_GROUP_MASK_ALL);}
inline MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint16_t nTimer){ return (nGroupMask<<16) | nTimer;}

MICROPROFILE_API void MicroProfileFlip(); //! called once per frame.
MICROPROFILE_API void MicroProfileTogglePause();
MICROPROFILE_API void MicroProfileGetState(MicroProfileState* pStateOut);
MICROPROFILE_API void MicroProfileSetState(MicroProfileState* pStateIn);
MICROPROFILE_API void MicroProfileForceEnableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API void MicroProfileForceDisableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API float MicroProfileGetTime(const char* pGroup, const char* pName);
MICROPROFILE_API void MicroProfileOnThreadCreate(const char* pThreadName); //should be called from newly created threads
MICROPROFILE_API void MicroProfileOnThreadExit(); //call on exit to reuse log
MICROPROFILE_API void MicroProfileInitThreadLog();
MICROPROFILE_API void MicroProfileSetForceEnable(bool bForceEnable);
MICROPROFILE_API bool MicroProfileGetForceEnable();
MICROPROFILE_API void MicroProfileSetEnableAllGroups(bool bEnable); 
MICROPROFILE_API bool MicroProfileGetEnableAllGroups();
MICROPROFILE_API void MicroProfileSetForceMetaCounters(bool bEnable); 
MICROPROFILE_API bool MicroProfileGetForceMetaCounters();
MICROPROFILE_API void MicroProfileSetAggregateFrames(int frames);
MICROPROFILE_API int MicroProfileGetAggregateFrames();
MICROPROFILE_API int MicroProfileGetCurrentAggregateFrames();

#if MICROPROFILE_WEBSERVER
MICROPROFILE_API void MicroProfileDumpHtml(const char* pFile);
#else
#define MicroProfileDumpHtml(c) do{} while(0)
#endif




#if MICROPROFILE_GPU_TIMERS
MICROPROFILE_API uint32_t MicroProfileGpuInsertTimeStamp();
MICROPROFILE_API uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
MICROPROFILE_API uint64_t MicroProfileTicksPerSecondGpu();
#else
#define MicroProfileGpuInsertTimeStamp() 1
#define MicroProfileGpuGetTimeStamp(a) 0
#define MicroProfileTicksPerSecondGpu() 1
#endif


#if MICROPROFILE_USE_THREAD_NAME_CALLBACK
MICROPROFILE_API const char* MicroProfileGetThreadName();
#else
#define MicroProfileGetThreadName() "<implement MicroProfileGetThreadName to get threadnames>"
#endif

struct MicroProfileScopeHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileEnter(nToken);
	}
	~MicroProfileScopeHandler()
	{
		MicroProfileLeave(nToken, nTick);
	}
};

struct MicroProfileScopeGpuHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeGpuHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileGpuEnter(nToken);
	}
	~MicroProfileScopeGpuHandler()
	{
		MicroProfileGpuLeave(nToken, nTick);
	}
};




#ifdef MICROPROFILE_IMPL

#ifndef MICROPROFILE_MAX_THREADS
#define MICROPROFILE_MAX_THREADS 32
#endif 

#ifndef MICROPROFILE_UNPACK_RED
#define MICROPROFILE_UNPACK_RED(c) ((c)>>16)
#endif

#ifndef MICROPROFILE_UNPACK_GREEN
#define MICROPROFILE_UNPACK_GREEN(c) ((c)>>8)
#endif

#ifndef MICROPROFILE_UNPACK_BLUE
#define MICROPROFILE_UNPACK_BLUE(c) ((c))
#endif


#ifdef _WIN32
#include <windows.h>
#define snprintf _snprintf

#pragma warning(push)
#pragma warning(disable: 4244)
int64_t MicroProfileTicksPerSecondCpu()
{
	static int64_t nTicksPerSecond = 0;	
	if(nTicksPerSecond == 0) 
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&nTicksPerSecond);
	}
	return nTicksPerSecond;
}
int64_t MicroProfileGetTick()
{
	int64_t ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
	return ticks;
}

#endif

#if MICROPROFILE_WEBSERVER

#ifdef _WIN32
typedef SOCKET MpSocket;
#define MP_INVALID_SOCKET(f) (f == INVALID_SOCKET)
#endif

#if defined(__APPLE__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
typedef int MpSocket;
#define MP_INVALID_SOCKET(f) (f < 0)
#endif

void MicroProfileWebServerStart();
void MicroProfileWebServerStop();
bool MicroProfileWebServerUpdate();
void MicroProfileDumpHtmlToFile();

#else

#define MicroProfileWebServerStart() do{}while(0)
#define MicroProfileWebServerStop() do{}while(0)
#define MicroProfileWebServerUpdate() false
#define MicroProfileDumpHtmlToFile() do{} while(0)

typedef int MpSocket;
#endif 


#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>


#ifndef MICROPROFILE_DEBUG
#define MICROPROFILE_DEBUG 0
#endif


#define S g_MicroProfile
#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 48 //dont bump! no. of bits used it bitmask
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_BUFFER_SIZE)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS 256
#define MICROPROFILE_STACK_MAX 32
#define MICROPROFILE_MAX_PRESETS 5
#define MICROPROFILE_TOOLTIP_MAX_STRINGS (32 + MICROPROFILE_MAX_GROUPS*2)
#define MICROPROFILE_TOOLTIP_STRING_BUFFER_SIZE (4*1024)
#define MICROPROFILE_TOOLTIP_MAX_LOCKED 3
#define MICROPROFILE_ANIM_DELAY_PRC 0.5f
#define MICROPROFILE_GAP_TIME 50 //extra ms to fetch to close timers from earlier frames

#ifndef MICROPROFILE_CONTEXT_SWITCH_TRACE 
#ifdef _WIN32
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 1
#else
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0
#endif
#endif

#if MICROPROFILE_CONTEXT_SWITCH_TRACE
#define MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE (128*1024) //2mb with 16 byte entry size
#else
#define MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE (1)
#endif

enum MicroProfileDrawMask
{
	MP_DRAW_OFF			= 0x0,
	MP_DRAW_BARS		= 0x1,
	MP_DRAW_DETAILED	= 0x2,
	MP_DRAW_HIDDEN		= 0x3,
};

enum MicroProfileDrawBarsMask
{
	MP_DRAW_TIMERS 				= 0x1,	
	MP_DRAW_AVERAGE				= 0x2,	
	MP_DRAW_MAX					= 0x4,	
	MP_DRAW_CALL_COUNT			= 0x8,
	MP_DRAW_TIMERS_EXCLUSIVE 	= 0x10,
	MP_DRAW_AVERAGE_EXCLUSIVE 	= 0x20,	
	MP_DRAW_MAX_EXCLUSIVE		= 0x40,
	MP_DRAW_META_FIRST			= 0x80,
	MP_DRAW_ALL 				= 0xffffffff,

};

struct MicroProfileTimer
{
	uint64_t nTicks;
	uint32_t nCount;
};

struct MicroProfileGroupInfo
{
	char pName[MICROPROFILE_NAME_MAX_LEN];
	uint32_t nNameLen;
	uint32_t nGroupIndex;
	uint32_t nNumTimers;
	uint32_t nMaxTimerNameLen;
	MicroProfileTokenType Type;
};

struct MicroProfileTimerInfo
{
	MicroProfileToken nToken;
	uint32_t nTimerIndex;
	uint32_t nGroupIndex;
	char pName[MICROPROFILE_NAME_MAX_LEN];
	uint32_t nNameLen;
	uint32_t nColor;
	bool bGraph;
};

struct MicroProfileGraphState
{
	int64_t nHistory[MICROPROFILE_GRAPH_HISTORY];
	MicroProfileToken nToken;
	int32_t nKey;
};

struct MicroProfileContextSwitch
{
	ThreadIdType nThreadOut;
	ThreadIdType nThreadIn;
	int64_t nCpu : 8;
	int64_t nTicks : 56;
};

#define MP_LOG_TICK_MASK  0x0000ffffffffffff
#define MP_LOG_INDEX_MASK 0x3fff000000000000
#define MP_LOG_BEGIN_MASK 0xc000000000000000
#define MP_LOG_META 0x2
#define MP_LOG_ENTER 0x1
#define MP_LOG_LEAVE 0x0

typedef uint64_t MicroProfileLogEntry;

inline int MicroProfileLogType(MicroProfileLogEntry Index)
{
	return ((MP_LOG_BEGIN_MASK & Index)>>62) & 0x3;
}

inline uint64_t MicroProfileLogTimerIndex(MicroProfileLogEntry Index)
{
	return (0x3fff&(Index>>48));
}

inline MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick)
{
	MicroProfileLogEntry Entry =  (nBegin<<62) | ((0x3fff&nToken)<<48) | (MP_LOG_TICK_MASK&nTick);
	int t = MicroProfileLogType(Entry);
	uint64_t nTimerIndex = MicroProfileLogTimerIndex(Entry);
	MP_ASSERT(t == nBegin);
	MP_ASSERT(nTimerIndex == (nToken&0x3fff));
	return Entry;

} 

inline int64_t MicroProfileLogTickDifference(MicroProfileLogEntry Start, MicroProfileLogEntry End)
{
	uint64_t nStart = Start;
	uint64_t nEnd = End;
	int64_t nDifference = ((nEnd<<16) - (nStart<<16)); 
	return nDifference >> 16;
}

inline int64_t MicroProfileLogGetTick(MicroProfileLogEntry e)
{
	return MP_LOG_TICK_MASK & e;
}

inline int64_t MicroProfileLogSetTick(MicroProfileLogEntry e, int64_t nTick)
{
	return (MP_LOG_TICK_MASK & nTick) | (e & ~MP_LOG_TICK_MASK);
}

struct MicroProfileFrameState
{
	int64_t nFrameStartCpu;
	int64_t nFrameStartGpu;
	uint32_t nLogStart[MICROPROFILE_MAX_THREADS];
};

struct MicroProfileThreadLog
{
	MicroProfileLogEntry	Log[MICROPROFILE_BUFFER_SIZE];

	std::atomic<uint32_t>	nPut;
	std::atomic<uint32_t>	nGet;
	uint32_t 				nActive;
	uint32_t 				nGpu;
	ThreadIdType			nThreadId;

	uint32_t				nStack[MICROPROFILE_STACK_MAX];
	int64_t					nChildTickStack[MICROPROFILE_STACK_MAX];
	uint32_t				nStackPos;

	enum
	{
		THREAD_MAX_LEN = 64,
	};
	char					ThreadName[64];
	int 					nFreeListNext;
};

struct MicroProfileStringArray
{
	const char* ppStrings[MICROPROFILE_TOOLTIP_MAX_STRINGS];
	char Buffer[MICROPROFILE_TOOLTIP_STRING_BUFFER_SIZE];
	char* pBufferPos;
	uint32_t nNumStrings;
};


struct 
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nAggregateClear;
	uint32_t nAggregateFlip;
	uint32_t nAggregateFlipCount;
	uint32_t nAggregateFrames;

	uint64_t nAggregateFlipTick;
	
	uint32_t nDisplay;
	uint32_t nBars;
	uint64_t nActiveGroup;
	uint32_t nActiveBars;

	uint64_t nForceGroup;
	uint32_t nForceEnable;
	uint32_t nForceMetaCounters;

	//menu/mouse over stuff
	uint64_t nMenuActiveGroup;
	uint32_t nMenuAllGroups;
	uint32_t nMenuAllThreads;
	uint64_t nHoverToken;
	int64_t  nHoverTime;
	int 	 nHoverFrame;
#if MICROPROFILE_DEBUG
	uint64_t nHoverAddressEnter;
	uint64_t nHoverAddressLeave;
#endif
	uint32_t nOverflow;

	uint64_t nGroupMask;
	uint32_t nRunning;
	uint32_t nToggleRunning;
	uint32_t nMaxGroupSize;
	uint32_t nDumpHtmlNextFrame;
	char HtmlDumpPath[512];

	int64_t nPauseTicks;

	float fReferenceTime;
	float fRcpReferenceTime;
	uint32_t nOpacityBackground;
	uint32_t nOpacityForeground;

	bool bShowSpikes;

	float fDetailedOffset; //display offset relative to start of latest displayable frame.
	float fDetailedRange; //no. of ms to display

	float fDetailedOffsetTarget;
	float fDetailedRangeTarget;

	int nOffsetY;

	uint32_t nWidth;
	uint32_t nHeight;

	uint32_t nBarWidth;
	// uint32_t nBarHeight;


	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateTimersExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimersExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	uint64_t				FrameExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];	
	uint64_t				AggregateExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMaxExclusive[MICROPROFILE_MAX_TIMERS];

	struct 
	{
		uint64_t nCounters[MICROPROFILE_MAX_TIMERS];
		const char* pName;
	} MetaCounters[MICROPROFILE_META_MAX];

	MicroProfileGraphState	Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t				nGraphPut;

	uint32_t 				nMouseX;
	uint32_t 				nMouseY;
	int						nMouseWheelDelta;
	uint32_t				nMouseDownLeft;
	uint32_t				nMouseDownRight;
	uint32_t 				nMouseLeft;
	uint32_t 				nMouseRight;
	uint32_t 				nMouseLeftMod;
	uint32_t 				nMouseRightMod;
	uint32_t				nModDown;
	uint32_t 				nActiveMenu;

	uint32_t				nThreadActive[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLog* 	Pool[MICROPROFILE_MAX_THREADS];
	uint32_t				nNumLogs;
	uint32_t 				nMemUsage;
	int 					nFreeListHead;

	uint32_t 				nFrameCurrent;
	uint32_t 				nFrameCurrentIndex;
	uint32_t 				nFramePut;
	uint64_t				nFramePutIndex;

	MicroProfileFrameState Frames[MICROPROFILE_MAX_FRAME_HISTORY];
	
	MicroProfileLogEntry* pDisplayMouseOver;


	uint64_t				nFlipTicks;
	uint64_t				nFlipAggregate;
	uint64_t				nFlipMax;
	uint64_t				nFlipAggregateDisplay;
	uint64_t				nFlipMaxDisplay;
		

	MicroProfileStringArray LockedToolTips[MICROPROFILE_TOOLTIP_MAX_LOCKED];	
	uint32_t  				nLockedToolTipColor[MICROPROFILE_TOOLTIP_MAX_LOCKED];	
	int 					LockedToolTipFront;


	int64_t					nRangeBegin;
	int64_t					nRangeEnd;
	int64_t					nRangeBeginGpu;
	int64_t					nRangeEndGpu;
	uint32_t				nRangeBeginIndex;
	uint32_t 				nRangeEndIndex;
	MicroProfileThreadLog* 	pRangeLog;
	uint32_t				nHoverColor;
	uint32_t				nHoverColorShared;


	std::thread* 				pContextSwitchThread;
	bool  						bContextSwitchRunning;
	bool						bContextSwitchStop;
	bool						bContextSwitchAllThreads;
	bool						bContextSwitchNoBars;
	uint32_t					nContextSwitchUsage;
	uint32_t					nContextSwitchLastPut;

	int64_t						nContextSwitchHoverTickIn;
	int64_t						nContextSwitchHoverTickOut;
	uint32_t					nContextSwitchHoverThread;
	uint32_t					nContextSwitchHoverThreadBefore;
	uint32_t					nContextSwitchHoverThreadAfter;
	uint8_t						nContextSwitchHoverCpu;
	uint8_t						nContextSwitchHoverCpuNext;

	uint32_t					nContextSwitchPut;	
	MicroProfileContextSwitch 	ContextSwitch[MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE];


	MpSocket 					ListenerSocket;

} g_MicroProfile;

MicroProfileThreadLog*			g_MicroProfileGpuLog = 0;
#ifdef MICROPROFILE_IOS
// iOS doesn't support __thread
static pthread_key_t g_MicroProfileThreadLogKey;
static pthread_once_t g_MicroProfileThreadLogKeyOnce = PTHREAD_ONCE_INIT;
static void MicroProfileCreateThreadLogKey()
{
	pthread_key_create(&g_MicroProfileThreadLogKey, NULL);
}
#else
MP_THREAD_LOCAL MicroProfileThreadLog* g_MicroProfileThreadLog = 0;
#endif
static bool g_bUseLock = false; /// This is used because windows does not support using mutexes under dll init(which is where global initialization is handled)
static uint32_t g_nMicroProfileBackColors[2] = {  0x474747, 0x313131 };

#define MICROPROFILE_NUM_CONTEXT_SWITCH_COLORS 16
static uint32_t g_nMicroProfileContextSwitchThreadColors[MICROPROFILE_NUM_CONTEXT_SWITCH_COLORS] = //palette generated by http://tools.medialab.sciences-po.fr/iwanthue/index.php
{
	0x63607B,
	0x755E2B,
	0x326A55,
	0x523135,
	0x904F42,
	0x87536B,
	0x346875,
	0x5E6046,
	0x35404C,
	0x224038,
	0x413D1E,
	0x5E3A26,
	0x5D6161,
	0x4C6234,
	0x7D564F,
	0x5C4352,
};
static uint32_t g_MicroProfileAggregatePresets[] = {0, 10, 20, 30, 60, 120};
static float g_MicroProfileReferenceTimePresets[] = {5.f, 10.f, 15.f,20.f, 33.33f, 66.66f, 100.f};
static uint32_t g_MicroProfileOpacityPresets[] = {0x40, 0x80, 0xc0, 0xff};
static const char* g_MicroProfilePresetNames[] = 
{
	"Default",
	"Render",
	"GPU",
	"Lighting",
	"AI",
	"Visibility",
	"Sound",
};


MICROPROFILE_DEFINE(g_MicroProfileDetailed, "MicroProfile", "Detailed View", 0x8888000);
MICROPROFILE_DEFINE(g_MicroProfileDrawGraph, "MicroProfile", "Draw Graph", 0xff44ee00);
MICROPROFILE_DEFINE(g_MicroProfileFlip, "MicroProfile", "MicroProfileFlip", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileThreadLoop, "MicroProfile", "ThreadLoop", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileClear, "MicroProfile", "Clear", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileAccumulate, "MicroProfile", "Accumulate", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileDrawBarView, "MicroProfile", "DrawBarView", 0x00dd77);
MICROPROFILE_DEFINE(g_MicroProfileDraw,"MicroProfile", "Draw", 0x737373);
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchDraw, "MicroProfile", "ContextSwitchDraw", 0x730073);
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchSearch,"MicroProfile", "ContextSwitchSearch", 0xDD7300);

void MicroProfileStartContextSwitchTrace();
void MicroProfileStopContextSwitchTrace();
bool MicroProfileIsLocalThread(uint32_t nThreadId);

inline std::recursive_mutex& MicroProfileMutex()
{
	static std::recursive_mutex Mutex;
	return Mutex;
}

template<typename T>
T MicroProfileMin(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T MicroProfileMax(T a, T b)
{ return a > b ? a : b; }



void MicroProfileStringArrayClear(MicroProfileStringArray* pArray)
{
	pArray->nNumStrings = 0;
	pArray->pBufferPos = &pArray->Buffer[0];
}

void MicroProfileStringArrayAddLiteral(MicroProfileStringArray* pArray, const char* pLiteral)
{
	MP_ASSERT(pArray->nNumStrings < MICROPROFILE_TOOLTIP_MAX_STRINGS);
	pArray->ppStrings[pArray->nNumStrings++] = pLiteral;
}

void MicroProfileStringArrayFormat(MicroProfileStringArray* pArray, const char* fmt, ...)
{
	MP_ASSERT(pArray->nNumStrings < MICROPROFILE_TOOLTIP_MAX_STRINGS);
	pArray->ppStrings[pArray->nNumStrings++] = pArray->pBufferPos;
	va_list args;
	va_start (args, fmt);
	pArray->pBufferPos += 1 + vsprintf(pArray->pBufferPos, fmt, args);
	va_end(args);
	MP_ASSERT(pArray->pBufferPos < pArray->Buffer + MICROPROFILE_TOOLTIP_STRING_BUFFER_SIZE);
}
void MicroProfileStringArrayCopy(MicroProfileStringArray* pDest, MicroProfileStringArray* pSrc)
{
	memcpy(&pDest->ppStrings[0], &pSrc->ppStrings[0], sizeof(pDest->ppStrings));
	memcpy(&pDest->Buffer[0], &pSrc->Buffer[0], sizeof(pDest->Buffer));
	for(uint32_t i = 0; i < MICROPROFILE_TOOLTIP_MAX_STRINGS; ++i)
	{
		if(i < pSrc->nNumStrings)
		{
			if(pSrc->ppStrings[i] >= &pSrc->Buffer[0] && pSrc->ppStrings[i] < &pSrc->Buffer[0] + MICROPROFILE_TOOLTIP_STRING_BUFFER_SIZE)
			{
				pDest->ppStrings[i] += &pDest->Buffer[0] - &pSrc->Buffer[0];
			}
		}
	}
	pDest->nNumStrings = pSrc->nNumStrings;
}

MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName);
void MicroProfileLoadPreset(const char* pSuffix);
void MicroProfileSavePreset(const char* pSuffix);


inline int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond)
{
	return (int64_t)(fMs*0.001f*nTicksPerSecond);
}

inline float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond)
{
	return 1000.f / nTicksPerSecond;
}

inline uint16_t MicroProfileGetGroupIndex(MicroProfileToken t)
{
	return (uint16_t)S.TimerInfo[MicroProfileGetTimerIndex(t)].nGroupIndex;
}


void MicroProfileInit()
{
	std::recursive_mutex& mutex = MicroProfileMutex();
	bool bUseLock = g_bUseLock;
	if(bUseLock)
		mutex.lock();
	static bool bOnce = true;
	if(bOnce)
	{
		S.nMemUsage += sizeof(S);
		bOnce = false;
		memset(&S, 0, sizeof(S));
		for(int i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
		{
			S.GroupInfo[i].pName[0] = '\0';
		}
		for(int i = 0; i < MICROPROFILE_MAX_TIMERS; ++i)
		{
			S.TimerInfo[i].pName[0] = '\0';
		}
		S.nGroupCount = 0;
		S.nBarWidth = 100;
		S.nAggregateFlipTick = MP_TICK();
		// S.nBarHeight = MICROPROFILE_TEXT_HEIGHT;//todo fix
		S.nActiveGroup = 0;
		S.nActiveBars = 0;
		S.nForceGroup = 0;
		S.nMenuAllGroups = 0;
		S.nMenuActiveGroup = 0;
		S.nMenuAllThreads = 1;
		S.nAggregateFlip = 0;
		S.nTotalTimers = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
		S.nBars = MP_DRAW_ALL;
		S.nRunning = 1;
		S.nWidth = 100;
		S.nHeight = 100;
		S.nActiveMenu = (uint32_t)-1;
		S.fReferenceTime = 33.33f;
		S.fRcpReferenceTime = 1.f / S.fReferenceTime;
		S.nFreeListHead = -1;
		int64_t nTick = MP_TICK();
		for(int i = 0; i < MICROPROFILE_MAX_FRAME_HISTORY; ++i)
		{
			S.Frames[i].nFrameStartCpu = nTick;
			S.Frames[i].nFrameStartGpu = -1;
		}

		MicroProfileThreadLog* pGpu = MicroProfileCreateThreadLog("GPU");
		g_MicroProfileGpuLog = pGpu;
		MP_ASSERT(S.Pool[0] == pGpu);
		pGpu->nGpu = 1;
		pGpu->nThreadId = 0;


		S.fDetailedOffsetTarget = S.fDetailedOffset = 0.f;
		S.fDetailedRangeTarget = S.fDetailedRange = 50.f;

		S.nOpacityBackground = 0xff<<24;
		S.nOpacityForeground = 0xff<<24;

		S.bShowSpikes = false;
		MicroProfileWebServerStart();
	}
	if(bUseLock)
		mutex.unlock();
}

void MicroProfileShutdown()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	MicroProfileWebServerStop();

#if MICROPROFILE_CONTEXT_SWITCH_TRACE
	if(S.pContextSwitchThread)
	{
		if(S.pContextSwitchThread->joinable())
		{
			S.bContextSwitchStop = true;
			S.pContextSwitchThread->join();
		}
		delete S.pContextSwitchThread;
	}
#endif


}

#ifdef MICROPROFILE_IOS
inline MicroProfileThreadLog* MicroProfileGetThreadLog()
{
	pthread_once(&g_MicroProfileThreadLogKeyOnce, MicroProfileCreateThreadLogKey);
	return (MicroProfileThreadLog*)pthread_getspecific(g_MicroProfileThreadLogKey);
}

inline void MicroProfileSetThreadLog(MicroProfileThreadLog* pLog)
{
	pthread_once(&g_MicroProfileThreadLogKeyOnce, MicroProfileCreateThreadLogKey);
	pthread_setspecific(g_MicroProfileThreadLogKey, pLog);
}
#else
MicroProfileThreadLog* MicroProfileGetThreadLog()
{
	return g_MicroProfileThreadLog;
}
inline void MicroProfileSetThreadLog(MicroProfileThreadLog* pLog)
{
	g_MicroProfileThreadLog = pLog;
}
#endif


MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName)
{
	MicroProfileThreadLog* pLog = 0;
	if(S.nFreeListHead != -1)
	{
		pLog = S.Pool[S.nFreeListHead];
		MP_ASSERT(pLog->nPut.load() == 0);
		MP_ASSERT(pLog->nGet.load() == 0);
		S.nFreeListHead = S.Pool[S.nFreeListHead]->nFreeListNext;
	}
	else
	{
		pLog = new MicroProfileThreadLog;
		S.nMemUsage += sizeof(MicroProfileThreadLog);
		S.Pool[S.nNumLogs++] = pLog;	
	}
	memset(pLog, 0, sizeof(*pLog));
	int len = (int)strlen(pName);
	int maxlen = sizeof(pLog->ThreadName)-1;
	len = len < maxlen ? len : maxlen;
	memcpy(&pLog->ThreadName[0], pName, len);
	pLog->ThreadName[len] = '\0';
	pLog->nThreadId = MP_GETCURRENTTHREADID();
	pLog->nFreeListNext = -1;
	pLog->nActive = 1;
	return pLog;
}

void MicroProfileOnThreadCreate(const char* pThreadName)
{
	g_bUseLock = true;
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	MP_ASSERT(MicroProfileGetThreadLog() == 0);
	MicroProfileThreadLog* pLog = MicroProfileCreateThreadLog(pThreadName ? pThreadName : MicroProfileGetThreadName());
	MP_ASSERT(pLog);
	MicroProfileSetThreadLog(pLog);
}

void MicroProfileOnThreadExit()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
	if(pLog)
	{
		int32_t nLogIndex = -1;
		for(int i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			if(pLog == S.Pool[i])
			{
				nLogIndex = i;
				break;
			}
		}
		MP_ASSERT(nLogIndex < MICROPROFILE_MAX_THREADS && nLogIndex > 0);
		pLog->nFreeListNext = S.nFreeListHead;
		pLog->nActive = 0;
		pLog->nPut.store(0);
		pLog->nGet.store(0);
		S.nFreeListHead = nLogIndex;
		for(int i = 0; i < MICROPROFILE_MAX_FRAME_HISTORY; ++i)
		{
			S.Frames[i].nLogStart[nLogIndex] = 0;
		}
	}
}

void MicroProfileInitThreadLog()
{
	MicroProfileOnThreadCreate(nullptr);
}


struct MicroProfileScopeLock
{
	bool bUseLock;
	std::recursive_mutex& m;
	MicroProfileScopeLock(std::recursive_mutex& m) : bUseLock(g_bUseLock), m(m)
	{
		if(bUseLock)
			m.lock();
	}
	~MicroProfileScopeLock()
	{
		if(bUseLock)
			m.unlock();
	}
};

MicroProfileToken MicroProfileFindToken(const char* pGroup, const char* pName)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		if(!MP_STRCASECMP(pName, S.TimerInfo[i].pName) && !MP_STRCASECMP(pGroup, S.GroupInfo[S.TimerInfo[i].nGroupIndex].pName))
		{
			return S.TimerInfo[i].nToken;
		}
	}
	return MICROPROFILE_INVALID_TOKEN;
}

uint16_t MicroProfileGetGroup(const char* pGroup, MicroProfileTokenType Type)
{
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		if(!MP_STRCASECMP(pGroup, S.GroupInfo[i].pName))
		{
			return i;
		}
	}
	uint16_t nGroupIndex = 0xffff;
	uint32_t nLen = (uint32_t)strlen(pGroup);
	if(nLen > MICROPROFILE_NAME_MAX_LEN-1)
		nLen = MICROPROFILE_NAME_MAX_LEN-1;
	memcpy(&S.GroupInfo[S.nGroupCount].pName[0], pGroup, nLen);
	S.GroupInfo[S.nGroupCount].pName[nLen] = '\0';
	S.GroupInfo[S.nGroupCount].nNameLen = nLen;
	S.GroupInfo[S.nGroupCount].nGroupIndex = S.nGroupCount;
	S.GroupInfo[S.nGroupCount].nNumTimers = 0;
	S.GroupInfo[S.nGroupCount].Type = Type;
	S.GroupInfo[S.nGroupCount].nMaxTimerNameLen = 0;
	nGroupIndex = S.nGroupCount++;
	S.nGroupMask = (S.nGroupMask<<1)|1;
	MP_ASSERT(nGroupIndex < MICROPROFILE_MAX_GROUPS);
	return nGroupIndex;
}


MicroProfileToken MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor, MicroProfileTokenType Type)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	MicroProfileToken ret = MicroProfileFindToken(pGroup, pName);
	if(ret != MICROPROFILE_INVALID_TOKEN)
		return ret;
	uint16_t nGroupIndex = MicroProfileGetGroup(pGroup, Type);
	uint16_t nTimerIndex = (uint16_t)(S.nTotalTimers++);
	uint64_t nGroupMask = 1ll << nGroupIndex;
	MicroProfileToken nToken = MicroProfileMakeToken(nGroupMask, nTimerIndex);
	S.GroupInfo[nGroupIndex].nNumTimers++;
	S.GroupInfo[nGroupIndex].nMaxTimerNameLen = MicroProfileMax(S.GroupInfo[nGroupIndex].nMaxTimerNameLen, (uint32_t)strlen(pName));
	MP_ASSERT(S.GroupInfo[nGroupIndex].Type == Type); //dont mix cpu & gpu timers in the same group
	S.nMaxGroupSize = MicroProfileMax(S.nMaxGroupSize, S.GroupInfo[nGroupIndex].nNumTimers);
	S.TimerInfo[nTimerIndex].nToken = nToken;
	uint32_t nLen = (uint32_t)strlen(pName);
	if(nLen > MICROPROFILE_NAME_MAX_LEN-1)
		nLen = MICROPROFILE_NAME_MAX_LEN-1;
	memcpy(&S.TimerInfo[nTimerIndex].pName, pName, nLen);
	S.TimerInfo[nTimerIndex].pName[nLen] = '\0';
	S.TimerInfo[nTimerIndex].nNameLen = nLen;
	S.TimerInfo[nTimerIndex].nColor = nColor&0xffffff;
	S.TimerInfo[nTimerIndex].nGroupIndex = nGroupIndex;
	S.TimerInfo[nTimerIndex].nTimerIndex = nTimerIndex;
	return nToken;
}

MicroProfileToken MicroProfileGetMetaToken(const char* pName)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	for(uint32_t i = 0; i < MICROPROFILE_META_MAX; ++i)
	{
		if(!S.MetaCounters[i].pName)
		{
			S.MetaCounters[i].pName = pName;
			return i;
		}
		else if(!MP_STRCASECMP(pName, S.MetaCounters[i].pName))
		{
			return i;
		}
	}
	MP_ASSERT(0);//out of slots, increase MICROPROFILE_META_MAX
	return (MicroProfileToken)-1;
}


inline void MicroProfileLogPut(MicroProfileToken nToken_, uint64_t nTick, uint64_t nBegin, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	uint32_t nPos = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPos+1) % MICROPROFILE_BUFFER_SIZE;
	if(nNextPos == pLog->nGet.load(std::memory_order_relaxed))
	{
		S.nOverflow = 100;
	}
	else
	{
		int64_t test = MicroProfileMakeLogIndex(nBegin, nToken_, nTick);;
		MP_ASSERT(MicroProfileLogType(test) == nBegin);
		MP_ASSERT(MicroProfileLogTimerIndex(test) == MicroProfileGetTimerIndex(nToken_));
		pLog->Log[nPos] = MicroProfileMakeLogIndex(nBegin, nToken_, nTick);
		pLog->nPut.store(nNextPos, std::memory_order_release);
	}
}

uint64_t MicroProfileEnter(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}
		uint64_t nTick = MP_TICK();
		MicroProfileLogPut(nToken_, nTick, MP_LOG_ENTER, MicroProfileGetThreadLog());
		return nTick;
	}
	return MICROPROFILE_INVALID_TICK;
}

void MicroProfileMetaUpdate(MicroProfileToken nToken, int nCount, MicroProfileTokenType eTokenType)
{
	if((MP_DRAW_META_FIRST<<nToken) & S.nActiveBars)
	{
		MicroProfileThreadLog* pLog = MicroProfileTokenTypeCpu == eTokenType ? MicroProfileGetThreadLog() : g_MicroProfileGpuLog;
		if(pLog)
		{
			MP_ASSERT(nToken < MICROPROFILE_META_MAX);
			MicroProfileLogPut(nToken, nCount, MP_LOG_META, pLog);
		}
	}
}


void MicroProfileLeave(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(MICROPROFILE_INVALID_TICK != nTickStart)
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}
		uint64_t nTick = MP_TICK();
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		MicroProfileLogPut(nToken_, nTick, MP_LOG_LEAVE, pLog);
	}
}


uint64_t MicroProfileGpuEnter(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp();
		MicroProfileLogPut(nToken_, nTimer, MP_LOG_ENTER, g_MicroProfileGpuLog);
		return 1;
	}
	return 0;
}

void MicroProfileGpuLeave(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(nTickStart)
	{
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp();
		MicroProfileLogPut(nToken_, nTimer, MP_LOG_LEAVE, g_MicroProfileGpuLog);
	}
}

void MicroProfileContextSwitchPut(MicroProfileContextSwitch* pContextSwitch)
{
	if(S.nRunning || pContextSwitch->nTicks <= S.nPauseTicks)
	{
		uint32_t nPut = S.nContextSwitchPut;
		S.ContextSwitch[nPut] = *pContextSwitch;
		S.nContextSwitchPut = (S.nContextSwitchPut+1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
	}
}


void MicroProfileGetRange(uint32_t nPut, uint32_t nGet, uint32_t nRange[2][2])
{
	if(nPut > nGet)
	{
		nRange[0][0] = nGet;
		nRange[0][1] = nPut;
		nRange[1][0] = nRange[1][1] = 0;
	}
	else if(nPut != nGet)
	{
		MP_ASSERT(nGet != MICROPROFILE_BUFFER_SIZE);
		uint32_t nCountEnd = MICROPROFILE_BUFFER_SIZE - nGet;
		nRange[0][0] = nGet;
		nRange[0][1] = nGet + nCountEnd;
		nRange[1][0] = 0;
		nRange[1][1] = nPut;
	}
}

void MicroProfileFlip()
{
	#if 0
	//verify LogEntry wraps correctly
	MicroProfileLogEntry c = MP_LOG_TICK_MASK-5000;
	for(int i = 0; i < 10000; ++i, c += 1)
	{
		MicroProfileLogEntry l2 = (c+2500) & MP_LOG_TICK_MASK;
		MP_ASSERT(2500 == MicroProfileLogTickDifference(c, l2));
	}
	#endif
	MICROPROFILE_SCOPE(g_MicroProfileFlip);
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());

	{
		static int once = 0;
		if(0 == once)
		{
			uint32_t nDisplay = S.nDisplay;
			MicroProfileLoadPreset(g_MicroProfilePresetNames[0]);
			once++;
			S.nDisplay = nDisplay;// dont load display, just state
		}
	}

	if(S.nToggleRunning)
	{
		S.nRunning = !S.nRunning;
		if(!S.nRunning)
			S.nPauseTicks = MP_TICK();
		S.nToggleRunning = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			if(pLog)
			{
				pLog->nStackPos = 0;
			}
		}
	}
	uint32_t nAggregateClear = S.nAggregateClear, nAggregateFlip = 0;
	if(S.nDumpHtmlNextFrame)
	{
		S.nDumpHtmlNextFrame = 0;
		MicroProfileDumpHtmlToFile();
	}
	if(MicroProfileWebServerUpdate())
	{	
		nAggregateClear = 1;
		nAggregateFlip = 1;
	}

	if(S.nRunning || S.nForceEnable)
	{
		S.nFramePutIndex++;
		S.nFramePut = (S.nFramePut+1) % MICROPROFILE_MAX_FRAME_HISTORY;
		MP_ASSERT((S.nFramePutIndex % MICROPROFILE_MAX_FRAME_HISTORY) == S.nFramePut);
		S.nFrameCurrent = (S.nFramePut + MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.nFrameCurrentIndex++;
		uint32_t nFrameNext = (S.nFrameCurrent+1) % MICROPROFILE_MAX_FRAME_HISTORY;

		uint32_t nContextSwitchPut = S.nContextSwitchPut;
		if(S.nContextSwitchLastPut < nContextSwitchPut)
		{
			S.nContextSwitchUsage = (nContextSwitchPut - S.nContextSwitchLastPut);
		}
		else
		{
			S.nContextSwitchUsage = MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - S.nContextSwitchLastPut + nContextSwitchPut;
		}
		S.nContextSwitchLastPut = nContextSwitchPut;

		MicroProfileFrameState* pFramePut = &S.Frames[S.nFramePut];
		MicroProfileFrameState* pFrameCurrent = &S.Frames[S.nFrameCurrent];
		MicroProfileFrameState* pFrameNext = &S.Frames[nFrameNext];
		
		pFramePut->nFrameStartCpu = MP_TICK();
		pFramePut->nFrameStartGpu = (uint32_t)MicroProfileGpuInsertTimeStamp();
		if(pFrameNext->nFrameStartGpu != (uint64_t)-1)
			pFrameNext->nFrameStartGpu = MicroProfileGpuGetTimeStamp((uint32_t)pFrameNext->nFrameStartGpu);

		if(pFrameCurrent->nFrameStartGpu == (uint64_t)-1)
			pFrameCurrent->nFrameStartGpu = pFrameNext->nFrameStartGpu + 1; 

		uint64_t nFrameStartCpu = pFrameCurrent->nFrameStartCpu;
		uint64_t nFrameEndCpu = pFrameNext->nFrameStartCpu;

		{
			uint64_t nTick = nFrameEndCpu - nFrameStartCpu;
			S.nFlipTicks = nTick;
			S.nFlipAggregate += nTick;
			S.nFlipMax = MicroProfileMax(S.nFlipMax, nTick);
		}

		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			if(!pLog)
			{
				pFramePut->nLogStart[i] = 0;
			}
			else
			{
				uint32_t nPut = pLog->nPut.load(std::memory_order_acquire);
				pFramePut->nLogStart[i] = nPut;
				MP_ASSERT(nPut< MICROPROFILE_BUFFER_SIZE);
				//need to keep last frame around to close timers. timers more than 1 frame old is ditched.
				pLog->nGet.store(nPut, std::memory_order_relaxed);
			}
		}

		if(S.nRunning)
		{
			{
				MICROPROFILE_SCOPE(g_MicroProfileClear);
				for(uint32_t i = 0; i < S.nTotalTimers; ++i)
				{
					S.Frame[i].nTicks = 0;
					S.Frame[i].nCount = 0;
					S.FrameExclusive[i] = 0;
				}
				for(uint32_t j = 0; j < MICROPROFILE_META_MAX; ++j)
				{
					if(S.MetaCounters[j].pName)
					{
						for(uint32_t i = 0; i < S.nTotalTimers; ++i)
						{
							S.MetaCounters[j].nCounters[i] = 0;
						}
					}
				}
			}
			{
				MICROPROFILE_SCOPE(g_MicroProfileThreadLoop);
				for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
				{
					MicroProfileThreadLog* pLog = S.Pool[i];
					if(!pLog) 
						continue;

					uint32_t nPut = pFrameNext->nLogStart[i];
					uint32_t nGet = pFrameCurrent->nLogStart[i];
					uint32_t nRange[2][2] = { {0, 0}, {0, 0}, };
					MicroProfileGetRange(nPut, nGet, nRange);


					//fetch gpu results.
					if(pLog->nGpu)
					{
						for(uint32_t j = 0; j < 2; ++j)
						{
							uint32_t nStart = nRange[j][0];
							uint32_t nEnd = nRange[j][1];
							for(uint32_t k = nStart; k < nEnd; ++k)
							{
								MicroProfileLogEntry L = pLog->Log[k];
								pLog->Log[k] = MicroProfileLogSetTick(L, MicroProfileGpuGetTimeStamp((uint32_t)MicroProfileLogGetTick(L)));
							}
						}
					}
					
					
					uint32_t* pStack = &pLog->nStack[0];
					int64_t* pChildTickStack = &pLog->nChildTickStack[0];
					uint32_t nStackPos = pLog->nStackPos;

					for(uint32_t j = 0; j < 2; ++j)
					{
						uint32_t nStart = nRange[j][0];
						uint32_t nEnd = nRange[j][1];
						for(uint32_t k = nStart; k < nEnd; ++k)
						{
							MicroProfileLogEntry LE = pLog->Log[k];
							int nType = MicroProfileLogType(LE);
							if(MP_LOG_ENTER == nType)
							{
								MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);								
								pStack[nStackPos++] = k;
								pChildTickStack[nStackPos] = 0;
							}
							else if(MP_LOG_META == nType)
							{
								if(nStackPos)
								{
									int64_t nMetaIndex = MicroProfileLogTimerIndex(LE);
									int64_t nMetaCount = MicroProfileLogGetTick(LE);
									MP_ASSERT(nMetaIndex < MICROPROFILE_META_MAX);
									int64_t nCounter = MicroProfileLogTimerIndex(pLog->Log[pStack[nStackPos-1]]);
									S.MetaCounters[nMetaIndex].nCounters[nCounter] += nMetaCount;
								}
							}
							else
							{
								MP_ASSERT(nType == MP_LOG_LEAVE);
								if(nStackPos)
								{									
									int64_t nTickStart = pLog->Log[pStack[nStackPos-1]];
									int64_t nTicks = MicroProfileLogTickDifference(nTickStart, LE);
									int64_t nChildTicks = pChildTickStack[nStackPos];
									nStackPos--;
									pChildTickStack[nStackPos] += nTicks;

									uint32_t nTimerIndex = MicroProfileLogTimerIndex(LE);
									S.Frame[nTimerIndex].nTicks += nTicks;
									S.FrameExclusive[nTimerIndex] += (nTicks-nChildTicks);
									S.Frame[nTimerIndex].nCount += 1;
								}
							}
						}
					}
					pLog->nStackPos = nStackPos;
				}
			}
			{
				MICROPROFILE_SCOPE(g_MicroProfileAccumulate);
				for(uint32_t i = 0; i < S.nTotalTimers; ++i)
				{
					S.AggregateTimers[i].nTicks += S.Frame[i].nTicks;				
					S.AggregateTimers[i].nCount += S.Frame[i].nCount;
					S.MaxTimers[i] = MicroProfileMax(S.MaxTimers[i], S.Frame[i].nTicks);
					S.AggregateTimersExclusive[i] += S.FrameExclusive[i];				
					S.MaxTimersExclusive[i] = MicroProfileMax(S.MaxTimersExclusive[i], S.FrameExclusive[i]);
				}
			}
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
			{
				if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
				{
					MicroProfileToken nToken = S.Graph[i].nToken;
					S.Graph[i].nHistory[S.nGraphPut] = S.Frame[MicroProfileGetTimerIndex(nToken)].nTicks;
				}
			}
			S.nGraphPut = (S.nGraphPut+1) % MICROPROFILE_GRAPH_HISTORY;

		}


		if(S.nRunning && S.nAggregateFlip <= ++S.nAggregateFlipCount)
		{
			nAggregateFlip = 1;
			if(S.nAggregateFlip) // if 0 accumulate indefinitely
			{
				nAggregateClear = 1;
			}
		}
	}
	if(nAggregateFlip)
	{
		memcpy(&S.Aggregate[0], &S.AggregateTimers[0], sizeof(S.Aggregate[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMax[0], &S.MaxTimers[0], sizeof(S.AggregateMax[0]) * S.nTotalTimers);
		memcpy(&S.AggregateExclusive[0], &S.AggregateTimersExclusive[0], sizeof(S.AggregateExclusive[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMaxExclusive[0], &S.MaxTimersExclusive[0], sizeof(S.AggregateMaxExclusive[0]) * S.nTotalTimers);
		
		S.nAggregateFrames = S.nAggregateFlipCount;
		S.nFlipAggregateDisplay = S.nFlipAggregate;
		S.nFlipMaxDisplay = S.nFlipMax;
		if(nAggregateClear)
		{
			memset(&S.AggregateTimers[0], 0, sizeof(S.Aggregate[0]) * S.nTotalTimers);
			memset(&S.MaxTimers[0], 0, sizeof(S.MaxTimers[0]) * S.nTotalTimers);
			memset(&S.AggregateTimersExclusive[0], 0, sizeof(S.AggregateExclusive[0]) * S.nTotalTimers);
			memset(&S.MaxTimersExclusive[0], 0, sizeof(S.MaxTimersExclusive[0]) * S.nTotalTimers);
			S.nAggregateFlipCount = 0;
			S.nFlipAggregate = 0;
			S.nFlipMax = 0;

			S.nAggregateFlipTick = MP_TICK();
		}
	}
	S.nAggregateClear = 0;

	uint64_t nNewActiveGroup = 0;
	if(S.nForceEnable || (S.nDisplay && S.nRunning))
		nNewActiveGroup = S.nMenuAllGroups ? S.nGroupMask : S.nMenuActiveGroup;
	nNewActiveGroup |= S.nForceGroup;
	if(S.nActiveGroup != nNewActiveGroup)
		S.nActiveGroup = nNewActiveGroup;
	uint32_t nNewActiveBars = 0;
	if(S.nDisplay && S.nRunning)
		nNewActiveBars = S.nBars;
	if(S.nForceMetaCounters)
	{
		for(int i = 0; i < MICROPROFILE_META_MAX; ++i)
		{
			if(S.MetaCounters[i].pName)
			{
				nNewActiveBars |= (MP_DRAW_META_FIRST<<i);
			}
		}
	}
	if(nNewActiveBars != S.nActiveBars)
		S.nActiveBars = nNewActiveBars;

	S.fDetailedOffset = S.fDetailedOffset + (S.fDetailedOffsetTarget - S.fDetailedOffset) * MICROPROFILE_ANIM_DELAY_PRC;
	S.fDetailedRange = S.fDetailedRange + (S.fDetailedRangeTarget - S.fDetailedRange) * MICROPROFILE_ANIM_DELAY_PRC;

}

void MicroProfileSetForceEnable(bool bEnable)
{
	S.nForceEnable = bEnable ? 1 : 0;
}
bool MicroProfileGetForceEnable()
{
	return S.nForceEnable != 0;
}

void MicroProfileSetEnableAllGroups(bool bEnableAllGroups)
{
	S.nMenuAllGroups = bEnableAllGroups ? 1 : 0;
}

bool MicroProfileGetEnableAllGroups()
{
	return 0 != S.nMenuAllGroups;
}

void MicroProfileSetForceMetaCounters(bool bForce)
{
	S.nForceMetaCounters = bForce ? 1 : 0;
}

bool MicroProfileGetForceMetaCounters()
{
	return 0 != S.nForceMetaCounters;
}

void MicroProfileSetAggregateFrames(int nFrames)
{
	S.nAggregateFlip = (uint32_t)nFrames;
	if(0 == nFrames)
	{
		S.nAggregateClear = 1;
	}
}

int MicroProfileGetAggregateFrames()
{
	return S.nAggregateFlip;
}

int MicroProfileGetCurrentAggregateFrames()
{
	return int(S.nAggregateFlip ? S.nAggregateFlip : S.nAggregateFlipCount);
}

void MicroProfileGetState(MicroProfileState* pStateOut)
{
	pStateOut->nDisplay = S.nDisplay;
	pStateOut->nMenuAllGroups = S.nMenuAllGroups;
	pStateOut->nMenuActiveGroup = S.nMenuActiveGroup;
	pStateOut->nMenuAllThreads = S.nMenuAllThreads;
	pStateOut->nAggregateFlip = S.nAggregateFlip;
	pStateOut->nBars = S.nBars;
	pStateOut->fReferenceTime = S.fReferenceTime;
}

void MicroProfileSetState(MicroProfileState* pStateOut)
{
	MicroProfileScopeLock L(MicroProfileMutex());
	S.nDisplay = pStateOut->nDisplay;
	S.nMenuAllGroups = pStateOut->nMenuAllGroups;
	S.nMenuActiveGroup = pStateOut->nMenuActiveGroup;
	S.nMenuAllThreads = pStateOut->nMenuAllThreads;
	S.nAggregateFlip = pStateOut->nAggregateFlip;
	S.nBars = pStateOut->nBars;
	S.fReferenceTime = pStateOut->fReferenceTime;
	S.fRcpReferenceTime = 1.f / S.fReferenceTime;
}


#include <stdio.h>

#define MICROPROFILE_PRESET_HEADER_MAGIC 0x28586813
#define MICROPROFILE_PRESET_HEADER_VERSION 0x00000102
struct MicroProfilePresetHeader
{
	uint32_t nMagic;
	uint32_t nVersion;
	//groups, threads, aggregate, reference frame, graphs timers
	uint32_t nGroups[MICROPROFILE_MAX_GROUPS];
	uint32_t nThreads[MICROPROFILE_MAX_THREADS];
	uint32_t nGraphName[MICROPROFILE_MAX_GRAPHS];
	uint32_t nGraphGroupName[MICROPROFILE_MAX_GRAPHS];
	uint32_t nMenuAllGroups;
	uint32_t nMenuAllThreads;
	uint32_t nAggregateFlip;
	float fReferenceTime;
	uint32_t nBars;
	uint32_t nDisplay;
	uint32_t nOpacityBackground;
	uint32_t nOpacityForeground;
	uint32_t nShowSpikes;
};

#ifndef MICROPROFILE_PRESET_FILENAME_FUNC
#define MICROPROFILE_PRESET_FILENAME_FUNC MicroProfilePresetFilename
static const char* MicroProfilePresetFilename(const char* pSuffix)
{
	static char filename[512];
	snprintf(filename, sizeof(filename)-1, ".microprofilepreset.%s", pSuffix);
	return filename;
}
#endif

void MicroProfileSavePreset(const char* pPresetName)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	FILE* F = fopen(MICROPROFILE_PRESET_FILENAME_FUNC(pPresetName), "wb");
	if(!F) return;

	MicroProfilePresetHeader Header;
	memset(&Header, 0, sizeof(Header));
	Header.nAggregateFlip = S.nAggregateFlip;
	Header.nBars = S.nBars;
	Header.fReferenceTime = S.fReferenceTime;
	Header.nMenuAllGroups = S.nMenuAllGroups;
	Header.nMenuAllThreads = S.nMenuAllThreads;
	Header.nMagic = MICROPROFILE_PRESET_HEADER_MAGIC;
	Header.nVersion = MICROPROFILE_PRESET_HEADER_VERSION;
	Header.nDisplay = S.nDisplay;
	Header.nOpacityBackground = S.nOpacityBackground;
	Header.nOpacityForeground = S.nOpacityForeground;
	Header.nShowSpikes = S.bShowSpikes ? 1 : 0;
	fwrite(&Header, sizeof(Header), 1, F);
	uint64_t nMask = 1;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
	{
		if(S.nMenuActiveGroup & nMask)
		{
			uint32_t offset = ftell(F);
			const char* pName = S.GroupInfo[i].pName;
			int nLen = (int)strlen(pName)+1;
			fwrite(pName, nLen, 1, F);
			Header.nGroups[i] = offset;
		}
		nMask <<= 1;
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog && S.nThreadActive[i])
		{
			uint32_t nOffset = ftell(F);
			const char* pName = &pLog->ThreadName[0];
			int nLen = (int)strlen(pName)+1;
			fwrite(pName, nLen, 1, F);
			Header.nThreads[i] = nOffset;
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		MicroProfileToken nToken = S.Graph[i].nToken;
		if(nToken != MICROPROFILE_INVALID_TOKEN)
		{
			uint32_t nGroupIndex = MicroProfileGetGroupIndex(nToken);
			uint32_t nTimerIndex = MicroProfileGetTimerIndex(nToken);
			const char* pGroupName = S.GroupInfo[nGroupIndex].pName;
			const char* pTimerName = S.TimerInfo[nTimerIndex].pName;
			MP_ASSERT(pGroupName);
			MP_ASSERT(pTimerName);
			int nGroupLen = (int)strlen(pGroupName)+1;
			int nTimerLen = (int)strlen(pTimerName)+1;

			uint32_t nOffsetGroup = ftell(F);
			fwrite(pGroupName, nGroupLen, 1, F);
			uint32_t nOffsetTimer = ftell(F);
			fwrite(pTimerName, nTimerLen, 1, F);
			Header.nGraphName[i] = nOffsetTimer;
			Header.nGraphGroupName[i] = nOffsetGroup;
		}
	}
	fseek(F, 0, SEEK_SET);
	fwrite(&Header, sizeof(Header), 1, F);

	fclose(F);

}



void MicroProfileLoadPreset(const char* pSuffix)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	FILE* F = fopen(MICROPROFILE_PRESET_FILENAME_FUNC(pSuffix), "rb");
	if(!F)
	{
	 	return;
	}
	fseek(F, 0, SEEK_END);
	int nSize = ftell(F);
	char* const pBuffer = (char*)alloca(nSize);
	fseek(F, 0, SEEK_SET);
	int nRead = (int)fread(pBuffer, nSize, 1, F);
	fclose(F);
	if(1 != nRead)
		return;
	
	MicroProfilePresetHeader& Header = *(MicroProfilePresetHeader*)pBuffer;

	if(Header.nMagic != MICROPROFILE_PRESET_HEADER_MAGIC || Header.nVersion != MICROPROFILE_PRESET_HEADER_VERSION)
	{
		return;
	}

	S.nAggregateFlip = Header.nAggregateFlip;
	S.nBars = Header.nBars;
	S.fReferenceTime = Header.fReferenceTime;
	S.fRcpReferenceTime = 1.f / Header.fReferenceTime;
	S.nMenuAllGroups = Header.nMenuAllGroups;
	S.nMenuAllThreads = Header.nMenuAllThreads;
	S.nDisplay = Header.nDisplay;
	S.nMenuActiveGroup = 0;
	S.nOpacityBackground = Header.nOpacityBackground;
	S.nOpacityForeground = Header.nOpacityForeground;
	S.bShowSpikes = Header.nShowSpikes == 1;

	memset(&S.nThreadActive[0], 0, sizeof(S.nThreadActive));

	for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
	{
		if(Header.nGroups[i])
		{
			const char* pGroupName = pBuffer + Header.nGroups[i];	
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				if(S.GroupInfo[j].pName && 0 == MP_STRCASECMP(pGroupName, S.GroupInfo[j].pName))
				{
					S.nMenuActiveGroup |= (1ll << j);
				}
			}
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		if(Header.nThreads[i])
		{
			const char* pThreadName = pBuffer + Header.nThreads[i];
			for(uint32_t j = 0; j < MICROPROFILE_MAX_THREADS; ++j)
			{
				MicroProfileThreadLog* pLog = S.Pool[j];
				if(pLog && 0 == MP_STRCASECMP(pThreadName, &pLog->ThreadName[0]))
				{
					S.nThreadActive[j] = 1;
				}
			}
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		MicroProfileToken nPrevToken = S.Graph[i].nToken;
		S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		if(Header.nGraphName[i] && Header.nGraphGroupName[i])
		{
			const char* pGraphName = pBuffer + Header.nGraphName[i];
			const char* pGraphGroupName = pBuffer + Header.nGraphGroupName[i];
			for(uint32_t j = 0; j < S.nTotalTimers; ++j)
			{
				uint64_t nGroupIndex = S.TimerInfo[j].nGroupIndex;
				if(0 == MP_STRCASECMP(pGraphName, S.TimerInfo[j].pName) && 0 == MP_STRCASECMP(pGraphGroupName, S.GroupInfo[nGroupIndex].pName))
				{
					MicroProfileToken nToken = MicroProfileMakeToken(1ll << nGroupIndex, (uint16_t)j);
					S.Graph[i].nToken = nToken;			// note: group index is stored here but is checked without in MicroProfileToggleGraph()!
					S.TimerInfo[j].bGraph = true;
					if(nToken != nPrevToken)
					{
						memset(&S.Graph[i].nHistory, 0, sizeof(S.Graph[i].nHistory));
					}
					break;
				}
			}
		}
	}
}

void MicroProfileForceEnableGroup(const char* pGroup, MicroProfileTokenType Type)
{
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	uint16_t nGroup = MicroProfileGetGroup(pGroup, Type);
	S.nForceGroup |= (1ll << nGroup);
}

void MicroProfileForceDisableGroup(const char* pGroup, MicroProfileTokenType Type)
{
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	uint16_t nGroup = MicroProfileGetGroup(pGroup, Type);
	S.nForceGroup &= ~(1ll << nGroup);
}


void MicroProfileCalcAllTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, float* pExclusive, float* pAverageExclusive, float* pMaxExclusive, uint32_t nSize)
{
	for(uint32_t i = 0; i < S.nTotalTimers && i < nSize; ++i)
	{
		const uint32_t nGroupId = S.TimerInfo[i].nGroupIndex;
		const float fToMs = MicroProfileTickToMsMultiplier(S.GroupInfo[nGroupId].Type == MicroProfileTokenTypeGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
		uint32_t nTimer = i;
		uint32_t nIdx = i * 2;
		uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
		uint32_t nAggregateCount = S.Aggregate[nTimer].nCount ? S.Aggregate[nTimer].nCount : 1;
		float fToPrc = S.fRcpReferenceTime;
		float fMs = fToMs * (S.Frame[nTimer].nTicks);
		float fPrc = MicroProfileMin(fMs * fToPrc, 1.f);
		float fAverageMs = fToMs * (S.Aggregate[nTimer].nTicks / nAggregateFrames);
		float fAveragePrc = MicroProfileMin(fAverageMs * fToPrc, 1.f);
		float fMaxMs = fToMs * (S.AggregateMax[nTimer]);
		float fMaxPrc = MicroProfileMin(fMaxMs * fToPrc, 1.f);
		float fCallAverageMs = fToMs * (S.Aggregate[nTimer].nTicks / nAggregateCount);
		float fCallAveragePrc = MicroProfileMin(fCallAverageMs * fToPrc, 1.f);
		float fMsExclusive = fToMs * (S.FrameExclusive[nTimer]);
		float fPrcExclusive = MicroProfileMin(fMsExclusive * fToPrc, 1.f);
		float fAverageMsExclusive = fToMs * (S.AggregateExclusive[nTimer] / nAggregateFrames);
		float fAveragePrcExclusive = MicroProfileMin(fAverageMsExclusive * fToPrc, 1.f);
		float fMaxMsExclusive = fToMs * (S.AggregateMaxExclusive[nTimer]);
		float fMaxPrcExclusive = MicroProfileMin(fMaxMsExclusive * fToPrc, 1.f);
		pTimers[nIdx] = fMs;
		pTimers[nIdx+1] = fPrc;
		pAverage[nIdx] = fAverageMs;
		pAverage[nIdx+1] = fAveragePrc;
		pMax[nIdx] = fMaxMs;
		pMax[nIdx+1] = fMaxPrc;
		pCallAverage[nIdx] = fCallAverageMs;
		pCallAverage[nIdx+1] = fCallAveragePrc;
		pExclusive[nIdx] = fMsExclusive;
		pExclusive[nIdx+1] = fPrcExclusive;
		pAverageExclusive[nIdx] = fAverageMsExclusive;
		pAverageExclusive[nIdx+1] = fAveragePrcExclusive;
		pMaxExclusive[nIdx] = fMaxMsExclusive;
		pMaxExclusive[nIdx+1] = fMaxPrcExclusive;
	}
}

void MicroProfileTogglePause()
{
	S.nToggleRunning = 1;
}

float MicroProfileGetTime(const char* pGroup, const char* pName)
{
	MicroProfileToken nToken = MicroProfileFindToken(pGroup, pName);
	if(nToken == MICROPROFILE_INVALID_TOKEN)
	{
		return 0.f;
	}
	uint32_t nTimerIndex = MicroProfileGetTimerIndex(nToken);
	uint32_t nGroupIndex = MicroProfileGetGroupIndex(nToken);
	float fToMs = MicroProfileTickToMsMultiplier(S.GroupInfo[nGroupIndex].Type == MicroProfileTokenTypeGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
	return S.Frame[nTimerIndex].nTicks * fToMs;
}

#if MICROPROFILE_WEBSERVER

#define MICROPROFILE_EMBED_HTML
extern const char g_MicroProfileHtml_begin[];
extern const char g_MicroProfileHtml_end[];
extern const size_t g_MicroProfileHtml_begin_size;
extern const size_t g_MicroProfileHtml_end_size;


typedef void MicroProfileWriteCallback(void* Handle, size_t size, const char* pData);

void MicroProfileDumpHtml(const char* pFile)
{
	uint32_t nLen = strlen(pFile);
	if(nLen > sizeof(S.HtmlDumpPath)-1)
	{
		return;
	}
	memcpy(S.HtmlDumpPath, pFile, nLen+1);
	S.nDumpHtmlNextFrame = 1;
}

void MicroProfilePrintf(MicroProfileWriteCallback CB, void* Handle, const char* pFmt, ...)
{
	char buffer[32*1024];
	va_list args;
	va_start (args, pFmt);
#ifdef _WIN32
	size_t size = vsprintf_s(buffer, pFmt, args);
#else
	size_t size = vsnprintf(buffer, sizeof(buffer)-1,  pFmt, args);
#endif
	CB(Handle, size, &buffer[0]);
	va_end (args);
}

void MicroProfileDumpHtml(MicroProfileWriteCallback CB, void* Handle, int nMaxFrames)
{
	CB(Handle, g_MicroProfileHtml_begin_size-1, &g_MicroProfileHtml_begin[0]);

	//dump info
	uint64_t nTicks = MP_TICK();
	float fAggregateMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu()) * (nTicks - S.nAggregateFlipTick);
	MicroProfilePrintf(CB, Handle, "var AggregateInfo = {'Frames':%d, 'Time':%f};\n", S.nAggregateFrames, fAggregateMs);


	//groups
	MicroProfilePrintf(CB, Handle, "var GroupInfo = Array(%d);\n\n",S.nGroupCount);
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		MP_ASSERT(i == S.GroupInfo[i].nGroupIndex);
		MicroProfilePrintf(CB, Handle, "GroupInfo[%d] = MakeGroup(%d, \"%s\", %d, %d);\n", S.GroupInfo[i].nGroupIndex, S.GroupInfo[i].nGroupIndex, S.GroupInfo[i].pName, S.GroupInfo[i].nNumTimers, S.GroupInfo[i].Type == MicroProfileTokenTypeGpu?1:0);
	}
	//timers

	uint32_t nNumTimers = S.nTotalTimers;
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 7 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	float* pTimersExclusive = pTimers + 4 * nBlockSize;
	float* pAverageExclusive = pTimers + 5 * nBlockSize;
	float* pMaxExclusive = pTimers + 6 * nBlockSize;

	MicroProfileCalcAllTimers(pTimers, pAverage, pMax, pCallAverage, pTimersExclusive, pAverageExclusive, pMaxExclusive, nNumTimers);

	MicroProfilePrintf(CB, Handle, "\nvar TimerInfo = Array(%d);\n\n", S.nTotalTimers);
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t nIdx = i * 2;
		MP_ASSERT(i == S.TimerInfo[i].nTimerIndex);
		MicroProfilePrintf(CB, Handle, "var Meta%d = [", i);
		bool bOnce = true;
		for(int j = 0; j < MICROPROFILE_META_MAX; ++j)
		{
			if(S.MetaCounters[j].pName)
			{
				uint32_t lala = S.MetaCounters[j].nCounters[i];
				MicroProfilePrintf(CB, Handle, bOnce ? "%d" : ",%d", lala);
				bOnce = false;
			}
		}
		MicroProfilePrintf(CB, Handle, "];\n");
		MicroProfilePrintf(CB, Handle, "TimerInfo[%d] = MakeTimer(%d, \"%s\", %d, '#%02x%02x%02x', %f, %f, %f, %f, %f, %d, Meta%d);\n", S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].pName, S.TimerInfo[i].nGroupIndex, 
		MICROPROFILE_UNPACK_RED(S.TimerInfo[i].nColor) & 0xff,
		MICROPROFILE_UNPACK_GREEN(S.TimerInfo[i].nColor) & 0xff,
		MICROPROFILE_UNPACK_BLUE(S.TimerInfo[i].nColor) & 0xff,
		pAverage[nIdx],
		pMax[nIdx],
		pAverageExclusive[nIdx],
		pMaxExclusive[nIdx],
		pCallAverage[nIdx],
		S.Aggregate[i].nCount,
		i
		);

	}

	MicroProfilePrintf(CB, Handle, "\nvar ThreadNames = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "'%s',", S.Pool[i]->ThreadName);

		}
		else
		{
			MicroProfilePrintf(CB, Handle, "'Thread %d',", i);
		}
	}
	MicroProfilePrintf(CB, Handle, "];\n\n");
	MicroProfilePrintf(CB, Handle, "\nvar MetaNames = [");
	for(int i = 0; i < MICROPROFILE_META_MAX; ++i)
	{
		if(S.MetaCounters[i].pName)
		{
			MicroProfilePrintf(CB, Handle, "'%s',", S.MetaCounters[i].pName);
		}
	}


	MicroProfilePrintf(CB, Handle, "];\n\n");



	uint32_t nNumFrames = (MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 1);
	if(S.nFrameCurrentIndex < nNumFrames)
		nNumFrames = S.nFrameCurrentIndex;
	if((int)nNumFrames > nMaxFrames) 
	{
		nNumFrames = nMaxFrames;
	}



#if MICROPROFILE_DEBUG
	printf("dumping %d frames\n", nNumFrames);
#endif
	uint32_t nFirstFrame = (S.nFrameCurrent + MICROPROFILE_MAX_FRAME_HISTORY - nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	uint32_t nFirstFrameIndex = S.nFrameCurrentIndex - nNumFrames;
	int64_t nTickStart = S.Frames[nFirstFrame].nFrameStartCpu;
	int64_t nTickStartGpu = S.Frames[nFirstFrame].nFrameStartGpu;



	MicroProfilePrintf(CB, Handle, "var Frames = Array(%d);\n", nNumFrames);
	for(uint32_t i = 0; i < nNumFrames; ++i)
	{
		uint32_t nFrameIndex = (nFirstFrame + i) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint32_t nFrameIndexNext = (nFrameIndex + 1) % MICROPROFILE_MAX_FRAME_HISTORY;

		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfileThreadLog* pLog = S.Pool[j];
			int64_t nStartTick = pLog->nGpu ? nTickStartGpu : nTickStart;
			uint32_t nLogStart = S.Frames[nFrameIndex].nLogStart[j];
			uint32_t nLogEnd = S.Frames[nFrameIndexNext].nLogStart[j];

			float fToMs = MicroProfileTickToMsMultiplier(pLog->nGpu ? MicroProfileTicksPerSecondGpu() : MicroProfileTicksPerSecondCpu());
			MicroProfilePrintf(CB, Handle, "var ts_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				uint32_t k = nLogStart;
				uint32_t nLogType = MicroProfileLogType(pLog->Log[k]);
				float fTime = nLogType == MP_LOG_META ? 0.f : MicroProfileLogTickDifference(nStartTick, pLog->Log[k]) * fToMs;
				MicroProfilePrintf(CB, Handle, "%f", fTime);
				for(k = (k+1) % MICROPROFILE_BUFFER_SIZE; k != nLogEnd; k = (k+1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint32_t nLogType = MicroProfileLogType(pLog->Log[k]);
					float fTime = nLogType == MP_LOG_META ? 0.f : MicroProfileLogTickDifference(nStartTick, pLog->Log[k]) * fToMs;
					MicroProfilePrintf(CB, Handle, ",%f", fTime);
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");
			MicroProfilePrintf(CB, Handle, "var tt_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				uint32_t k = nLogStart;
				MicroProfilePrintf(CB, Handle, "%d", MicroProfileLogType(pLog->Log[k]));
				for(k = (k+1) % MICROPROFILE_BUFFER_SIZE; k != nLogEnd; k = (k+1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint32_t nLogType = MicroProfileLogType(pLog->Log[k]);
					if(nLogType == MP_LOG_META)
					{
						//for meta, store the count + 2, which is the tick part
						nLogType = 2 + MicroProfileLogGetTick(pLog->Log[k]);
					}
					MicroProfilePrintf(CB, Handle, ",%d", nLogType);
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");

			MicroProfilePrintf(CB, Handle, "var ti_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				uint32_t k = nLogStart;
				MicroProfilePrintf(CB, Handle, "%d", (uint32_t)MicroProfileLogTimerIndex(pLog->Log[k]));
				for(k = (k+1) % MICROPROFILE_BUFFER_SIZE; k != nLogEnd; k = (k+1) % MICROPROFILE_BUFFER_SIZE)
				{
					MicroProfilePrintf(CB, Handle, ",%d", (uint32_t)MicroProfileLogTimerIndex(pLog->Log[k]));
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");

		}

		MicroProfilePrintf(CB, Handle, "var ts%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "ts_%d_%d,", i, j);
		}
		MicroProfilePrintf(CB, Handle, "];\n");
		MicroProfilePrintf(CB, Handle, "var tt%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "tt_%d_%d,", i, j);
		}
		MicroProfilePrintf(CB, Handle, "];\n");

		MicroProfilePrintf(CB, Handle, "var ti%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "ti_%d_%d,", i, j);
		}
		MicroProfilePrintf(CB, Handle, "];\n");


		int64_t nFrameStart = S.Frames[nFrameIndex].nFrameStartCpu;
		int64_t nFrameEnd = S.Frames[nFrameIndexNext].nFrameStartCpu;

		float fToMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		float fFrameMs = MicroProfileLogTickDifference(nTickStart, nFrameStart) * fToMs;
		float fFrameEndMs = MicroProfileLogTickDifference(nTickStart, nFrameEnd) * fToMs;
		MicroProfilePrintf(CB, Handle, "Frames[%d] = MakeFrame(%d, %f, %f, ts%d, tt%d, ti%d);\n", i, nFirstFrameIndex, fFrameMs,fFrameEndMs, i, i, i);
	}

	CB(Handle, g_MicroProfileHtml_end_size-1, &g_MicroProfileHtml_end[0]);
}

void MicroProfileWriteFile(void* Handle, size_t nSize, const char* pData)
{
	fwrite(pData, nSize, 1, (FILE*)Handle);
}

void MicroProfileDumpHtmlToFile()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	FILE* F = fopen(S.HtmlDumpPath, "w");
	if(F)
	{
		MicroProfileDumpHtml(MicroProfileWriteFile, F, MICROPROFILE_WEBSERVER_MAXFRAMES);
		fclose(F);
	}
}

static uint64_t g_nMicroProfileDataSent = 0;
void MicroProfileWriteSocket(void* Handle, size_t nSize, const char* pData)
{
	g_nMicroProfileDataSent += nSize;
	send(*(MpSocket*)Handle, pData, nSize, 0);
}


#ifndef MicroProfileSetNonBlocking //fcntl doesnt work on a some unix like platforms..
void MicroProfileSetNonBlocking(MpSocket Socket, int NonBlocking)
{
#ifdef _WIN32
	u_long nonBlocking = NonBlocking ? 1 : 0; 
	ioctlsocket(Socket, FIONBIO, &nonBlocking);
#else
	int Options = fcntl(Socket, F_GETFL);
	if(NonBlocking)
	{
		fcntl(Socket, F_SETFL, Options|O_NONBLOCK);
	}
	else
	{
		fcntl(Socket, F_SETFL, Options&(~O_NONBLOCK));
	}
#endif
}
#endif

void MicroProfileWebServerStart()
{
#ifdef _WIN32
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa))
	{
		S.ListenerSocket = -1;
		return;
	}
#endif

	S.ListenerSocket = socket(PF_INET, SOCK_STREAM, 6);
	MP_ASSERT(!MP_INVALID_SOCKET(S.ListenerSocket));
	MicroProfileSetNonBlocking(S.ListenerSocket, 1);

	struct sockaddr_in Addr; 
	Addr.sin_family = AF_INET; 
	Addr.sin_addr.s_addr = INADDR_ANY; 
	for(int i = 0; i < 20; ++i)
	{
		Addr.sin_port = htons(MICROPROFILE_WEBSERVER_PORT+i); 
		if(0 == bind(S.ListenerSocket, (sockaddr*)&Addr, sizeof(Addr)))
		{
			printf("MicroProfile Webserver Port %d\n", MICROPROFILE_WEBSERVER_PORT + i);
			break;
		}
	}
	listen(S.ListenerSocket, 8);
}

void MicroProfileWebServerStop()
{
#ifdef _WIN32
	closesocket(S.ListenerSocket);
	WSACleanup();
#else
	close(S.ListenerSocket);
#endif
}
bool MicroProfileWebServerUpdate()
{
	MICROPROFILE_SCOPEI("MicroProfile", "Webserver-update", -1);
	MpSocket Connection = accept(S.ListenerSocket, 0, 0);
	bool bServed = false;
	if(!MP_INVALID_SOCKET(Connection))
	{
		std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
		char Req[8192];
		MicroProfileSetNonBlocking(Connection, 0);
		int nReceived = recv(Connection, Req, sizeof(Req)-1, 0);
		if(nReceived > 0)
		{
			Req[nReceived] = '\0';
#if MICROPROFILE_DEBUG
			printf("got request \n%s\n", Req);
#endif
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
			char* pHttp = strstr(Req, "HTTP/");
			char* pGet = strstr(Req, "GET / ");
			char* pGetParam = strstr(Req, "GET /?");
			if(pHttp && (pGet || pGetParam))
			{
				int nMaxFrames = MICROPROFILE_WEBSERVER_MAXFRAMES;
				if(pGetParam)
				{
					*pHttp = '\0';
					pGetParam += sizeof("GET /?")-1;
					while(pGetParam) //split url pairs foo=bar&lala=lele etc
					{
						char* pSplit = strstr(pGetParam, "&");
						if(pSplit)
						{
							*pSplit++ = '\0';
						}
						char* pKey = pGetParam;
						char* pValue = strstr(pGetParam, "=");
						if(pValue)
						{
							*pValue++ = '\0';
						}
						if(0 == MP_STRCASECMP(pKey, "frames"))
						{
							if(pValue)
							{
								nMaxFrames = atoi(pValue);
							}
						}
						pGetParam = pSplit;
					}
				}
				uint64_t nTickStart = MP_TICK();
				send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER)-1, 0);
				uint64_t nDataStart = g_nMicroProfileDataSent;
				MicroProfileDumpHtml(MicroProfileWriteSocket, &Connection, nMaxFrames);
				uint64_t nDataEnd = g_nMicroProfileDataSent;
				uint64_t nTickEnd = MP_TICK();
				float fMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu()) * (nTickEnd - nTickStart);
				MicroProfilePrintf(MicroProfileWriteSocket, &Connection, "\n<!-- Sent %dkb in %6.2fms-->\n\n",((nDataEnd-nDataStart)>>10) + 1, fMs);
#if MICROPROFILE_DEBUG
				printf("\nSent %lldkb, in %6.3fms\n\n", ((nDataEnd-nDataStart)>>10) + 1, fMs);
#endif
				bServed = true;
			}
		}
#ifdef _WIN32
		closesocket(Connection);
#else
		close(Connection);
#endif
	}
	return bServed;
}
#endif




#if MICROPROFILE_CONTEXT_SWITCH_TRACE
#ifdef _WIN32
#define INITGUID
#include <evntrace.h>
#include <evntcons.h>
#include <strsafe.h>


static GUID g_MicroProfileThreadClassGuid = { 0x3d6fa8d1, 0xfe05, 0x11d0, 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c };

struct MicroProfileSCSwitch
{
	uint32_t NewThreadId;
	uint32_t OldThreadId;
	int8_t   NewThreadPriority;
	int8_t   OldThreadPriority;
	uint8_t  PreviousCState;
	int8_t   SpareByte;
	int8_t   OldThreadWaitReason;
	int8_t   OldThreadWaitMode;
	int8_t   OldThreadState;
	int8_t   OldThreadWaitIdealProcessor;
	uint32_t NewThreadWaitTime;
	uint32_t Reserved;
};


VOID WINAPI MicroProfileContextSwitchCallback(PEVENT_TRACE pEvent)
{
	if (pEvent->Header.Guid == g_MicroProfileThreadClassGuid)
	{
		if (pEvent->Header.Class.Type == 36)
		{
			MicroProfileSCSwitch* pCSwitch = (MicroProfileSCSwitch*) pEvent->MofData;
			if ((pCSwitch->NewThreadId != 0) || (pCSwitch->OldThreadId != 0))
			{
				MicroProfileContextSwitch Switch;
				Switch.nThreadOut = pCSwitch->OldThreadId;
				Switch.nThreadIn = pCSwitch->NewThreadId;
				Switch.nCpu = pEvent->BufferContext.ProcessorNumber;
				Switch.nTicks = pEvent->Header.TimeStamp.QuadPart;
				MicroProfileContextSwitchPut(&Switch);
			}
		}
	}
}

ULONG WINAPI MicroProfileBufferCallback(PEVENT_TRACE_LOGFILE Buffer)
{
	return (S.bContextSwitchStop || !S.bContextSwitchRunning) ? FALSE : TRUE;
}


struct MicroProfileKernelTraceProperties : public EVENT_TRACE_PROPERTIES
{
	char dummy[sizeof(KERNEL_LOGGER_NAME)];
};

void MicroProfileContextSwitchStopTrace()
{
	TRACEHANDLE SessionHandle = 0;
	MicroProfileKernelTraceProperties sessionProperties;

	ZeroMemory(&sessionProperties, sizeof(sessionProperties));
	sessionProperties.Wnode.BufferSize = sizeof(sessionProperties);
	sessionProperties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	sessionProperties.Wnode.ClientContext = 1; //QPC clock resolution	
	sessionProperties.Wnode.Guid = SystemTraceControlGuid;
	sessionProperties.BufferSize = 1;
	sessionProperties.NumberOfBuffers = 128;
	sessionProperties.EnableFlags = EVENT_TRACE_FLAG_CSWITCH;
	sessionProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	sessionProperties.MaximumFileSize = 0;  
	sessionProperties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	sessionProperties.LogFileNameOffset = 0;

	EVENT_TRACE_LOGFILE log;
	ZeroMemory(&log, sizeof(log));
	log.LoggerName = KERNEL_LOGGER_NAME;
	log.ProcessTraceMode = 0;
	TRACEHANDLE hLog = OpenTrace(&log);
	if (hLog)
	{
		ControlTrace(SessionHandle, KERNEL_LOGGER_NAME, &sessionProperties, EVENT_TRACE_CONTROL_STOP);
	}
	CloseTrace(hLog);


}

void MicroProfileTraceThread(int unused)
{

	MicroProfileContextSwitchStopTrace();
	ULONG status = ERROR_SUCCESS;
	TRACEHANDLE SessionHandle = 0;
	MicroProfileKernelTraceProperties sessionProperties;

	ZeroMemory(&sessionProperties, sizeof(sessionProperties));
	sessionProperties.Wnode.BufferSize = sizeof(sessionProperties);
	sessionProperties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	sessionProperties.Wnode.ClientContext = 1; //QPC clock resolution	
	sessionProperties.Wnode.Guid = SystemTraceControlGuid;
	sessionProperties.BufferSize = 1;
	sessionProperties.NumberOfBuffers = 128;
	sessionProperties.EnableFlags = EVENT_TRACE_FLAG_CSWITCH|EVENT_TRACE_FLAG_PROCESS;
	sessionProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	sessionProperties.MaximumFileSize = 0;  
	sessionProperties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	sessionProperties.LogFileNameOffset = 0;


	status = StartTrace((PTRACEHANDLE) &SessionHandle, KERNEL_LOGGER_NAME, &sessionProperties);

	if (ERROR_SUCCESS != status)
	{
		S.bContextSwitchRunning = false;
		return;
	}

	EVENT_TRACE_LOGFILE log;
	ZeroMemory(&log, sizeof(log));

	log.LoggerName = KERNEL_LOGGER_NAME;
	log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
	log.EventCallback = MicroProfileContextSwitchCallback;
	log.BufferCallback = MicroProfileBufferCallback;

	TRACEHANDLE hLog = OpenTrace(&log);
	ProcessTrace(&hLog, 1, 0, 0);
	CloseTrace(hLog);
	MicroProfileContextSwitchStopTrace();

	S.bContextSwitchRunning = false;
}

void MicroProfileStartContextSwitchTrace()
{
	if(!S.bContextSwitchRunning)
	{
		if(!S.pContextSwitchThread)
			S.pContextSwitchThread = new std::thread();
		if(S.pContextSwitchThread->joinable())
		{
			S.bContextSwitchStop = true;
			S.pContextSwitchThread->join();
		}
		S.bContextSwitchRunning	= true;
		S.bContextSwitchStop = false;
		*S.pContextSwitchThread = std::thread(&MicroProfileTraceThread, 0);
	}
}

void MicroProfileStopContextSwitchTrace()
{
	if(S.bContextSwitchRunning && S.pContextSwitchThread)
	{
		S.bContextSwitchStop = true;
		S.pContextSwitchThread->join();
	}
}

bool MicroProfileIsLocalThread(uint32_t nThreadId) 
{
	HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, nThreadId);
	if(h == NULL)
		return false;
	DWORD hProcess = GetProcessIdOfThread(h);
	CloseHandle(h);
	return GetCurrentProcessId() == hProcess;
}

#else
#error "context switch trace not supported/implemented on platform"
#endif
#else

bool MicroProfileIsLocalThread(uint32_t nThreadId){return false;}
void MicroProfileStopContextSwitchTrace(){}
void MicroProfileStartContextSwitchTrace(){}

#endif


#undef S

#ifdef _WIN32
#pragma warning(pop)
#endif
#endif
#endif

