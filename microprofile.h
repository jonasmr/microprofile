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
//  Gpu time stamps: (See below for d3d/opengl helper)
// 		uint32_t MicroProfileGpuInsertTimeStamp();
// 		uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
// 		uint64_t MicroProfileTicksPerSecondGpu();
//  threading:
//      const char* MicroProfileGetThreadName(); Threadnames in detailed view
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



#ifndef MICROPROFILE_ENABLED
#define MICROPROFILE_ENABLED 1
#endif

#include <stdint.h>
typedef uint64_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;

#if 0 == MICROPROFILE_ENABLED

#define MICROPROFILE_DECLARE(var)
#define MICROPROFILE_DEFINE(var, group, name, color)
#define MICROPROFILE_REGISTER_GROUP(group, color, category)
#define MICROPROFILE_DECLARE_GPU(var)
#define MICROPROFILE_DEFINE_GPU(var, name, color)
#define MICROPROFILE_SCOPE(var) do{}while(0)
#define MICROPROFILE_SCOPEI(group, name, color) do{}while(0)
#define MICROPROFILE_SCOPEGPU(var) do{}while(0)
#define MICROPROFILE_SCOPEGPUI( name, color) do{}while(0)
#define MICROPROFILE_META_CPU(name, count)
#define MICROPROFILE_META_GPU(name, count)
#define MICROPROFILE_FORCEENABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEENABLEGPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLEGPUGROUP(s) do{} while(0)
#define MICROPROFILE_SCOPE_TOKEN(token)

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
#define MicroProfileEnableCategory(a) do{} while(0)
#define MicroProfileDisableCategory(a) do{} while(0)
#define MicroProfileGetEnableAllGroups() false
#define MicroProfileSetForceMetaCounters(a)
#define MicroProfileGetForceMetaCounters() 0
#define MicroProfileEnableMetaCounter(c) do{}while(0)
#define MicroProfileDisableMetaCounter(c) do{}while(0)
#define MicroProfileDumpFile(html,csv) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)

#else

#include <stdint.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <atomic>

#ifndef MICROPROFILE_API
#define MICROPROFILE_API
#endif

MICROPROFILE_API int64_t MicroProfileTicksPerSecondCpu();


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
inline uint64_t MicroProfileGetCurrentThreadId()
{	
	uint64_t tid;
	pthread_threadid_np(pthread_self(), &tid);
	return tid;
}

#define MP_BREAK() __builtin_trap()
#define MP_THREAD_LOCAL __thread
#define MP_STRCASECMP strcasecmp
#define MP_GETCURRENTTHREADID() MicroProfileGetCurrentThreadId()
typedef uint64_t ThreadIdType;
#elif defined(_WIN32)
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


#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)
#define MICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu)
#define MICROPROFILE_REGISTER_GROUP(group, category, color) MicroProfileRegisterGroup(group, category, color)
#define MICROPROFILE_DECLARE_GPU(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE_GPU(var, name, color) MicroProfileToken g_mp_##var = MicroProfileGetToken("GPU", name, color, MicroProfileTokenTypeGpu)
#define MICROPROFILE_TOKEN_PASTE0(a, b) a ## b
#define MICROPROFILE_TOKEN_PASTE(a, b)  MICROPROFILE_TOKEN_PASTE0(a,b)
#define MICROPROFILE_SCOPE(var) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPE_TOKEN(token) MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token)
#define MICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color, MicroProfileTokenTypeCpu); MicroProfileScopeHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_SCOPEGPU(var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEGPUI(name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken("GPU", name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_META_CPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeCpu)
#define MICROPROFILE_META_GPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeGpu)


#ifndef MICROPROFILE_USE_THREAD_NAME_CALLBACK
#define MICROPROFILE_USE_THREAD_NAME_CALLBACK 0
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

#ifndef MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE
#define MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE (16<<10)
#endif

#ifndef MICROPROFILE_GPU_TIMERS
#define MICROPROFILE_GPU_TIMERS 1
#endif

#ifndef MICROPROFILE_GPU_FRAME_DELAY
#define MICROPROFILE_GPU_FRAME_DELAY 3 //must be > 0
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



struct MicroProfile;

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

MICROPROFILE_API void MicroProfileFlip(); //! call once per frame.
MICROPROFILE_API void MicroProfileTogglePause();
MICROPROFILE_API void MicroProfileForceEnableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API void MicroProfileForceDisableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API float MicroProfileGetTime(const char* pGroup, const char* pName);
MICROPROFILE_API void MicroProfileContextSwitchSearch(uint32_t* pContextSwitchStart, uint32_t* pContextSwitchEnd, uint64_t nBaseTicksCpu, uint64_t nBaseTicksEndCpu);
MICROPROFILE_API void MicroProfileOnThreadCreate(const char* pThreadName); //should be called from newly created threads
MICROPROFILE_API void MicroProfileOnThreadExit(); //call on exit to reuse log
MICROPROFILE_API void MicroProfileInitThreadLog();
MICROPROFILE_API void MicroProfileSetForceEnable(bool bForceEnable);
MICROPROFILE_API bool MicroProfileGetForceEnable();
MICROPROFILE_API void MicroProfileSetEnableAllGroups(bool bEnable); 
MICROPROFILE_API void MicroProfileEnableCategory(const char* pCategory); 
MICROPROFILE_API void MicroProfileDisableCategory(const char* pCategory); 
MICROPROFILE_API bool MicroProfileGetEnableAllGroups();
MICROPROFILE_API void MicroProfileSetForceMetaCounters(bool bEnable); 
MICROPROFILE_API bool MicroProfileGetForceMetaCounters();
MICROPROFILE_API void MicroProfileEnableMetaCounter(const char* pMet);
MICROPROFILE_API void MicroProfileDisableMetaCounter(const char* pMet);
MICROPROFILE_API void MicroProfileSetAggregateFrames(int frames);
MICROPROFILE_API int MicroProfileGetAggregateFrames();
MICROPROFILE_API int MicroProfileGetCurrentAggregateFrames();
MICROPROFILE_API MicroProfile* MicroProfileGet();
MICROPROFILE_API void MicroProfileGetRange(uint32_t nPut, uint32_t nGet, uint32_t nRange[2][2]);
MICROPROFILE_API std::recursive_mutex& MicroProfileGetMutex();
MICROPROFILE_API void MicroProfileStartContextSwitchTrace();
MICROPROFILE_API void MicroProfileStopContextSwitchTrace();
MICROPROFILE_API bool MicroProfileIsLocalThread(uint32_t nThreadId);


#if MICROPROFILE_WEBSERVER
MICROPROFILE_API void MicroProfileDumpFile(const char* pHtml, const char* pCsv);
MICROPROFILE_API uint32_t MicroProfileWebServerPort();
#else
#define MicroProfileDumpFile(c) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)
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

#if MICROPROFILE_GPU_TIMERS_D3D11
#define MICROPROFILE_D3D_MAX_QUERIES (8<<10)
MICROPROFILE_API void MicroProfileGpuInitD3D11(void* pDevice, void* pDeviceContext);
#endif

#if MICROPROFILE_GPU_TIMERS_GL
#define MICROPROFILE_GL_MAX_QUERIES (8<<10)
MICROPROFILE_API void MicroProfileGpuInitGL();
#endif



#if MICROPROFILE_USE_THREAD_NAME_CALLBACK
MICROPROFILE_API const char* MicroProfileGetThreadName();
#else
#define MicroProfileGetThreadName() "<implement MicroProfileGetThreadName to get threadnames>"
#endif

#if !defined(MICROPROFILE_THREAD_NAME_FROM_ID)
#define MICROPROFILE_THREAD_NAME_FROM_ID(a) ""
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



#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 48 //dont bump! no. of bits used it bitmask
#define MICROPROFILE_MAX_CATEGORIES 16
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_BUFFER_SIZE)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS 256
#define MICROPROFILE_STACK_MAX 32
//#define MICROPROFILE_MAX_PRESETS 5
#define MICROPROFILE_ANIM_DELAY_PRC 0.5f
#define MICROPROFILE_GAP_TIME 50 //extra ms to fetch to close timers from earlier frames


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

#ifndef MICROPROFILE_DEFAULT_PRESET
#define MICROPROFILE_DEFAULT_PRESET "Default"
#endif


#ifndef MICROPROFILE_CONTEXT_SWITCH_TRACE 
#if defined(_WIN32) 
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 1
#elif defined(__APPLE__)
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0 //disabled until dtrace script is working.
#else
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0
#endif
#endif

#if MICROPROFILE_CONTEXT_SWITCH_TRACE
#define MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE (128*1024) //2mb with 16 byte entry size
#else
#define MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE (1)
#endif

#ifdef _WIN32
#include <basetsd.h>
typedef UINT_PTR MpSocket;
#else
typedef int MpSocket;
#endif


#if defined(__APPLE__) || defined(__linux__)
typedef pthread_t MicroProfileThread;
#elif defined(_WIN32)
typedef HANDLE MicroProfileThread;
#else
typedef std::thread* MicroProfileThread;
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

typedef uint64_t MicroProfileLogEntry;

struct MicroProfileTimer
{
	uint64_t nTicks;
	uint32_t nCount;
};

struct MicroProfileCategory
{
	char pName[MICROPROFILE_NAME_MAX_LEN];
	uint64_t nGroupMask;
};

struct MicroProfileGroupInfo
{
	char pName[MICROPROFILE_NAME_MAX_LEN];
	uint32_t nNameLen;
	uint32_t nGroupIndex;
	uint32_t nNumTimers;
	uint32_t nMaxTimerNameLen;
	uint32_t nColor;
	uint32_t nCategory;
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


	uint8_t					nGroupStackPos[MICROPROFILE_MAX_GROUPS];
	int64_t 				nGroupTicks[MICROPROFILE_MAX_GROUPS];
	int64_t 				nAggregateGroupTicks[MICROPROFILE_MAX_GROUPS];
	enum
	{
		THREAD_MAX_LEN = 64,
	};
	char					ThreadName[64];
	int 					nFreeListNext;
};

#if MICROPROFILE_GPU_TIMERS_D3D11
struct MicroProfileD3D11Frame
{
	uint32_t m_nQueryStart;
	uint32_t m_nQueryCount;
	uint32_t m_nRateQueryStarted;
	void* m_pRateQuery;
};

struct MicroProfileGpuTimerState
{
	uint32_t bInitialized;
	void* m_pDevice;
	void* m_pDeviceContext;
	void* m_pQueries[MICROPROFILE_D3D_MAX_QUERIES];
	int64_t m_nQueryResults[MICROPROFILE_D3D_MAX_QUERIES];
	uint32_t m_nQueryPut;
	uint32_t m_nQueryGet;
	uint32_t m_nQueryFrame;
	int64_t m_nQueryFrequency;
	MicroProfileD3D11Frame m_QueryFrames[MICROPROFILE_GPU_FRAME_DELAY];
};
#elif MICROPROFILE_GPU_TIMERS_GL
struct MicroProfileGpuTimerState
{
	uint32_t GLTimers[MICROPROFILE_GL_MAX_QUERIES];
	uint32_t GLTimerPos;
};
#else
struct MicroProfileGpuTimerState{};
#endif

struct MicroProfile
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nCategoryCount;
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

	uint64_t nActiveGroupWanted;
	uint32_t nAllGroupsWanted;
	uint32_t nAllThreadsWanted;

	uint32_t nOverflow;

	uint64_t nGroupMask;
	uint32_t nRunning;
	uint32_t nToggleRunning;
	uint32_t nMaxGroupSize;
	uint32_t nDumpFileNextFrame;
	uint32_t nAutoClearFrames;
	char HtmlDumpPath[512];
	char CsvDumpPath[512];

	int64_t nPauseTicks;

	float fReferenceTime;
	float fRcpReferenceTime;

	MicroProfileCategory	CategoryInfo[MICROPROFILE_MAX_CATEGORIES];
	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	uint8_t					TimerToGroup[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		AccumTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumMaxTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumTimersExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumMaxTimersExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	uint64_t				FrameExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];	
	uint64_t				AggregateExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMaxExclusive[MICROPROFILE_MAX_TIMERS];


	uint64_t 				FrameGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t 				AccumGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t 				AccumGroupMax[MICROPROFILE_MAX_GROUPS];
	
	uint64_t 				AggregateGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t 				AggregateGroupMax[MICROPROFILE_MAX_GROUPS];


	struct 
	{
		uint64_t nCounters[MICROPROFILE_MAX_TIMERS];

		uint64_t nAccum[MICROPROFILE_MAX_TIMERS];
		uint64_t nAccumMax[MICROPROFILE_MAX_TIMERS];

		uint64_t nAggregate[MICROPROFILE_MAX_TIMERS];
		uint64_t nAggregateMax[MICROPROFILE_MAX_TIMERS];

		uint64_t nSum;
		uint64_t nSumAccum;
		uint64_t nSumAccumMax;
		uint64_t nSumAggregate;
		uint64_t nSumAggregateMax;

		const char* pName;
	} MetaCounters[MICROPROFILE_META_MAX];

	MicroProfileGraphState	Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t				nGraphPut;

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

	uint64_t				nFlipTicks;
	uint64_t				nFlipAggregate;
	uint64_t				nFlipMax;
	uint64_t				nFlipAggregateDisplay;
	uint64_t				nFlipMaxDisplay;

	MicroProfileThread 			ContextSwitchThread;
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
	uint32_t					nWebServerPort;

	char						WebServerBuffer[MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE];
	uint32_t					WebServerPut;

	uint64_t 					nWebServerDataSent;

	MicroProfileGpuTimerState 	GPU;


};

#define MP_LOG_TICK_MASK  0x0000ffffffffffff
#define MP_LOG_INDEX_MASK 0x3fff000000000000
#define MP_LOG_BEGIN_MASK 0xc000000000000000
#define MP_LOG_META 0x2
#define MP_LOG_ENTER 0x1
#define MP_LOG_LEAVE 0x0


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

template<typename T>
T MicroProfileMin(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T MicroProfileMax(T a, T b)
{ return a > b ? a : b; }

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
	return (uint16_t)MicroProfileGet()->TimerToGroup[MicroProfileGetTimerIndex(t)];
}



#ifdef MICROPROFILE_IMPL

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
#define MP_INVALID_SOCKET(f) (f == INVALID_SOCKET)
#endif

#if defined(__APPLE__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#define MP_INVALID_SOCKET(f) (f < 0)
#endif



typedef void* (*MicroProfileThreadFunc)(void*);

#if defined(__APPLE__) || defined(__linux__)
typedef pthread_t MicroProfileThread;
void MicroProfileThreadStart(MicroProfileThread* pThread, MicroProfileThreadFunc Func)
{	
	pthread_attr_t Attr;
	int r  = pthread_attr_init(&Attr);
	MP_ASSERT(r == 0);
	pthread_create(pThread, &Attr, Func, 0);
}
void MicroProfileThreadJoin(MicroProfileThread* pThread)
{
	int r = pthread_join(*pThread, 0);
	MP_ASSERT(r == 0);
}
#elif defined(_WIN32)
typedef HANDLE MicroProfileThread;
DWORD _stdcall ThreadTrampoline(void* pFunc)
{
	MicroProfileThreadFunc F = (MicroProfileThreadFunc)pFunc;
	return (uint32_t)F(0);
}

void MicroProfileThreadStart(MicroProfileThread* pThread, MicroProfileThreadFunc Func)
{	
	*pThread = CreateThread(0, 0, ThreadTrampoline, Func, 0, 0);
}
void MicroProfileThreadJoin(MicroProfileThread* pThread)
{
	WaitForSingleObject(*pThread, INFINITE);
	CloseHandle(*pThread);
}
#else
#include <thread>
typedef std::thread* MicroProfileThread;
inline void MicroProfileThreadStart(MicroProfileThread* pThread, MicroProfileThreadFunc Func)
{
	*pThread = new std::thread(Func, nullptr);
}
inline void MicroProfileThreadJoin(MicroProfileThread* pThread)
{
	(*pThread)->join();
	delete *pThread;
}
#endif
void MicroProfileWebServerStart();
void MicroProfileWebServerStop();
bool MicroProfileWebServerUpdate();
void MicroProfileDumpToFile();

#else

#define MicroProfileWebServerStart() do{}while(0)
#define MicroProfileWebServerStop() do{}while(0)
#define MicroProfileWebServerUpdate() false
#define MicroProfileDumpToFile() do{} while(0)
#endif 


#if MICROPROFILE_GPU_TIMERS_D3D11
void MicroProfileGpuFlip();
void MicroProfileGpuShutdown();
#else
#define MicroProfileGpuFlip() do{}while(0)
#define MicroProfileGpuShutdown() do{}while(0)
#endif



#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>


#ifndef MICROPROFILE_DEBUG
#define MICROPROFILE_DEBUG 0
#endif


#define S g_MicroProfile

MicroProfile g_MicroProfile;
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


MICROPROFILE_DEFINE(g_MicroProfileFlip, "MicroProfile", "MicroProfileFlip", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileThreadLoop, "MicroProfile", "ThreadLoop", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileClear, "MicroProfile", "Clear", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileAccumulate, "MicroProfile", "Accumulate", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchSearch,"MicroProfile", "ContextSwitchSearch", 0xDD7300);

inline std::recursive_mutex& MicroProfileMutex()
{
	static std::recursive_mutex Mutex;
	return Mutex;
}
std::recursive_mutex& MicroProfileGetMutex()
{
	return MicroProfileMutex();
}

MICROPROFILE_API MicroProfile* MicroProfileGet()
{
	return &g_MicroProfile;
}


MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName);


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
		for(int i = 0; i < MICROPROFILE_MAX_CATEGORIES; ++i)
		{
			S.CategoryInfo[i].pName[0] = '\0';
			S.CategoryInfo[i].nGroupMask = 0;
		}
		strcpy(&S.CategoryInfo[0].pName[0], "default");
		S.nCategoryCount = 1;
		for(int i = 0; i < MICROPROFILE_MAX_TIMERS; ++i)
		{
			S.TimerInfo[i].pName[0] = '\0';
		}
		S.nGroupCount = 0;
		S.nAggregateFlipTick = MP_TICK();
		S.nActiveGroup = 0;
		S.nActiveBars = 0;
		S.nForceGroup = 0;
		S.nAllGroupsWanted = 0;
		S.nActiveGroupWanted = 0;
		S.nAllThreadsWanted = 1;
		S.nAggregateFlip = 0;
		S.nTotalTimers = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
		S.nRunning = 1;
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

		S.nWebServerDataSent = (uint64_t)-1;
	}
	if(bUseLock)
		mutex.unlock();
}

void MicroProfileShutdown()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	MicroProfileWebServerStop();
	MicroProfileStopContextSwitchTrace();
	MicroProfileGpuShutdown();
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
		memset(pLog->nGroupStackPos, 0, sizeof(pLog->nGroupStackPos));
		memset(pLog->nGroupTicks, 0, sizeof(pLog->nGroupTicks));
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
		if(!MP_STRCASECMP(pName, S.TimerInfo[i].pName) && !MP_STRCASECMP(pGroup, S.GroupInfo[S.TimerToGroup[i]].pName))
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
	S.GroupInfo[S.nGroupCount].nNumTimers = 0;
	S.GroupInfo[S.nGroupCount].nGroupIndex = S.nGroupCount;
	S.GroupInfo[S.nGroupCount].Type = Type;
	S.GroupInfo[S.nGroupCount].nMaxTimerNameLen = 0;
	S.GroupInfo[S.nGroupCount].nColor = 0x88888888;
	S.GroupInfo[S.nGroupCount].nCategory = 0;
	S.CategoryInfo[0].nGroupMask |= (1ll << (uint64_t)S.nGroupCount);
	nGroupIndex = S.nGroupCount++;
	S.nGroupMask = (S.nGroupMask<<1)|1;
	MP_ASSERT(nGroupIndex < MICROPROFILE_MAX_GROUPS);
	return nGroupIndex;
}

void MicroProfileRegisterGroup(const char* pGroup, const char* pCategory, uint32_t nColor)
{
	int nCategoryIndex = -1;
	for(uint32_t i = 0; i < S.nCategoryCount; ++i)
	{
		if(!MP_STRCASECMP(pCategory, S.CategoryInfo[i].pName))
		{
			nCategoryIndex = (int)i;
			break;
		}
	}
	if(-1 == nCategoryIndex && S.nCategoryCount < MICROPROFILE_MAX_CATEGORIES)
	{
		MP_ASSERT(S.CategoryInfo[S.nCategoryCount].pName[0] == '\0');
		nCategoryIndex = (int)S.nCategoryCount++;
		uint32_t nLen = (uint32_t)strlen(pCategory);
		if(nLen > MICROPROFILE_NAME_MAX_LEN-1)
			nLen = MICROPROFILE_NAME_MAX_LEN-1;
		memcpy(&S.CategoryInfo[nCategoryIndex].pName[0], pCategory, nLen);
		S.CategoryInfo[nCategoryIndex].pName[nLen] = '\0';	
	}
	uint16_t nGroup = MicroProfileGetGroup(pGroup, 0 != MP_STRCASECMP(pGroup, "gpu")?MicroProfileTokenTypeCpu : MicroProfileTokenTypeGpu);
	S.GroupInfo[nGroup].nColor = nColor;
	if(nCategoryIndex >= 0)
	{
		uint64_t nBit = 1ll << nGroup;
		uint32_t nOldCategory = S.GroupInfo[nGroup].nCategory;
		S.CategoryInfo[nOldCategory].nGroupMask &= ~nBit;
		S.CategoryInfo[nCategoryIndex].nGroupMask |= nBit;
		S.GroupInfo[nGroup].nCategory = nCategoryIndex;
	}
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
	S.TimerToGroup[nTimerIndex] = nGroupIndex;
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


	MicroProfileGpuFlip();

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
	uint32_t nAggregateClear = S.nAggregateClear || S.nAutoClearFrames, nAggregateFlip = 0;
	if(S.nDumpFileNextFrame)
	{
		MicroProfileDumpToFile();
		S.nDumpFileNextFrame = 0;
		S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; //hide spike from dumping webpage
	}
	if(S.nWebServerDataSent == (uint64_t)-1)
	{
		MicroProfileWebServerStart();
		S.nWebServerDataSent = 0;
	}

	if(MicroProfileWebServerUpdate())
	{	
		S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; //hide spike from dumping webpage
	}

	if(S.nAutoClearFrames)
	{
		nAggregateClear = 1;
		nAggregateFlip = 1;
		S.nAutoClearFrames -= 1;
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

		uint8_t* pTimerToGroup = &S.TimerToGroup[0];
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
			uint64_t* pFrameGroup = &S.FrameGroup[0];
			{
				MICROPROFILE_SCOPE(g_MicroProfileClear);
				for(uint32_t i = 0; i < S.nTotalTimers; ++i)
				{
					S.Frame[i].nTicks = 0;
					S.Frame[i].nCount = 0;
					S.FrameExclusive[i] = 0;
				}
				for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
				{
					pFrameGroup[i] = 0;
				}
				for(uint32_t j = 0; j < MICROPROFILE_META_MAX; ++j)
				{
					if(S.MetaCounters[j].pName && 0 != (S.nActiveBars & (MP_DRAW_META_FIRST<<j)))
					{
						auto& Meta = S.MetaCounters[j];
						for(uint32_t i = 0; i < S.nTotalTimers; ++i)
						{
							Meta.nCounters[i] = 0;
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

					uint8_t* pGroupStackPos = &pLog->nGroupStackPos[0];
					int64_t nGroupTicks[MICROPROFILE_MAX_GROUPS] = {0};


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
								int nTimer = MicroProfileLogTimerIndex(LE);
								uint8_t nGroup = pTimerToGroup[nTimer];
								MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
								MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
								pGroupStackPos[nGroup]++;
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
								int nTimer = MicroProfileLogTimerIndex(LE);
								uint8_t nGroup = pTimerToGroup[nTimer];
								MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
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

									MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
									uint8_t nGroupStackPos = pGroupStackPos[nGroup];
									if(nGroupStackPos)
									{
										nGroupStackPos--;
										if(0 == nGroupStackPos)
										{
											nGroupTicks[nGroup] += nTicks;
										}
										pGroupStackPos[nGroup] = nGroupStackPos;
									}
								}
							}
						}
					}
					for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
					{
						pLog->nGroupTicks[i] += nGroupTicks[i];
						pFrameGroup[i] += nGroupTicks[i];
					}
					pLog->nStackPos = nStackPos;
				}
			}
			{
				MICROPROFILE_SCOPE(g_MicroProfileAccumulate);
				for(uint32_t i = 0; i < S.nTotalTimers; ++i)
				{
					S.AccumTimers[i].nTicks += S.Frame[i].nTicks;				
					S.AccumTimers[i].nCount += S.Frame[i].nCount;
					S.AccumMaxTimers[i] = MicroProfileMax(S.AccumMaxTimers[i], S.Frame[i].nTicks);
					S.AccumTimersExclusive[i] += S.FrameExclusive[i];				
					S.AccumMaxTimersExclusive[i] = MicroProfileMax(S.AccumMaxTimersExclusive[i], S.FrameExclusive[i]);
				}

				for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
				{
					S.AccumGroup[i] += pFrameGroup[i];
					S.AccumGroupMax[i] = MicroProfileMax(S.AccumGroupMax[i], pFrameGroup[i]);
				}

				for(uint32_t j = 0; j < MICROPROFILE_META_MAX; ++j)
				{
					if(S.MetaCounters[j].pName && 0 != (S.nActiveBars & (MP_DRAW_META_FIRST<<j)))
					{
						auto& Meta = S.MetaCounters[j];
						uint64_t nSum = 0;;
						for(uint32_t i = 0; i < S.nTotalTimers; ++i)
						{
							uint64_t nCounter = Meta.nCounters[i];
							Meta.nAccumMax[i] = MicroProfileMax(Meta.nAccumMax[i], nCounter);
							Meta.nAccum[i] += nCounter;
							nSum += nCounter;
						}
						Meta.nSumAccum += nSum;
						Meta.nSumAccumMax = MicroProfileMax(Meta.nSumAccumMax, nSum);
					}
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
		memcpy(&S.Aggregate[0], &S.AccumTimers[0], sizeof(S.Aggregate[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMax[0], &S.AccumMaxTimers[0], sizeof(S.AggregateMax[0]) * S.nTotalTimers);
		memcpy(&S.AggregateExclusive[0], &S.AccumTimersExclusive[0], sizeof(S.AggregateExclusive[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMaxExclusive[0], &S.AccumMaxTimersExclusive[0], sizeof(S.AggregateMaxExclusive[0]) * S.nTotalTimers);

		memcpy(&S.AggregateGroup[0], &S.AccumGroup[0], sizeof(S.AggregateGroup));
		memcpy(&S.AggregateGroupMax[0], &S.AccumGroupMax[0], sizeof(S.AggregateGroup));		

		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			if(!pLog) 
				continue;
			
			memcpy(&pLog->nAggregateGroupTicks[0], &pLog->nGroupTicks[0], sizeof(pLog->nAggregateGroupTicks));
			
			if(nAggregateClear)
			{
				memset(&pLog->nGroupTicks[0], 0, sizeof(pLog->nGroupTicks));
			}
		}

		for(uint32_t j = 0; j < MICROPROFILE_META_MAX; ++j)
		{
			if(S.MetaCounters[j].pName && 0 != (S.nActiveBars & (MP_DRAW_META_FIRST<<j)))
			{
				auto& Meta = S.MetaCounters[j];
				memcpy(&Meta.nAggregateMax[0], &Meta.nAccumMax[0], sizeof(Meta.nAggregateMax[0]) * S.nTotalTimers);
				memcpy(&Meta.nAggregate[0], &Meta.nAccum[0], sizeof(Meta.nAggregate[0]) * S.nTotalTimers);
				Meta.nSumAggregate = Meta.nSumAccum;
				Meta.nSumAggregateMax = Meta.nSumAccumMax;
				if(nAggregateClear)
				{
					memset(&Meta.nAccumMax[0], 0, sizeof(Meta.nAccumMax[0]) * S.nTotalTimers);
					memset(&Meta.nAccum[0], 0, sizeof(Meta.nAccum[0]) * S.nTotalTimers);
 					Meta.nSumAccum = 0;
 					Meta.nSumAccumMax = 0;
				}
			}
		}





		S.nAggregateFrames = S.nAggregateFlipCount;
		S.nFlipAggregateDisplay = S.nFlipAggregate;
		S.nFlipMaxDisplay = S.nFlipMax;
		if(nAggregateClear)
		{
			memset(&S.AccumTimers[0], 0, sizeof(S.Aggregate[0]) * S.nTotalTimers);
			memset(&S.AccumMaxTimers[0], 0, sizeof(S.AccumMaxTimers[0]) * S.nTotalTimers);
			memset(&S.AccumTimersExclusive[0], 0, sizeof(S.AggregateExclusive[0]) * S.nTotalTimers);
			memset(&S.AccumMaxTimersExclusive[0], 0, sizeof(S.AccumMaxTimersExclusive[0]) * S.nTotalTimers);
			memset(&S.AccumGroup[0], 0, sizeof(S.AggregateGroup));
			memset(&S.AccumGroupMax[0], 0, sizeof(S.AggregateGroup));		

			S.nAggregateFlipCount = 0;
			S.nFlipAggregate = 0;
			S.nFlipMax = 0;

			S.nAggregateFlipTick = MP_TICK();
		}
	}
	S.nAggregateClear = 0;

	uint64_t nNewActiveGroup = 0;
	if(S.nForceEnable || (S.nDisplay && S.nRunning))
		nNewActiveGroup = S.nAllGroupsWanted ? S.nGroupMask : S.nActiveGroupWanted;
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
	S.nAllGroupsWanted = bEnableAllGroups ? 1 : 0;
}

void MicroProfileEnableCategory(const char* pCategory, bool bEnabled)
{
	int nCategoryIndex = -1;
	for(uint32_t i = 0; i < S.nCategoryCount; ++i)
	{
		if(!MP_STRCASECMP(pCategory, S.CategoryInfo[i].pName))
		{
			nCategoryIndex = (int)i;
			break;
		}
	}
	if(nCategoryIndex >= 0)
	{
		if(bEnabled)
		{
			S.nActiveGroupWanted |= S.CategoryInfo[nCategoryIndex].nGroupMask;
		}
		else
		{
			S.nActiveGroupWanted &= ~S.CategoryInfo[nCategoryIndex].nGroupMask;
		}
	}
}


void MicroProfileEnableCategory(const char* pCategory)
{
	MicroProfileEnableCategory(pCategory, true);
}
void MicroProfileDisableCategory(const char* pCategory)
{
	MicroProfileEnableCategory(pCategory, false);
}

bool MicroProfileGetEnableAllGroups()
{
	return 0 != S.nAllGroupsWanted;
}

void MicroProfileSetForceMetaCounters(bool bForce)
{
	S.nForceMetaCounters = bForce ? 1 : 0;
}

bool MicroProfileGetForceMetaCounters()
{
	return 0 != S.nForceMetaCounters;
}

void MicroProfileEnableMetaCounter(const char* pMeta)
{
	for(uint32_t i = 0; i < MICROPROFILE_META_MAX; ++i)
	{
		if(S.MetaCounters[i].pName && 0 == MP_STRCASECMP(S.MetaCounters[i].pName, pMeta))
		{
			S.nBars |= (MP_DRAW_META_FIRST<<i);
			return;
		}
	}
}
void MicroProfileDisableMetaCounter(const char* pMeta)
{
	for(uint32_t i = 0; i < MICROPROFILE_META_MAX; ++i)
	{
		if(S.MetaCounters[i].pName && 0 == MP_STRCASECMP(S.MetaCounters[i].pName, pMeta))
		{
			S.nBars &= ~(MP_DRAW_META_FIRST<<i);
			return;
		}
	}
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


void MicroProfileCalcAllTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, float* pExclusive, float* pAverageExclusive, float* pMaxExclusive, float* pTotal, uint32_t nSize)
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
		float fTotalMs = fToMs * S.Aggregate[nTimer].nTicks;
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
		pTotal[nIdx] = fTotalMs;
		pTotal[nIdx+1] = 0.f;
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


void MicroProfileContextSwitchSearch(uint32_t* pContextSwitchStart, uint32_t* pContextSwitchEnd, uint64_t nBaseTicksCpu, uint64_t nBaseTicksEndCpu)
{
	MICROPROFILE_SCOPE(g_MicroProfileContextSwitchSearch);
	uint32_t nContextSwitchPut = S.nContextSwitchPut;
	uint64_t nContextSwitchStart, nContextSwitchEnd;
	nContextSwitchStart = nContextSwitchEnd = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;		
	int64_t nSearchEnd = nBaseTicksEndCpu + MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());
	int64_t nSearchBegin = nBaseTicksCpu - MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());
	for(uint32_t i = 0; i < MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE; ++i)
	{
		uint32_t nIndex = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - (i+1)) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
		MicroProfileContextSwitch& CS = S.ContextSwitch[nIndex];
		if(CS.nTicks > nSearchEnd)
		{
			nContextSwitchEnd = nIndex;
		}
		if(CS.nTicks > nSearchBegin)
		{
			nContextSwitchStart = nIndex;
		}
	}
	*pContextSwitchStart = nContextSwitchStart;
	*pContextSwitchEnd = nContextSwitchEnd;
}



#if MICROPROFILE_WEBSERVER

#define MICROPROFILE_EMBED_HTML

extern const char* g_MicroProfileHtml_begin[];
extern size_t g_MicroProfileHtml_begin_sizes[];
extern size_t g_MicroProfileHtml_begin_count;
extern const char* g_MicroProfileHtml_end[];
extern size_t g_MicroProfileHtml_end_sizes[];
extern size_t g_MicroProfileHtml_end_count;

typedef void MicroProfileWriteCallback(void* Handle, size_t size, const char* pData);

uint32_t MicroProfileWebServerPort()
{
	return S.nWebServerPort;
}

void MicroProfileDumpFile(const char* pHtml, const char* pCsv)
{
	S.nDumpFileNextFrame = 0;
	if(pHtml)
	{
		uint32_t nLen = strlen(pHtml);
		if(nLen > sizeof(S.HtmlDumpPath)-1)
		{
			return;
		}
		memcpy(S.HtmlDumpPath, pHtml, nLen+1);
		S.nDumpFileNextFrame |= 1;
	}
	if(pCsv)
	{
		uint32_t nLen = strlen(pCsv);
		if(nLen > sizeof(S.CsvDumpPath)-1)
		{
			return;
		}
		memcpy(S.CsvDumpPath, pCsv, nLen+1);
		S.nDumpFileNextFrame |= 2;
	}
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

#define printf(...) MicroProfilePrintf(CB, Handle, __VA_ARGS__)
void MicroProfileDumpCsv(MicroProfileWriteCallback CB, void* Handle, int nMaxFrames)
{
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());

	printf("frames,%d\n", nAggregateFrames);
	printf("group,name,average,max,callaverage\n");

	uint32_t nNumTimers = S.nTotalTimers;
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 8 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	float* pTimersExclusive = pTimers + 4 * nBlockSize;
	float* pAverageExclusive = pTimers + 5 * nBlockSize;
	float* pMaxExclusive = pTimers + 6 * nBlockSize;
	float* pTotal = pTimers + 7 * nBlockSize;

	MicroProfileCalcAllTimers(pTimers, pAverage, pMax, pCallAverage, pTimersExclusive, pAverageExclusive, pMaxExclusive, pTotal, nNumTimers);

	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t nIdx = i * 2;
		printf("\"%s\",\"%s\",%f,%f,%f\n", S.TimerInfo[i].pName, S.GroupInfo[S.TimerInfo[i].nGroupIndex].pName, pAverage[nIdx], pMax[nIdx], pCallAverage[nIdx]);
	}

	printf("\n\n");

	printf("group,average,max,total\n");
	for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		const char* pGroupName = S.GroupInfo[j].pName;
		float fToMs =  S.GroupInfo[j].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;
		if(pGroupName[0] != '\0')
		{
			printf("\"%s\",%.3f,%.3f,%.3f\n", pGroupName, fToMs * S.AggregateGroup[j] / nAggregateFrames, fToMs * S.AggregateGroup[j] / nAggregateFrames, fToMs * S.AggregateGroup[j]);
		}
	}

	printf("\n\n");
	printf("group,thread,average,total\n");
	for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		for(uint32_t i = 0; i < S.nNumLogs; ++i)
		{
			if(S.Pool[i])
			{
				const char* pThreadName = &S.Pool[i]->ThreadName[0];
				// MicroProfilePrintf(CB, Handle, "var ThreadGroupTime%d = [", i);
				float fToMs = S.Pool[i]->nGpu ? fToMsGPU : fToMsCPU;
				{
					uint64_t nTicks = S.Pool[i]->nAggregateGroupTicks[j];
					float fTime = nTicks / nAggregateFrames * fToMs;
					float fTimeTotal = nTicks * fToMs;
					if(fTimeTotal > 0.01f)
					{
						const char* pGroupName = S.GroupInfo[j].pName;
						printf("\"%s\",\"%s\",%.3f,%.3f\n", pGroupName, pThreadName, fTime, fTimeTotal);
					}
				}
			}
		}
	}

	printf("\n\n");
	printf("frametimecpu\n");

	const uint32_t nCount = MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 3;
	const uint32_t nStart = S.nFrameCurrent;
	for(uint32_t i = nCount; i > 0; i--)
	{
		uint32_t nFrame = (nStart + MICROPROFILE_MAX_FRAME_HISTORY - i) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint32_t nFrameNext = (nStart + MICROPROFILE_MAX_FRAME_HISTORY - i + 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint64_t nTicks = S.Frames[nFrameNext].nFrameStartCpu - S.Frames[nFrame].nFrameStartCpu;
		printf("%f,", nTicks * fToMsCPU);
	}
	printf("\n");

	printf("\n\n");
	printf("frametimegpu\n");

	for(uint32_t i = nCount; i > 0; i--)
	{
		uint32_t nFrame = (nStart + MICROPROFILE_MAX_FRAME_HISTORY - i) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint32_t nFrameNext = (nStart + MICROPROFILE_MAX_FRAME_HISTORY - i + 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint64_t nTicks = S.Frames[nFrameNext].nFrameStartGpu - S.Frames[nFrame].nFrameStartGpu;
		printf("%f,", nTicks * fToMsGPU);
	}
	printf("\n\n");
	printf("Meta\n");//only single frame snapshot
	printf("name,average,max,total\n");
	for(int j = 0; j < MICROPROFILE_META_MAX; ++j)
	{
		if(S.MetaCounters[j].pName)
		{
			printf("\"%s\",%f,%lld,%lld\n",S.MetaCounters[j].pName, S.MetaCounters[j].nSumAggregate / (float)nAggregateFrames, S.MetaCounters[j].nSumAggregateMax,S.MetaCounters[j].nSumAggregate);
		}
	}
}
#undef printf

void MicroProfileDumpHtml(MicroProfileWriteCallback CB, void* Handle, int nMaxFrames)
{
	uint32_t nRunning = S.nRunning;
	S.nRunning = 0;
	//stall pushing of timers
	uint64_t nActiveGroup = S.nActiveGroup;
	S.nActiveGroup = 0;
	S.nPauseTicks = MP_TICK();


	for(size_t i = 0; i < g_MicroProfileHtml_begin_count; ++i)
	{
		CB(Handle, g_MicroProfileHtml_begin_sizes[i]-1, g_MicroProfileHtml_begin[i]);
	}
	//dump info
	uint64_t nTicks = MP_TICK();
	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
	float fAggregateMs = fToMsCPU * (nTicks - S.nAggregateFlipTick);
	MicroProfilePrintf(CB, Handle, "var AggregateInfo = {'Frames':%d, 'Time':%f};\n", S.nAggregateFrames, fAggregateMs);

	//categories
	MicroProfilePrintf(CB, Handle, "var CategoryInfo = Array(%d);\n",S.nCategoryCount);
	for(uint32_t i = 0; i < S.nCategoryCount; ++i)
	{
		MicroProfilePrintf(CB, Handle, "CategoryInfo[%d] = \"%s\";\n", i, S.CategoryInfo[i].pName);
	}

	//groups
	MicroProfilePrintf(CB, Handle, "var GroupInfo = Array(%d);\n\n",S.nGroupCount);
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	float fRcpAggregateFrames = 1.f / nAggregateFrames;
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		MP_ASSERT(i == S.GroupInfo[i].nGroupIndex);
		float fToMs = S.GroupInfo[i].Type == MicroProfileTokenTypeCpu ? fToMsCPU : fToMsGPU;
		MicroProfilePrintf(CB, Handle, "GroupInfo[%d] = MakeGroup(%d, \"%s\", %d, %d, %d, %f, %f, %f, '#%02x%02x%02x');\n", 
			S.GroupInfo[i].nGroupIndex, 
			S.GroupInfo[i].nGroupIndex, 
			S.GroupInfo[i].pName, 
			S.GroupInfo[i].nCategory, 
			S.GroupInfo[i].nNumTimers, 
			S.GroupInfo[i].Type == MicroProfileTokenTypeGpu?1:0, 
			fToMs * S.AggregateGroup[i], 
			fToMs * S.AggregateGroup[i] / nAggregateFrames, 
			fToMs * S.AggregateGroupMax[i],
			MICROPROFILE_UNPACK_RED(S.GroupInfo[i].nColor) & 0xff,
			MICROPROFILE_UNPACK_GREEN(S.GroupInfo[i].nColor) & 0xff,
			MICROPROFILE_UNPACK_BLUE(S.GroupInfo[i].nColor) & 0xff);
	}
	//timers

	uint32_t nNumTimers = S.nTotalTimers;
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 8 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	float* pTimersExclusive = pTimers + 4 * nBlockSize;
	float* pAverageExclusive = pTimers + 5 * nBlockSize;
	float* pMaxExclusive = pTimers + 6 * nBlockSize;
	float* pTotal = pTimers + 7 * nBlockSize;

	MicroProfileCalcAllTimers(pTimers, pAverage, pMax, pCallAverage, pTimersExclusive, pAverageExclusive, pMaxExclusive, pTotal, nNumTimers);

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
		MicroProfilePrintf(CB, Handle, "var MetaAvg%d = [", i);
		bOnce = true;
		for(int j = 0; j < MICROPROFILE_META_MAX; ++j)
		{
			if(S.MetaCounters[j].pName)
			{
				MicroProfilePrintf(CB, Handle, bOnce ? "%f" : ",%f", fRcpAggregateFrames * S.MetaCounters[j].nAggregate[i]);
				bOnce = false;
			}
		}
		MicroProfilePrintf(CB, Handle, "];\n");
		MicroProfilePrintf(CB, Handle, "var MetaMax%d = [", i);
		bOnce = true;
		for(int j = 0; j < MICROPROFILE_META_MAX; ++j)
		{
			if(S.MetaCounters[j].pName)
			{
				MicroProfilePrintf(CB, Handle, bOnce ? "%d" : ",%d", S.MetaCounters[j].nAggregateMax[i]);
				bOnce = false;
			}
		}
		MicroProfilePrintf(CB, Handle, "];\n");



		MicroProfilePrintf(CB, Handle, "TimerInfo[%d] = MakeTimer(%d, \"%s\", %d, '#%02x%02x%02x', %f, %f, %f, %f, %f, %d, %f, Meta%d, MetaAvg%d, MetaMax%d);\n", S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].pName, S.TimerInfo[i].nGroupIndex, 
			MICROPROFILE_UNPACK_RED(S.TimerInfo[i].nColor) & 0xff,
			MICROPROFILE_UNPACK_GREEN(S.TimerInfo[i].nColor) & 0xff,
			MICROPROFILE_UNPACK_BLUE(S.TimerInfo[i].nColor) & 0xff,
			pAverage[nIdx],
			pMax[nIdx],
			pAverageExclusive[nIdx],
			pMaxExclusive[nIdx],
			pCallAverage[nIdx],
			S.Aggregate[i].nCount,
			pTotal[nIdx],
			i,i,i);

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


	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "var ThreadGroupTime%d = [", i);
			float fToMs = S.Pool[i]->nGpu ? fToMsGPU : fToMsCPU;
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				MicroProfilePrintf(CB, Handle, "%f,", S.Pool[i]->nAggregateGroupTicks[j]/nAggregateFrames * fToMs);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
		}
	}
	MicroProfilePrintf(CB, Handle, "\nvar ThreadGroupTimeArray = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "ThreadGroupTime%d,", i);
		}
	}
	MicroProfilePrintf(CB, Handle, "];\n");


	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "var ThreadGroupTimeTotal%d = [", i);
			float fToMs = S.Pool[i]->nGpu ? fToMsGPU : fToMsCPU;
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				MicroProfilePrintf(CB, Handle, "%f,", S.Pool[i]->nAggregateGroupTicks[j] * fToMs);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
		}
	}
	MicroProfilePrintf(CB, Handle, "\nvar ThreadGroupTimeTotalArray = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "ThreadGroupTimeTotal%d,", i);
		}
	}
	MicroProfilePrintf(CB, Handle, "];");




	MicroProfilePrintf(CB, Handle, "\nvar ThreadIds = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			ThreadIdType ThreadId = S.Pool[i]->nThreadId;
			if(!ThreadId)
			{
				ThreadId = (ThreadIdType)-1;
			}
			MicroProfilePrintf(CB, Handle, "%d,", ThreadId);
		}
		else
		{
			MicroProfilePrintf(CB, Handle, "-1,", i);
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



	uint32_t nNumFrames = (MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 3); //leave a few to not overwrite
	nNumFrames = MicroProfileMin(nNumFrames, (uint32_t)nMaxFrames);


	uint32_t nFirstFrame = (S.nFrameCurrent + MICROPROFILE_MAX_FRAME_HISTORY - nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	uint32_t nLastFrame = (nFirstFrame + nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	MP_ASSERT(nLastFrame == (S.nFrameCurrent % MICROPROFILE_MAX_FRAME_HISTORY));
	MP_ASSERT(nFirstFrame < MICROPROFILE_MAX_FRAME_HISTORY);
	MP_ASSERT(nLastFrame  < MICROPROFILE_MAX_FRAME_HISTORY);
	const int64_t nTickStart = S.Frames[nFirstFrame].nFrameStartCpu;
	const int64_t nTickEnd = S.Frames[nLastFrame].nFrameStartCpu;
	int64_t nTickStartGpu = S.Frames[nFirstFrame].nFrameStartGpu;
#if MICROPROFILE_DEBUG
	printf("dumping %d frames\n", nNumFrames);
	printf("dumping frame %d to %d\n", nFirstFrame, nLastFrame);
#endif


	uint32_t* nTimerCounter = (uint32_t*)alloca(sizeof(uint32_t)* S.nTotalTimers);
	memset(nTimerCounter, 0, sizeof(uint32_t) * S.nTotalTimers);

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
					uint32_t nTimerIndex = (uint32_t)MicroProfileLogTimerIndex(pLog->Log[k]);
					MicroProfilePrintf(CB, Handle, ",%d", nTimerIndex);
					nTimerCounter[nTimerIndex]++;
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
		MicroProfilePrintf(CB, Handle, "Frames[%d] = MakeFrame(%d, %f, %f, ts%d, tt%d, ti%d);\n", i, 0, fFrameMs, fFrameEndMs, i, i, i);
	}
	
	uint32_t nContextSwitchStart = 0;
	uint32_t nContextSwitchEnd = 0;
	MicroProfileContextSwitchSearch(&nContextSwitchStart, &nContextSwitchEnd, nTickStart, nTickEnd);

	uint32_t nWrittenBefore = S.nWebServerDataSent;
	MicroProfilePrintf(CB, Handle, "var CSwitchThreadInOutCpu = [");
	for(uint32_t j = nContextSwitchStart; j != nContextSwitchEnd; j = (j+1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE)
	{
		MicroProfileContextSwitch CS = S.ContextSwitch[j];
		int nCpu = CS.nCpu;
		MicroProfilePrintf(CB, Handle, "%d,%d,%d,", CS.nThreadIn, CS.nThreadOut, nCpu);
	}
	MicroProfilePrintf(CB, Handle, "];\n");
	MicroProfilePrintf(CB, Handle, "var CSwitchTime = [");
	float fToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	for(uint32_t j = nContextSwitchStart; j != nContextSwitchEnd; j = (j+1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE)
	{
		MicroProfileContextSwitch CS = S.ContextSwitch[j];
		float fTime = MicroProfileLogTickDifference(nTickStart, CS.nTicks) * fToMsCpu;
		MicroProfilePrintf(CB, Handle, "%f,", fTime);
	}
	MicroProfilePrintf(CB, Handle, "];\n");
	uint32_t nWrittenAfter = S.nWebServerDataSent;
	MicroProfilePrintf(CB, Handle, "//CSwitch Size %d\n", nWrittenAfter - nWrittenBefore);


	for(size_t i = 0; i < g_MicroProfileHtml_end_count; ++i)
	{
		CB(Handle, g_MicroProfileHtml_end_sizes[i]-1, g_MicroProfileHtml_end[i]);
	}

	uint32_t* nGroupCounter = (uint32_t*)alloca(sizeof(uint32_t)* S.nGroupCount);

	memset(nGroupCounter, 0, sizeof(uint32_t) * S.nGroupCount);
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t nGroupIndex = S.TimerInfo[i].nGroupIndex;
		nGroupCounter[nGroupIndex] += nTimerCounter[i];
	}

	uint32_t* nGroupCounterSort = (uint32_t*)alloca(sizeof(uint32_t)* S.nGroupCount);
	uint32_t* nTimerCounterSort = (uint32_t*)alloca(sizeof(uint32_t)* S.nTotalTimers);
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		nGroupCounterSort[i] = i;
	}
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		nTimerCounterSort[i] = i;
	}
	std::sort(nGroupCounterSort, nGroupCounterSort + S.nGroupCount, 
		[nGroupCounter](const uint32_t l, const uint32_t r)
		{
			return nGroupCounter[l] > nGroupCounter[r];
		}
	);

	std::sort(nTimerCounterSort, nTimerCounterSort + S.nTotalTimers, 
		[nTimerCounter](const uint32_t l, const uint32_t r)
		{
			return nTimerCounter[l] > nTimerCounter[r];
		}
	);

	MicroProfilePrintf(CB, Handle, "\n<!--\nMarker Per Group\n");
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		uint32_t idx = nGroupCounterSort[i];
		MicroProfilePrintf(CB, Handle, "%8d:%s\n", nGroupCounter[idx], S.GroupInfo[idx].pName);
	}
	MicroProfilePrintf(CB, Handle, "Marker Per Timer\n");
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t idx = nTimerCounterSort[i];
		MicroProfilePrintf(CB, Handle, "%8d:%s(%s)\n", nTimerCounter[idx], S.TimerInfo[idx].pName, S.GroupInfo[S.TimerInfo[idx].nGroupIndex].pName);
	}
	MicroProfilePrintf(CB, Handle, "\n-->\n");

	S.nActiveGroup = nActiveGroup;
	S.nRunning = nRunning;

#if MICROPROFILE_DEBUG
	int64_t nTicksEnd = MP_TICK();
	float fMs = fToMsCpu * (nTicksEnd - S.nPauseTicks);
	printf("html dump took %6.2fms\n", fMs);
#endif


}

void MicroProfileWriteFile(void* Handle, size_t nSize, const char* pData)
{
	fwrite(pData, nSize, 1, (FILE*)Handle);
}

void MicroProfileDumpToFile()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	if(S.nDumpFileNextFrame&1)
	{
		FILE* F = fopen(S.HtmlDumpPath, "w");
		if(F)
		{
			MicroProfileDumpHtml(MicroProfileWriteFile, F, MICROPROFILE_WEBSERVER_MAXFRAMES);
			fclose(F);
		}
	}
	if(S.nDumpFileNextFrame&2)
	{
		FILE* F = fopen(S.CsvDumpPath, "w");
		if(F)
		{
			MicroProfileDumpCsv(MicroProfileWriteFile, F, MICROPROFILE_WEBSERVER_MAXFRAMES);
			fclose(F);
		}
	}
}

void MicroProfileFlushSocket(MpSocket Socket)
{
	send(Socket, &S.WebServerBuffer[0], S.WebServerPut, 0);
	S.WebServerPut = 0;

}

void MicroProfileWriteSocket(void* Handle, size_t nSize, const char* pData)
{
	S.nWebServerDataSent += nSize;
	MpSocket Socket = *(MpSocket*)Handle;
	if(nSize > MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE / 2)
	{
		MicroProfileFlushSocket(Socket);
		send(Socket, pData, nSize, 0);

	}
	else
	{
		memcpy(&S.WebServerBuffer[S.WebServerPut], pData, nSize);
		S.WebServerPut += nSize;
		if(S.WebServerPut > MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE/2)
		{
			MicroProfileFlushSocket(Socket);
		}
	}
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

	S.nWebServerPort = (uint32_t)-1;
	struct sockaddr_in Addr; 
	Addr.sin_family = AF_INET; 
	Addr.sin_addr.s_addr = INADDR_ANY; 
	for(int i = 0; i < 20; ++i)
	{
		Addr.sin_port = htons(MICROPROFILE_WEBSERVER_PORT+i); 
		if(0 == bind(S.ListenerSocket, (sockaddr*)&Addr, sizeof(Addr)))
		{
			S.nWebServerPort = MICROPROFILE_WEBSERVER_PORT+i;
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
// 			printf("got request \n%s\n", Req);
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
				uint64_t nDataStart = S.nWebServerDataSent;
				S.WebServerPut = 0;
				MicroProfileDumpHtml(MicroProfileWriteSocket, &Connection, nMaxFrames);
				uint64_t nDataEnd = S.nWebServerDataSent;
				uint64_t nTickEnd = MP_TICK();
				uint64_t nDiff = (nTickEnd - nTickStart);
				float fMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu()) * nDiff;
				int nKb = ((nDataEnd-nDataStart)>>10) + 1;
				MicroProfilePrintf(MicroProfileWriteSocket, &Connection, "\n<!-- Sent %dkb in %.2fms-->\n\n",nKb, fMs);
				MicroProfileFlushSocket(Connection);
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
//functions that need to be implemented per platform.
void* MicroProfileTraceThread(void* unused);
bool MicroProfileIsLocalThread(uint32_t nThreadId);


void MicroProfileStartContextSwitchTrace()
{
    if(!S.bContextSwitchRunning)
    {
        S.bContextSwitchRunning    = true;
        S.bContextSwitchStop = false;
        MicroProfileThreadStart(&S.ContextSwitchThread, MicroProfileTraceThread);
    }
}

void MicroProfileStopContextSwitchTrace()
{
    if(S.bContextSwitchRunning)
    {
        S.bContextSwitchStop = true;
        MicroProfileThreadJoin(&S.ContextSwitchThread);
    }
}


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

void MicroProfileContextSwitchShutdownTrace()
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

void* MicroProfileTraceThread(void* unused)
{

	MicroProfileContextSwitchShutdownTrace();
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
		return 0;
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
	MicroProfileContextSwitchShutdownTrace();

	S.bContextSwitchRunning = false;
	return 0;
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

#elif defined(__APPLE__)
#include <sys/time.h>
void* MicroProfileTraceThread(void* unused)
{
	FILE* pFile = fopen("mypipe", "r");
	if(!pFile)
	{
		printf("CONTEXT SWITCH FAILED TO OPEN FILE: make sure to run dtrace script\n");
		S.bContextSwitchRunning = false;
		return 0;
	}
	printf("STARTING TRACE THREAD\n");
	char* pLine = 0;
	size_t cap = 0;
	size_t len = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uint64_t nsSinceEpoch = ((uint64_t)(tv.tv_sec) * 1000000 + (uint64_t)(tv.tv_usec)) * 1000;
	uint64_t nTickEpoch = MP_TICK();
	uint32_t nLastThread[MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS] = {0};
	mach_timebase_info_data_t sTimebaseInfo;	
	mach_timebase_info(&sTimebaseInfo);
	S.bContextSwitchRunning = true;

	uint64_t nProcessed = 0;
	uint64_t nProcessedLast = 0;
	while((len = getline(&pLine, &cap, pFile))>0 && !S.bContextSwitchStop)
	{
		nProcessed += len;
		if(nProcessed - nProcessedLast > 10<<10)
		{
			nProcessedLast = nProcessed;
			printf("processed %llukb %llukb\n", (nProcessed-nProcessedLast)>>10,nProcessed >>10);
		}

		char* pX = strchr(pLine, 'X');
		if(pX)
		{
			int cpu = atoi(pX+1);
			char* pX2 = strchr(pX + 1, 'X');
			char* pX3 = strchr(pX2 + 1, 'X');
			int thread = atoi(pX2+1);
			char* lala;
			int64_t timestamp = strtoll(pX3 + 1, &lala, 10);
			MicroProfileContextSwitch Switch;

			//convert to ticks.
			uint64_t nDeltaNsSinceEpoch = timestamp - nsSinceEpoch;
			uint64_t nDeltaTickSinceEpoch = sTimebaseInfo.numer * nDeltaNsSinceEpoch / sTimebaseInfo.denom;
			uint64_t nTicks = nDeltaTickSinceEpoch + nTickEpoch;
			if(cpu < MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS)
			{
				Switch.nThreadOut = nLastThread[cpu];
				Switch.nThreadIn = thread;
				nLastThread[cpu] = thread;
				Switch.nCpu = cpu;
				Switch.nTicks = nTicks;
				MicroProfileContextSwitchPut(&Switch);
			}
		}
	}
	printf("EXITING TRACE THREAD\n");
	S.bContextSwitchRunning = false;
	return 0;
}

bool MicroProfileIsLocalThread(uint32_t nThreadId) 
{
	return false;
}

#endif
#else

bool MicroProfileIsLocalThread(uint32_t nThreadId){return false;}
void MicroProfileStopContextSwitchTrace(){}
void MicroProfileStartContextSwitchTrace(){}

#endif




#if MICROPROFILE_GPU_TIMERS_D3D11
uint32_t MicroProfileGpuInsertTimeStamp()
{
	MicroProfileD3D11Frame& Frame = S.GPU.m_QueryFrames[S.GPU.m_nQueryFrame];
	if(Frame.m_nRateQueryStarted)
	{
		uint32_t nCurrent = (Frame.m_nQueryStart + Frame.m_nQueryCount) % MICROPROFILE_D3D_MAX_QUERIES;
		uint32_t nNext = (nCurrent + 1) % MICROPROFILE_D3D_MAX_QUERIES;
		if(nNext != S.GPU.m_nQueryGet)
		{
			Frame.m_nQueryCount++;
			ID3D11Query* pQuery = (ID3D11Query*)S.GPU.m_pQueries[nCurrent];
			ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)S.GPU.m_pDeviceContext;
			pContext->End(pQuery);
			S.GPU.m_nQueryPut = nNext;
			return nCurrent;
		}
	}
	return (uint32_t)-1;
}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nIndex)
{
	if(nIndex == (uint32_t)-1)
	{
		return (uint64_t)-1;
	}
	int64_t nResult = S.GPU.m_nQueryResults[nIndex];
	MP_ASSERT(nResult != -1);
	return nResult;	
}

bool MicroProfileGpuGetData(void* pQuery, void* pData, uint32_t nDataSize)
{
	HRESULT hr;
	do
	{
		hr = ((ID3D11DeviceContext*)S.GPU.m_pDeviceContext)->GetData((ID3D11Query*)pQuery, pData, nDataSize, 0);
	}while(hr == S_FALSE);
	switch(hr)
	{
		case DXGI_ERROR_DEVICE_REMOVED:
		case DXGI_ERROR_INVALID_CALL:
		case E_INVALIDARG:
			MP_BREAK();
			return false;

	}
	return true;
}

uint64_t MicroProfileTicksPerSecondGpu()
{
	return S.GPU.m_nQueryFrequency;
}

void MicroProfileGpuFlip()
{
	MicroProfileD3D11Frame& CurrentFrame = S.GPU.m_QueryFrames[S.GPU.m_nQueryFrame];
	ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)S.GPU.m_pDeviceContext;
	if(CurrentFrame.m_nRateQueryStarted)
	{
		pContext->End((ID3D11Query*)CurrentFrame.m_pRateQuery);
	}
	uint32_t nNextFrame = (S.GPU.m_nQueryFrame + 1) % MICROPROFILE_GPU_FRAME_DELAY;
	MicroProfileD3D11Frame& OldFrame = S.GPU.m_QueryFrames[nNextFrame];
	if(OldFrame.m_nRateQueryStarted)
	{
		struct RateQueryResult
		{
			uint64_t nFrequency;
			BOOL bDisjoint;
		};
		RateQueryResult Result;
		if(MicroProfileGpuGetData(OldFrame.m_pRateQuery, &Result, sizeof(Result)))
		{
			if(S.GPU.m_nQueryFrequency != (int64_t)Result.nFrequency)
			{
				if(S.GPU.m_nQueryFrequency)
				{
					OutputDebugString("Query freq changing");
				}
				S.GPU.m_nQueryFrequency = Result.nFrequency;
			}
			uint32_t nStart = OldFrame.m_nQueryStart;
			uint32_t nCount = OldFrame.m_nQueryCount;
			for(uint32_t i = 0; i < nCount; ++i)
			{
				uint32_t nIndex = (i + nStart) % MICROPROFILE_D3D_MAX_QUERIES;



				if(!MicroProfileGpuGetData(S.GPU.m_pQueries[nIndex], &S.GPU.m_nQueryResults[nIndex], sizeof(uint64_t)))
				{
					S.GPU.m_nQueryResults[nIndex] = -1;
				}
			}
		}
		else
		{
			uint32_t nStart = OldFrame.m_nQueryStart;
			uint32_t nCount = OldFrame.m_nQueryCount;
			for(uint32_t i = 0; i < nCount; ++i)
			{
				uint32_t nIndex = (i + nStart) % MICROPROFILE_D3D_MAX_QUERIES;
				S.GPU.m_nQueryResults[nIndex] = -1;
			}
		}
		S.GPU.m_nQueryGet = (OldFrame.m_nQueryStart + OldFrame.m_nQueryCount) % MICROPROFILE_D3D_MAX_QUERIES;
	}

	S.GPU.m_nQueryFrame = nNextFrame;
	MicroProfileD3D11Frame& NextFrame = S.GPU.m_QueryFrames[nNextFrame];
	pContext->Begin((ID3D11Query*)NextFrame.m_pRateQuery);
	NextFrame.m_nQueryStart = S.GPU.m_nQueryPut;
	NextFrame.m_nQueryCount = 0;

	NextFrame.m_nRateQueryStarted = 1;
}

void MicroProfileGpuInitD3D11(void* pDevice_, void* pDeviceContext_)
{
	ID3D11Device* pDevice = (ID3D11Device*)pDevice_;
	ID3D11DeviceContext* pDeviceContext = (ID3D11DeviceContext*)pDeviceContext_;
	S.GPU.m_pDeviceContext = pDeviceContext_;

	D3D11_QUERY_DESC Desc;
	Desc.MiscFlags = 0;
	Desc.Query = D3D11_QUERY_TIMESTAMP;
	for(uint32_t i = 0; i < MICROPROFILE_D3D_MAX_QUERIES; ++i)
	{
		HRESULT hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.GPU.m_pQueries[i]);
		MP_ASSERT(hr == S_OK);
		S.GPU.m_nQueryResults[i] = -1;
	}
	S.GPU.m_nQueryPut = 0;
	S.GPU.m_nQueryGet = 0;
	S.GPU.m_nQueryFrame = 0;
	S.GPU.m_nQueryFrequency = 0;
	Desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY; ++i)
	{
		S.GPU.m_QueryFrames[i].m_nQueryStart = 0;
		S.GPU.m_QueryFrames[i].m_nQueryCount = 0;
		S.GPU.m_QueryFrames[i].m_nRateQueryStarted = 0;
		HRESULT hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.GPU.m_QueryFrames[i].m_pRateQuery);
		MP_ASSERT(hr == S_OK);
	}
}


void MicroProfileGpuShutdown()
{
	for(uint32_t i = 0; i < MICROPROFILE_D3D_MAX_QUERIES; ++i)
	{
		((ID3D11Query*)&S.GPU.m_pQueries[i])->Release();
		S.GPU.m_pQueries[i] = 0;
	}
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY; ++i)
	{
		((ID3D11Query*)S.GPU.m_QueryFrames[i].m_pRateQuery)->Release();
		S.GPU.m_QueryFrames[i].m_pRateQuery = 0;
	}
}
#elif MICROPROFILE_GPU_TIMERS_GL
void MicroProfileGpuInitGL()
{
	S.GPU.GLTimerPos = 0;
	glGenQueries(MICROPROFILE_GL_MAX_QUERIES, &S.GPU.GLTimers[0]);		
}

uint32_t MicroProfileGpuInsertTimeStamp()
{
	uint32_t nIndex = (S.GPU.GLTimerPos+1)%MICROPROFILE_GL_MAX_QUERIES;
	glQueryCounter(S.GPU.GLTimers[nIndex], GL_TIMESTAMP);
	S.GPU.GLTimerPos = nIndex;
	return nIndex;
}
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey)
{
	uint64_t result;
	glGetQueryObjectui64v(S.GPU.GLTimers[nKey], GL_QUERY_RESULT, &result);
	return result;
}

uint64_t MicroProfileTicksPerSecondGpu()
{
	return 1000000000ll;
}

#endif

#undef S

#ifdef _WIN32
#pragma warning(pop)
#endif





#endif
#endif



///start embedded file from microprofile.html
#ifdef MICROPROFILE_EMBED_HTML
const char g_MicroProfileHtml_begin_0[] =
"<!DOCTYPE HTML>\n"
"<html>\n"
"<head>\n"
"<title>MicroProfile Capture</title>\n"
"<style>\n"
"/* about css: http://bit.ly/1eMQ42U */\n"
"body {margin: 0px;padding: 0px; font: 12px Courier New;background-color:#474747; color:white;overflow:hidden;}\n"
"ul {list-style-type: none;margin: 0;padding: 0;}\n"
"li{display: inline; float:left;border:5px; position:relative;text-align:center;}\n"
"a {\n"
"    float:left;\n"
"    text-decoration:none;\n"
"    display: inline;\n"
"    text-align: center;\n"
"	padding:5px;\n"
"	padding-bottom:0px;\n"
"	padding-top:0px;\n"
"    color: #FFFFFF;\n"
"    background-color: #474747;\n"
"}\n"
"a:hover, a:active{\n"
"	background-color: #000000;\n"
"}\n"
"\n"
"ul ul {\n"
"    position:absolute;\n"
"    left:0;\n"
"    top:100%;\n"
"    margin-left:-999em;\n"
"}\n"
"li:hover ul {\n"
"    margin-left:0;\n"
"    margin-right:0;\n"
"}\n"
"ul li ul{ display:block;float:none;width:100%;}\n"
"ul li ul li{ display:block;float:none;width:100%;}\n"
"li li a{ display:block;float:none;width:100%;text-align:left;}\n"
"#nav li:hover div {margin-left:0;}\n"
".help {position:absolute;z-index:5;text-align:left;padding:2px;margin-left:-999em;background-color: #313131;}\n"
".root {z-index:1;position:absolute;top:0px;left:0px;}\n"
"</style>\n"
"</head>\n"
"<body style=\"\">\n"
"<canvas id=\"History\" height=\"70\" style=\"background-color:#474747;margin:0px;padding:0px;\"></canvas><canvas id=\"DetailedView\" height=\"200\" style=\"background-color:#474747;margin:0px;padding:0px;\"></canvas>\n"
"<div id=\"root\" class=\"root\">\n"
"<ul id=\"nav\">\n"
"<li><a href=\"#\" onclick=\"ToggleDebugMode();\">?</a>\n"
"<div class=\"help\" style=\"left:20px;top:20px;width:220px;\">\n"
"Use Cursor to Inspect<br>\n"
"Shift-Drag to Pan view<br>\n"
"Ctrl-Drag to Zoom view<br>\n"
"Click to Zoom to selected range<br>\n"
"</div>\n"
"\n"
"<div class=\"help\" id=\"divFrameInfo\" style=\"left:20px;top:300px;width:auto;\">\n"
"</div>\n"
"\n"
"</li>\n"
"<li><a href=\"#\" id=\'ModeSubMenuText\'>Mode</a>\n"
"    <ul id=\'ModeSubMenu\'>\n"
"		<li><a href=\"#\" onclick=\"SetMode(\'timers\', 0);\" id=\"buttonTimers\">Timers</a></li>\n"
"		<li><a href=\"#\" onclick=\"SetMode(\'timers\', 1);\" id=\"buttonGroups\">Groups</a></li> \n"
"		<li><a href=\"#\" onclick=\"SetMode(\'timers\', 2);\" id=\"buttonThreads\">Threads</a></li>\n"
"		<li><a href=\"#\" onclick=\"SetMode(\'detailed\', 0);\" id=\"buttonDetailed\">Detailed</a></li>\n"
"	</ul>\n"
"</li>\n"
"<li><a href=\"#\">Reference</a>\n"
"    <ul id=\'ReferenceSubMenu\'>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'5ms\');\">5ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'10ms\');\">10ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'15ms\');\">15ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'20ms\');\">20ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'33ms\');\">33ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'50ms\');\">50ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'100ms\');\">100ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'250ms\');\">250ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'500ms\');\">500ms</a></li>\n"
"        <li><a href=\"#\" onclick=\"SetReferenceTime(\'1000ms\');\">1000ms</a></li>\n"
"    </ul>\n"
"</li>\n"
"<li id=\"ilThreads\"><a href=\"#\">Threads</a>\n"
"    <ul id=\"ThreadSubMenu\">\n"
"        <li><a href=\"#\" onclick=\"ToggleThread();\">All</a></li>\n"
"        <li><a href=\"#\">---</a></li>\n"
"    </ul>\n"
"</li>\n"
"<li id=\"ilGroups\"><a href=\"#\">Groups</a>\n"
"    <ul id=\"GroupSubMenu\">\n"
"        <li><a href=\"#\" onclick=\"ToggleGroup();\">All</a></li>\n"
"        <li><a href=\"#\">---</a></li>\n"
"    </ul>\n"
"</li>\n"
"<li id=\"ilOptions\"><a href=\"#\">Options&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</a>\n"
"    <ul id=\'OptionsMenu\'>\n"
"        <li><a href=\"#\" onclick=\"ToggleContextSwitch();\">Context Switch</a></li>\n"
"		<li><a href=\"#\" onclick=\"ToggleDisableMerge();\">MergeDisable</a></li>\n"
"		<li><a href=\"#\" onclick=\"ToggleDisableLod();\">LodDisable</a></li>\n"
"		<li id=\'GroupColors\'><a href=\"#\" onclick=\"ToggleGroupColors();\">Group Colors</a></li>\n"
"        <li id=\'TimersMeta\'><a href=\"#\" onclick=\"ToggleTimersMeta();\">Meta</a></li>\n"
"<!--      	<li><a href=\"#\" onclick=\"ToggleDebug();\">DEBUG</a></li> -->\n"
"    </ul>\n"
"</li>\n"
"</ul>\n"
"</div>\n"
"<script>\n"
"function InvertColor(hexTripletColor) {\n"
"	var color = hexTripletColor;\n"
"	color = color.substring(1); // remove #\n"
"	color = parseInt(color, 16); // convert to integer\n"
"	var R = ((color >> 16) % 256)/255.0;\n"
"	var G = ((color >> 8) % 256)/255.0;\n"
"	var B = ((color >> 0) % 256)/255.0;\n"
"	var lum = (0.2126*R + 0.7152*G + 0.0722*B);\n"
"	if(lum < 0.7)\n"
"	{\n"
"		return \'#ffffff\';\n"
"	}\n"
"	else\n"
"	{\n"
"		return \'#333333\';\n"
"	}\n"
"}\n"
"function InvertColorIndex(hexTripletColor) {\n"
"	var color = hexTripletColor;\n"
"	color = color.substring(1); // remove #\n"
"	color = parseInt(color, 16); // convert to integer\n"
"	var R = ((color >> 16) % 256)/255.0;\n"
"	var G = ((color >> 8) % 256)/255.0;\n"
"	var B = ((color >> 0) % 256)/255.0;\n"
"	var lum = (0.2126*R + 0.7152*G + 0.0722*B);\n"
"	if(lum < 0.7)\n"
"	{\n"
"		return 0;\n"
"	}\n"
"	else\n"
"	{\n"
"		return 1;\n"
"	}\n"
"}\n"
"function MakeGroup(id, name, category, numtimers, isgpu, total, average, max, color)\n"
"{\n"
"	var group = {\"id\":id, \"name\":name, \"category\":category, \"numtimers\":numtimers, \"isgpu\":isgpu, \"total\": total, \"average\" : average, \"max\" : max, \"color\":color};\n"
"	return group;\n"
"}\n"
"\n"
"function MakeTimer(id, name, group, color, average, max, exclaverage, exclmax, callaverage, callcount, total, meta, metaavg, metamax)\n"
"{\n"
"	var timer = {\"id\":id, \"name\":name, \"len\":name.length, \"color\":color, \"timercolor\":color, \"textcolor\":InvertColor(color), \"group\":group, \"average\":average, \"max\":max, \"exclaverage\":exclaverage, \"exclmax\":exclmax, \"callaverage\":callaverage, \"callcount\":callcount, \"total\":total, \"meta\":meta, \"textcolorindex\":InvertColorIndex(color), \"metaavg\":metaavg, \"metamax\":metamax};\n"
"	return timer;\n"
"}\n"
"function MakeFrame(id, framestart, frameend, ts, tt, ti)\n"
"{\n"
"	var frame = {\"id\":id, \"framestart\":framestart, \"frameend\":frameend, \"ts\":ts, \"tt\":tt, \"ti\":ti};\n"
"	return frame;\n"
"}\n"
"\n"
"";

const size_t g_MicroProfileHtml_begin_0_size = sizeof(g_MicroProfileHtml_begin_0);
const char* g_MicroProfileHtml_begin[] = {
&g_MicroProfileHtml_begin_0[0],
};
size_t g_MicroProfileHtml_begin_sizes[] = {
sizeof(g_MicroProfileHtml_begin_0),
};
size_t g_MicroProfileHtml_begin_count = 1;
const char g_MicroProfileHtml_end_0[] =
"\n"
"\n"
"\n"
"var CanvasDetailedView = document.getElementById(\'DetailedView\');\n"
"var CanvasHistory = document.getElementById(\'History\');\n"
"var CanvasDetailedOffscreen = document.createElement(\'canvas\');\n"
"var g_Msg = \'0\';\n"
"\n"
"var Initialized = 0;\n"
"var fDetailedOffset = Frames[0].framestart;\n"
"var fDetailedRange = Frames[0].frameend - fDetailedOffset;\n"
"var nWidth = CanvasDetailedView.width;\n"
"var nHeight = CanvasDetailedView.height;\n"
"var ReferenceTime = 33;\n"
"var nHistoryHeight = 70;\n"
"var nOffsetY = 0;\n"
"var nOffsetBarsX = 0;\n"
"var nOffsetBarsY = 0;\n"
"var nBarsWidth = 80;\n"
"var NameWidth = 200;\n"
"var MouseButtonState = [0,0,0,0,0,0,0,0];\n"
"var KeyShiftDown = 0;\n"
"var MouseDragButton = 0;\n"
"var KeyCtrlDown = 0;\n"
"var FlipToolTip = 0;\n"
"var DetailedViewMouseX = 0;\n"
"var DetailedViewMouseY = 0;\n"
"var HistoryViewMouseX = -1;\n"
"var HistoryViewMouseY = -1;\n"
"var MouseHistory = 0;\n"
"var MouseDetailed = 0;\n"
"var FontHeight = 10;\n"
"var FontWidth = 1;\n"
"var FontAscent = 3; //Set manually\n"
"var Font = \'Bold \' + FontHeight + \'px Courier New\';\n"
"var BoxHeight = FontHeight + 2;\n"
"var ThreadsActive = new Object();\n"
"var ThreadsAllActive = 1;\n"
"var GroupsActive = new Object();\n"
"var GroupsAllActive = 1;\n"
"var nMinWidth = 0.01;//subpixel width\n"
"var nMinWidthPan = 1.0;//subpixel width when panning\n"
"var nContextSwitchEnabled = 1;\n"
"var DisableLod = 0;\n"
"var DisableMerge = 0;\n"
"var GroupColors = 0;\n"
"var nModDown = 0;\n"
"var g_MSG = \'no\';\n"
"var nDrawCount = 0;\n"
"var nBackColors = [\'#474747\', \'#313131\' ];\n"
"var nBackColorOffset = \'#606060\';\n"
"var CSwitchColors =[\"#9DD8AF\",\"#D7B6DA\",\"#EAAC76\",\"#DBDA61\",\"#8AD5E1\",\"#8CE48B\",\"#C4D688\",\"#57E5C4\"];//generated by http://tools.medialab.sciences-po.fr/iwanthue/index.php\n"
"var CSwitchHeight = 5;\n"
"var FRAME_HISTORY_COLOR_CPU = \'#ff7f27\';\n"
"var FRAME_HISTORY_COLOR_GPU = \'#ffffff\';\n"
"var ZOOM_TIME = 0.5;\n"
"var AnimationActive = false;\n"
"var nHoverCSCpu = -1;\n"
"var nHoverCSCpuNext = -1;\n"
"var nHoverCSToolTip = null;\n"
"var nHoverToken = -1;\n"
"var nHoverFrame = -1;\n"
"var nHoverTokenIndex = -1;\n"
"var nHoverTokenLogIndex = -1;\n"
"var nHoverCounter = 0;\n"
"var nHoverCounterDelta = 8;\n"
"var nHoverTokenNext = -1;\n"
"var nHoverTokenLogIndexNext = -1;\n"
"var nHoverTokenIndexNext = -1;\n"
"\n"
"\n"
"\n"
"var fFrameScale = 33.33;\n"
"var fRangeBegin = 0;\n"
"var fRangeEnd = -1;\n"
"var fRangeBeginNext = 0;\n"
"var fRangeEndNext = 0;\n"
"var fRangeBeginHistory = -1;\n"
"var fRangeEndHistory = -1;\n"
"\n"
"var ModeDetailed = 0;\n"
"var ModeTimers = 1;\n"
"var Mode = ModeDetailed;\n"
"\n"
"var DebugDrawQuadCount = 0;\n"
"var DebugDrawTextCount = 0;\n"
"var ProfileMode = 0;\n"
"var ProfileFps = 0;\n"
"var ProfileFpsAggr = 0;\n"
"var ProfileFpsCount = 0;\n"
"var ProfileLastTimeStamp = new Date();\n"
"\n"
"var CSwitchCache = {};\n"
"var CSwitchOnlyThreads = [];\n"
"var ProfileData = {};\n"
"var ProfileStackTime = {};\n"
"var ProfileStackName = {};\n"
"var Debug = 1;\n"
"\n"
"var g_MaxStack = Array();\n"
"var g_TypeArray;\n"
"var g_TimeArray;\n"
"var g_IndexArray;\n"
"var LodData = new Array();\n"
"var NumLodSplits = 10;\n"
"var SplitMin = 100;\n"
"var SPLIT_LIMIT = 1e20;\n"
"var DPR = 1;\n"
"var DetailedRedrawState = {};\n"
"var OffscreenData;\n"
"var DetailedFrameCounter = 0;\n"
"var Invalidate = 0;\n"
"var GroupOrder = Array();\n"
"var ThreadOrder = Array();\n"
"var TimersGroups = 0;\n"
"var TimersMeta = 1;\n"
"var MetaLengths = Array();\n"
"var MetaLengthsAvg = Array();\n"
"var MetaLengthsMax = Array();\n"
"\n"
"\n"
"function ProfileModeClear()\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		ProfileData = new Object();\n"
"		ProfileStackTime = new Array();\n"
"		ProfileStackName = new Array();\n"
"	}\n"
"}\n"
"function ProfileEnter(Name)\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		ProfileStackTime.push(new Date());\n"
"		ProfileStackName.push(Name);\n"
"	}\n"
"}\n"
"function ProfileLeave()\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		var Time = new Date();\n"
"		var Delta = Time - ProfileStackTime.pop();\n"
"		var Name = ProfileStackName.pop();\n"
"		var Obj = ProfileData[Name];\n"
"		if(!Obj)\n"
"		{\n"
"			Obj = new Object();\n"
"			Obj.Count = 0;\n"
"			Obj.Name = Name;\n"
"			Obj.Time = 0;\n"
"			ProfileData[Name] = Obj;\n"
"		}\n"
"		Obj.Time += Delta;\n"
"		Obj.Count += 1;\n"
"	}\n"
"}\n"
"\n"
"function ProfilePlot(s)\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		var A = ProfileData.Plot;\n"
"		if(!A)\n"
"		{\n"
"			ProfileData.Plot = Array();\n"
"			A = ProfileData.Plot;\n"
"		}\n"
"		if(A.length<10)\n"
"		{\n"
"			A.push(s);\n"
"		}\n"
"	}\n"
"}\n"
"function ProfileModeDump()\n"
"{\n"
"	for(var idx in ProfileData)\n"
"	{\n"
"		var Timer = ProfileData[idx];\n"
"		console.log(Timer.Name + \" \" + Timer.Time + \"ms \" + Timer.Count);\n"
"	}\n"
"\n"
"}\n"
"function ProfileModeDraw(Canvas)\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		var StringArray = [];\n"
"		for(var idx in ProfileData)\n"
"		{\n"
"			if(idx == \"Plot\")\n"
"				continue;\n"
"			var Timer = ProfileData[idx];\n"
"			StringArray.push(Timer.Name);\n"
"			StringArray.push(Timer.Time + \"ms\");\n"
"			StringArray.push(\"#\");\n"
"			StringArray.push(\"\" + Timer.Count);\n"
"		}\n"
"		StringArray.push(\"debug\");\n"
"		StringArray.push(Debug);\n"
"		var Time = new Date();\n"
"		var Delta = Time - ProfileLastTimeStamp;\n"
"		ProfileLastTimeStamp = Time;\n"
"		StringArray.push(\"Frame Delta\");\n"
"		StringArray.push(Delta + \"ms\");\n"
"		if(ProfileMode == 2)\n"
"		{\n"
"			ProfileFpsAggr += Delta;\n"
"			ProfileFpsCount ++ ;\n"
"			var AggrFrames = 10;\n"
"			if(ProfileFpsCount == AggrFrames)\n"
"			{\n"
"				ProfileFps = 1000 / (ProfileFpsAggr / AggrFrames);\n"
"				ProfileFpsAggr = 0;\n"
"				ProfileFpsCount = 0;\n"
"			}\n"
"			StringArray.push(\"FPS\");\n"
"			StringArray.push(\"\" + ProfileFps.toFixed(2));\n"
"		}\n"
"\n"
"\n"
"		for(var i = 0; i < ProfileData.Plot; ++i)\n"
"		{\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(ProfileData.Plot[i]);\n"
"		}\n"
"		ProfileData.Plot = Array();\n"
"		DrawToolTip(StringArray, Canvas, 0, 200);\n"
"	}\n"
"}\n"
"\n"
"function ToggleDebugMode()\n"
"{\n"
"	ProfileMode = (ProfileMode+1)%4;\n"
"	console.log(\'Toggle Debug Mode \' + ProfileMode);\n"
"}\n"
"\n"
"function DetailedTotal()\n"
"{\n"
"	var Total = 0;\n"
"	for(var i = 0; i < Frames.length; i++)\n"
"	{\n"
"		var frfr = Frames[i];\n"
"		Total += frfr.frameend - frfr.framestart;\n"
"	}\n"
"	return Total;\n"
"}\n"
"\n"
"function InitFrameInfo()\n"
"{\n"
"\n"
"	var div = document.getElementById(\'divFrameInfo\');\n"
"	var txt = \'\';\n"
"	txt = txt + \'Timers View\' + \'<br>\';\n"
"	txt = txt + \'Frames:\' + AggregateInfo.Frames +\'<br>\';\n"
"	txt = txt + \'Time:\' + AggregateInfo.Time.toFixed(2) +\'ms<br>\';\n"
"	txt = txt + \'<hr>\';\n"
"	txt = txt + \'Detailed View\' + \'<br>\';\n"
"	txt = txt + \'Frames:\' + Frames.length +\'<br>\';\n"
"	txt = txt + \'Time:\' + DetailedTotal().toFixed(2) +\'ms<br>\';\n"
"	div.innerHTML = txt;\n"
"}\n"
"function InitGroups()\n"
"{\n"
"	for(groupid in GroupInfo)\n"
"	{\n"
"		var TimerArray = Array();\n"
"		for(timerid in TimerInfo)\n"
"		{\n"
"			if(TimerInfo[timerid].group == groupid)\n"
"			{\n"
"				TimerArray.push(timerid);\n"
"			}\n"
"		}\n"
"		GroupInfo[groupid].TimerArray = TimerArray;\n"
"	}\n"
"}\n"
"\n"
"function InitThreadMenu()\n"
"{\n"
"	var ulThreadMenu = document.getElementById(\'ThreadSubMenu\');\n"
"	var MaxLen = 7;\n"
"	ThreadOrder = CreateOrderArray(ThreadNames, function(a){return a;});\n"
"	for(var idx in ThreadOrder)\n"
"	{\n"
"		var name = ThreadNames[ThreadOrder[idx]];\n"
"		var li = document.createElement(\'li\');\n"
"		if(name.length > MaxLen)\n"
"		{\n"
"			MaxLen = name.length;\n"
"		}\n"
"		li.innerText = name;\n"
"		var asText = li.innerHTML;\n"
"		var html = \'<a href=\"#\" onclick=\"ToggleThread(\\'\' + name + \'\\');\">\' + asText + \'</a>\';\n"
"		li.innerHTML = html;\n"
"		ulThreadMenu.appendChild(li);\n"
"	}\n"
"	var LenStr = (5+(1+MaxLen) * (1+FontWidth)) + \'px\';\n"
"	var Lis = ulThreadMenu.getElementsByTagName(\'li\');\n"
"	for(var i = 0; i < Lis.length; ++i)\n"
"	{\n"
"		Lis[i].style[\'width\'] = LenStr;\n"
"	}\n"
"}\n"
"\n"
"function UpdateThreadMenu()\n"
"{\n"
"	var ulThreadMenu = document.getElementById(\'ThreadSubMenu\');\n"
"	var as = ulThreadMenu.getElementsByTagName(\'a\');\n"
"	for(var i = 0; i < as.length; ++i)\n"
"	{\n"
"		var elem = as[i];\n"
"		var inner = elem.innerText;\n"
"		var bActive = false;\n"
"		if(i < 2)\n"
"		{\n"
"			if(inner == \'All\')\n"
"			{\n"
"				bActive = ThreadsAllActive;\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			bActive = ThreadsActive[inner];\n"
"		}\n"
"		if(bActive)\n"
"		{\n"
"			elem.style[\'text-decoration\'] = \'underline\';\n"
"		}\n"
"		else\n"
"		{\n"
"			elem.style[\'text-decoration\'] = \'none\';\n"
"		}\n"
"	}\n"
"}\n"
"\n"
"function ToggleThread(ThreadName)\n"
"{\n"
"	if(ThreadName)\n"
"	{\n"
"		if(ThreadsActive[ThreadName])\n"
"		{\n"
"			ThreadsActive[ThreadName] = false;\n"
"		}\n"
"		else\n"
"		{\n"
"			ThreadsActive[ThreadName] = true;\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		if(ThreadsAllActive)\n"
"		{\n"
"			ThreadsAllActive = 0;\n"
"		}\n"
"		else\n"
"		{\n"
"			ThreadsAllActive = 1;\n"
"		}\n"
"	}\n"
"	Invalidate = 0;\n"
"	UpdateThreadMenu();\n"
"	WriteCookie();\n"
"	Draw();\n"
"\n"
"}\n"
"\n"
"function CreateOrderArray(Source, NameFunc)\n"
"{\n"
"	var Temp = Array(Source.length);\n"
"	for(var i = 0; i < Source.length; ++i)\n"
"	{\n"
"		Temp[i] = {};\n"
"		Temp[i].index = i;\n"
"		Temp[i].namezz = NameFunc(Source[i]).toLowerCase();\n"
"	}\n"
"	Temp.sort(function(l, r)\n"
"	{ \n"
"		if(r.namezz<l.namezz)\n"
"			{return 1;}\n"
"		if(l.namezz<r.namezz)\n"
"			{return -1;} \n"
"		return 0;\n"
"	} );\n"
"	var OrderArray = Array(Source.length);\n"
"	for(var i = 0; i < Source.length; ++i)\n"
"	{\n"
"		OrderArray[i] = Temp[i].index;\n"
"	}\n"
"	return OrderArray;\n"
"}\n"
"\n"
"\n"
"function InitGroupMenu()\n"
"{\n"
"	var ulGroupMenu = document.getElementById(\'GroupSubMenu\');\n"
"	var MaxLen = 7;\n"
"	var MenuArray = Array();\n"
"	for(var i = 0; i < GroupInfo.length; ++i)\n"
"	{\n"
"		var x = {};\n"
"		x.IsCategory = 0;\n"
"		x.category = GroupInfo[i].category;\n"
"		x.name = GroupInfo[i].name;\n"
"		x.index = i;\n"
"		MenuArray.push(x);\n"
"	}\n"
"	for(var i = 0; i < CategoryInfo.length; ++i)\n"
"	{\n"
"		var x = {};\n"
"		x.IsCategory = 1;\n"
"		x.category = i;\n"
"		x.name = CategoryInfo[i];\n"
"		x.index = i;\n"
"		MenuArray.push(x);\n"
"	}\n"
"	var OrderFunction = function(a){ return a.category + \"__\" + a.name; };\n"
"	var OrderFunctionMenu = function(a){ return a.IsCategory ? (a.category + \'\') : (a.category + \"__\" + a.name); };\n"
"	GroupOrder = CreateOrderArray(GroupInfo, OrderFunction);\n"
"	var MenuOrder = CreateOrderArray(MenuArray, OrderFunctionMenu);\n"
"\n"
"	for(var idx in MenuOrder)\n"
"	{\n"
"		var MenuItem = MenuArray[MenuOrder[idx]];\n"
"		var name = MenuItem.name;\n"
"		var li = document.createElement(\'li\');\n"
"		if(name.length > MaxLen)\n"
"		{\n"
"			MaxLen = name.length;\n"
"		}\n"
"		var jsfunc = \'\';\n"
"		if(MenuItem.IsCategory)\n"
"		{				\n"
"			li.innerText = \'[\' + name + \']\';\n"
"			jsfunc = \"ToggleCategory\";\n"
"		}\n"
"		else\n"
"		{\n"
"			li.innerText = name;\n"
"			jsfunc = \"ToggleGroup\";\n"
"		}\n"
"		var asText = li.innerHTML;\n"
"		var html = \'<a href=\"#\" onclick=\"\' + jsfunc + \'(\\'\' + name + \'\\');\">\' + asText + \'</a>\';\n"
"		li.innerHTML = html;\n"
"		ulGroupMenu.appendChild(li);\n"
"	}\n"
"	var LenStr = (5+(1+MaxLen) * FontWidth) + \'px\';\n"
"	var Lis = ulGroupMenu.getElementsByTagName(\'li\');\n"
"	for(var i = 0; i < Lis.length; ++i)\n"
"	{\n"
"		Lis[i].style[\'width\'] = LenStr;\n"
"	}\n"
"	UpdateGroupMenu();\n"
"}\n"
"\n"
"function UpdateGroupMenu()\n"
"{\n"
"	var ulThreadMenu = document.getElementById(\'GroupSubMenu\');\n"
"	var as = ulThreadMenu.getElementsByTagName(\'a\');\n"
"	for(var i = 0; i < as.length; ++i)\n"
"	{\n"
"		var elem = as[i];\n"
"		var inner = elem.innerText;\n"
"		var bActive = false;\n"
"		if(i < 2)\n"
"		{\n"
"			if(inner == \'All\')\n"
"			{\n"
"				bActive = GroupsAllActive;\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			var CategoryString = inner.length>2 ? inner.substring(1, inner.length-2) : \"\";\n"
"			var CategoryIdx = CategoryIndex(CategoryString);\n"
"			if(inner[0] == \'[\' && inner[inner.length-1] == \']\' && CategoryIdx >= 0)\n"
"			{\n"
"				bActive = IsCategoryActive(CategoryIdx);\n"
"			}\n"
"			else\n"
"			{\n"
"				bActive = GroupsActive[inner];\n"
"			}\n"
"		}\n"
"		if(bActive)\n"
"		{\n"
"			elem.style[\'text-decoration\'] = \'underline\';\n"
"		}\n"
"		else\n"
"		{\n"
"			elem.style[\'text-decoration\'] = \'none\';\n"
"		}\n"
"	}\n"
"}\n"
"function CategoryIndex(CategoryName)\n"
"{\n"
"	for(var i = 0; i < CategoryInfo.length; ++i)\n"
"	{\n"
"		if(CategoryInfo[i] == CategoryName)\n"
"		{\n"
"			return i;\n"
"		}\n"
"	}\n"
"	return -1;\n"
"}\n"
"function IsCategoryActive(CategoryIdx)\n"
"{\n"
"	for(var i = 0; i < GroupInfo.length; ++i)\n"
"	{\n"
"		if(GroupInfo[i].category == CategoryIdx)\n"
"		{\n"
"			var Name = GroupInfo[i].name;\n"
"			if(!GroupsActive[Name])\n"
"			{\n"
"				return false;\n"
"			}\n"
"		}\n"
"	}\n"
"	return true;\n"
"\n"
"}\n"
"function ToggleCategory(CategoryName)\n"
"{\n"
"	var CategoryIdx = CategoryIndex(CategoryName);\n"
"	if(CategoryIdx < 0)\n"
"		return;\n"
"	var CategoryActive = IsCategoryActive(CategoryIdx);\n"
"	for(var i = 0; i < GroupInfo.length; ++i)\n"
"	{\n"
"		if(GroupInfo[i].category == CategoryIdx)\n"
"		{\n"
"			var Name = GroupInfo[i].name;\n"
"			if(CategoryActive)\n"
"			{\n"
"				GroupsActive[Name] = false;\n"
"			}\n"
"			else\n"
"			{\n"
"				GroupsActive[Name] = true;\n"
"			}\n"
"		}\n"
"	}\n"
"	UpdateGroupMenu();\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"}\n"
"\n"
"function ToggleGroup(GroupName)\n"
"{\n"
"	if(GroupName)\n"
"	{\n"
"		if(GroupsActive[GroupName])\n"
"		{\n"
"			GroupsActive[GroupName] = false;\n"
"		}\n"
"		else\n"
"		{\n"
"			GroupsActive[GroupName] = true;\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		if(GroupsAllActive)\n"
"		{\n"
"			GroupsAllActive = 0;\n"
"		}\n"
"		else\n"
"		{\n"
"			GroupsAllActive = 1;\n"
"		}\n"
"	}\n"
"	UpdateGroupMenu();\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"}\n"
"function UpdateGroupColors()\n"
"{\n"
"	for(var i = 0; i < TimerInfo.length; ++i)\n"
"	{\n"
"		if(GroupColors)\n"
"		{\n"
"			TimerInfo[i].color = GroupInfo[TimerInfo[i].group].color;\n"
"		}\n"
"		else\n"
"		{\n"
"			TimerInfo[i].color = TimerInfo[i].timercolor;\n"
"		}\n"
"		TimerInfo[i].textcolorindex = InvertColorIndex(TimerInfo[i].color);\n"
"	}\n"
"}\n"
"\n"
"function ToggleGroupColors()\n"
"{\n"
"	GroupColors = !GroupColors;\n"
"	UpdateGroupColors();\n"
"	UpdateOptionsMenu();\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"}\n"
"\n"
"function UpdateOptionsMenu()\n"
"{\n"
"	var ulTimersMeta = document.getElementById(\'TimersMeta\');\n"
"	ulTimersMeta.style[\'text-decoration\'] = TimersMeta ? \'underline\' : \'none\';\n"
"	var ulGroupColors = document.getElementById(\'GroupColors\');\n"
"	ulGroupColors.style[\'text-decoration\'] = GroupColors ? \'underline\' : \'none\';\n"
"}\n"
"\n"
"function ToggleTimersMeta()\n"
"{\n"
"	TimersMeta = TimersMeta ? 0 : 1;\n"
"	WriteCookie();\n"
"	UpdateOptionsMenu();\n"
"	RequestRedraw();\n"
"}\n"
"\n"
"function SetMode(NewMode, Groups)\n"
"{\n"
"	var buttonTimers = document.getElementById(\'buttonTimers\');\n"
"	var buttonDetailed = document.getElementById(\'buttonDetailed\');\n"
"	var buttonGroups = document.getElementById(\'buttonGroups\');\n"
"	var buttonThreads = document.getElementById(\'buttonThreads\');\n"
"	var ilThreads = document.getElementById(\'ilThreads\');\n"
"	var ilGroups = document.getElementById(\'ilGroups\');\n"
"	var ModeElement = null;\n"
"	if(NewMode == \'timers\' || NewMode == ModeTimers)\n"
"	{\n"
"		TimersGroups = Groups;\n"
"		buttonTimers.style[\'text-decoration\'] = TimersGroups ? \'none\' : \'underline\';\n"
"		buttonGroups.style[\'text-decoration\'] = TimersGroups == 1 ? \'underline\' : \'none\';\n"
"		buttonThreads.style[\'text-decoration\'] = TimersGroups == 2 ? \'underline\' : \'none\';\n"
"		buttonDetailed.style[\'text-decoration\'] = \'none\';\n"
"		if(TimersGroups == 0)\n"
"		{\n"
"			ilThreads.style[\'display\'] = \'none\';\n"
"		}\n"
"		else\n"
"		{\n"
"			ilThreads.style[\'display\'] = \'block\';\n"
"		}\n"
"		ilGroups.style[\'display\'] = \'block\';\n"
"		Mode = ModeTimers;\n"
"		ModeElement = TimersGroups == 2 ? buttonThreads : TimersGroups == 1 ? buttonGroups : buttonTimers;\n"
"	}\n"
"	else if(NewMode == \'detailed\' || NewMode == ModeDetailed)\n"
"	{\n"
"		buttonTimers.style[\'text-decoration\'] = \'none\';\n"
"		buttonGroups.style[\'text-decoration\'] = \'none\';\n"
"		buttonThreads.style[\'text-decoration\'] = \'none\';\n"
"		buttonDetailed.style[\'text-decoration\'] = \'underline\';\n"
"		ilThreads.style[\'display\'] = \'block\';\n"
"		ilGroups.style[\'display\'] = \'none\';\n"
"		Mode = ModeDetailed;\n"
"		ModeElement = buttonDetailed;\n"
"	}\n"
"	var ModeSubMenuText = document.getElementById(\'ModeSubMenuText\');\n"
"	ModeSubMenuText.innerText = \'Mode[\' + ModeElement.innerText + \']\';\n"
"\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"\n"
"}\n"
"\n"
"function SetReferenceTime(TimeString)\n"
"{\n"
"	ReferenceTime = parseInt(TimeString);\n"
"	var ReferenceMenu = document.getElementById(\'ReferenceSubMenu\');\n"
"	var Links = ReferenceMenu.getElementsByTagName(\'a\');\n"
"	for(var i = 0; i < Links.length; ++i)\n"
"	{\n"
"		if(Links[i].innerHTML.match(\'^\' + TimeString))\n"
"		{\n"
"			Links[i].style[\'text-decoration\'] = \'underline\';\n"
"		}\n"
"		else\n"
"		{\n"
"			Links[i].style[\'text-decoration\'] = \'none\';\n"
"		}\n"
"	}\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"\n"
"}\n"
"\n"
"function ToggleContextSwitch()\n"
"{\n"
"	SetContextSwitch(nContextSwitchEnabled ? 0 : 1);\n"
"}\n"
"function SetContextSwitch(Enabled)\n"
"{\n"
"	nContextSwitchEnabled = Enabled ? 1 : 0;\n"
"	var ReferenceMenu = document.getElementById(\'OptionsMenu\');\n"
"	var Links = ReferenceMenu.getElementsByTagName(\'a\');\n"
"	Links[0].style[\'text-decoration\'] = nContextSwitchEnabled ? \'underline\' : \'none\';\n"
"	WriteCookie();\n"
"	RequestRedraw();\n"
"}\n"
"\n"
"function ToggleDebug()\n"
"{\n"
"	Debug = (Debug + 1) % 2;\n"
"}\n"
"\n"
"function ToggleDisableMerge()\n"
"{\n"
"	DisableMerge = DisableMerge ? 0 : 1;\n"
"	var ReferenceMenu = document.getElementById(\'OptionsMenu\');\n"
"	var Links = ReferenceMenu.getElementsByTagName(\'a\');\n"
"	if(DisableMerge)\n"
"	{\n"
"		Links[1].style[\'text-decoration\'] = \'underline\';\n"
"	}\n"
"	else\n"
"	{\n"
"		Links[1].style[\'text-decoration\'] = \'none\';\n"
"	}\n"
"\n"
"}\n"
"\n"
"function ToggleDisableLod()\n"
"{\n"
"	DisableLod = DisableLod ? 0 : 1;\n"
"	var ReferenceMenu = document.getElementById(\'OptionsMenu\');\n"
"	var Links = ReferenceMenu.getElementsByTagName(\'a\');\n"
"	if(DisableLod)\n"
"	{\n"
"		Links[2].style[\'text-decoration\'] = \'underline\';\n"
"	}\n"
"	else\n"
"	{\n"
"		Links[2].style[\'text-decoration\'] = \'none\';\n"
"	}\n"
"\n"
"}\n"
"\n"
"function GatherHoverMetaCounters(TimerIndex, StartIndex, nLog, nFrameLast)\n"
"{\n"
"	var HoverInfo = new Object();\n"
"	var StackPos = 1;\n"
"	//search backwards, count meta counters \n"
"	for(var i = nFrameLast; i >= 0; i--)\n"
"	{\n"
"		var fr = Frames[i];\n"
"		var ts = fr.ts[nLog];\n"
"		var ti = fr.ti[nLog];\n"
"		var tt = fr.tt[nLog];\n"
"		var start = i == nFrameLast ? StartIndex-1 : ts.length-1;\n"
"\n"
"		for(var j = start; j >= 0; j--)\n"
"		{\n"
"			var type = tt[j];\n"
"			var index = ti[j];\n"
"			var time = ts[j];\n"
"			if(type == 1)\n"
"			{\n"
"				StackPos--;\n"
"				if(StackPos == 0 && index == TimerIndex)\n"
"				{\n"
"					return HoverInfo;\n"
"				}\n"
"			}\n"
"			else if(type == 0)\n"
"			{\n"
"				StackPos++;\n"
"			}\n"
"			else\n"
"			{\n"
"				var nMetaCount = type - 2;\n"
"				var nMetaIndex = MetaNames[index];\n"
"				if(nMetaIndex in HoverInfo)\n"
"				{\n"
"					HoverInfo[nMetaIndex] += nMetaCount;\n"
"				}\n"
"				else\n"
"				{\n"
"					HoverInfo[nMetaIndex] = nMetaCount;\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"\n"
"}\n"
"function CalculateTimers(Result, TimerIndex, nFrameFirst, nFrameLast)\n"
"{\n"
"	if(!nFrameFirst || nFrameFirst < 0)\n"
"		nFrameFirst = 0;\n"
"	if(!nFrameLast || nFrameLast > Frames.length)\n"
"		nFrameLast = Frames.length;\n"
"	var FrameCount = nFrameLast - nFrameFirst;\n"
"	if(0 == FrameCount)\n"
"		return;\n"
"	var CallCount = 0;\n"
"	var Sum = 0;\n"
"	var Max = 0;\n"
"	var FrameMax = 0;\n"
"\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	var StackPosArray = Array(nNumLogs);\n"
"	var StackArray = Array(nNumLogs);\n"
"	for(var i = 0; i < nNumLogs; ++i)\n"
"	{\n"
"		StackPosArray[i] = 0;\n"
"		StackArray[i] = Array(20);\n"
"	}\n"
"\n"
"	for(var i = nFrameFirst; i < nFrameLast; i++)\n"
"	{\n"
"		var FrameSum = 0;\n"
"		var fr = Frames[i];\n"
"		for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"		{\n"
"			var StackPos = StackPosArray[nLog];\n"
"			var Stack = StackArray[nLog];\n"
"			var ts = fr.ts[nLog];\n"
"			var ti = fr.ti[nLog];\n"
"			var tt = fr.tt[nLog];\n"
"			var count = ts.length;\n"
"			for(j = 0; j < count; j++)\n"
"			{\n"
"				var type = tt[j];\n"
"				var index = ti[j];\n"
"				var time = ts[j];\n"
"				if(type == 1) //enter\n"
"				{\n"
"					//push\n"
"					Stack[StackPos] = time;\n"
"					if(StackArray[nLog][StackPos] != time)\n"
"					{\n"
"						console.log(\'fail fail fail\');\n"
"					}\n"
"					StackPos++;\n"
"				}\n"
"				else if(type == 0) // leave\n"
"				{\n"
"					var timestart;\n"
"					var timeend = time;\n"
"					if(StackPos>0)\n"
"					{\n"
"						StackPos--;\n"
"						timestart = Stack[StackPos];\n"
"					}\n"
"					else\n"
"					{\n"
"						timestart = Frames[nFrameFirst].framestart;\n"
"					}\n"
"					if(index == TimerIndex)\n"
"					{\n"
"						var TimeDelta = timeend - timestart;\n"
"						CallCount++;\n"
"						FrameSum += TimeDelta;\n"
"						Sum += TimeDelta;\n"
"						if(TimeDelta > Max)\n"
"							Max = TimeDelta;\n"
"					}\n"
"				}\n"
"				else\n"
"				{\n"
"					//meta\n"
"				}\n"
"			}\n"
"			if(FrameSum > FrameMax)\n"
"			{\n"
"				FrameMax = FrameSum;\n"
"			}\n"
"			StackPosArray[nLog] = StackPos;\n"
"		}\n"
"	}\n"
"\n"
"	Result.CallCount = CallCount;\n"
"	Result.Sum = Sum.toFixed(3);\n"
"	Result.Max = Max.toFixed(3);\n"
"	Result.Average = (Sum / CallCount).toFixed(3);\n"
"	Result.FrameAverage = (Sum / FrameCount).toFixed(3);\n"
"	Result.FrameCallAverage = (CallCount / FrameCount).toFixed(3);\n"
"	Result.FrameMax = FrameMax.toFixed(3);\n"
"	return Result;\n"
"\n"
"}\n"
"\n"
"function DrawDetailedFrameHistory()\n"
"{\n"
"	ProfileEnter(\"DrawDetailedFrameHistory\");\n"
"	var x = HistoryViewMouseX;\n"
"\n"
"	var context = CanvasHistory.getContext(\'2d\');\n"
"	context.clearRect(0, 0, CanvasHistory.width, CanvasHistory.height);\n"
"\n"
"	var fHeight = nHistoryHeight;\n"
"	var fWidth = nWidth / Frames.length;\n"
"	var fHeightScale = fHeight / ReferenceTime;\n"
"	var fX = 0;\n"
"	var FrameIndex = -1;\n"
"	var MouseDragging = MouseDragState != MouseDragOff;\n"
"	fRangeBeginHistory = fRangeEndHistory = -1;\n"
"\n"
"	var FrameFirst = -1;\n"
"	var FrameLast = nWidth;\n"
"	var fDetailedOffsetEnd = fDetailedOffset + fDetailedRange;\n"
"	for(i = 0; i < Frames.length; i++)\n"
"	{\n"
"		var fMs = Frames[i].frameend - Frames[i].framestart;\n"
"		if(fDetailedOffset <= Frames[i].frameend && fDetailedOffset >= Frames[i].framestart)\n"
"		{\n"
"			var lerp = (fDetailedOffset - Frames[i].framestart) / (Frames[i].frameend - Frames[i].framestart);\n"
"			FrameFirst = fX + fWidth * lerp;\n"
"		}\n"
"		if(fDetailedOffsetEnd <= Frames[i].frameend && fDetailedOffsetEnd >= Frames[i].framestart)\n"
"		{\n"
"			var lerp = (fDetailedOffsetEnd - Frames[i].framestart) / (Frames[i].frameend - Frames[i].framestart);\n"
"			FrameLast = fX + fWidth * lerp;\n"
"		}\n"
"		var fH = fHeightScale * fMs;\n"
"		var bMouse = x > fX && x < fX + fWidth;\n"
"		if(bMouse && !MouseDragging)\n"
"		{\n"
"			context.fillStyle = FRAME_HISTORY_COLOR_GPU;\n"
"			fRangeBeginHistory = Frames[i].framestart;\n"
"			fRangeEndHistory = Frames[i].frameend;\n"
"			FrameIndex = i;\n"
"		}\n"
"		else\n"
"		{\n"
"			context.fillStyle = FRAME_HISTORY_COLOR_CPU;\n"
"		}\n"
"		context.fillRect(fX, fHeight - fH, fWidth-1, fH);\n"
"		fX += fWidth;\n"
"	}\n"
"\n"
"	var fRangeHistoryBegin = FrameFirst;\n"
"	var fRangeHistoryEnd = FrameLast;\n"
"	var X = fRangeHistoryBegin;\n"
"	var Y = 0;\n"
"	var W = fRangeHistoryEnd - fRangeHistoryBegin;\n"
"	context.globalAlpha = 0.35;\n"
"	context.fillStyle = \'#009900\';\n"
"	context.fillRect(X, Y, W, fHeight);\n"
"	context.globalAlpha = 1;\n"
"	context.strokeStyle = \'#00ff00\';\n"
"	context.beginPath();\n"
"	context.moveTo(X, Y);\n"
"	context.lineTo(X, Y+fHeight);\n"
"	context.moveTo(X+W, Y);\n"
"	context.lineTo(X+W, Y+fHeight);\n"
"	context.stroke();\n"
"\n"
"	if(FrameIndex>=0 && !MouseDragging)\n"
"	{\n"
"		var StringArray = [];\n"
"		StringArray.push(\"Frame\");\n"
"		StringArray.push(\"\" + FrameIndex);\n"
"		StringArray.push(\"Time\");\n"
"		StringArray.push(\"\" + (Frames[FrameIndex].frameend - Frames[FrameIndex].framestart).toFixed(3));\n"
"\n"
"		DrawToolTip(StringArray, CanvasHistory, HistoryViewMouseX, HistoryViewMouseY+20);\n"
"\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function DrawDetailedBackground(context)\n"
"{\n"
"	var fMs = fDetailedRange;\n"
"	var fMsEnd = fMs + fDetailedOffset;\n"
"	var fMsToScreen = nWidth / fMs;\n"
"	var fRate = Math.floor(2*((Math.log(fMs)/Math.log(10))-1))/2;\n"
"	var fStep = Math.pow(10, fRate);\n"
"	var fRcpStep = 1.0 / fStep;\n"
"	var nColorIndex = Math.floor(fDetailedOffset * fRcpStep) % 2;\n"
"	if(nColorIndex < 0)\n"
"		nColorIndex = -nColorIndex;\n"
"	var fStart = Math.floor(fDetailedOffset * fRcpStep) * fStep;\n"
"	var fHeight = CanvasDetailedView.height;\n"
"	var fScaleX = nWidth / fDetailedRange; \n"
"	for(f = fStart; f < fMsEnd; )\n"
"	{\n"
"		var fNext = f + fStep;\n"
"		var X = (f - fDetailedOffset) * fScaleX;\n"
"		var W = (fNext-f)*fScaleX;\n"
"		context.fillStyle = nBackColors[nColorIndex];\n"
"		context.fillRect(X, 0, W+2, fHeight);\n"
"		nColorIndex = 1 - nColorIndex;\n"
"		f = fNext;\n"
"	}\n"
"	var fScaleX = nWidth / fDetailedRange; \n"
"	context.globalAlpha = 0.5;\n"
"	context.strokeStyle = \'#bbbbbb\';\n"
"	context.beginPath();\n"
"	for(var i = 0; i < Frames.length; i++)\n"
"	{\n"
"		var frfr = Frames[i];\n"
"		if(frfr.frameend < fDetailedOffset || frfr.framestart > fDetailedOffset + fDetailedRange)\n"
"		{\n"
"			continue;\n"
"		}\n"
"		var X = (frfr.framestart - fDetailedOffset) * fScaleX;\n"
"		if(X >= 0 && X < nWidth)\n"
"		{\n"
"			context.moveTo(X, 0);\n"
"			context.lineTo(X, nHeight);\n"
"		}\n"
"	}\n"
"	context.stroke();\n"
"	context.globalAlpha = 1;\n"
"\n"
"}\n"
"function DrawToolTip(StringArray, Canvas, x, y)\n"
"{\n"
"	var context = Canvas.getContext(\'2d\');\n"
"	context.font = Font;\n"
"	var WidthArray = Array(StringArray.length);\n"
"	var nMaxWidth = 0;\n"
"	var nHeight = 0;\n"
"	for(i = 0; i < StringArray.length; i += 2)\n"
"	{\n"
"		var nWidth0 = context.measureText(StringArray[i]).width;\n"
"		var nWidth1 = context.measureText(StringArray[i+1]).width;\n"
"		var nSum = nWidth0 + nWidth1;\n"
"		WidthArray[i] = nWidth0;\n"
"		WidthArray[i+1] = nWidth1;\n"
"		if(nSum > nMaxWidth)\n"
"		{\n"
"			nMaxWidth = nSum;\n"
"		}\n"
"		nHeight += BoxHeight;\n"
"	}\n"
"	nMaxWidth += 15;\n"
"	//bounds check.\n"
"	var CanvasRect = Canvas.getBoundingClientRect();\n"
"	if(y + nHeight > CanvasRect.height)\n"
"	{\n"
"		y = CanvasRect.height - nHeight;\n"
"		x += 20;\n"
"	}\n"
"	if(x + nMaxWidth > CanvasRect.width)\n"
"	{\n"
"		x = CanvasRect.width - nMaxWidth;\n"
"	}\n"
"\n"
"	context.fillStyle = \'black\';\n"
"	context.fillRect(x-1, y, nMaxWidth+2, nHeight);\n"
"	context.fillStyle = \'white\';\n"
"\n"
"	var XPos = x;\n"
"	var XPosRight = x + nMaxWidth;\n"
"	var YPos = y + BoxHeight-2;\n"
"	for(i = 0; i < StringArray.length; i += 2)\n"
"	{\n"
"		context.fillText(StringArray[i], XPos, YPos);\n"
"		context.fillText(StringArray[i+1], XPosRight - WidthArray[i+1], YPos);\n"
"		YPos += BoxHeight;\n"
"	}\n"
"}\n"
"function DrawHoverToolTip()\n"
"{\n"
"	ProfileEnter(\"DrawHoverToolTip\");\n"
"	if(nHoverToken != -1)\n"
"	{\n"
"		var StringArray = [];\n"
"		var groupid = TimerInfo[nHoverToken].group;\n"
"		StringArray.push(\"Timer\");\n"
"		StringArray.push(TimerInfo[nHoverToken].name);\n"
"		StringArray.push(\"Group\");\n"
"		StringArray.push(GroupInfo[groupid].name);\n"
"\n"
"		var bShowTimers = Mode == ModeTimers;\n"
"		if(FlipToolTip)\n"
"		{\n"
"			bShowTimers = !bShowTimers;\n"
"		}\n"
"		if(bShowTimers)\n"
"		{\n"
"\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"			var Timer = TimerInfo[nHoverToken];\n"
"			StringArray.push(\"Average\");\n"
"			StringArray.push(Timer.average);\n"
"			StringArray.push(\"Max\");\n"
"			StringArray.push(Timer.max);\n"
"			StringArray.push(\"Excl Max\");\n"
"			StringArray.push(Timer.exclmax);\n"
"			StringArray.push(\"Excl Average\");\n"
"			StringArray.push(Timer.exclaverage);\n"
"			StringArray.push(\"Call Average\");\n"
"			StringArray.push(Timer.callaverage);\n"
"			StringArray.push(\"Call Count\");\n"
"			StringArray.push(Timer.callcount);\n"
"\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"\n"
"\n"
"			StringArray.push(\"Group\");\n"
"			StringArray.push(GroupInfo[groupid].name);\n"
"			StringArray.push(\"Average\");\n"
"			StringArray.push(GroupInfo[groupid].average);\n"
"			StringArray.push(\"Max\");\n"
"			StringArray.push(GroupInfo[groupid].max);\n"
"\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"\n"
"			StringArray.push(\"Timer Capture\");\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"Frames\");\n"
"			StringArray.push(AggregateInfo.Frames);\n"
"			StringArray.push(\"Time\");\n"
"			StringArray.push(AggregateInfo.Time.toFixed(2) + \"ms\");\n"
"\n"
"\n"
"\n"
"\n"
"		}\n"
"		else\n"
"		{\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"\n"
"\n"
"\n"
"			StringArray.push(\"Time\");\n"
"			StringArray.push((fRangeEnd-fRangeBegin).toFixed(3));\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"Total\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].Sum);\n"
"			StringArray.push(\"Max\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].Max);\n"
"			StringArray.push(\"Average\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].Average);\n"
"			StringArray.push(\"Count\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].CallCount);\n"
"\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"\n"
"			StringArray.push(\"Max/Frame\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].FrameMax);\n"
"\n"
"			StringArray.push(\"Average Time/Frame\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].FrameAverage);\n"
"\n"
"			StringArray.push(\"Average Count/Frame\");\n"
"			StringArray.push(\"\" + TimerInfo[nHoverToken].FrameCallAverage);\n"
"\n"
"\n"
"\n"
"\n"
"\n"
"		\n"
"			if(nHoverFrame != -1)\n"
"			{\n"
"				StringArray.push(\"\");\n"
"				StringArray.push(\"\");\n"
"				StringArray.push(\"Frame \" + nHoverFrame);\n"
"				StringArray.push(\"\");\n"
"\n"
"				var FrameTime = new Object();\n"
"				CalculateTimers(FrameTime, nHoverToken, nHoverFrame, nHoverFrame+1);\n"
"				StringArray.push(\"Total\");\n"
"				StringArray.push(\"\" + FrameTime.Sum);\n"
"				StringArray.push(\"Count\");\n"
"				StringArray.push(\"\" + FrameTime.CallCount);\n"
"				StringArray.push(\"Average\");\n"
"				StringArray.push(\"\" + FrameTime.Average);\n"
"				StringArray.push(\"Max\");\n"
"				StringArray.push(\"\" + FrameTime.Max);\n"
"			}\n"
"\n"
"			var HoverInfo = GatherHoverMetaCounters(nHoverToken, nHoverTokenIndex, nHoverTokenLogIndex, nHoverFrame);\n"
"			var Header = 0;\n"
"			for(index in HoverInfo)\n"
"			{\n"
"				if(0 == Header)\n"
"				{\n"
"					Header = 1;\n"
"					StringArray.push(\"\");\n"
"					StringArray.push(\"\");\n"
"					StringArray.push(\"Meta\");\n"
"					StringArray.push(\"\");\n"
"\n"
"				}\n"
"				StringArray.push(\"\"+index);\n"
"				StringArray.push(\"\"+HoverInfo[index]);\n"
"			}\n"
"\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"\");\n"
"\n"
"			StringArray.push(\"Detailed Capture\");\n"
"			StringArray.push(\"\");\n"
"			StringArray.push(\"Frames\");\n"
"			StringArray.push(Frames.length);\n"
"			StringArray.push(\"Time\");\n"
"			StringArray.push(DetailedTotal().toFixed(2) + \"ms\");\n"
"\n"
"\n"
"		}\n"
"		DrawToolTip(StringArray, CanvasDetailedView, DetailedViewMouseX, DetailedViewMouseY+20);\n"
"	}\n"
"	else if(nHoverCSCpu >= 0)\n"
"	{\n"
"		var StringArray = [];\n"
"		StringArray.push(\"Context Switch\");\n"
"		StringArray.push(\"\");\n"
"		StringArray.push(\"\");\n"
"		StringArray.push(\"\");\n"
"		StringArray.push(\"Cpu\");\n"
"		StringArray.push(\"\" + nHoverCSCpu);\n"
"		StringArray.push(\"Begin\");\n"
"		StringArray.push(\"\" + fRangeBegin);\n"
"		StringArray.push(\"End\");\n"
"		StringArray.push(\"\" + fRangeEnd);\n"
"		DrawToolTip(StringArray, CanvasDetailedView, DetailedViewMouseX, DetailedViewMouseY+20);\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function FormatMeta(Value, Dec)\n"
"{\n"
"	if(!Value)\n"
"	{\n"
"		Value = \"0\";\n"
"	}\n"
"	else\n"
"	{\n"
"		Value = \'\' + Value.toFixed(Dec);\n"
"	}\n"
"	return Value;\n"
"}\n"
"\n"
"function DrawBarView()\n"
"{\n"
"	ProfileEnter(\"DrawBarView\");\n"
"	Invalidate++;\n"
"	nHoverToken = -1;\n"
"	nHoverFrame = -1;\n"
"	var context = CanvasDetailedView.getContext(\'2d\');\n"
"	context.clearRect(0, 0, nWidth, nHeight);\n"
"\n"
"	var Height = BoxHeight;\n"
"	var Width = nWidth;\n"
"\n"
"	//clamp offset to prevent scrolling into the void\n"
"	var nTotalRows = 0;\n"
"	for(var groupid in GroupInfo)\n"
"	{\n"
"		if(GroupsAllActive || GroupsActive[GroupInfo[groupid].name])\n"
"		{\n"
"			nTotalRows += GroupInfo[groupid].TimerArray.length + 1;\n"
"		}\n"
"	}\n"
"	var nTotalRowPixels = nTotalRows * Height;\n"
"	var nFrameRows = nHeight - BoxHeight;\n"
"	if(nOffsetBarsY + nFrameRows > nTotalRowPixels && nTotalRowPixels > nFrameRows)\n"
"	{\n"
"		nOffsetBarsY = nTotalRowPixels - nFrameRows;\n"
"	}\n"
"\n"
"\n"
"	var Y = -nOffsetBarsY + BoxHeight;\n"
"	if(TimersGroups)\n"
"	{\n"
"		nOffsetBarsX = 0;\n"
"	}\n"
"	var XBase = -nOffsetBarsX;\n"
"	var nColorIndex = 0;\n"
"\n"
"	context.fillStyle = \'white\';\n"
"	context.font = Font;\n"
"	var bMouseIn = 0;\n"
"	var RcpReferenceTime = 1.0 / ReferenceTime;\n"
"	var CountWidth = 8 * FontWidth;\n"
"	var nMetaLen = TimerInfo[0].meta.length;\n"
"	var nMetaCharacters = 10;\n"
"	for(var i = 0; i < nMetaLen; ++i)\n"
"	{\n"
"		if(nMetaCharacters < MetaNames[i].length)\n"
"			nMetaCharacters = MetaNames[i].length;\n"
"	}\n"
"	var nWidthMeta = nMetaCharacters * FontWidth + 6;\n"
"	function DrawHeaderSplit(Header)\n"
"	{\n"
"		context.fillStyle = \'white\';\n"
"		context.fillText(Header, X, Height-FontAscent);\n"
"		X += nWidthBars;\n"
"		context.fillStyle = nBackColorOffset;\n"
"		X += nWidthMs;\n"
"		if(X >= NameWidth)\n"
"		{\n"
"			context.fillRect(X-3, 0, 1, nHeight);\n"
"		}\n"
"	}\n"
"	function DrawHeaderSplitSingle(Header, Width)\n"
"	{\n"
"		context.fillStyle = \'white\';\n"
"		context.fillText(Header, X, Height-FontAscent);\n"
"		X += Width;\n"
"		context.fillStyle = nBackColorOffset;\n"
"		if(X >= NameWidth)\n"
"		{\n"
"			context.fillRect(X-3, 0, 1, nHeight);\n"
"		}\n"
"	}\n"
"	function DrawHeaderSplitLeftRight(HeaderLeft, HeaderRight, Width)\n"
"	{\n"
"		context.textAlign = \'left\';\n"
"		context.fillStyle = \'white\';\n"
"		context.fillText(HeaderLeft, X, Height-FontAscent);\n"
"		X += Width;\n"
"		context.textAlign = \'right\';\n"
"		context.fillText(HeaderRight, X-5, Height-FontAscent);\n"
"		context.textAlign = \'left\';\n"
"		context.fillStyle = nBackColorOffset;\n"
"		if(X >= NameWidth)\n"
"		{\n"
"			context.fillRect(X-3, 0, 1, nHeight);\n"
"		}\n"
"	}\n"
"	function DrawTimer(Value, Color)\n"
"	{\n"
"		var Prc = Value * RcpReferenceTime;\n"
"		var YText = Y+Height-FontAscent;\n"
"		if(Prc > 1)\n"
"		{\n"
"			Prc = 1;\n"
"		}\n"
"		context.fillStyle = Color;\n"
"		context.fillRect(X+1, Y+1, Prc * nBarsWidth, InnerBoxHeight);\n"
"		X += nWidthBars;\n"
"		context.fillStyle = \'white\';\n"
"		context.fillText((\"      \" + Value.toFixed(2)).slice(-TimerLen), X, YText);\n"
"		X += nWidthMs;\n"
"	}\n"
"	function DrawMeta(Value, Width, Dec)\n"
"	{\n"
"		Value = FormatMeta(Value, Dec);\n"
"		X += (FontWidth*Width);\n"
"		context.textAlign = \'right\';\n"
"		context.fillText(Value, X-FontWidth, YText);\n"
"		context.textAlign = \'left\';\n"
"	}\n"
"	var InnerBoxHeight = BoxHeight-2;\n"
"	var TimerLen = 6;\n"
"	var TimerWidth = TimerLen * FontWidth;\n"
"	var nWidthBars = nBarsWidth+2;\n"
"	var nWidthMs = TimerWidth+2+10;\n"
"\n"
"\n"
"	if(2 == TimersGroups)\n"
"	{\n"
"		for(var i = 0; i < ThreadNames.length; ++i)\n"
"		{\n"
"			if(ThreadsActive[ThreadNames[i]] || ThreadsAllActive)\n"
"			{\n"
"				var X = 0;\n"
"				var YText = Y+Height-FontAscent;\n"
"				bMouseIn = DetailedViewMouseY >= Y && DetailedViewMouseY < Y + BoxHeight;\n"
"				nColorIndex = 1-nColorIndex;\n"
"				context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"				context.fillRect(0, Y, Width, Height);\n"
"				var ThreadColor = CSwitchColors[i % CSwitchColors.length];\n"
"				context.fillStyle = ThreadColor;\n"
"				context.fillText(ThreadNames[i], 1, YText);\n"
"				context.textAlign = \'left\';\n"
"				Y += Height;\n"
"				for(var idx in GroupOrder)\n"
"				{\n"
"					var groupid = GroupOrder[idx];\n"
"					var Group = GroupInfo[groupid];\n"
"					var PerThreadTimer = ThreadGroupTimeArray[i][groupid];\n"
"					var PerThreadTimerTotal = ThreadGroupTimeTotalArray[i][groupid];\n"
"					if((PerThreadTimer > 0.0001|| PerThreadTimerTotal>0.1) && (GroupsAllActive || GroupsActive[Group.name]))\n"
"					{\n"
"						var GColor = GroupColors ? GroupInfo[groupid].color : \'white\';\n"
"						var X = 0;\n"
"						nColorIndex = 1-nColorIndex;\n"
"						bMouseIn = DetailedViewMouseY >= Y && DetailedViewMouseY < Y + BoxHeight;\n"
"						context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"						context.fillRect(0, Y, Width, nHeight);\n"
"						context.fillStyle = GColor;\n"
"						context.textAlign = \'right\';\n"
"						context.fillText(Group.name, NameWidth - 5, Y+Height-FontAscent);\n"
"						context.textAlign = \'left\';\n"
"						X += NameWidth;\n"
"						DrawTimer(PerThreadTimer, GColor);\n"
"						X += nWidthBars + nWidthMs;	\n"
"						DrawTimer(PerThreadTimerTotal, GColor);\n"
"\n"
"						Y += Height;\n"
"			";

const size_t g_MicroProfileHtml_end_0_size = sizeof(g_MicroProfileHtml_end_0);
const char g_MicroProfileHtml_end_1[] =
"		}\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		for(var idx in GroupOrder)\n"
"		{\n"
"			var groupid = GroupOrder[idx];\n"
"			var Group = GroupInfo[groupid];\n"
"			var GColor = GroupColors ? GroupInfo[groupid].color : \'white\';\n"
"			if(GroupsAllActive || GroupsActive[Group.name])\n"
"			{\n"
"				var TimerArray = Group.TimerArray;\n"
"				var X = XBase;\n"
"				nColorIndex = 1-nColorIndex;\n"
"				bMouseIn = DetailedViewMouseY >= Y && DetailedViewMouseY < Y + BoxHeight;\n"
"				context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"				context.fillRect(0, Y, Width, nHeight);\n"
"				context.fillStyle = GColor;\n"
"				context.fillText(Group.name, 1, Y+Height-FontAscent);\n"
"				X += NameWidth;\n"
"				DrawTimer(Group.average, GColor);\n"
"				DrawTimer(Group.max, GColor);\n"
"				DrawTimer(Group.total, GColor);\n"
"\n"
"				context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"				context.fillRect(0, Y, NameWidth, nHeight);\n"
"				context.fillStyle = GColor;\n"
"				context.fillText(Group.name, 1, Y+Height-FontAscent);\n"
"\n"
"\n"
"\n"
"				Y += Height;\n"
"				if(TimersGroups)\n"
"				{\n"
"					for(var i = 0; i < ThreadNames.length; ++i)\n"
"					{\n"
"						var PerThreadTimer = ThreadGroupTimeArray[i][groupid];\n"
"						var PerThreadTimerTotal = ThreadGroupTimeTotalArray[i][groupid];\n"
"						if((PerThreadTimer > 0.0001|| PerThreadTimerTotal>0.1) && (ThreadsActive[ThreadNames[i]] || ThreadsAllActive))\n"
"						{\n"
"							var YText = Y+Height-FontAscent;\n"
"							bMouseIn = DetailedViewMouseY >= Y && DetailedViewMouseY < Y + BoxHeight;\n"
"							nColorIndex = 1-nColorIndex;\n"
"							context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"							context.fillRect(0, Y, Width, Height);\n"
"							var ThreadColor = CSwitchColors[i % CSwitchColors.length];\n"
"							context.fillStyle = ThreadColor;\n"
"							context.textAlign = \'right\';\n"
"							context.fillText(ThreadNames[i], NameWidth - 5, YText);\n"
"							context.textAlign = \'left\';\n"
"							X = NameWidth;\n"
"							DrawTimer(PerThreadTimer, ThreadColor);\n"
"							X += nWidthBars + nWidthMs;	\n"
"							DrawTimer(PerThreadTimerTotal, ThreadColor);\n"
"							Y += Height;\n"
"						}\n"
"					}\n"
"				}\n"
"				else\n"
"				{\n"
"					for(var timerindex in TimerArray)\n"
"					{\n"
"						var timerid = TimerArray[timerindex];\n"
"						var Timer = TimerInfo[timerid];\n"
"						var Average = Timer.average;\n"
"						var Max = Timer.max;\n"
"						var ExclusiveMax = Timer.exclmax;\n"
"						var ExclusiveAverage = Timer.exclaverage;\n"
"						var CallAverage = Timer.callaverage;\n"
"						var CallCount = Timer.callcount;\n"
"						var YText = Y+Height-FontAscent;\n"
"						X = NameWidth + XBase;\n"
"\n"
"						nColorIndex = 1-nColorIndex;\n"
"						bMouseIn = DetailedViewMouseY >= Y && DetailedViewMouseY < Y + BoxHeight;\n"
"						if(bMouseIn)\n"
"						{\n"
"							nHoverToken = timerid;\n"
"						}\n"
"						context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"						context.fillRect(0, Y, Width, Height);\n"
"\n"
"						DrawTimer(Average, Timer.color);\n"
"						DrawTimer(Max,Timer.color);\n"
"						DrawTimer(Timer.total,Timer.color);\n"
"						DrawTimer(CallAverage,Timer.color);\n"
"						context.fillStyle = \'white\';\n"
"						context.fillText(CallCount, X, YText);\n"
"						X += CountWidth;\n"
"						DrawTimer(ExclusiveAverage,Timer.color);\n"
"						DrawTimer(ExclusiveMax,Timer.color);\n"
"\n"
"						if(TimersMeta)\n"
"						{\n"
"							context.fillStyle = \'white\';\n"
"							for(var j = 0; j < nMetaLen; ++j)\n"
"							{\n"
"								// var Len = MetaNames[j].length + 1;\n"
"								DrawMeta(Timer.meta[j], MetaLengths[j], 0);\n"
"								DrawMeta(Timer.metaavg[j], MetaLengthsAvg[j], 2);\n"
"								DrawMeta(Timer.metamax[j], MetaLengthsMax[j], 0);\n"
"							}\n"
"						}\n"
"						context.fillStyle = bMouseIn ? nBackColorOffset : nBackColors[nColorIndex];\n"
"						context.fillRect(0, Y, NameWidth, Height);\n"
"						context.textAlign = \'right\';\n"
"						context.fillStyle = Timer.color;\n"
"						context.fillText(Timer.name, NameWidth - 5, YText);\n"
"						context.textAlign = \'left\';\n"
"\n"
"\n"
"						Y += Height;\n"
"					}			\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"	X = 0;\n"
"	context.fillStyle = nBackColorOffset;\n"
"	context.fillRect(0, 0, Width, Height);\n"
"	context.fillStyle = \'white\';\n"
"	if(TimersGroups)\n"
"	{\n"
"		if(2 == TimersGroups)\n"
"		{\n"
"			DrawHeaderSplitLeftRight(\'Thread\', \'Group\', NameWidth);\n"
"			DrawHeaderSplit(\'Average\');\n"
"		}\n"
"		else\n"
"		{\n"
"			DrawHeaderSplitLeftRight(\'Group\', \'Thread\', NameWidth);\n"
"			DrawHeaderSplit(\'Average\');\n"
"			DrawHeaderSplit(\'Max\');\n"
"			DrawHeaderSplit(\'Total\');\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		X = NameWidth + XBase;\n"
"		DrawHeaderSplit(\'Average\');\n"
"		DrawHeaderSplit(\'Max\');\n"
"		DrawHeaderSplit(\'Total\');\n"
"		DrawHeaderSplit(\'Call Average\');\n"
"		DrawHeaderSplitSingle(\'Count\', CountWidth);\n"
"		DrawHeaderSplit(\'Excl Average\');\n"
"		DrawHeaderSplit(\'Excl Max\');\n"
"		if(TimersMeta)\n"
"		{\n"
"			for(var i = 0; i < nMetaLen; ++i)\n"
"			{\n"
"				DrawHeaderSplitSingle(MetaNames[i], MetaLengths[i] * FontWidth);\n"
"				DrawHeaderSplitSingle(MetaNames[i] + \" Avg\", MetaLengthsAvg[i] * FontWidth);\n"
"				DrawHeaderSplitSingle(MetaNames[i] + \" Max\", MetaLengthsMax[i] * FontWidth);\n"
"			}\n"
"		}\n"
"		X = 0;\n"
"		context.fillStyle = nBackColorOffset;\n"
"		context.fillRect(0, 0, NameWidth, Height);\n"
"		context.fillStyle = \'white\';\n"
"	\n"
"		DrawHeaderSplitLeftRight(\'Group\', \'Timer\', NameWidth);\n"
"	\n"
"	}\n"
"\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"\n"
"//preprocess context switch data to contain array per thread\n"
"function PreprocessContextSwitchCacheItem(ThreadId)\n"
"{\n"
"	console.log(\'context switch preparing \' + ThreadId);\n"
"	var CSObject = CSwitchCache[ThreadId];\n"
"	if(ThreadId > 0 && !CSObject)\n"
"	{\n"
"		CSArrayIn = new Array();\n"
"		CSArrayOut = new Array();\n"
"		CSArrayCpu = new Array();\n"
"		var nCount = CSwitchTime.length;\n"
"		var j = 0;\n"
"		var TimeIn = -1.0;\n"
"		for(var i = 0; i < nCount; ++i)\n"
"		{	\n"
"			var ThreadIn = CSwitchThreadInOutCpu[j];\n"
"			var ThreadOut = CSwitchThreadInOutCpu[j+1];\n"
"			var Cpu = CSwitchThreadInOutCpu[j+2];\n"
"			if(TimeIn < 0)\n"
"			{\n"
"				if(ThreadIn == ThreadId)\n"
"				{\n"
"					TimeIn = CSwitchTime[i];\n"
"				}\n"
"			}\n"
"			else\n"
"			{\n"
"				if(ThreadOut == ThreadId)\n"
"				{\n"
"					var TimeOut = CSwitchTime[i];\n"
"					CSArrayIn.push(TimeIn);\n"
"					CSArrayOut.push(TimeOut);\n"
"					CSArrayCpu.push(Cpu);\n"
"					TimeIn = -1;\n"
"				}\n"
"			}\n"
"			j += 3;\n"
"		}\n"
"		CSObject = new Object();\n"
"		CSObject.Size = CSArrayIn.length;\n"
"		CSObject.In = CSArrayIn;\n"
"		CSObject.Out = CSArrayOut;\n"
"		CSObject.Cpu = CSArrayCpu;\n"
"		CSwitchCache[ThreadId] = CSObject;\n"
"	}\n"
"\n"
"}\n"
"function PreprocessContextSwitchCache()\n"
"{\n"
"	ProfileEnter(\"PreprocessContextSwitchCache\");\n"
"	var AllThreads = {};\n"
"	var nCount = CSwitchTime.length;\n"
"	for(var i = 0; i < nCount; ++i)\n"
"	{	\n"
"		var nThreadIn = CSwitchThreadInOutCpu[j];\n"
"		if(!AllThreads[nThreadIn])\n"
"		{\n"
"			AllThreads[nThreadIn] = \'\' + nThreadIn;\n"
"		}\n"
"	}\n"
"	for(var key in AllThreads)\n"
"	{\n"
"		var value = AllThreads[key];\n"
"		var FoundThread = false;\n"
"		for(var i = 0; i < ThreadIds.length; ++i)\n"
"		{\n"
"			if(ThreadIds[i] == key)\n"
"			{\n"
"				FoundThread = true;\n"
"			}\n"
"		}\n"
"		if(!FoundThread)\n"
"		{\n"
"			console.log(\'CS only thread\' + value);\n"
"			CSwitchOnlyThreads.push(value);\n"
"		}\n"
"	}\n"
"	for(var i = 0; i < CSwitchOnlyThreads.length; ++i)\n"
"	{\n"
"		PreprocessContextSwitchCacheItem(CSwitchOnlyThreads[i]);\n"
"	}\n"
"	for(var i = 0; i < ThreadIds.length; ++i)\n"
"	{\n"
"		PreprocessContextSwitchCacheItem(ThreadIds[i]);	\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function DrawContextSwitchBars(context, ThreadId, fScaleX, fOffsetY, fDetailedOffset, nHoverColor, MinWidth, bDrawEnabled)\n"
"{\n"
"	ProfileEnter(\"DrawContextSwitchBars\");\n"
"	var CSObject = CSwitchCache[ThreadId];\n"
"	if(ThreadId > 0 && !CSObject)\n"
"	{\n"
"		CSArrayIn = new Array();\n"
"		CSArrayOut = new Array();\n"
"		CSArrayCpu = new Array();\n"
"		var nCount = CSwitchTime.length;\n"
"		var j = 0;\n"
"		var TimeIn = -1.0;\n"
"		for(var i = 0; i < nCount; ++i)\n"
"		{	\n"
"			var ThreadIn = CSwitchThreadInOutCpu[j];\n"
"			var ThreadOut = CSwitchThreadInOutCpu[j+1];\n"
"			var Cpu = CSwitchThreadInOutCpu[j+2];\n"
"			if(TimeIn < 0)\n"
"			{\n"
"				if(ThreadIn == ThreadId)\n"
"				{\n"
"					TimeIn = CSwitchTime[i];\n"
"				}\n"
"			}\n"
"			else\n"
"			{\n"
"				if(ThreadOut == ThreadId)\n"
"				{\n"
"					var TimeOut = CSwitchTime[i];\n"
"					CSArrayIn.push(TimeIn);\n"
"					CSArrayOut.push(TimeOut);\n"
"					CSArrayCpu.push(Cpu);\n"
"					TimeIn = -1;\n"
"				}\n"
"			}\n"
"			j += 3;\n"
"		}\n"
"		CSObject = new Object();\n"
"		CSObject.Size = CSArrayIn.length;\n"
"		CSObject.In = CSArrayIn;\n"
"		CSObject.Out = CSArrayOut;\n"
"		CSObject.Cpu = CSArrayCpu;\n"
"		CSwitchCache[ThreadId] = CSObject;\n"
"	}\n"
"	if(CSObject)\n"
"	{\n"
"		var Size = CSObject.Size;		\n"
"		var In = CSObject.In;\n"
"		var Out = CSObject.Out;\n"
"		var Cpu = CSObject.Cpu;\n"
"		var nNumColors = CSwitchColors.length;\n"
"		for(var i = 0; i < Size; ++i)\n"
"		{\n"
"			var TimeIn = In[i];\n"
"			var TimeOut = Out[i];\n"
"			var ActiveCpu = Cpu[i];\n"
"\n"
"			var X = (TimeIn - fDetailedOffset) * fScaleX;\n"
"			if(X > nWidth)\n"
"			{\n"
"				break;\n"
"			}\n"
"			var W = (TimeOut - TimeIn) * fScaleX;\n"
"			if(W > MinWidth && X+W > 0)\n"
"			{\n"
"				if(nHoverCSCpu == ActiveCpu || bDrawEnabled)\n"
"				{\n"
"					if(nHoverCSCpu == ActiveCpu)\n"
"					{\n"
"						context.fillStyle = nHoverColor;\n"
"					}\n"
"					else\n"
"					{\n"
"						context.fillStyle = CSwitchColors[ActiveCpu % nNumColors];\n"
"					}\n"
"					context.fillRect(X, fOffsetY, W, CSwitchHeight);\n"
"				}\n"
"				if(DetailedViewMouseX >= X && DetailedViewMouseX <= X+W && DetailedViewMouseY < fOffsetY+CSwitchHeight && DetailedViewMouseY >= fOffsetY)\n"
"				{\n"
"					nHoverCSCpuNext = ActiveCpu;\n"
"					fRangeBeginNext = TimeIn;\n"
"					fRangeEndNext = TimeOut;\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function DrawDetailedView(context, MinWidth, bDrawEnabled)\n"
"{\n"
"	if(bDrawEnabled)\n"
"	{\n"
"		DrawDetailedBackground(context);\n"
"	}\n"
"\n"
"	var colors = [ \'#ff0000\', \'#ff00ff\', \'#ffff00\'];\n"
"\n"
"	var fScaleX = nWidth / fDetailedRange; \n"
"	var fOffsetY = -nOffsetY + BoxHeight;\n"
"	nHoverTokenNext = -1;\n"
"	nHoverTokenLogIndexNext = -1;\n"
"	nHoverTokenIndexNext = -1;\n"
"	nHoverCounter += nHoverCounterDelta;\n"
"	if(nHoverCounter >= 255) \n"
"	{\n"
"		nHoverCounter = 255;\n"
"		nHoverCounterDelta = -nHoverCounterDelta;\n"
"	}\n"
"	if(nHoverCounter < 128) \n"
"	{\n"
"		nHoverCounter = 128;\n"
"		nHoverCounterDelta = -nHoverCounterDelta;\n"
"	}\n"
"	var nHoverHigh = nHoverCounter.toString(16);\n"
"	var nHoverLow = (127+255-nHoverCounter).toString(16);\n"
"	var nHoverColor = \'#\' + nHoverHigh + nHoverHigh + nHoverHigh;\n"
"\n"
"	context.fillStyle = \'black\';\n"
"	context.font = Font;\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	var fTimeEnd = fDetailedOffset + fDetailedRange;\n"
"\n"
"	var FirstFrame = 0;\n"
"	for(var i = 0; i < Frames.length ; i++)\n"
"	{\n"
"		if(Frames[i].frameend < fDetailedOffset)\n"
"		{\n"
"			FirstFrame = i;\n"
"		}\n"
"	}\n"
"	var nMinTimeMs = MinWidth / fScaleX;\n"
"	{\n"
"\n"
"		var Batches = new Array(TimerInfo.length);\n"
"		var BatchesTxt = Array();\n"
"		var BatchesTxtPos = Array();\n"
"		var BatchesTxtColor = [\'#ffffff\', \'#333333\'];\n"
"\n"
"		for(var i = 0; i < 2; ++i)\n"
"		{\n"
"			BatchesTxt[i] = Array();\n"
"			BatchesTxtPos[i] = Array();\n"
"		}\n"
"		for(var i = 0; i < Batches.length; ++i)\n"
"		{\n"
"			Batches[i] = Array();\n"
"		}\n"
"		for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"		{\n"
"			var ThreadName = ThreadNames[nLog];\n"
"			if(ThreadsAllActive || ThreadsActive[ThreadName])\n"
"			{\n"
"\n"
"				var LodIndex = 0;\n"
"				var MinDelta = 0;\n"
"				var NextLod = 1;\n"
"				while(NextLod < LodData.length && LodData[NextLod].MinDelta[nLog] < nMinTimeMs)\n"
"				{\n"
"					LodIndex = NextLod;\n"
"					NextLod = NextLod + 1;\n"
"					MinDelta = LodData[LodIndex].MinDelta[nLog];\n"
"				}\n"
"				if(LodIndex == LodData.length)\n"
"				{\n"
"					LodIndex = LodData.length-1;\n"
"				}\n"
"				if(DisableLod)\n"
"				{\n"
"					LodIndex = 0;\n"
"				}\n"
"\n"
"				context.fillStyle = \'white\';\n"
"				context.fillText(ThreadName, 0, fOffsetY);\n"
"				fOffsetY += BoxHeight;\n"
"				if(nContextSwitchEnabled)\n"
"				{\n"
"					DrawContextSwitchBars(context, ThreadIds[nLog], fScaleX, fOffsetY, fDetailedOffset, nHoverColor, MinWidth, bDrawEnabled);\n"
"					fOffsetY += CSwitchHeight+1;\n"
"				}\n"
"				var MaxDepth = 1;\n"
"				var StackPos = 0;\n"
"				var Stack = Array(20);\n"
"				var Lod = LodData[LodIndex];\n"
"\n"
"				var TypeArray = Lod.TypeArray[nLog];\n"
"				var IndexArray = Lod.IndexArray[nLog];\n"
"				var TimeArray = Lod.TimeArray[nLog];\n"
"\n"
"				var LocalFirstFrame = Frames[FirstFrame].FirstFrameIndex[nLog];\n"
"				var IndexStart = Lod.LogStart[LocalFirstFrame][nLog];\n"
"				var IndexEnd = TimeArray.length;\n"
"				IndexEnd = TimeArray.length;\n"
"				var HasSetHover = 0;\n"
"\n"
"\n"
"				for(var j = IndexStart; j < IndexEnd; ++j)\n"
"				{\n"
"					var type = TypeArray[j];\n"
"					var index = IndexArray[j];\n"
"					var time = TimeArray[j];\n"
"					if(type == 1)\n"
"					{\n"
"						//push\n"
"						Stack[StackPos] = time;\n"
"						StackPos++;\n"
"						if(StackPos > MaxDepth)\n"
"						{\n"
"							MaxDepth = StackPos;\n"
"						}\n"
"					}\n"
"					else if(type == 0)\n"
"					{\n"
"						if(StackPos>0)\n"
"						{\n"
"							StackPos--;\n"
"\n"
"							var timestart = Stack[StackPos];\n"
"							var timeend = time;\n"
"							var X = (timestart - fDetailedOffset) * fScaleX;\n"
"							var Y = fOffsetY + StackPos * BoxHeight;\n"
"							var W = (timeend-timestart)*fScaleX;\n"
"\n"
"							if(W > MinWidth && X < nWidth && X+W > 0)\n"
"							{\n"
"								if(bDrawEnabled || index == nHoverToken)\n"
"								{\n"
"									Batches[index].push(X);\n"
"									Batches[index].push(Y);\n"
"									Batches[index].push(W);\n"
"									DebugDrawQuadCount++;\n"
"\n"
"									var XText = X < 0 ? 0 : X;\n"
"									var WText = W - (XText-X);\n"
"									var name = TimerInfo[index].name;\n"
"									var len = TimerInfo[index].len;\n"
"									var sublen = Math.floor((WText-2)/FontWidth);\n"
"									if(sublen >= 2)\n"
"									{\n"
"										if(sublen < len)\n"
"											name = name.substr(0, sublen);\n"
"										var txtidx = TimerInfo[index].textcolorindex;\n"
"										BatchesTxt[txtidx].push(name);\n"
"										BatchesTxtPos[txtidx].push(XText+1);\n"
"										BatchesTxtPos[txtidx].push(Y+BoxHeight-FontAscent);\n"
"										DebugDrawTextCount++;\n"
"									}\n"
"								}\n"
"\n"
"								if(DetailedViewMouseX >= X && DetailedViewMouseX <= X+W && DetailedViewMouseY < Y+BoxHeight && DetailedViewMouseY >= Y)\n"
"								{\n"
"									fRangeBeginNext = timestart;\n"
"									fRangeEndNext = timeend;\n"
"									nHoverTokenNext = index;\n"
"									nHoverTokenIndexNext = j;\n"
"									nHoverTokenLogIndexNext = nLog;\n"
"									bHasSetHover = 1;\n"
"								}\n"
"							}\n"
"							if(StackPos == 0 && time > fTimeEnd)\n"
"								break;											\n"
"						}\n"
"					}\n"
"				}\n"
"				fOffsetY += (1+g_MaxStack[nLog]) * BoxHeight;\n"
"\n"
"				if(HasSetHover)\n"
"				{\n"
"					for(var i = 0; i < Frames.length-1; ++i)\n"
"					{\n"
"						var IndexStart = Lod.LogStart[i][nLog];\n"
"						if(nHoverTokenNext >= IndexStart)\n"
"						{\n"
"							nHoverFrame = i;\n"
"						}\n"
"					}\n"
"				}\n"
"			}\n"
"		}\n"
"\n"
"		if(0 && nContextSwitchEnabled) //non instrumented threads.\n"
"		{\n"
"			var ContextSwitchThreads = CSwitchOnlyThreads;\n"
"			for(var i = 0; i < ContextSwitchThreads.length; ++i)\n"
"			{\n"
"				var ThreadId = ContextSwitchThreads[i];\n"
"				var ThreadName = \'\' + ThreadId;\n"
"				DrawContextSwitchBars(context, ThreadId, fScaleX, fOffsetY, fDetailedOffset, nHoverColor, MinWidth, bDrawEnabled);\n"
"				context.fillStyle = \'white\';\n"
"				context.fillText(ThreadName, 0, fOffsetY);\n"
"				fOffsetY += CSwitchHeight+1;\n"
"			}\n"
"		}\n"
"\n"
"\n"
"		{\n"
"			for(var i = 0; i < Batches.length; ++i)\n"
"			{\n"
"				var a = Batches[i];\n"
"				if(a.length)\n"
"				{\n"
"					if(i == nHoverToken)\n"
"					{\n"
"						context.fillStyle = nHoverColor;\n"
"					}\n"
"					else\n"
"					{\n"
"						context.fillStyle = TimerInfo[i].color;\n"
"					}\n"
"					for(var j = 0; j < a.length; j += 3)\n"
"					{						\n"
"						var X = a[j];\n"
"						var Y = a[j+1];\n"
"						var W = a[j+2];\n"
"						while(j+1 < a.length)\n"
"						{\n"
"							var jnext = j+3;\n"
"							var XNext = a[jnext];\n"
"							var YNext = a[jnext+1];\n"
"							var WNext = a[jnext+2];\n"
"							var Delta = XNext - (X+W);\n"
"							var YDelta = Math.abs(Y - YNext);							\n"
"							if(Delta < 0.3 && YDelta < 0.5 && !DisableMerge)\n"
"							{\n"
"								W = (XNext+WNext) - X;\n"
"								j += 3;\n"
"							}\n"
"							else\n"
"							{\n"
"								break;\n"
"							}\n"
"\n"
"						}\n"
"						context.fillRect(X, Y, W, BoxHeight-1);\n"
"					}\n"
"				}\n"
"			}	\n"
"		}\n"
"		for(var i = 0; i < BatchesTxt.length; ++i)\n"
"		{\n"
"			context.fillStyle = BatchesTxtColor[i];\n"
"			var TxtArray = BatchesTxt[i];\n"
"			var PosArray = BatchesTxtPos[i];\n"
"			for(var j = 0; j < TxtArray.length; ++j)\n"
"			{\n"
"				var k = j * 2;\n"
"				context.fillText(TxtArray[j], PosArray[k],PosArray[k+1]);\n"
"			}\n"
"		}\n"
"\n"
"	}\n"
"}\n"
"function DrawTextBox(context, text, x, y, align)\n"
"{\n"
"	var textsize = context.measureText(text).width;\n"
"	var offsetx = 0;\n"
"	var offsety = -FontHeight;\n"
"	if(align == \'center\')\n"
"	{\n"
"		offsetx = -textsize / 2.0;\n"
"	}\n"
"	else if(align == \'right\')\n"
"	{\n"
"		offsetx = -textsize;\n"
"	}\n"
"	context.fillStyle = nBackColors[0];\n"
"	context.fillRect(x + offsetx, y + offsety, textsize+2, FontHeight + 2);\n"
"	context.fillStyle = \'white\';\n"
"	context.fillText(text, x, y);\n"
"\n"
"}\n"
"function DrawDetailed(Animation)\n"
"{\n"
"	if(AnimationActive != Animation || !Initialized)\n"
"	{\n"
"		return;\n"
"	}\n"
"	ProfileEnter(\"DrawDetailed\");\n"
"	DebugDrawQuadCount = 0;\n"
"	DebugDrawTextCount = 0;\n"
"	nHoverCSCpuNext = -1;\n"
"\n"
"	fRangeBeginNext = fRangeEndNext = -1;\n"
"\n"
"	var start = new Date();\n"
"	nDrawCount++;\n"
"\n"
"	var context = CanvasDetailedView.getContext(\'2d\');\n"
"	var offscreen = CanvasDetailedOffscreen.getContext(\'2d\');\n"
"	var fScaleX = nWidth / fDetailedRange; \n"
"	var fOffsetY = -nOffsetY + BoxHeight;\n"
"\n"
"	if(DetailedRedrawState.fOffsetY == fOffsetY && DetailedRedrawState.fDetailedOffset == fDetailedOffset && DetailedRedrawState.fDetailedRange == fDetailedRange && !KeyCtrlDown && !KeyShiftDown && !MouseDragButton)\n"
"	{\n"
"		Invalidate++;\n"
"	}\n"
"	else\n"
"	{\n"
"		Invalidate = 0;\n"
"		DetailedRedrawState.fOffsetY = fOffsetY;\n"
"		DetailedRedrawState.fDetailedOffset = fDetailedOffset;\n"
"		DetailedRedrawState.fDetailedRange = fDetailedRange;\n"
"	}\n"
"	if(Invalidate == 0) //when panning, only draw bars that are a certain width to keep decent framerate\n"
"	{\n"
"		context.clearRect(0, 0, CanvasDetailedView.width, CanvasDetailedView.height);\n"
"		DrawDetailedView(context, nMinWidthPan, true);\n"
"	}\n"
"	else if(Invalidate == 1) //draw full and store\n"
"	{\n"
"		offscreen.clearRect(0, 0, CanvasDetailedView.width, CanvasDetailedView.height);\n"
"		DrawDetailedView(offscreen, nMinWidth, true);\n"
"		OffscreenData = offscreen.getImageData(0, 0, CanvasDetailedOffscreen.width, CanvasDetailedOffscreen.height);\n"
"	}\n"
"	else//reuse stored result untill next time viewport is changed.\n"
"	{\n"
"		context.clearRect(0, 0, CanvasDetailedView.width, CanvasDetailedView.height);\n"
"		context.putImageData(OffscreenData, 0, 0);\n"
"		DrawDetailedView(context, nMinWidth, false);\n"
"	}\n"
"\n"
"	if(KeyShiftDown || KeyCtrlDown || MouseDragButton || MouseDragSelectRange())\n"
"	{\n"
"		nHoverToken = -1;\n"
"		nHoverTokenIndex = -1;\n"
"		nHoverTokenLogIndex = -1;\n"
"\n"
"		if(!MouseDragSelectRange())\n"
"		{\n"
"			fRangeBegin = fRangeEnd = -1;\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		nHoverToken = nHoverTokenNext;\n"
"		nHoverTokenIndex = nHoverTokenIndexNext;\n"
"		nHoverTokenLogIndex = nHoverTokenLogIndexNext;\n"
"		if(fRangeBeginHistory < fRangeEndHistory)\n"
"		{\n"
"			fRangeBegin = fRangeBeginHistory;\n"
"			fRangeEnd = fRangeEndHistory;\n"
"		}\n"
"		else\n"
"		{\n"
"			fRangeBegin = fRangeBeginNext;\n"
"			fRangeEnd = fRangeEndNext;\n"
"		}\n"
"	}\n"
"	if(fRangeBegin < fRangeEnd)\n"
"	{\n"
"		var X = (fRangeBegin - fDetailedOffset) * fScaleX;\n"
"		var Y = 0;\n"
"		var W = (fRangeEnd - fRangeBegin) * fScaleX;\n"
"		context.globalAlpha = 0.1;\n"
"		context.fillStyle = \'#009900\';\n"
"		context.fillRect(X, Y, W, nHeight);\n"
"		context.globalAlpha = 1;\n"
"		context.strokeStyle = \'#00ff00\';\n"
"		context.beginPath();\n"
"		context.moveTo(X, Y);\n"
"		context.lineTo(X, Y+nHeight);\n"
"		context.moveTo(X+W, Y);\n"
"		context.lineTo(X+W, Y+nHeight);\n"
"		context.stroke();\n"
"		var Duration = (fRangeEnd - fRangeBegin).toFixed(2) + \"ms\";\n"
"		var Center = ((fRangeBegin + fRangeEnd) / 2.0) - fDetailedOffset;\n"
"		var DurationWidth = context.measureText(Duration+ \"   \").width;\n"
"\n"
"		context.fillStyle = \'white\';\n"
"		context.textAlign = \'right\';\n"
"		DrawTextBox(context, \'\' + fRangeBegin.toFixed(2), X-3, 9, \'right\');\n"
"		if(DurationWidth < W + 10)\n"
"		{\n"
"			context.textAlign = \'center\';\n"
"			DrawTextBox(context,\'\' + Duration,Center * fScaleX, 9, \'center\');\n"
"\n"
"			var W0 = W - DurationWidth + FontWidth*1.5;\n"
"			if(W0 > 6)\n"
"			{\n"
"				var Y0 = Y + FontHeight * 0.5;\n"
"				W0 = W0 / 2.0;\n"
"				var X0 = X + W0;\n"
"				var X1 = X + W - W0;\n"
"				context.strokeStyle = \'#00ff00\';\n"
"				context.beginPath();\n"
"				context.moveTo(X, Y0);\n"
"				context.lineTo(X0, Y0);\n"
"				context.moveTo(X0, Y0-2);\n"
"				context.lineTo(X0, Y0+2);\n"
"				context.moveTo(X1, Y0-2);\n"
"				context.lineTo(X1, Y0+2);\n"
"				context.moveTo(X1, Y0);\n"
"				context.lineTo(X + W, Y0);\n"
"				context.stroke();\n"
"			}\n"
"		}\n"
"		context.textAlign = \'left\';\n"
"		DrawTextBox(context, \'\' + fRangeEnd.toFixed(2), X + W + 2, 9, \'left\');\n"
"	}\n"
"	nHoverCSCpu = nHoverCSCpuNext;\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function ZoomTo(fZoomBegin, fZoomEnd)\n"
"{\n"
"	if(fZoomBegin < fZoomEnd)\n"
"	{\n"
"		AnimationActive = true;\n"
"		var fDetailedOffsetOriginal = fDetailedOffset;\n"
"		var fDetailedRangeOriginal = fDetailedRange;\n"
"		var fDetailedOffsetTarget = fZoomBegin;\n"
"		var fDetailedRangeTarget = fZoomEnd - fZoomBegin;\n"
"		var TimestampStart = new Date();\n"
"		var count = 0;\n"
"		function ZoomFunc(Timestamp)\n"
"		{\n"
"			var fPrc = (new Date() - TimestampStart) / (ZOOM_TIME * 1000.0);\n"
"			if(fPrc > 1.0)\n"
"			{\n"
"				fPrc = 1.0;\n"
"			}\n"
"			fPrc = Math.pow(fPrc, 0.3);\n"
"			fDetailedOffset = fDetailedOffsetOriginal + (fDetailedOffsetTarget - fDetailedOffsetOriginal) * fPrc;\n"
"			fDetailedRange = fDetailedRangeOriginal + (fDetailedRangeTarget - fDetailedRangeOriginal) * fPrc;\n"
"			DrawDetailed(true);\n"
"			if(fPrc >= 1.0)\n"
"			{\n"
"				AnimationActive = false;\n"
"				fDetailedOffset = fDetailedOffsetTarget;\n"
"				fDetailedRange = fDetailedRangeTarget;\n"
"			}\n"
"			else\n"
"			{\n"
"				requestAnimationFrame(ZoomFunc);\n"
"			}\n"
"		}\n"
"		requestAnimationFrame(ZoomFunc);\n"
"	}\n"
"}\n"
"function RequestRedraw()\n"
"{\n"
"	Invalidate = 0;\n"
"	Draw();\n"
"}\n"
"function Draw()\n"
"{\n"
"	if(ProfileMode)\n"
"	{\n"
"		ProfileModeClear();\n"
"		ProfileEnter(\"Total\");\n"
"	}\n"
"\n"
"	if(Mode == ModeTimers)\n"
"	{\n"
"		DrawBarView();\n"
"		DrawHoverToolTip();\n"
"	}\n"
"	else if(Mode == ModeDetailed)\n"
"	{\n"
"		DrawDetailed(false);\n"
"		DrawHoverToolTip();\n"
"	}\n"
"\n"
"	DrawDetailedFrameHistory();\n"
"\n"
"	if(ProfileMode)\n"
"	{\n"
"		ProfileLeave();\n"
"		ProfileModeDraw(CanvasDetailedView);\n"
"	}\n"
"}\n"
"\n"
"function AutoRedraw(Timestamp)\n"
"{\n"
"	if(Mode == ModeDetailed)\n"
"	{\n"
"		if(ProfileMode == 2 || ((nHoverCSCpu >= 0 || nHoverToken != -1) && !KeyCtrlDown && !KeyShiftDown && !MouseDragButton)||(Invalidate<2 && !KeyCtrlDown && !KeyShiftDown && !MouseDragButton))\n"
"		{\n"
"			Draw(1);\n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		if(Invalidate < 1)\n"
"		{\n"
"			Draw(1);\n"
"		}\n"
"\n"
"\n"
"	}\n"
"	requestAnimationFrame(AutoRedraw);\n"
"}\n"
"\n"
"\n"
"function ZoomGraph(nZoom)\n"
"{\n"
"	var fOldRange = fDetailedRange;\n"
"	if(nZoom>0)\n"
"	{\n"
"		fDetailedRange *= Math.pow(nModDown ? 1.40 : 1.03, nZoom);\n"
"	}\n"
"	else\n"
"	{\n"
"		var fNewDetailedRange = fDetailedRange / Math.pow((nModDown ? 1.40 : 1.03), -nZoom);\n"
"		if(fNewDetailedRange < 0.0001) //100ns\n"
"			fNewDetailedRange = 0.0001;\n"
"		fDetailedRange = fNewDetailedRange;\n"
"	}\n"
"\n"
"	var fDiff = fOldRange - fDetailedRange;\n"
"	var fMousePrc = DetailedViewMouseX / nWidth;\n"
"	if(fMousePrc < 0)\n"
"	{\n"
"		fMousePrc = 0;\n"
"	}\n"
"	fDetailedOffset += fDiff * fMousePrc;\n"
"\n"
"}\n"
"\n"
"function MeasureFont()\n"
"{\n"
"	var context = CanvasDetailedView.getContext(\'2d\');\n"
"	context.font = Font;\n"
"	FontWidth = context.measureText(\'W\').width;\n"
"\n"
"}\n"
"function ResizeCanvas() \n"
"{\n"
"	nWidth = window.innerWidth;\n"
"	nHeight = window.innerHeight - CanvasHistory.height-2;\n"
"	DPR = window.devicePixelRatio;\n"
"\n"
"	if(DPR)\n"
"	{\n"
"		CanvasDetailedView.style.width = nWidth + \'px\'; \n"
"		CanvasDetailedView.style.height = nHeight + \'px\';\n"
"		CanvasDetailedView.width = nWidth * DPR;\n"
"		CanvasDetailedView.height = nHeight * DPR;\n"
"		CanvasHistory.style.width = window.innerWidth + \'px\';\n"
"		CanvasHistory.style.height = 70 + \'px\';\n"
"		CanvasHistory.width = window.innerWidth * DPR;\n"
"		CanvasHistory.height = 70 * DPR;\n"
"		CanvasHistory.getContext(\'2d\').scale(DPR,DPR);\n"
"		CanvasDetailedView.getContext(\'2d\').scale(DPR,DPR);\n"
"\n"
"		CanvasDetailedOffscreen.style.width = nWidth + \'px\';\n"
"		CanvasDetailedOffscreen.style.height = nHeight + \'px\';\n"
"		CanvasDetailedOffscreen.width = nWidth * DPR;\n"
"		CanvasDetailedOffscreen.height = nHeight * DPR;\n"
"		CanvasDetailedOffscreen.getContext(\'2d\').scale(DPR,DPR);\n"
"\n"
"	}\n"
"	else\n"
"	{\n"
"		DPR = 1;\n"
"		CanvasDetailedView.width = nWidth;\n"
"		CanvasDetailedView.height = nHeight;\n"
"		CanvasDetailedOffscreen.width = nWidth;\n"
"		CanvasDetailedOffscreen.height = nHeight;\n"
"		CanvasHistory.width = window.innerWidth;\n"
"	}\n"
"	RequestRedraw();\n"
"}\n"
"\n"
"var MouseDragOff = 0;\n"
"var MouseDragDown = 1;\n"
"var MouseDragUp = 2;\n"
"var MouseDragMove = 3;\n"
"var MouseDragState = MouseDragOff;\n"
"var MouseDragTarget = 0;\n"
"var MouseDragButton = 0;\n"
"var MouseDragKeyShift = 0;\n"
"var MouseDragKeyCtrl = 0;\n"
"var MouseDragX = 0;\n"
"var MouseDragY = 0;\n"
"var MouseDragXLast = 0;\n"
"var MouseDragYLast = 0;\n"
"var MouseDragXStart = 0;\n"
"var MouseDragYStart = 0;\n"
"\n"
"function clamp(number, min, max)\n"
"{\n"
"  return Math.max(min, Math.min(number, max));\n"
"}\n"
"\n"
"function MouseDragPan()\n"
"{\n"
"	return MouseDragButton == 1 || MouseDragKeyShift;\n"
"}\n"
"function MouseDragSelectRange()\n"
"{\n"
"	return MouseDragState == MouseDragMove && (MouseDragButton == 3 || (MouseDragKeyShift && MouseDragKeyCtrl));\n"
"}\n"
"function MouseHandleDrag()\n"
"{\n"
"	if(MouseDragTarget == CanvasDetailedView)\n"
"	{\n"
"		if(Mode == ModeDetailed)\n"
"		{\n"
"			if(MouseDragSelectRange())\n"
"			{\n"
"				var xStart = MouseDragXStart;\n"
"				var xEnd = MouseDragX;\n"
"				if(xStart > xEnd)\n"
"				{\n"
"					var Temp = xStart;\n"
"					xStart = xEnd;\n"
"					xEnd = Temp;\n"
"				}\n"
"				if(xEnd - xStart > 1)\n"
"				{\n"
"					fRangeBegin = fDetailedOffset + fDetailedRange * (xStart / nWidth);\n"
"					fRangeEnd = fDetailedOffset + fDetailedRange * (xEnd / nWidth);\n"
"				}\n"
"			}\n"
"			else if(MouseDragPan())\n"
"			{\n"
"				var X = MouseDragX - MouseDragXLast;\n"
"				var Y = MouseDragY - MouseDragYLast;\n"
"				if(X)\n"
"				{\n"
"					fDetailedOffset += -X * fDetailedRange / nWidth;\n"
"				}\n"
"				nOffsetY -= Y;\n"
"				if(nOffsetY < 0)\n"
"				{\n"
"					nOffsetY = 0;\n"
"				}\n"
"			}\n"
"			else if(MouseDragKeyCtrl)\n"
"			{\n"
"				if(MouseDragY != MouseDragYLast)\n"
"				{\n"
"					ZoomGraph(MouseDragY - MouseDragYLast);\n"
"				}\n"
"			}\n"
"		}\n"
"		else if(Mode == ModeTimers)\n"
"		{\n"
"			if(MouseDragKeyShift || MouseDragButton == 1)\n"
"			{\n"
"				var X = MouseDragX - MouseDragXLast;\n"
"				var Y = MouseDragY - MouseDragYLast;\n"
"				nOffsetBarsY -= Y;\n"
"				nOffsetBarsX -= X;\n"
"				if(nOffsetBarsY < 0)\n"
"				{\n"
"					nOffsetBarsY = 0;\n"
"				}\n"
"				if(nOffsetBarsX < 0)\n"
"				{\n"
"					nOffsetBarsX = 0;\n"
"				}\n"
"			}\n"
"\n"
"		}\n"
"\n"
"	}\n"
"	else if(MouseDragTarget == CanvasHistory)\n"
"	{\n"
"		function HistoryFrameTime(x)\n"
"		{\n"
"			var NumFrames = Frames.length;\n"
"			var fBarWidth = nWidth / NumFrames;\n"
"			var Index = clamp(Math.floor(NumFrames * x / nWidth), 0, NumFrames-1);\n"
"			var Lerp = clamp((x/fBarWidth - Index) , 0, 1);\n"
"			var time = Frames[Index].framestart + (Frames[Index].frameend - Frames[Index].framestart) * Lerp;\n"
"			return time;\n"
"		}\n"
"		if(MouseDragSelectRange())\n"
"		{\n"
"			fRangeBegin = fRangeEnd = -1;\n"
"\n"
"			var xStart = MouseDragXStart;\n"
"			var xEnd = MouseDragX;\n"
"			if(xStart > xEnd)\n"
"			{\n"
"				var Temp = xStart;\n"
"				xStart = xEnd;\n"
"				xEnd = Temp;\n"
"			}\n"
"			if(xEnd - xStart > 2)\n"
"			{\n"
"				var timestart = HistoryFrameTime(xStart);\n"
"				var timeend = HistoryFrameTime(xEnd);\n"
"				fDetailedOffset = timestart;\n"
"				fDetailedRange = timeend-timestart;\n"
"			}\n"
"		}\n"
"		else if(MouseDragPan())\n"
"		{\n"
"			var Time = HistoryFrameTime(MouseDragX);\n"
"			fDetailedOffset = Time - fDetailedRange / 2.0;\n"
"		}\n"
"	}\n"
"}\n"
"function MouseHandleDragEnd()\n"
"{\n"
"	if(MouseDragTarget == CanvasDetailedView)\n"
"	{\n"
"		if(MouseDragSelectRange())\n"
"		{\n"
"			ZoomTo(fRangeBegin, fRangeEnd);\n"
"			fRangeBegin = fRangeEnd = -1;\n"
"		}\n"
"	}\n"
"	else if(MouseDragTarget == CanvasHistory)\n"
"	{\n"
"		if(!MouseDragSelectRange() && !MouseDragPan())\n"
"		{\n"
"			ZoomTo(fRangeBegin, fRangeEnd);\n"
"			fRangeBegin = fRangeEnd = -1;\n"
"		}\n"
"\n"
"\n"
"	}\n"
"\n"
"}\n"
"\n"
"function MouseHandleDragClick()\n"
"{\n"
"	if(MouseDragTarget == CanvasDetailedView)\n"
"	{\n"
"		ZoomTo(fRangeBegin, fRangeEnd);\n"
"	}\n"
"	else if(MouseDragTarget == CanvasHistory)\n"
"	{\n"
"		if(Mode == ModeDetailed)\n"
"		{\n"
"			ZoomTo(fRangeBegin, fRangeEnd);\n"
"		}\n"
"	}\n"
"}\n"
"\n"
"function MapMouseButton(Event)\n"
"{\n"
"	if(event.button == 1 || event.which == 1)\n"
"	{\n"
"		return 1;\n"
"	}\n"
"	else if(event.button == 3 || event.which == 3)\n"
"	{\n"
"		return 3;\n"
"	}\n"
"	else\n"
"	{\n"
"		return 0;\n"
"	}\n"
"}\n"
"\n"
"function MouseDragReset()\n"
"{\n"
"	MouseDragState = MouseDragOff;\n"
"	MouseDragTarget = 0;\n"
"	MouseDragKeyShift = 0;\n"
"	MouseDragKeyCtrl = 0;\n"
"	MouseDragButton = 0;\n"
"}\n"
"function MouseDragKeyUp()\n"
"{\n"
"	if((MouseDragKeyShift && !KeyShiftDown) || (MouseDragKeyCtrl && !KeyCtrlDown))\n"
"	{\n"
"		MouseHandleDragEnd();\n"
"		MouseDragReset();\n"
"	}\n"
"}\n"
"function MouseDrag(Source, Event)\n"
"{\n"
"	if(Source == MouseDragOff || (MouseDragTarget && MouseDragTarget != Event.target))\n"
"	{\n"
"		MouseDragReset();\n"
"		return;\n"
"	}\n"
"\n"
"	var LocalRect = Event.target.getBoundingClientRect();\n"
"	MouseDragX = Event.clientX - LocalRect.left;\n"
"	MouseDragY = Event.clientY - LocalRect.top;\n"
"	// console.log(\'cur drag state \' + MouseDragState + \' Source \' + Source);\n"
"	if(MouseDragState == MouseDragMove)\n"
"	{\n"
"		var dx = Math.abs(MouseDragX - MouseDragXStart);\n"
"		var dy = Math.abs(MouseDragY - MouseDragYStart);\n"
"		if((Source == MouseDragUp && MapMouseButton(Event) == MouseDragButton) ||\n"
"			(MouseDragKeyCtrl && !KeyCtrlDown) ||\n"
"			(MouseDragKeyShift && !KeyShiftDown))\n"
"		{\n"
"			MouseHandleDragEnd();\n"
"			MouseDragReset();\n"
"			return;\n"
"		}\n"
"		else\n"
"		{\n"
"			MouseHandleDrag();\n"
"		}\n"
"	}\n"
"	else if(MouseDragState == MouseDragOff)\n"
"	{\n"
"		if(Source == MouseDragDown || KeyShiftDown || KeyCtrlDown)\n"
"		{\n"
"			MouseDragTarget = Event.target;\n"
"			MouseDragButton = MapMouseButton(Event);\n"
"			MouseDragState = MouseDragDown;\n"
"			MouseDragXStart = MouseDragX;\n"
"			MouseDragYStart = MouseDragY;\n"
"			MouseDragKeyCtrl = 0;\n"
"			MouseDragKeyShift = 0;\n"
"\n"
"			if(KeyShiftDown || KeyCtrlDown)\n"
"			{\n"
"				MouseDragKeyShift = KeyShiftDown;\n"
"				MouseDragKeyCtrl = KeyCtrlDown;\n"
"				MouseDragState = MouseDragMove;\n"
"			}\n"
"		}\n"
"	}\n"
"	else if(MouseDragState == MouseDragDown)\n"
"	{\n"
"		if(Source == MouseDragUp)\n"
"		{\n"
"			MouseHandleDragClick();\n"
"			MouseDragReset();\n"
"		}\n"
"		else if(Source == MouseDragMove)\n"
"		{\n"
"			var dx = Math.abs(MouseDragX - MouseDragXStart);\n"
"			var dy = Math.abs(MouseDragY - MouseDragYStart);\n"
"			if(dx+dy>1)\n"
"			{\n"
"				MouseDragState = MouseDragMove;\n"
"			}\n"
"		}\n"
"	}\n"
"	MouseDragXLast = MouseDragX;\n"
"	MouseDragYLast = MouseDragY;\n"
"}\n"
"\n"
"function MouseMove(evt)\n"
"{\n"
"    evt.preventDefault();\n"
"    MouseDrag(MouseDragMove, evt);\n"
" 	MouseHistory = 0;\n"
"	MouseDetailed = 0;\n"
"	HistoryViewMouseX = HistoryViewMouseY = -1;\n"
"	var rect = evt.target.getBoundingClientRect();\n"
"	var x = evt.clientX - rect.left;\n"
"	var y = evt.clientY - rect.top;\n"
"	if(evt.target == CanvasDetailedView)\n"
"	{\n"
"		if(!MouseDragSelectRange())\n"
"		{\n"
"			fRangeBegin = fRangeEnd = -1;\n"
"		}\n"
"		DetailedViewMouseX = x;\n"
"		DetailedViewMouseY = y;\n"
"	}\n"
"	else if(evt.target = CanvasHistory)\n"
"	{\n"
"		var Rect = CanvasHistory.getBoundingClientRect();\n"
"		HistoryViewMouseX = x;\n"
"		HistoryViewMouseY = y;\n"
"	}\n"
"	Draw();\n"
"}\n"
"\n"
"function MouseButton(bPressed, evt)\n"
"{\n"
"    evt.preventDefault();\n"
"	MouseDrag(bPressed ? MouseDragDown : MouseDragUp, evt);\n"
"}\n"
"\n"
"function MouseOut(evt)\n"
"{\n"
"	MouseDrag(MouseDragOff, evt);\n"
"	KeyCtrlDown = 0;\n"
"	KeyShiftDown = 0;\n"
"	MouseDragButton = 0;\n"
"	nHoverToken = -1;\n"
"	fRangeBegin = fRangeEnd = -1;\n"
"}\n"
"\n"
"function MouseWheel(e)\n"
"{\n"
"    var e = window.event || e;\n"
"    var delta = (e.wheelDelta || e.detail * (-120));\n"
"    ZoomGraph((-4 * delta / 120.0) | 0);\n"
"    Draw();\n"
"}\n"
"\n"
"\n"
"function KeyUp(evt)\n"
"{\n"
"	if(evt.keyCode == 17)\n"
"	{\n"
"		KeyCtrlDown = 0;\n"
"		MouseDragKeyUp();\n"
"	}\n"
"	else if(evt.keyCode == 16)\n"
"	{\n"
"		KeyShiftDown = 0;\n"
"		MouseDragKeyUp();\n"
"	}\n"
"	if(evt.keyCode == 18)\n"
"	{\n"
"		FlipToolTip = 0;\n"
"	}\n"
"	Invalidate = 0;\n"
"\n"
"}\n"
"\n"
"function KeyDown(evt)\n"
"{\n"
"	if(evt.keyCode == 18)\n"
"	{\n"
"		FlipToolTip = 1;\n"
"	}\n"
"	if(evt.keyCode == 17)\n"
"	{\n"
"		KeyCtrlDown = 1;\n"
"	}\n"
"	else if(evt.keyCode == 16)\n"
"	{\n"
"		KeyShiftDown = 1;\n"
"	}\n"
"	Invalidate = 0;\n"
"}\n"
"\n"
"function ReadCookie()\n"
"{\n"
"	var result = document.cookie.match(/fisk=([^;]+)/);\n"
"	var NewMode = ModeDetailed;\n"
"	var ReferenceTimeString = \'33ms\';\n"
"	if(result && result.length > 0)\n"
"	{\n"
"		var Obj = JSON.parse(result[1]);\n"
"		if(Obj.Mode)\n"
"		{\n"
"			NewMode = Obj.Mode;\n"
"		}\n"
"		if(Obj.ReferenceTime)\n"
"		{\n"
"			ReferenceTimeString = Obj.ReferenceTime;\n"
"		}\n"
"		if(Obj.ThreadsAllActive || Obj.ThreadsAllActive == 0 || Obj.ThreadsAllActive == false)\n"
"		{\n"
"			ThreadsAllActive = Obj.ThreadsAllActive;\n"
"		}\n"
"		else\n"
"		{\n"
"			ThreadsAllActive = 1;\n"
"		}\n"
"		if(Obj.ThreadsActive)\n"
"		{\n"
"			ThreadsActive = Obj.ThreadsActive;\n"
"		}\n"
"		if(Obj.GroupsAllActive || Obj.GroupsAllActive == 0 || Obj.GroupsAllActive)\n"
"		{\n"
"			GroupsAllActive = Obj.GroupsAllActive;\n"
"		}\n"
"		else\n"
"		{\n"
"			GroupsAllActive = 1;\n"
"		}\n"
"		if(Obj.GroupsActive)\n"
"		{\n"
"			GroupsActive = Obj.GroupsActive;\n"
"		}\n"
"		if(Obj.nContextSwitchEnabled)\n"
"		{\n"
"			nContextSwitchEnabled = Obj.nContextSwitchEnabled; \n"
"		}\n"
"		else\n"
"		{\n"
"			nContextSwitchEnabled = 1;\n"
"		}\n"
"		if(Obj.GroupColors)\n"
"		{\n"
"			GroupColors = Obj.GroupColors;\n"
"		}\n"
"		else\n"
"		{\n"
"			GroupColors = 0;\n"
"		}\n"
"		TimersGroups = Obj.TimersGroups?Obj.TimersGroups:0;\n"
"		TimersMeta = Obj.TimersMeta?0:1;\n"
"	}\n"
"	SetContextSwitch(nContextSwitchEnabled);\n"
"	SetMode(NewMode, TimersGroups);\n"
"	SetReferenceTime(ReferenceTimeString);\n"
"	UpdateOptionsMenu();\n"
"	UpdateGroupColors();\n"
"}\n"
"function WriteCookie()\n"
"{\n"
"	var Obj = new Object();\n"
"	Obj.Mode = Mode;\n"
"	Obj.ReferenceTime = ReferenceTime + \'ms\';\n"
"	Obj.ThreadsActive = ThreadsActive;\n"
"	Obj.ThreadsAllActive = ThreadsAllActive;\n"
"	Obj.GroupsActive = GroupsActive;\n"
"	Obj.GroupsAllActive = GroupsAllActive;\n"
"	Obj.nContextSwitchEnabled = nContextSwitchEnabled;\n"
"	Obj.TimersGroups = TimersGroups?TimersGroups:0;\n"
"	Obj.TimersMeta = TimersMeta?0:1;\n"
"	Obj.GroupColors = GroupColors;\n"
"	var date = new Date();\n"
"	date.setFullYear(2099);\n"
"	var cookie = \'fisk=\' + JSON.stringify(Obj) + \';expires=\' + date;\n"
"	document.cookie = cookie;\n"
"}\n"
"\n"
"var mousewheelevt = (/Firefox/i.test(navigator.userAgent)) ? \"DOMMouseScroll\" : \"mousewheel\" //FF doesn\'t recognize mousewheel as of FF3.x\n"
"\n"
"CanvasDetailedView.addEventListener(\'mousemove\', MouseMove, false);\n"
"CanvasDetailedView.addEventListener(\'mousedown\', function(evt) { MouseButton(true, evt); });\n"
"CanvasD";

const size_t g_MicroProfileHtml_end_1_size = sizeof(g_MicroProfileHtml_end_1);
const char g_MicroProfileHtml_end_2[] =
"etailedView.addEventListener(\'mouseup\', function(evt) { MouseButton(false, evt); } );\n"
"CanvasDetailedView.addEventListener(\'mouseout\', MouseOut);\n"
"CanvasDetailedView.addEventListener(\"contextmenu\", function (e) { e.preventDefault(); }, false);\n"
"CanvasDetailedView.addEventListener(mousewheelevt, MouseWheel, false);\n"
"CanvasHistory.addEventListener(\'mousemove\', MouseMove);\n"
"CanvasHistory.addEventListener(\'mousedown\', function(evt) { MouseButton(true, evt); });\n"
"CanvasHistory.addEventListener(\'mouseup\', function(evt) { MouseButton(false, evt); } );\n"
"CanvasHistory.addEventListener(\'mouseout\', MouseOut);\n"
"CanvasHistory.addEventListener(\"contextmenu\", function (e) { e.preventDefault(); }, false);\n"
"CanvasHistory.addEventListener(mousewheelevt, MouseWheel, false);\n"
"window.addEventListener(\'keydown\', KeyDown);\n"
"window.addEventListener(\'keyup\', KeyUp);\n"
"window.addEventListener(\'resize\', ResizeCanvas, false);\n"
"\n"
"function CalcAverage()\n"
"{\n"
"	var Sum = 0;\n"
"	var Count = 0;\n"
"	for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"	{\n"
"		StackPos = 0;\n"
"		for(var i = 0; i < Frames.length; i++)\n"
"		{\n"
"			var Frame_ = Frames[i];			\n"
"			var tt = Frame_.tt[nLog];\n"
"			var ts = Frame_.ts[nLog];\n"
"\n"
"			var count = tt.length;\n"
"			for(var j = 0; j < count; j++)\n"
"			{\n"
"				var type = tt[j];\n"
"				var time = ts[j];\n"
"				if(type == 1)\n"
"				{\n"
"					Stack[StackPos] = time;//store the frame which it comes from\n"
"					StackPos++;\n"
"				}\n"
"				else if(type == 0)\n"
"				{\n"
"					if(StackPos>0)\n"
"					{\n"
"\n"
"						StackPos--;\n"
"						var localtime = time - Stack[StackPos];\n"
"						Count++;\n"
"						Sum += localtime;\n"
"					}\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"	return Sum / Count;\n"
"\n"
"}\n"
"\n"
"function MakeLod(index, MinDelta, TimeArray, TypeArray, IndexArray, LogStart)\n"
"{\n"
"	if(LodData[index])\n"
"	{\n"
"		console.log(\"error!!\");\n"
"	}\n"
"	// debugger;\n"
"	var o = new Object();\n"
"	o.MinDelta = MinDelta;\n"
"	o.TimeArray = TimeArray;\n"
"	o.TypeArray = TypeArray;\n"
"	o.IndexArray = IndexArray;\n"
"	o.LogStart = LogStart;\n"
"	LodData[index] = o;\n"
"}\n"
"\n"
"function PreprocessBuildSplitArray()\n"
"{\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"\n"
"	ProfileEnter(\"PreprocessBuildSplitArray\");\n"
"	var SplitArrays = new Array(nNumLogs);\n"
"\n"
"	for(nLog = 0; nLog < nNumLogs; ++nLog)\n"
"	{\n"
"		console.log(\"source log \" + nLog + \" size \" + LodData[0].TypeArray[nLog].length);\n"
"	}\n"
"\n"
"\n"
"	for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"	{\n"
"		var MaxDepth = 1;\n"
"		var StackPos = 0;\n"
"		var Stack = Array(20);\n"
"		var TypeArray = LodData[0].TypeArray[nLog];\n"
"		var TimeArray = LodData[0].TimeArray[nLog];\n"
"		var DeltaTimes = new Array(TypeArray.length);\n"
"\n"
"		for(var j = 0; j < TypeArray.length; ++j)\n"
"		{\n"
"			var type = TypeArray[j];\n"
"			var time = TimeArray[j];\n"
"			if(type == 1)\n"
"			{\n"
"				//push\n"
"				Stack[StackPos] = time;\n"
"				StackPos++;\n"
"			}\n"
"			else if(type == 0)\n"
"			{\n"
"				if(StackPos>0)\n"
"				{\n"
"					StackPos--;\n"
"					DeltaTimes[j] = time - Stack[StackPos];\n"
"				}\n"
"				else\n"
"				{\n"
"					DeltaTimes[j] = 0;\n"
"				}\n"
"			}\n"
"		}\n"
"		DeltaTimes.sort(function(a,b){return b-a;});\n"
"		var SplitArray = Array(NumLodSplits);\n"
"		var SplitIndex = DeltaTimes.length;\n"
"\n"
"		var j = 0;\n"
"		for(j = 0; j < NumLodSplits; ++j)\n"
"		{\n"
"			SplitIndex = Math.floor(SplitIndex / 2);\n"
"			while(SplitIndex > 0 && !DeltaTimes[SplitIndex])\n"
"			{\n"
"				SplitIndex--;\n"
"			}\n"
"			if(SplitIndex < SplitMin)\n"
"			{\n"
"				break;\n"
"			}\n"
"			//search.. if 0\n"
"			var SplitTime = DeltaTimes[SplitIndex];\n"
"			if(SplitTime>=0)\n"
"			{\n"
"				SplitArray[j] = SplitTime;\n"
"			}\n"
"			else\n"
"			{\n"
"				SplitArray[j] = SPLIT_LIMIT;\n"
"			}\n"
"			if(j>0)\n"
"			{\n"
"				console.assert(SplitArray[j-1] <= SplitArray[j], \"must be less\");\n"
"			}\n"
"\n"
"		}\n"
"		for(; j < NumLodSplits; ++j)\n"
"		{\n"
"			SplitArray[j] = SPLIT_LIMIT;\n"
"			// console.log(\"split skipping \" + j + \" \" + SPLIT_LIMIT);\n"
"		}\n"
"\n"
"\n"
"		SplitArrays[nLog] = SplitArray;\n"
"	}\n"
"	ProfileLeave();\n"
"	return SplitArrays;\n"
"}\n"
"\n"
"function PreprocessBuildDurationArray()\n"
"{\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	ProfileEnter(\"PreprocessBuildDurationArray\");\n"
"	var DurationArrays = new Array(nNumLogs);\n"
"	for(nLog = 0; nLog < nNumLogs; ++nLog)\n"
"	{\n"
"		var MaxDepth = 1;\n"
"		var StackPos = 0;\n"
"		var Stack = Array(20);\n"
"		var StackIndex = Array(20);\n"
"		var TypeArray = LodData[0].TypeArray[nLog];\n"
"		var TimeArray = LodData[0].TimeArray[nLog];\n"
"		var DurationArray = Array(LodData[0].TypeArray[nLog].length);\n"
"		for(var j = 0; j < TypeArray.length; ++j)\n"
"		{\n"
"			var type = TypeArray[j];\n"
"			var time = TimeArray[j];\n"
"			if(type == 1)\n"
"			{\n"
"				//push\n"
"				Stack[StackPos] = time;\n"
"				StackIndex[StackPos] = j;\n"
"				StackPos++;\n"
"			}\n"
"			else if(type == 0)\n"
"			{\n"
"				if(StackPos>0)\n"
"				{\n"
"					StackPos--;\n"
"					var Duration = time - Stack[StackPos];\n"
"					DurationArray[StackIndex[StackPos]] = Duration;\n"
"					DurationArray[j] = Duration;\n"
"				}\n"
"				else\n"
"				{\n"
"					DurationArray[j] = 0;\n"
"				}\n"
"			}\n"
"		}\n"
"		for(var j = 0; j < StackPos; ++j)\n"
"		{\n"
"			DurationArray[j] = 0;\n"
"		}\n"
"		DurationArrays[nLog] = DurationArray;\n"
"	}\n"
"	ProfileLeave();\n"
"	return DurationArrays;\n"
"\n"
"}\n"
"function PreprocessLods()\n"
"{\n"
"	ProfileEnter(\"PreprocessLods\");\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	// debugger;\n"
"	var SplitArrays = PreprocessBuildSplitArray();\n"
"	var DurationArrays = PreprocessBuildDurationArray();\n"
"	var Source = LodData[0];\n"
"	var SourceLogStart = Source.LogStart;\n"
"	var NumFrames = SourceLogStart.length;\n"
"\n"
"	for(var i = 0; i < NumLodSplits-1; ++i)\n"
"	{\n"
"		// debugger;\n"
"		// console.log(\"doing lod split \" + i);\n"
"		// var LodObject = new Object();\n"
"		var DestLogStart = Array(SourceLogStart.length);\n"
"		for(var j = 0; j < DestLogStart.length; ++j)\n"
"		{\n"
"			DestLogStart[j] = Array(nNumLogs);\n"
"			// console.log(\"destlogstart \" + j + \" \" + DestLogStart[j].length);\n"
"		}\n"
"		//LodObject.LogStart = DestLogStart;\n"
"		var MinDelta = Array(nNumLogs);\n"
"		var TimeArray = Array(nNumLogs);\n"
"		var IndexArray = Array(nNumLogs);\n"
"		var TypeArray = Array(nNumLogs);\n"
"\n"
"\n"
"\n"
"		for(nLog = 0; nLog < nNumLogs; ++nLog)\n"
"		{\n"
"			var SourceTypeArray = Source.TypeArray[nLog];\n"
"			var SourceTimeArray = Source.TimeArray[nLog];\n"
"			var SourceIndexArray = Source.IndexArray[nLog];\n"
"			var Duration = DurationArrays[nLog];\n"
"			console.assert(Duration.length == SourceTypeArray.length, \"must be equal!\");\n"
"			var SplitTime = SplitArrays[nLog][i];\n"
"\n"
"			MinDelta[nLog] = SplitTime;\n"
"			if(SplitTime < SPLIT_LIMIT)\n"
"			{\n"
"				var SourceCount = SourceTypeArray.length;\n"
"				var DestTypeArray = Array();\n"
"				var DestTimeArray = Array();\n"
"				var DestIndexArray = Array();\n"
"				var RemapArray = Array(SourceCount);\n"
"\n"
"				for(var j = 0; j < SourceCount; ++j)\n"
"				{\n"
"					RemapArray[j] = DestTypeArray.length;\n"
"					if(Duration[j] >= SplitTime)\n"
"					{\n"
"						DestTypeArray.push(SourceTypeArray[j]);\n"
"						DestTimeArray.push(SourceTimeArray[j]);\n"
"						DestIndexArray.push(SourceIndexArray[j]);\n"
"					}\n"
"				}\n"
"				TimeArray[nLog] = DestTimeArray;\n"
"				IndexArray[nLog] = DestIndexArray;\n"
"				TypeArray[nLog] = DestTypeArray;\n"
"				for(var j = 0; j < NumFrames; ++j)\n"
"				{\n"
"					var OldStart = SourceLogStart[j][nLog];\n"
"					var NewStart = RemapArray[OldStart];\n"
"					var FrameArray = DestLogStart[j];\n"
"					FrameArray[nLog] = NewStart;\n"
"				}\n"
"			}\n"
"			else\n"
"			{\n"
"\n"
"				for(var j = 0; j < NumFrames; ++j)\n"
"				{\n"
"					var FrameArray = DestLogStart[j];\n"
"	\n"
"					FrameArray[nLog] = 0;\n"
"				}\n"
"\n"
"			}\n"
"\n"
"		}\n"
"		MakeLod(i+1, MinDelta, TimeArray, TypeArray, IndexArray, DestLogStart);\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"function PreprocessGlobalArray()\n"
"{\n"
"	ProfileEnter(\"PreprocessGlobalArray\");\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	var CaptureStart = Frames[0].framestart;\n"
"	var CaptureEnd = Frames[Frames.length-1].frameend;\n"
"	g_TypeArray = new Array(nNumLogs);\n"
"	g_TimeArray = new Array(nNumLogs);\n"
"	g_IndexArray = new Array(nNumLogs);\n"
"	var StackPos = 0;\n"
"	var Stack = Array(20);\n"
"	var LogStartArray = new Array(Frames.length);\n"
"	for(var i = 0; i < Frames.length; i++)\n"
"	{\n"
"		Frames[i].LogStart = new Array(nNumLogs);	\n"
"		LogStartArray[i] = Frames[i].LogStart;\n"
"\n"
"		Frames[i].LogEnd = new Array(nNumLogs);\n"
"	}\n"
"	var MinDelta = Array(nNumLogs);\n"
"	for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"	{\n"
"		MinDelta[nLog] = 0;\n"
"		var Discard = 0;\n"
"		var TypeArray = new Array();\n"
"		var TimeArray = new Array();\n"
"		var IndexArray = new Array();\n"
"		for(var i = 0; i < Frames.length; i++)\n"
"		{\n"
"			var Frame_ = Frames[i];	\n"
"			Frame_.LogStart[nLog] = TimeArray.length;\n"
"			var FrameDiscard = Frame_.frameend + 33;//if timestamps are more than 33ms after current frame, we assume buffer has wrapped.\n"
"			var tt = Frame_.tt[nLog];\n"
"			var ts = Frame_.ts[nLog];\n"
"			var ti = Frame_.ti[nLog];\n"
"			var len = tt.length;\n"
"			for(var xx = 0; xx < len; ++xx)\n"
"			{\n"
"				if(ts[xx] > FrameDiscard)\n"
"				{\n"
"					Discard++;\n"
"				}\n"
"				else\n"
"				{\n"
"					TypeArray.push(tt[xx]);\n"
"					TimeArray.push(ts[xx]);\n"
"					IndexArray.push(ti[xx]);\n"
"				}\n"
"			}\n"
"			Frame_.LogEnd[nLog] = TimeArray.length;\n"
"		}\n"
"		g_TypeArray[nLog] = TypeArray;\n"
"		g_TimeArray[nLog] = TimeArray;\n"
"		g_IndexArray[nLog] = IndexArray;\n"
"		if(Discard)\n"
"		{\n"
"			console.log(\'discarded \' + Discard + \' markers from \' + ThreadNames[nLog]);\n"
"		}\n"
"	}\n"
"	MakeLod(0, MinDelta, g_TimeArray, g_TypeArray, g_IndexArray, LogStartArray);\n"
"	ProfileLeave();\n"
"}\n"
"\n"
"function PreprocessFindFirstFrames()\n"
"{\n"
"	ProfileEnter(\"PreprocesFindFirstFrames\");\n"
"	//create arrays that show how far back we need to start search in order to get all markers.\n"
"	var nNumLogs = Frames[0].ts.length;\n"
"	for(var i = 0; i < Frames.length; i++)\n"
"	{\n"
"		Frames[i].FirstFrameIndex = new Array(nNumLogs);\n"
"	}\n"
"\n"
"	var StackPos = 0;\n"
"	var Stack = Array(20);\n"
"	g_MaxStack = Array(nNumLogs);\n"
"	\n"
"	for(nLog = 0; nLog < nNumLogs; nLog++)\n"
"	{\n"
"		var MaxStack = 0;\n"
"		StackPos = 0;\n"
"		for(var i = 0; i < Frames.length; i++)\n"
"		{\n"
"			var Frame_ = Frames[i];			\n"
"			var tt = Frame_.tt[nLog];\n"
"			var count = tt.length;\n"
"\n"
"			var FirstFrame = i;\n"
"			if(StackPos>0)\n"
"			{\n"
"				FirstFrame = Stack[0];\n"
"			}\n"
"			Frames[i].FirstFrameIndex[nLog] = FirstFrame;\n"
"\n"
"			for(var j = 0; j < count; j++)\n"
"			{\n"
"				var type = tt[j];\n"
"				if(type == 1)\n"
"				{\n"
"					Stack[StackPos] = i;//store the frame which it comes from\n"
"					StackPos++;\n"
"					if(StackPos > MaxStack)\n"
"					{\n"
"						MaxStack = StackPos;\n"
"					}\n"
"				}\n"
"				else if(type == 0)\n"
"				{\n"
"					if(StackPos>0)\n"
"					{\n"
"						StackPos--;\n"
"					}\n"
"				}\n"
"			}\n"
"		}\n"
"		g_MaxStack[nLog] = MaxStack;\n"
"	}\n"
"	ProfileLeave();\n"
"}\n"
"function PreprocessMeta()\n"
"{\n"
"	MetaLengths = Array(MetaNames.length);\n"
"	MetaLengthsAvg = Array(MetaNames.length);\n"
"	MetaLengthsMax = Array(MetaNames.length);\n"
"	for(var i = 0; i < MetaNames.length; ++i)\n"
"	{\n"
"		MetaLengths[i] = MetaNames[i].length+1;\n"
"		MetaLengthsAvg[i] = MetaNames[i].length+5;\n"
"		MetaLengthsMax[i] = MetaNames[i].length+5;\n"
"		if(MetaLengths[i]<12)\n"
"			MetaLengths[i] = 12;\n"
"		if(MetaLengthsAvg[i]<12)\n"
"			MetaLengthsAvg[i] = 12;\n"
"		if(MetaLengthsMax[i]<12)\n"
"			MetaLengthsMax[i] = 12;\n"
"	}\n"
"	for(var i = 0; i < TimerInfo.length; ++i)\n"
"	{\n"
"		var Timer = TimerInfo[i];\n"
"		for(var j = 0; j < MetaNames.length; ++j)\n"
"		{\n"
"			var Len = FormatMeta(Timer.meta[j],0).length + 2;\n"
"			var LenAvg = FormatMeta(Timer.meta[j],2).length + 2;\n"
"			var LenMax = FormatMeta(Timer.meta[j],0).length + 2;\n"
"			if(Len > MetaLengths[j])\n"
"			{\n"
"				MetaLengths[j] = Len;\n"
"			}\n"
"			if(LenAvg > MetaLengthsAvg[j])\n"
"			{\n"
"				MetaLengthsAvg[j] = LenAvg;\n"
"			}\n"
"			if(LenMax > MetaLengthsMax[j])\n"
"			{\n"
"				MetaLengthsMax[j] = LenMax;\n"
"			}\n"
"		}\n"
"	}\n"
"}\n"
"\n"
"function Preprocess()\n"
"{\n"
"	var ProfileModeOld = ProfileMode;\n"
"	ProfileMode = 1;\n"
"	ProfileModeClear();\n"
"	ProfileEnter(\"Preprocess\");\n"
"	for(var i = 0; i < TimerInfo.length; i++)\n"
"	{\n"
"		var v = CalculateTimers(TimerInfo[i], i);\n"
"	}\n"
"	PreprocessFindFirstFrames();\n"
"	PreprocessGlobalArray();\n"
"	PreprocessLods();\n"
"	PreprocessMeta();\n"
"	PreprocessContextSwitchCache();\n"
"	ProfileLeave();\n"
"	ProfileModeDump();\n"
"	ProfileMode = ProfileModeOld;\n"
"	Initialized = 1;\n"
"}\n"
"\n"
"InitGroups();\n"
"ReadCookie();\n"
"MeasureFont()\n"
"InitThreadMenu();\n"
"InitGroupMenu();\n"
"InitFrameInfo();\n"
"UpdateThreadMenu();\n"
"ResizeCanvas();\n"
"Preprocess();\n"
"Draw();\n"
"AutoRedraw();\n"
"\n"
"</script>\n"
"</body>\n"
"</html>      ";

const size_t g_MicroProfileHtml_end_2_size = sizeof(g_MicroProfileHtml_end_2);
const char* g_MicroProfileHtml_end[] = {
&g_MicroProfileHtml_end_0[0],
&g_MicroProfileHtml_end_1[0],
&g_MicroProfileHtml_end_2[0],
};
size_t g_MicroProfileHtml_end_sizes[] = {
sizeof(g_MicroProfileHtml_end_0),
sizeof(g_MicroProfileHtml_end_1),
sizeof(g_MicroProfileHtml_end_2),
};
size_t g_MicroProfileHtml_end_count = 3;
#endif //MICROPROFILE_EMBED_HTML


///end embedded file from microprofile.html
