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
// 		uint32_t MicroProfileGpuInsertTimeStamp(void*);
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
#if defined(_WIN32) && _MSC_VER == 1700
#define PRIx64 "llx"
#define PRIu64 "ull"
#define PRId64 "dll"
#else
#include <inttypes.h>
#endif
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
#define MICROPROFILE_SCOPE_TOKEN(token) do {} while(0)
#define MICROPROFILE_SCOPEGPU_TOKEN(token)
#define MICROPROFILE_SCOPEGPU(var) do{}while(0)
#define MICROPROFILE_SCOPEGPUI( name, color) do{}while(0)
#define MICROPROFILE_SCOPEGPU_TOKEN_L(Log, token) do{}while(0)
#define MICROPROFILE_SCOPEGPU_L(Log, var) do{}while(0)
#define MICROPROFILE_SCOPEGPUI_L(Log, name, color) do{}while(0)
#define MICROPROFILE_GPU_INIT_QUEUE(QueueName) do {} while(0)
#define MICROPROFILE_GPU_GET_QUEUE(QueueName) do {} while(0)
#define MICROPROFILE_GPU_BEGIN(pContext, pLog) do {} while(0)
#define MICROPROFILE_GPU_SET_CONTEXT(pContext, pLog) do {} while(0)
#define MICROPROFILE_GPU_END(pLog) do {} while(0)
#define MICROPROFILE_GPU_SUBMIT(Queue, Work) do {} while(0)
#define MICROPROFILE_META_CPU(name, count)
#define MICROPROFILE_META_GPU(name, count)
#define MICROPROFILE_FORCEENABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLECPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEENABLEGPUGROUP(s) do{} while(0)
#define MICROPROFILE_FORCEDISABLEGPUGROUP(s) do{} while(0)
#define MICROPROFILE_COUNTER_ADD(name, count) do{} while(0)
#define MICROPROFILE_COUNTER_SUB(name, count) do{} while(0)
#define MICROPROFILE_COUNTER_SET(name, count) do{} while(0)
#define MICROPROFILE_COUNTER_SET_INT32_PTR(name, ptr) do{} while(0)
#define MICROPROFILE_COUNTER_SET_INT64_PTR(name, ptr) do{} while(0)
#define MICROPROFILE_COUNTER_CLEAR_PTR(name) do{} while(0)
#define MICROPROFILE_COUNTER_SET_LIMIT(name, count) do{} while(0)
#define MICROPROFILE_CONDITIONAL(expr) 
#define MICROPROFILE_COUNTER_CONFIG(name, type, limit)
#define MICROPROFILE_DECLARE_LOCAL_COUNTER(var) 
#define MICROPROFILE_DEFINE_LOCAL_COUNTER(var, name) 
#define MICROPROFILE_DECLARE_LOCAL_ATOMIC_COUNTER(var) 
#define MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(var, name) 
#define MICROPROFILE_COUNTER_LOCAL_ADD(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SUB(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SET(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD(var) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_SET(var) do{}while(0)

#define MicroProfileGetTime(group, name) 0.f
#define MicroProfileOnThreadCreate(foo) do{}while(0)
#define MicroProfileFlip(pContext) do{}while(0)
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
#define MicroProfileDumpFile(html,csv,spikecpu,spikegpu) do{} while(0)
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
#define MICROPROFILE_SCOPEGPU_TOKEN(token) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token, MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_SCOPEGPU(var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var, MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_SCOPEGPUI(name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken("GPU", name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_SCOPEGPU_TOKEN_L(Log, token) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token, Log)
#define MICROPROFILE_SCOPEGPU_L(Log, var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var, Log)
#define MICROPROFILE_SCOPEGPUI_L(Log, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken("GPU", name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), Log)
#define MICROPROFILE_GPU_INIT_QUEUE(QueueName) MicroProfileInitGpuQueue(QueueName)
#define MICROPROFILE_GPU_GET_QUEUE(QueueName) MicroProfileGetGpuQueue(QueueName)
#define MICROPROFILE_GPU_BEGIN(pContext, pLog) MicroProfileGpuBegin(pContext, pLog)
#define MICROPROFILE_GPU_SET_CONTEXT(pContext, pLog) MicroProfileGpuSetContext(pContext, pLog)
#define MICROPROFILE_GPU_END(pLog) MicroProfileGpuEnd(pLog)
#define MICROPROFILE_GPU_SUBMIT(Queue, Work) MicroProfileGpuSubmit(Queue, Work)
#define MICROPROFILE_META_CPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeCpu)
//#define MICROPROFILE_META_GPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeGpu)
#define MICROPROFILE_COUNTER_ADD(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterAdd(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), count)
#define MICROPROFILE_COUNTER_SUB(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterAdd(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), -(int64_t)count)
#define MICROPROFILE_COUNTER_SET(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter_set,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterSet(MICROPROFILE_TOKEN_PASTE(g_mp_counter_set,__LINE__), count)
#define MICROPROFILE_COUNTER_SET_INT32_PTR(name, ptr) MicroProfileCounterSetPtr(name, ptr, sizeof(int32_t))
#define MICROPROFILE_COUNTER_SET_INT64_PTR(name, ptr) MicroProfileCounterSetPtr(name, ptr, sizeof(int64_t))
#define MICROPROFILE_COUNTER_CLEAR_PTR(name) MicroProfileCounterSetPtr(name, 0, 0)
#define MICROPROFILE_COUNTER_SET_LIMIT(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterSetLimit(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), count)
#define MICROPROFILE_COUNTER_CONFIG(name, type, limit, flags) MicroProfileCounterConfig(name, type, limit, flags)
#define MICROPROFILE_DECLARE_LOCAL_COUNTER(var) extern int64_t g_mp_local_counter##var; extern MicroProfileToken g_mp_counter_token##var;
#define MICROPROFILE_DEFINE_LOCAL_COUNTER(var, name) int64_t g_mp_local_counter##var = 0;MicroProfileToken g_mp_counter_token##var = MicroProfileGetCounterToken(name)
#define MICROPROFILE_DECLARE_LOCAL_ATOMIC_COUNTER(var) extern std::atomic<int64_t> g_mp_local_counter##var; extern MicroProfileToken g_mp_counter_token##var;
#define MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(var, name) std::atomic<int64_t> g_mp_local_counter##var;MicroProfileToken g_mp_counter_token##var = MicroProfileGetCounterToken(name)
#define MICROPROFILE_COUNTER_LOCAL_ADD(var, count) MicroProfileLocalCounterAdd(&g_mp_local_counter##var, (count))
#define MICROPROFILE_COUNTER_LOCAL_SUB(var, count) MicroProfileLocalCounterAdd(&g_mp_local_counter##var, -(int64_t)(count))
#define MICROPROFILE_COUNTER_LOCAL_SET(var, count) MicroProfileLocalCounterSet(&g_mp_local_counter##var, count)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD(var) MicroProfileCounterAdd(g_mp_counter_token##var, MicroProfileLocalCounterSet(&g_mp_local_counter##var, 0))
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_SET(var) MicroProfileCounterSet(g_mp_counter_token##var, MicroProfileLocalCounterSet(&g_mp_local_counter##var, 0))
#define MICROPROFILE_CONDITIONAL(expr) expr

#ifndef MICROPROFILE_USE_THREAD_NAME_CALLBACK
#define MICROPROFILE_USE_THREAD_NAME_CALLBACK 0
#endif

#ifndef MICROPROFILE_PER_THREAD_BUFFER_SIZE
#define MICROPROFILE_PER_THREAD_BUFFER_SIZE (2048<<10)
#endif

#ifndef MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE
#define MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE (128<<10)
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
#define MICROPROFILE_GPU_FRAME_DELAY 5 //must be > 0
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
struct MicroProfileThreadLogGpu;

MICROPROFILE_API void MicroProfileInit();
MICROPROFILE_API void MicroProfileShutdown();
MICROPROFILE_API MicroProfileToken MicroProfileFindToken(const char* sGroup, const char* sName);
MICROPROFILE_API MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token = MicroProfileTokenTypeCpu);
MICROPROFILE_API MicroProfileToken MicroProfileGetMetaToken(const char* pName);
MICROPROFILE_API MicroProfileToken MicroProfileGetCounterToken(const char* pName);
MICROPROFILE_API void MicroProfileMetaUpdate(MicroProfileToken, int nCount, MicroProfileTokenType eTokenType);
MICROPROFILE_API void MicroProfileCounterAdd(MicroProfileToken nToken, int64_t nCount);
MICROPROFILE_API void MicroProfileCounterSet(MicroProfileToken nToken, int64_t nCount);
MICROPROFILE_API void MicroProfileCounterSetLimit(MicroProfileToken nToken, int64_t nCount);
MICROPROFILE_API void MicroProfileCounterConfig(const char* pCounterName, uint32_t nFormat, int64_t nLimit, uint32_t nFlags);
MICROPROFILE_API void MicroProfileCounterSetPtr(const char* pCounterName, void* pValue, uint32_t nSize);
MICROPROFILE_API void MicroProfileCounterFetchCounters();
MICROPROFILE_API void MicroProfileLocalCounterAdd(int64_t* pCounter, int64_t nCount);
MICROPROFILE_API int64_t MicroProfileLocalCounterSet(int64_t* pCounter, int64_t nCount);
MICROPROFILE_API void MicroProfileLocalCounterAdd(std::atomic<int64_t>* pCounter, int64_t nCount);
MICROPROFILE_API int64_t MicroProfileLocalCounterSet(std::atomic<int64_t>* pCounter, int64_t nCount);
MICROPROFILE_API uint64_t MicroProfileEnter(MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileLeave(MicroProfileToken nToken, uint64_t nTick);
MICROPROFILE_API uint64_t MicroProfileGpuEnter(MicroProfileThreadLogGpu* pLog, MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileGpuLeave(MicroProfileThreadLogGpu* pLog, MicroProfileToken nToken, uint64_t nTick);
MICROPROFILE_API void MicroProfileGpuBegin(void* pContext, MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileGpuSetContext(void* pContext, MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API uint64_t MicroProfileGpuEnd(MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAlloc();
MICROPROFILE_API void MicroProfileThreadLogGpuFree(MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileThreadLogGpuReset(MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileGpuSubmit(int nQueue, uint64_t nWork);
MICROPROFILE_API int MicroProfileInitGpuQueue(const char* pQueueName);
MICROPROFILE_API int MicroProfileGetGpuQueue(const char* pQueueName);
inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t){ return (t&0xffff); }
inline uint64_t MicroProfileGetGroupMask(MicroProfileToken t){ return ((t>>16)&MICROPROFILE_GROUP_MASK_ALL);}
inline MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint16_t nTimer){ return (nGroupMask<<16) | nTimer;}
MICROPROFILE_API void MicroProfileFlip(void* pGpuContext); //! call once per frame.
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
MICROPROFILE_API int MicroProfileFormatCounter(int eFormat, int64_t nCounter, char* pOut, uint32_t nBufferSize);
MICROPROFILE_API MicroProfileThreadLogGpu* MicroProfileGetGlobaGpuThreadLog();
MICROPROFILE_API int MicroProfileGetGlobaGpuQueue();
MICROPROFILE_API void MicroProfileRegisterGroup(const char* pGroup, const char* pCategory, uint32_t nColor);

#if defined(MICROPROFILE_GPU_TIMERS_D3D12)
MICROPROFILE_API void MicroProfileGpuInitD3D12(void* pDevice, void* pCommandQueue);
MICROPROFILE_API void MicroProfileGpuShutdown();
#endif

#if MICROPROFILE_WEBSERVER
MICROPROFILE_API void MicroProfileDumpFile(const char* pHtml, const char* pCsv, float fCpuSpike = -1.f, float fGpuSpike = -1.f);
MICROPROFILE_API uint32_t MicroProfileWebServerPort();
#else
#define MicroProfileDumpFile(html,csv,cpu, gpu) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)
#endif




#if MICROPROFILE_GPU_TIMERS
MICROPROFILE_API uint32_t MicroProfileGpuInsertTimeStamp(void* pContext);
MICROPROFILE_API uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
MICROPROFILE_API uint64_t MicroProfileTicksPerSecondGpu();
MICROPROFILE_API int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu);
#else
#define MicroProfileGpuInsertTimeStamp(a) 1
#define MicroProfileGpuGetTimeStamp(a) 0
#define MicroProfileTicksPerSecondGpu() 1
#define MicroProfileGetGpuTickReference(a,b) 0
#endif

#if MICROPROFILE_GPU_TIMERS_D3D11
#define MICROPROFILE_D3D_MAX_QUERIES (8<<10)
MICROPROFILE_API void MicroProfileGpuInitD3D11(void* pDevice, void* pDeviceContext);
MICROPROFILE_API void MicroProfileGpuShutdown();
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
	MicroProfileThreadLogGpu* pLog;
	uint64_t nTick;
	MicroProfileScopeGpuHandler(MicroProfileToken Token, MicroProfileThreadLogGpu* pLog):nToken(Token), pLog(pLog)
	{
		nTick = MicroProfileGpuEnter(pLog, nToken);
	}
	~MicroProfileScopeGpuHandler()
	{
		MicroProfileGpuLeave(pLog, nToken, nTick);
	}
};


#define MICROPROFILE_MAX_COUNTERS 512
#define MICROPROFILE_MAX_COUNTER_NAME_CHARS (MICROPROFILE_MAX_COUNTERS*16)

#define MICROPROFILE_MAX_GROUPS 48 //dont bump! no. of bits used it bitmask
#define MICROPROFILE_MAX_CATEGORIES 16
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_BUFFER_SIZE)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_GPU_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS 256
#define MICROPROFILE_STACK_MAX 32
//#define MICROPROFILE_MAX_PRESETS 5
#define MICROPROFILE_ANIM_DELAY_PRC 0.5f
#define MICROPROFILE_GAP_TIME 50 //extra ms to fetch to close timers from earlier frames


#ifndef MICROPROFILE_MAX_TIMERS
#define MICROPROFILE_MAX_TIMERS 1024
#endif

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

#ifndef MICROPROFILE_MINIZ
#define MICROPROFILE_MINIZ 0
#endif

#ifndef MICROPROFILE_COUNTER_HISTORY
#define MICROPROFILE_COUNTER_HISTORY 1
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
#if _MSC_VER == 1900
typedef void * HANDLE;
#endif

typedef HANDLE MicroProfileThread;
#else
typedef std::thread* MicroProfileThread;
#endif



enum MicroProfileDrawMask
{
	MP_DRAW_OFF			= 0x0,
	MP_DRAW_BARS		= 0x1,
	MP_DRAW_DETAILED	= 0x2,
	MP_DRAW_COUNTERS	= 0x3,
	MP_DRAW_HIDDEN		= 0x4,
	MP_DRAW_SIZE		= 0x5,
};

enum MicroProfileDrawBarsMask
{
	MP_DRAW_TIMERS 				= 0x1,	
	MP_DRAW_AVERAGE				= 0x2,	
	MP_DRAW_MAX					= 0x4,	
	MP_DRAW_MIN					= 0x8,
	MP_DRAW_CALL_COUNT			= 0x10,
	MP_DRAW_TIMERS_EXCLUSIVE 	= 0x20,
	MP_DRAW_AVERAGE_EXCLUSIVE 	= 0x40,	
	MP_DRAW_MAX_EXCLUSIVE		= 0x80,
	MP_DRAW_META_FIRST			= 0x100,
	MP_DRAW_ALL 				= 0xffffffff,

};

enum MicroProfileCounterFormat
{
	MICROPROFILE_COUNTER_FORMAT_DEFAULT,
	MICROPROFILE_COUNTER_FORMAT_BYTES,
};

enum MicroProfileCounterFlags
{
	MICROPROFILE_COUNTER_FLAG_NONE 			= 0,
	MICROPROFILE_COUNTER_FLAG_DETAILED		= 0x1,
	MICROPROFILE_COUNTER_FLAG_DETAILED_GRAPH= 0x2,
	//internal:
	MICROPROFILE_COUNTER_FLAG_INTERNAL_MASK = ~0x3,
	MICROPROFILE_COUNTER_FLAG_HAS_LIMIT 	= 0x4,
	MICROPROFILE_COUNTER_FLAG_CLOSED		= 0x8,
	MICROPROFILE_COUNTER_FLAG_MANUAL_SWAP	= 0x10,
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

struct MicroProfileCounterInfo
{
	int nParent;
	int nSibling;
	int nFirstChild;
	uint16_t nNameLen;
	uint8_t nLevel;
	char* pName;
	uint32_t nFlags;
	int64_t nLimit;
	MicroProfileCounterFormat eFormat;
};

struct MicroProfileCounterHistory
{
	uint32_t nPut;
	uint64_t nHistory[MICROPROFILE_GRAPH_HISTORY];
};


struct MicroProfileCounterSource
{
	void* pSource;
	uint32_t nSourceSize;
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
	uint32_t 				nLogIndex;

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



//linear, per-frame per-thread gpu log
struct MicroProfileThreadLogGpu
{
	MicroProfileLogEntry Log[MICROPROFILE_GPU_BUFFER_SIZE];
	uint32_t nPut;
	uint32_t nStart;
	uint32_t nId;
	void* pContext;
	uint32_t nAllocated;
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
	void* pSyncQuery;
	MicroProfileD3D11Frame m_QueryFrames[MICROPROFILE_GPU_FRAME_DELAY];
};
#elif defined(MICROPROFILE_GPU_TIMERS_D3D12)
#include <d3d12.h>

#ifndef MICROPROFILE_D3D_MAX_QUERIES
#define MICROPROFILE_D3D_MAX_QUERIES (32<<10)
#endif

#define MICROPROFILE_D3D_INTERNAL_DELAY 8
struct MicroProfileD3D12Frame
{
	uint32_t nBegin;
	uint32_t nCount;
	ID3D12GraphicsCommandList* pCommandList;
	ID3D12CommandAllocator* pCommandAllocator;
};
struct MicroProfileGpuTimerState
{
	uint64_t nFrame;
	uint64_t nPendingFrame;

	uint32_t nFrameStart;
	std::atomic<uint32_t> nFrameCount;
	int64_t nFrequency;


	ID3D12CommandQueue* pCommandQueue;
	ID3D12QueryHeap* pHeap;
	ID3D12Fence* pFence;
	ID3D12Device* pDevice;
	ID3D12Resource* pBuffer;


	uint16_t nQueryFrames[MICROPROFILE_D3D_MAX_QUERIES];
	int64_t nResults[MICROPROFILE_D3D_MAX_QUERIES];

	MicroProfileD3D12Frame Frames[MICROPROFILE_D3D_INTERNAL_DELAY];

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

	uint64_t nForceGroupUI;
	uint64_t nActiveGroupWanted;
	uint32_t nAllGroupsWanted;
	uint32_t nAllThreadsWanted;

	uint32_t nOverflow;

	uint64_t nGroupMask;
	uint32_t nRunning;
	uint32_t nToggleRunning;
	uint32_t nMaxGroupSize;
	uint32_t nDumpFileNextFrame;
	uint32_t nDumpFileCountDown;
	uint32_t nDumpSpikeMask;
	uint32_t nAutoClearFrames;

	float 	fDumpCpuSpike;
	float 	fDumpGpuSpike;
	char 	HtmlDumpPath[512];
	char 	CsvDumpPath[512];


	int64_t nPauseTicks;

	float fReferenceTime;
	float fRcpReferenceTime;

	MicroProfileCategory	CategoryInfo[MICROPROFILE_MAX_CATEGORIES];
	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	uint8_t					TimerToGroup[MICROPROFILE_MAX_TIMERS];
	
	MicroProfileTimer 		AccumTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumMaxTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumMinTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumTimersExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t				AccumMaxTimersExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	uint64_t				FrameExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];	
	uint64_t				AggregateMin[MICROPROFILE_MAX_TIMERS];
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
	MicroProfileThreadLog* 		Pool[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLogGpu* 	PoolGpu[MICROPROFILE_MAX_THREADS];
	uint32_t				nNumLogs;
	uint32_t 				nNumLogsGpu;
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



	char 						CounterNames[MICROPROFILE_MAX_COUNTER_NAME_CHARS];
	MicroProfileCounterInfo 	CounterInfo[MICROPROFILE_MAX_COUNTERS];
	MicroProfileCounterSource	CounterSource[MICROPROFILE_MAX_COUNTERS];
	uint32_t					nNumCounters;
	uint32_t					nCounterNamePos;
	std::atomic<int64_t> 		Counters[MICROPROFILE_MAX_COUNTERS];
#if MICROPROFILE_COUNTER_HISTORY // uses 1kb per allocated counter. 512kb for default counter count
	uint32_t					nCounterHistoryPut;
	int64_t 					nCounterHistory[MICROPROFILE_GRAPH_HISTORY][MICROPROFILE_MAX_COUNTERS]; //flipped to make swapping cheap, drawing more expensive.
	int64_t 					nCounterMax[MICROPROFILE_MAX_COUNTERS];
	int64_t 					nCounterMin[MICROPROFILE_MAX_COUNTERS];	
#endif
	
	int							GpuQueue; 
	MicroProfileThreadLogGpu* 	pGpuGlobal;

	MicroProfileGpuTimerState 	GPU;

};

#define MP_LOG_TICK_MASK  0x0000ffffffffffff
#define MP_LOG_INDEX_MASK 0x3fff000000000000
#define MP_LOG_BEGIN_MASK 0xc000000000000000
#define MP_LOG_GPU_EXTRA 0x3
#define MP_LOG_META 0x2
#define MP_LOG_ENTER 0x1
#define MP_LOG_LEAVE 0x0


inline uint64_t MicroProfileLogType(MicroProfileLogEntry Index)
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
	uint64_t t = MicroProfileLogType(Entry);
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
template<typename T>
T MicroProfileClamp(T a, T min_, T max_)
{ return MicroProfileMin(max_, MicroProfileMax(min_, a));  }

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

#if defined(MICROPROFILE_WEBSERVER) || defined(MICROPROFILE_CONTEXT_SWITCH_TRACE)


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
    return (uint32_t)(uintptr_t)F(0);
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


#if defined(MICROPROFILE_GPU_TIMERS_D3D11) || defined(MICROPROFILE_GPU_TIMERS_D3D12)
uint32_t MicroProfileGpuFlip(void*);
void MicroProfileGpuShutdown();
#else
#define MicroProfileGpuFlip(a) MicroProfileGpuInsertTimeStamp(a)
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
#ifdef MICROPROFILE_IOS
// iOS doesn't support __thread
static pthread_key_t g_MicroProfileThreadLogKey;
static pthread_once_t g_MicroProfileThreadLogKeyOnce = PTHREAD_ONCE_INIT;

static void MicroProfileCreateThreadLogKey()
{
	pthread_key_create(&g_MicroProfileThreadLogKey, NULL);
}
#else
MP_THREAD_LOCAL MicroProfileThreadLog* g_MicroProfileThreadLogThreadLocal = 0;
#endif
static bool g_bUseLock = false; /// This is used because windows does not support using mutexes under dll init(which is where global initialization is handled)


MICROPROFILE_DEFINE(g_MicroProfileFlip, "MicroProfile", "MicroProfileFlip", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileThreadLoop, "MicroProfile", "ThreadLoop", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileClear, "MicroProfile", "Clear", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileAccumulate, "MicroProfile", "Accumulate", 0x3355ee);
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchSearch,"MicroProfile", "ContextSwitchSearch", 0xDD7300);
MICROPROFILE_DEFINE(g_MicroProfileGpuSubmit, "MicroProfile", "MicroProfileGpuSubmit", 0x3355ee);

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
MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAllocInternal();


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
		S.nWebServerDataSent = (uint64_t)-1;


#if MICROPROFILE_COUNTER_HISTORY
		S.nCounterHistoryPut = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_COUNTERS; ++i)
		{
			S.nCounterMin[i] = 0x7fffffffffffffff;
			S.nCounterMax[i] = 0x8000000000000000;
		}
#endif
		S.GpuQueue = MICROPROFILE_GPU_INIT_QUEUE("GPU");
		S.pGpuGlobal = MicroProfileThreadLogGpuAllocInternal();
		MicroProfileGpuBegin(0, S.pGpuGlobal);

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
	return g_MicroProfileThreadLogThreadLocal;
}
inline void MicroProfileSetThreadLog(MicroProfileThreadLog* pLog)
{
	g_MicroProfileThreadLogThreadLocal = pLog;
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
		memset(pLog, 0, sizeof(*pLog));
		S.nMemUsage += sizeof(MicroProfileThreadLog);
		pLog->nLogIndex = S.nNumLogs;
		S.Pool[S.nNumLogs++] = pLog;	
	}
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
	MP_ASSERT(MicroProfileGetThreadLog() == 0);
	MicroProfileThreadLog* pLog = MicroProfileCreateThreadLog(pThreadName ? pThreadName : MicroProfileGetThreadName());
	MP_ASSERT(pLog);
	MicroProfileSetThreadLog(pLog);
}

void MicroProfileThreadLogGpuReset(MicroProfileThreadLogGpu* pLog)
{
	MP_ASSERT(pLog->nAllocated);
	pLog->pContext = (void*)-1;
	pLog->nStart = (uint32_t)-1;
	pLog->nPut = 0;
}

MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAllocInternal()
{
	MicroProfileThreadLogGpu* pLog = 0;
	for (uint32_t i = 0; i < S.nNumLogsGpu; ++i)
	{
		MicroProfileThreadLogGpu* pNextLog = S.PoolGpu[i];
		if (pNextLog && !pNextLog->nAllocated)
		{
			pLog = pNextLog;
			break;
		}
	}
	if (!pLog)
	{
		pLog = new MicroProfileThreadLogGpu;
		int nLogIndex = S.nNumLogsGpu++;
		MP_ASSERT(nLogIndex < MICROPROFILE_MAX_THREADS);
		pLog->nId = nLogIndex;
		S.PoolGpu[nLogIndex] = pLog;
	}
	pLog->nAllocated = 1;
	MicroProfileThreadLogGpuReset(pLog);
	return pLog;
}

MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAlloc()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	return MicroProfileThreadLogGpuAllocInternal();
}

 void MicroProfileThreadLogGpuFree(MicroProfileThreadLogGpu* pLog)
 {
 	MP_ASSERT(pLog->nAllocated);
 	pLog->nAllocated = 0;
 }

 int MicroProfileGetGpuQueue(const char* pQueueName)
 {
	 for (uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; i++)
	 {
		 MicroProfileThreadLog* pLog = S.Pool[i];
		 if (pLog && pLog->nGpu && pLog->nActive && 0 == MP_STRCASECMP(pQueueName, pLog->ThreadName))
		 {
			 return i;
		 }
	 }
	 MP_ASSERT(0); //call MicroProfileInitGpuQueue
	 return 0;
 }

MicroProfileThreadLog* MicroProfileGetGpuQueueLog(const char* pQueueName)
{
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; i++)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog && pLog->nGpu && pLog->nActive && 0 == MP_STRCASECMP(pQueueName, pLog->ThreadName))
		{
			return pLog;
		}
	}
	MP_ASSERT(0); //call MicroProfileInitGpuQueue
	return 0;
}

int MicroProfileInitGpuQueue(const char* pQueueName)
{
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog && 0 == MP_STRCASECMP(pQueueName, pLog->ThreadName))
		{

			MP_ASSERT(0); // call MicroProfileInitGpuQueue only once per CommandQueue. name must not clash with threadname
		}
	}
	MicroProfileThreadLog* pLog = MicroProfileCreateThreadLog(pQueueName);
	pLog->nGpu = 1;
	pLog->nThreadId = 0;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		if(S.Pool[i] == pLog)
		{
			MicroProfileRegisterGroup(pQueueName, "GPU", (uint32_t)-1);
			return i;
		}
	}
	MP_BREAK();
	return 0;
}
MicroProfileThreadLogGpu* MicroProfileGetGlobaGpuThreadLog()
{
	return S.pGpuGlobal;
}

MICROPROFILE_API int MicroProfileGetGlobaGpuQueue()
{
	return S.GpuQueue;
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
	MP_ASSERT(nTimerIndex < MICROPROFILE_MAX_TIMERS);
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

const char* MicroProfileNextName(const char* pName, char* pNameOut, uint32_t* nSubNameLen)
{
	int nMaxLen = MICROPROFILE_NAME_MAX_LEN-1;
	const char* pRet = 0;
	bool bDone = false;
	uint32_t nChars = 0;
	for(int i = 0; i < nMaxLen && !bDone; ++i)
	{
		char c = *pName++;
		switch(c)
		{
			case 0:
				bDone = true;
				break;
			case '\\':
			case '/':
				if(nChars)
				{
					bDone = true;
					pRet = pName;
				}
				break;
			default:
				nChars++;
				*pNameOut++ = c;
		}
	}
	*nSubNameLen = nChars;
	*pNameOut = '\0';
	return pRet;
}


const char* MicroProfileCounterFullName(int nCounter)
{
	static char Buffer[1024];
	int nNodes[32];
	int nIndex = 0;
	do
	{
		nNodes[nIndex++] = nCounter;
		nCounter = S.CounterInfo[nCounter].nParent;
	}while(nCounter >= 0);
	int nOffset = 0;
	while(nIndex >= 0 && nOffset < (int)sizeof(Buffer)-2)
	{
		uint32_t nLen = S.CounterInfo[nNodes[nIndex]].nNameLen + nOffset;// < sizeof(Buffer)-1 
		nLen = MicroProfileMin((uint32_t)(sizeof(Buffer) - 2 - nOffset), nLen);
		memcpy(&Buffer[nOffset], S.CounterInfo[nNodes[nIndex]].pName, nLen);

		nOffset += S.CounterInfo[nNodes[nIndex]].nNameLen+1;
		if(nIndex)
		{
			Buffer[nOffset++] = '/';
		}
		nIndex--;
	}
	return &Buffer[0];
}
MicroProfileToken MicroProfileGetCounterTokenByParent(int nParent, const char* pName)
{
	for(uint32_t i = 0; i < S.nNumCounters; ++i)
	{
		if(nParent == S.CounterInfo[i].nParent && !MP_STRCASECMP(S.CounterInfo[i].pName, pName))
		{
			return i;
		}
	}
	MicroProfileToken nResult = S.nNumCounters++;
	S.CounterInfo[nResult].nParent = nParent;
	S.CounterInfo[nResult].nSibling = -1;
	S.CounterInfo[nResult].nFirstChild = -1;
	S.CounterInfo[nResult].nFlags = 0;
	S.CounterInfo[nResult].eFormat = MICROPROFILE_COUNTER_FORMAT_DEFAULT;
	S.CounterInfo[nResult].nLimit = 0;
	S.CounterSource[nResult].pSource = 0;
	S.CounterSource[nResult].nSourceSize = 0;
	int nLen = (int)strlen(pName)+1;

	MP_ASSERT(nLen + S.nCounterNamePos <= MICROPROFILE_MAX_COUNTER_NAME_CHARS);
	uint32_t nPos = S.nCounterNamePos;
	S.nCounterNamePos += nLen;
	memcpy(&S.CounterNames[nPos], pName, nLen);
	S.CounterInfo[nResult].nNameLen = nLen-1;
	S.CounterInfo[nResult].pName = &S.CounterNames[nPos];
	if(nParent >= 0)
	{
		S.CounterInfo[nResult].nSibling = S.CounterInfo[nParent].nFirstChild;
		S.CounterInfo[nResult].nLevel = S.CounterInfo[nParent].nLevel + 1;
		S.CounterInfo[nParent].nFirstChild = nResult;
	}
	else
	{
		S.CounterInfo[nResult].nLevel = 0;
	}

	return nResult;
}

MicroProfileToken MicroProfileGetCounterToken(const char* pName)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	char SubName[MICROPROFILE_NAME_MAX_LEN];
	MicroProfileToken nResult = MICROPROFILE_INVALID_TOKEN;
	do
	{
		uint32_t nLen = 0;
		pName = MicroProfileNextName(pName, &SubName[0], &nLen);
		if(0 == nLen)
		{
			break;
		}
		nResult = MicroProfileGetCounterTokenByParent(nResult, SubName);

	}while(pName != 0);

	MP_ASSERT((int)nResult >= 0);
	return nResult;
}

inline void MicroProfileLogPut(MicroProfileLogEntry LE, MicroProfileThreadLog* pLog)
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
		pLog->Log[nPos] = LE;
		pLog->nPut.store(nNextPos, std::memory_order_release);
	}
}

inline void MicroProfileLogPut(MicroProfileToken nToken_, uint64_t nTick, uint64_t nBegin, MicroProfileThreadLog* pLog)
{
	MicroProfileLogPut(MicroProfileMakeLogIndex(nBegin, nToken_, nTick), pLog);
}

inline void MicroProfileLogPutGpu(MicroProfileLogEntry LE, MicroProfileThreadLogGpu* pLog)
{
	uint32_t nPos = pLog->nPut;
	if(nPos < MICROPROFILE_GPU_BUFFER_SIZE)
	{
		pLog->Log[nPos] = LE;
		pLog->nPut = nPos+1;
	}
}

inline void MicroProfileLogPutGpu(MicroProfileToken nToken_, uint64_t nTick, uint64_t nBegin, MicroProfileThreadLogGpu* pLog)
{
	MicroProfileLogPutGpu(MicroProfileMakeLogIndex(nBegin, nToken_, nTick), pLog);
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
		MP_ASSERT(nToken < MICROPROFILE_META_MAX);
		if(MicroProfileTokenTypeCpu == eTokenType)
		{
			MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
			if(pLog)
			{
				MicroProfileLogPut(nToken, nCount, MP_LOG_META, pLog);
			}
		}
		else
		{
			MP_BREAK(); //no longer supported
			// MicroProfileThreadLogGpu* pLogGpu = MicroProfileGetThreadLogGpu_Global();
			// if(pLogGpu)
			// {
			// 	MicroProfileLogPutGpu(nToken, nCount, MP_LOG_META, pLogGpu);
			// }


		}
	}
}


void MicroProfileLocalCounterAdd(int64_t* pCounter, int64_t nCount)
{
	*pCounter += nCount;
}
int64_t MicroProfileLocalCounterSet(int64_t* pCounter, int64_t nCount)
{
	int64_t r = *pCounter;
	*pCounter = nCount;
	return r;
}

void MicroProfileLocalCounterAdd(std::atomic<int64_t>* pCounter, int64_t nCount)
{
	pCounter->fetch_add(nCount);
}
int64_t MicroProfileLocalCounterSet(std::atomic<int64_t>* pCounter, int64_t nCount)
{
	return pCounter->exchange(nCount);
}


void MicroProfileCounterAdd(MicroProfileToken nToken, int64_t nCount)
{
	MP_ASSERT(nToken < S.nNumCounters);
	S.Counters[nToken].fetch_add(nCount);
}
void MicroProfileCounterSet(MicroProfileToken nToken, int64_t nCount)
{
	MP_ASSERT(nToken < S.nNumCounters);
	S.Counters[nToken].store(nCount);
}

void MicroProfileCounterSetLimit(MicroProfileToken nToken, int64_t nCount)
{
	MP_ASSERT(nToken < S.nNumCounters);
	S.CounterInfo[nToken].nLimit = nCount;
}

void MicroProfileCounterConfig(const char* pName, uint32_t eFormat, int64_t nLimit, uint32_t nFlags)
{
	MicroProfileToken nToken = MicroProfileGetCounterToken(pName);
	S.CounterInfo[nToken].eFormat = (MicroProfileCounterFormat)eFormat;
	S.CounterInfo[nToken].nLimit = nLimit;
	S.CounterInfo[nToken].nFlags |= (nFlags & ~MICROPROFILE_COUNTER_FLAG_INTERNAL_MASK);
}

void MicroProfileCounterSetPtr(const char* pCounterName, void* pSource, uint32_t nSize)
{
	MicroProfileToken nToken = MicroProfileGetCounterToken(pCounterName);
	S.CounterSource[nToken].pSource = pSource;
	S.CounterSource[nToken].nSourceSize = nSize;
}

inline void MicroProfileFetchCounter(uint32_t i)
{
	switch (S.CounterSource[i].nSourceSize)
	{
	case sizeof(int32_t):
		S.Counters[i] = *(int32_t*)S.CounterSource[i].pSource;
		break;
	case sizeof(int64_t):
		S.Counters[i] = *(int64_t*)S.CounterSource[i].pSource;
		break;
	default: break;
	}
}
void MicroProfileCounterFetchCounters()
{
	for(uint32_t i = 0; i < S.nNumCounters; ++i)
	{
		MicroProfileFetchCounter(i);
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

void MicroProfileGpuBegin(void* pContext, MicroProfileThreadLogGpu* pLog)
{
	MP_ASSERT(pLog->pContext == (void*)-1); // dont call begin without calling end
	MP_ASSERT(pLog->nStart == (uint32_t)-1);
	MP_ASSERT(pContext != (void*)-1);

	pLog->pContext = pContext;
	pLog->nStart = pLog->nPut;
	MicroProfileLogPutGpu(0, pLog);
}

void MicroProfileGpuSetContext(void* pContext, MicroProfileThreadLogGpu* pLog)
{
	MP_ASSERT(pLog->pContext != (void*)-1); // dont call begin without calling end
	MP_ASSERT(pLog->nStart != (uint32_t)-1);
	pLog->pContext = pContext;
}

uint64_t MicroProfileGpuEnd(MicroProfileThreadLogGpu* pLog)
{
	uint64_t nStart = pLog->nStart;
	uint32_t nEnd = pLog->nPut;
	uint64_t nId = pLog->nId;
	if(nStart < MICROPROFILE_GPU_BUFFER_SIZE)
	{
		pLog->Log[nStart] = nEnd - nStart - 1;
	}
	pLog->pContext = (void*)-1;
	pLog->nStart = (uint32_t)-1;
	return nStart | (nId << 32);
}

void MicroProfileGpuSubmit(int nQueue, uint64_t nWork)
{
	MP_ASSERT(nQueue >= 0 && nQueue < MICROPROFILE_MAX_THREADS);
	MICROPROFILE_SCOPE(g_MicroProfileGpuSubmit);
	uint32_t nStart = (uint32_t)nWork;
	uint32_t nThreadLog = uint32_t(nWork >> 32);

	MicroProfileThreadLog* pQueueLog = S.Pool[nQueue];
	MP_ASSERT(nQueue < MICROPROFILE_MAX_THREADS);
	MicroProfileThreadLogGpu* pGpuLog = S.PoolGpu[nThreadLog];
	MP_ASSERT(pGpuLog);
	
	int64_t nCount = 0;
	if(nStart < MICROPROFILE_GPU_BUFFER_SIZE)
	{
		nCount = pGpuLog->Log[nStart];
	}
	nStart++;
	for(int32_t i = 0; i < nCount; ++i)
	{
		MicroProfileLogEntry LE = pGpuLog->Log[nStart++];
		MicroProfileLogPut(LE, pQueueLog);
	}
}

uint64_t MicroProfileGpuEnter(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd		
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		MicroProfileLogPutGpu(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		MicroProfileLogPutGpu(pLog->nLogIndex, MP_TICK(), MP_LOG_GPU_EXTRA, pGpuLog);
		return 1;
	}
	return 0;
}

void MicroProfileGpuLeave(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(nTickStart)
	{
		// MicroProfileThreadLogGpu* pGpuLog = MicroProfileGetThreadLogGpu();		
		MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		MicroProfileLogPutGpu(nToken_, nTimer, MP_LOG_LEAVE, pGpuLog);
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		MicroProfileLogPutGpu(pLog->nLogIndex, MP_TICK(), MP_LOG_GPU_EXTRA, pGpuLog);
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

void MicroProfileFlip(void* pContext)
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

	int64_t nGpuWork = MicroProfileGpuEnd(S.pGpuGlobal);
	MicroProfileGpuSubmit(S.GpuQueue, nGpuWork);
	MicroProfileThreadLogGpuReset(S.pGpuGlobal);
	for (uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
	{
		if (S.PoolGpu[i])
		{
			S.PoolGpu[i]->nPut = 0;
		}
	}

	MicroProfileGpuBegin(0, S.pGpuGlobal);

	uint32_t nGpuTimeStamp = MicroProfileGpuFlip(pContext);

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
		if(0 == S.nDumpFileCountDown)
		{
			MicroProfileDumpToFile();
			S.nDumpFileNextFrame = 0;
			S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; //hide spike from dumping webpage
		}
		else
		{
			S.nDumpFileCountDown--;
		}
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
		
		pFramePut->nFrameStartGpu = nGpuTimeStamp;
		{
			const float fDumpTimeThreshold = 1000.f * 60 * 60 * 24 * 365.f; // if time above this, then we're handling uninitialized counters
			int nDumpNextFrame = 0;
			float fTimeGpu = 0.f;
			if(pFrameNext->nFrameStartGpu != -1)
			{

				uint64_t nTickCurrent = pFrameCurrent->nFrameStartGpu;
				uint64_t nTickNext = pFrameNext->nFrameStartGpu = MicroProfileGpuGetTimeStamp((uint32_t)pFrameNext->nFrameStartGpu);
				float fTime = 1000.f * (nTickNext - nTickCurrent) / MicroProfileTicksPerSecondGpu();
				fTime = fTimeGpu;
				if(S.fDumpGpuSpike > 0.f && fTime > S.fDumpGpuSpike && fTime < fDumpTimeThreshold)
				{
					nDumpNextFrame = 1;
				}
			}
			float fTimeCpu = 1000.f * (pFrameNext->nFrameStartCpu - pFrameCurrent->nFrameStartCpu) / MicroProfileTicksPerSecondCpu();
			if(S.fDumpCpuSpike > 0.f && fTimeCpu > S.fDumpCpuSpike && fTimeCpu < fDumpTimeThreshold)
			{
				nDumpNextFrame = 1;
			}
			if(nDumpNextFrame)
			{
				S.nDumpFileNextFrame = S.nDumpSpikeMask;
				S.nDumpSpikeMask = 0;
				S.nDumpFileCountDown = 5;
			}
		}

		if(pFrameCurrent->nFrameStartGpu == -1)
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
								if(MicroProfileLogType(L) < MP_LOG_META)
								{
									pLog->Log[k] = MicroProfileLogSetTick(L, MicroProfileGpuGetTimeStamp((uint32_t)MicroProfileLogGetTick(L)));
								}
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
							uint64_t nType = MicroProfileLogType(LE);

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
							else if(MP_LOG_LEAVE == nType)
							{
								int nTimer = MicroProfileLogTimerIndex(LE);
								uint8_t nGroup = pTimerToGroup[nTimer];
								MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
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
					S.AccumMinTimers[i] = MicroProfileMin(S.AccumMinTimers[i], S.Frame[i].nTicks);
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
		memcpy(&S.AggregateMin[0], &S.AccumMinTimers[0], sizeof(S.AggregateMin[0]) * S.nTotalTimers);
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
			memset(&S.AccumMinTimers[0], 0xFF, sizeof(S.AccumMinTimers[0]) * S.nTotalTimers);
			memset(&S.AccumTimersExclusive[0], 0, sizeof(S.AggregateExclusive[0]) * S.nTotalTimers);
			memset(&S.AccumMaxTimersExclusive[0], 0, sizeof(S.AccumMaxTimersExclusive[0]) * S.nTotalTimers);
			memset(&S.AccumGroup[0], 0, sizeof(S.AggregateGroup));
			memset(&S.AccumGroupMax[0], 0, sizeof(S.AggregateGroup));		

			S.nAggregateFlipCount = 0;
			S.nFlipAggregate = 0;
			S.nFlipMax = 0;

			S.nAggregateFlipTick = MP_TICK();
		}

		#if MICROPROFILE_COUNTER_HISTORY
		int64_t* pDest = &S.nCounterHistory[S.nCounterHistoryPut][0];
		S.nCounterHistoryPut = (S.nCounterHistoryPut+1) % MICROPROFILE_GRAPH_HISTORY;
		for(uint32_t i = 0; i < S.nNumCounters; ++i)
		{
			if(0 != (S.CounterInfo[i].nFlags & MICROPROFILE_COUNTER_FLAG_DETAILED))
			{
				MicroProfileFetchCounter(i);
				uint64_t nValue = S.Counters[i].load(std::memory_order_relaxed);
				pDest[i] = nValue;
				S.nCounterMin[i] = MicroProfileMin(S.nCounterMin[i], (int64_t)nValue);
				S.nCounterMax[i] = MicroProfileMax(S.nCounterMax[i], (int64_t)nValue);
			}
		}
		#endif
	}
	S.nAggregateClear = 0;

	uint64_t nNewActiveGroup = 0;
	if(S.nForceEnable || (S.nDisplay && S.nRunning))
		nNewActiveGroup = S.nAllGroupsWanted ? S.nGroupMask : S.nActiveGroupWanted;
	nNewActiveGroup |= S.nForceGroup;
	nNewActiveGroup |= S.nForceGroupUI;
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


void MicroProfileCalcAllTimers(float* pTimers, float* pAverage, float* pMax, float* pMin, float* pCallAverage, float* pExclusive, float* pAverageExclusive, float* pMaxExclusive, float* pTotal, uint32_t nSize)
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
		float fMinMs = fToMs * (S.AggregateMin[nTimer] != uint64_t(-1) ? S.AggregateMin[nTimer] : 0);
		float fMinPrc = MicroProfileMin(fMinMs * fToPrc, 1.f);
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
		pMin[nIdx] = fMinMs;
		pMin[nIdx + 1] = fMinPrc;
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

int MicroProfileFormatCounter(int eFormat, int64_t nCounter, char* pOut, uint32_t nBufferSize)
{
	if (!nCounter)
	{
		pOut[0] = '\0';
		return 0;
	}
	int nLen = 0;
	char* pBase = pOut;
	char* pTmp = pOut;
	char* pEnd = pOut + nBufferSize;
	int nNegative = 0;
	if(nCounter < 0)
	{
		nCounter = -nCounter;
		nNegative = 1;
	}


	switch (eFormat)
	{
	case MICROPROFILE_COUNTER_FORMAT_DEFAULT:
	{
		int nSeperate = 0;
		while (nCounter)
		{
			if (nSeperate)
			{
				*pTmp++ = '.';
			}
			nSeperate = 1;
			for (uint32_t i = 0; nCounter && i < 3; ++i)
			{
				int nDigit = nCounter % 10;
				nCounter /= 10;
				*pTmp++ = '0' + nDigit;
			}
		}
		if(nNegative)
		{
			*pTmp++ = '-';
		}
		nLen = pTmp - pOut;
		--pTmp;
		MP_ASSERT(pTmp <= pEnd);
		while (pTmp > pOut) //reverse string
		{
			char c = *pTmp;
			*pTmp = *pOut;
			*pOut = c;
			pTmp--;
			pOut++;
		}
	}
	break;
	case MICROPROFILE_COUNTER_FORMAT_BYTES:
	{
		const char* pExt[] = { "b","kb","mb","gb","tb","pb", };
		size_t nNumExt = sizeof(pExt) / sizeof(pExt[0]);
		int64_t nShift = 0;
		int64_t nDivisor = 1;
		int64_t nCountShifted = nCounter >> 10;
		while (nCountShifted)
		{
			nDivisor <<= 10;
			nCountShifted >>= 10;
			nShift++;
		}
		MP_ASSERT(nShift < (int64_t)nNumExt);
		if (nShift)
		{
			nLen = snprintf(pOut, nBufferSize - 1, "%c%3.2f%s", nNegative?'-':' ',(double)nCounter / nDivisor, pExt[nShift]);
		}
		else
		{
			nLen = snprintf(pOut, nBufferSize - 1, "%c%" PRId64 "%s",nNegative?'-':' ', nCounter, pExt[nShift]);
		}
	}
	break;
	}
	pBase[nLen] = '\0';

	return nLen;
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

void MicroProfileDumpFile(const char* pHtml, const char* pCsv, float fCpuSpike, float fGpuSpike)
{
	S.fDumpCpuSpike = fCpuSpike;
	S.fDumpGpuSpike = fGpuSpike;
	uint32_t nDumpMask = 0;
	if(pHtml)
	{
		size_t nLen = strlen(pHtml);
		if(nLen > sizeof(S.HtmlDumpPath)-1)
		{
			return;
		}
		memcpy(S.HtmlDumpPath, pHtml, nLen+1);

		nDumpMask |= 1;
	}
	if(pCsv)
	{
		size_t nLen = strlen(pCsv);
		if(nLen > sizeof(S.CsvDumpPath)-1)
		{
			return;
		}
		memcpy(S.CsvDumpPath, pCsv, nLen+1);
		nDumpMask |= 2;
	}
	if(fCpuSpike > 0.f || fGpuSpike > 0.f)
	{
		S.nDumpFileNextFrame = 0;
		S.nDumpSpikeMask = nDumpMask;
	}
	else
	{
		S.nDumpFileNextFrame = nDumpMask;
		S.nDumpSpikeMask = 0;
		S.nDumpFileCountDown = 0;
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
	float* pTimers = (float*)alloca(nBlockSize * 9 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pMin = pTimers + 3 * nBlockSize;
	float* pCallAverage = pTimers + 4 * nBlockSize;
	float* pTimersExclusive = pTimers + 5 * nBlockSize;
	float* pAverageExclusive = pTimers + 6 * nBlockSize;
	float* pMaxExclusive = pTimers + 7 * nBlockSize;
	float* pTotal = pTimers + 8 * nBlockSize;

	MicroProfileCalcAllTimers(pTimers, pAverage, pMax, pMin, pCallAverage, pTimersExclusive, pAverageExclusive, pMaxExclusive, pTotal, nNumTimers);

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


void MicroProfileDumpHtml(MicroProfileWriteCallback CB, void* Handle, int nMaxFrames, const char* pHost)
{
	uint32_t nRunning = S.nRunning;
	S.nRunning = 0;
	//stall pushing of timers
	uint64_t nActiveGroup = S.nActiveGroup;
	S.nActiveGroup = 0;
	S.nPauseTicks = MP_TICK();

	MicroProfileCounterFetchCounters();
	for(size_t i = 0; i < g_MicroProfileHtml_begin_count; ++i)
	{
		CB(Handle, g_MicroProfileHtml_begin_sizes[i]-1, g_MicroProfileHtml_begin[i]);
	}
	//dump info
	uint64_t nTicks = MP_TICK();

	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
	float fAggregateMs = fToMsCPU * (nTicks - S.nAggregateFlipTick);
	MicroProfilePrintf(CB, Handle, "var DumpHost = '%s';\n", pHost ? pHost : "");
	time_t CaptureTime;
	time(&CaptureTime);
	MicroProfilePrintf(CB, Handle, "var DumpUtcCaptureTime = %ld;\n", CaptureTime);
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
	float* pTimers = (float*)alloca(nBlockSize * 9 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pMin = pTimers + 3 * nBlockSize;
	float* pCallAverage = pTimers + 4 * nBlockSize;
	float* pTimersExclusive = pTimers + 5 * nBlockSize;
	float* pAverageExclusive = pTimers + 6 * nBlockSize;
	float* pMaxExclusive = pTimers + 7 * nBlockSize;
	float* pTotal = pTimers + 8 * nBlockSize;

	MicroProfileCalcAllTimers(pTimers, pAverage, pMax, pMin, pCallAverage, pTimersExclusive, pAverageExclusive, pMaxExclusive, pTotal, nNumTimers);

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


		uint32_t nColor = S.TimerInfo[i].nColor;
		uint32_t nColorDark = (nColor >> 1) & ~0x80808080;
		MicroProfilePrintf(CB, Handle, "TimerInfo[%d] = MakeTimer(%d, \"%s\", %d, '#%02x%02x%02x','#%02x%02x%02x', %f, %f, %f, %f, %f, %f, %d, %f, Meta%d, MetaAvg%d, MetaMax%d);\n", S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].nTimerIndex, S.TimerInfo[i].pName, S.TimerInfo[i].nGroupIndex, 
			MICROPROFILE_UNPACK_RED(nColor) & 0xff,
			MICROPROFILE_UNPACK_GREEN(nColor) & 0xff,
			MICROPROFILE_UNPACK_BLUE(nColor) & 0xff,
			MICROPROFILE_UNPACK_RED(nColorDark) & 0xff,
			MICROPROFILE_UNPACK_GREEN(nColorDark) & 0xff,
			MICROPROFILE_UNPACK_BLUE(nColorDark) & 0xff,
			pAverage[nIdx],
			pMax[nIdx],
			pMin[nIdx],
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
	MicroProfilePrintf(CB, Handle, "\nvar ISGPU = [");
	for (uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		MicroProfilePrintf(CB, Handle, "%d,", (S.Pool[i] && S.Pool[i]->nGpu) ? 1 : 0);
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

	MicroProfilePrintf(CB, Handle, "\nvar CounterInfo = [");

	for(int i = 0; i < (int)S.nNumCounters; ++i)
	{
		uint64_t nCounter = S.Counters[i].load();
		uint64_t nLimit = S.CounterInfo[i].nLimit;
		float fCounterPrc = 0.f;
		float fBoxPrc = 1.f;
		if(nLimit)
		{
			fCounterPrc = (float)nCounter / nLimit;
			if(fCounterPrc>1.f)
			{
				fBoxPrc = 1.f / fCounterPrc;
				fCounterPrc = 1.f;
			}
		}

		char Formatted[64];
		char FormattedLimit[64];
		MicroProfileFormatCounter(S.CounterInfo[i].eFormat, nCounter, Formatted, sizeof(Formatted)-1);
		MicroProfileFormatCounter(S.CounterInfo[i].eFormat, S.CounterInfo[i].nLimit, FormattedLimit, sizeof(FormattedLimit)-1);
		MicroProfilePrintf(CB, Handle, "MakeCounter(%d, %d, %d, %d, %d, '%s', %lld, '%s', %lld, '%s', %f, %f),\n", 
			i,
			S.CounterInfo[i].nParent,
			S.CounterInfo[i].nSibling,
 	 		S.CounterInfo[i].nFirstChild,
 	 		S.CounterInfo[i].nLevel,
			S.CounterInfo[i].pName,
			nCounter,
			Formatted,
			nLimit,
			FormattedLimit,
			fCounterPrc,
			fBoxPrc
			);
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
	//uprintf("got timestamp %llp from frame %d\n", nTickStartGpu, nFirstFrame );

	int64_t nTickReferenceCpu, nTickReferenceGpu;
	int64_t nTicksPerSecondCpu = MicroProfileTicksPerSecondCpu();
	int64_t nTicksPerSecondGpu = MicroProfileTicksPerSecondGpu();
	int nTickReference = 0;
	if(MicroProfileGetGpuTickReference(&nTickReferenceCpu, &nTickReferenceGpu))
	{
		nTickStartGpu = (nTickStart - nTickReferenceCpu) * nTicksPerSecondGpu / nTicksPerSecondCpu + nTickReferenceGpu;
		nTickReference = 1;
	}


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
			int64_t nStartTickBase = pLog->nGpu ? nTickStartGpu : nTickStart;
			uint32_t nLogStart = S.Frames[nFrameIndex].nLogStart[j];
			uint32_t nLogEnd = S.Frames[nFrameIndexNext].nLogStart[j];

			float fToMsCpu = MicroProfileTickToMsMultiplier(nTicksPerSecondCpu);
			float fToMsBase = MicroProfileTickToMsMultiplier(pLog->nGpu ? nTicksPerSecondGpu : nTicksPerSecondCpu);
			MicroProfilePrintf(CB, Handle, "var ts_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				uint32_t k = nLogStart;
				uint64_t nLogType = MicroProfileLogType(pLog->Log[k]);
				float fToMs = nLogType == MP_LOG_GPU_EXTRA ? fToMsCpu : fToMsBase;
				int64_t nStartTick = nLogType == MP_LOG_GPU_EXTRA ? nTickStart : nStartTickBase;
				float fTime = nLogType == MP_LOG_META ? 0.f : MicroProfileLogTickDifference(nStartTick, pLog->Log[k]) * fToMs;
				MicroProfilePrintf(CB, Handle, "%f", fTime);
				for(k = (k+1) % MICROPROFILE_BUFFER_SIZE; k != nLogEnd; k = (k+1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint64_t nLogType = MicroProfileLogType(pLog->Log[k]);
					float fToMs = nLogType == MP_LOG_GPU_EXTRA ? fToMsCpu : fToMsBase;
					nStartTick = nLogType == MP_LOG_GPU_EXTRA ? nTickStart : nStartTickBase;
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
					uint64_t nLogType = MicroProfileLogType(pLog->Log[k]);
					if(nLogType == MP_LOG_META)
					{
						//for meta, store the count + 3, which is the tick part
						nLogType = 3 + MicroProfileLogGetTick(pLog->Log[k]);
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

		float fToMs = MicroProfileTickToMsMultiplier(nTicksPerSecondCpu);
		float fFrameMs = MicroProfileLogTickDifference(nTickStart, nFrameStart) * fToMs;
		float fFrameEndMs = MicroProfileLogTickDifference(nTickStart, nFrameEnd) * fToMs;
		float fFrameGpuMs = 0;
		float fFrameGpuEndMs = 0;
		if(nTickReference)
		{
			fFrameGpuMs = MicroProfileLogTickDifference(nTickStartGpu, S.Frames[nFrameIndex].nFrameStartGpu) * fToMsGPU;
			fFrameGpuEndMs = MicroProfileLogTickDifference(nTickStartGpu, S.Frames[nFrameIndexNext].nFrameStartGpu) * fToMsGPU;
		}
		MicroProfilePrintf(CB, Handle, "Frames[%d] = MakeFrame(%d, %f, %f, %f, %f, ts%d, tt%d, ti%d);\n", i, 0, fFrameMs, fFrameEndMs, fFrameGpuMs, fFrameGpuEndMs, i, i, i);
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
			MicroProfileDumpHtml(MicroProfileWriteFile, F, MICROPROFILE_WEBSERVER_MAXFRAMES, S.HtmlDumpPath);
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
		send(Socket, pData, (int)nSize, 0);

	}
	else
	{
		memcpy(&S.WebServerBuffer[S.WebServerPut], pData, nSize);
		S.WebServerPut += (uint32_t)nSize;
		if(S.WebServerPut > MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE/2)
		{
			MicroProfileFlushSocket(Socket);
		}
	}
}

#if MICROPROFILE_MINIZ
#ifndef MICROPROFILE_COMPRESS_BUFFER_SIZE
#define MICROPROFILE_COMPRESS_BUFFER_SIZE (256<<10)
#endif

#define MICROPROFILE_COMPRESS_CHUNK (MICROPROFILE_COMPRESS_BUFFER_SIZE/2)
struct MicroProfileCompressedSocketState
{
	unsigned char DeflateOut[MICROPROFILE_COMPRESS_CHUNK];
	unsigned char DeflateIn[MICROPROFILE_COMPRESS_CHUNK];
	mz_stream Stream;
	MpSocket Socket;
	uint32_t nSize;
	uint32_t nCompressedSize;
	uint32_t nFlushes;
	uint32_t nMemmoveBytes;
};

void MicroProfileCompressedSocketFlush(MicroProfileCompressedSocketState* pState)
{
	mz_stream& Stream = pState->Stream;
	unsigned char* pSendStart = &pState->DeflateOut[0];
	unsigned char* pSendEnd = &pState->DeflateOut[MICROPROFILE_COMPRESS_CHUNK - Stream.avail_out];
	if(pSendStart != pSendEnd)
	{
		send(pState->Socket, (const char*)pSendStart, pSendEnd - pSendStart, 0);
		pState->nCompressedSize += pSendEnd - pSendStart;
	}
	Stream.next_out = &pState->DeflateOut[0];
	Stream.avail_out = MICROPROFILE_COMPRESS_CHUNK;

}
void MicroProfileCompressedSocketStart(MicroProfileCompressedSocketState* pState, MpSocket Socket)
{
	mz_stream& Stream = pState->Stream;
	memset(&Stream, 0, sizeof(Stream));
	Stream.next_out = &pState->DeflateOut[0];
	Stream.avail_out = MICROPROFILE_COMPRESS_CHUNK;
	Stream.next_in = &pState->DeflateIn[0];
	Stream.avail_in = 0;
	mz_deflateInit(&Stream, Z_DEFAULT_COMPRESSION);
	pState->Socket = Socket;
	pState->nSize = 0;
	pState->nCompressedSize = 0;
	pState->nFlushes = 0;
	pState->nMemmoveBytes = 0;

}
void MicroProfileCompressedSocketFinish(MicroProfileCompressedSocketState* pState)
{
	mz_stream& Stream = pState->Stream;
	MicroProfileCompressedSocketFlush(pState);
	int r = mz_deflate(&Stream, MZ_FINISH);
	MP_ASSERT(r == MZ_STREAM_END);
	MicroProfileCompressedSocketFlush(pState);
	r = mz_deflateEnd(&Stream);
	MP_ASSERT(r == MZ_OK);
}

void MicroProfileCompressedWriteSocket(void* Handle, size_t nSize, const char* pData)
{
	MicroProfileCompressedSocketState* pState = (MicroProfileCompressedSocketState*)Handle;
	mz_stream& Stream = pState->Stream;
	const unsigned char* pDeflateInEnd = Stream.next_in + Stream.avail_in;
	const unsigned char* pDeflateInStart = &pState->DeflateIn[0];
	const unsigned char* pDeflateInRealEnd = &pState->DeflateIn[MICROPROFILE_COMPRESS_CHUNK];	
	pState->nSize += (uint32_t)nSize;
	if((ptrdiff_t)nSize <= pDeflateInRealEnd - pDeflateInEnd)
	{
		memcpy((void*)pDeflateInEnd, pData, nSize);
		Stream.avail_in += (uint32_t)nSize;
		MP_ASSERT(Stream.next_in + Stream.avail_in <= pDeflateInRealEnd);
		return;
	}
	int Flush = 0;
	while(nSize)
	{
		pDeflateInEnd = Stream.next_in + Stream.avail_in;
		if(Flush)
		{
			pState->nFlushes++;
			MicroProfileCompressedSocketFlush(pState);
			pDeflateInRealEnd = &pState->DeflateIn[MICROPROFILE_COMPRESS_CHUNK];	
			if(pDeflateInEnd == pDeflateInRealEnd)
			{
				if(Stream.avail_in)
				{
					MP_ASSERT(pDeflateInStart != Stream.next_in);
					memmove((void*)pDeflateInStart, Stream.next_in, Stream.avail_in);
					pState->nMemmoveBytes += Stream.avail_in;
				}
				Stream.next_in = pDeflateInStart;
				pDeflateInEnd = Stream.next_in + Stream.avail_in;
			}
		}
		size_t nSpace = pDeflateInRealEnd - pDeflateInEnd;
		size_t nBytes = MicroProfileMin(nSpace, nSize);
		MP_ASSERT(nBytes + pDeflateInEnd <= pDeflateInRealEnd);
		memcpy((void*)pDeflateInEnd, pData, nBytes); 
		Stream.avail_in += (uint32_t)nBytes;
		nSize -= nBytes;
		pData += nBytes;
		int r = mz_deflate(&Stream, MZ_NO_FLUSH);
		Flush = r == MZ_BUF_ERROR || nBytes == 0 || Stream.avail_out == 0 ? 1 : 0;
		MP_ASSERT(r == MZ_BUF_ERROR || r == MZ_OK);
		if(r == MZ_BUF_ERROR)
		{
			r = mz_deflate(&Stream, MZ_SYNC_FLUSH);
		}
	}
}
#endif


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

int MicroProfileParseGet(const char* pGet)
{
	const char* pStart = pGet;
	while(*pGet != '\0')
	{
		if(*pGet < '0' || *pGet > '9')
			return 0;
		pGet++;
	}
	int nFrames = atoi(pStart);
	if(nFrames)
	{
		return nFrames;
	}
	else
	{
		return MICROPROFILE_WEBSERVER_MAXFRAMES;
	}
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
		int nReceived = recv(Connection, Req, sizeof(Req)-1, 0);
		if(nReceived > 0)
		{
			Req[nReceived] = '\0';
#if MICROPROFILE_MINIZ
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Encoding: deflate\r\nExpires: Tue, 01 Jan 2199 16:00:00 GMT\r\n\r\n"
#else
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nExpires: Tue, 01 Jan 2199 16:00:00 GMT\r\n\r\n"
#endif
			char* pHttp = strstr(Req, "HTTP/");
			char* pGet = strstr(Req, "GET /");
			char* pHost = strstr(Req, "Host: ");
			auto Terminate = [](char* pString)
			{
				char* pEnd = pString;
				while(*pEnd != '\0')
				{
					if(*pEnd == '\r' || *pEnd == '\n' || *pEnd == ' ')
					{
						*pEnd = '\0';
						return;
					}
					pEnd++;
				}
			};
			if(pHost)
			{
				pHost += sizeof("Host: ")-1;
				Terminate(pHost);
			}

			if(pHttp && pGet)
			{
				*pHttp = '\0';
				pGet += sizeof("GET /")-1;
				Terminate(pGet);
				int nFrames = MicroProfileParseGet(pGet);
				if(nFrames)
				{
					MicroProfileSetNonBlocking(Connection, 0);
					uint64_t nTickStart = MP_TICK();
					send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER)-1, 0);
					uint64_t nDataStart = S.nWebServerDataSent;
					S.WebServerPut = 0;
	#if 0 == MICROPROFILE_MINIZ
					MicroProfileDumpHtml(MicroProfileWriteSocket, &Connection, nFrames, pHost);
					uint64_t nDataEnd = S.nWebServerDataSent;
					uint64_t nTickEnd = MP_TICK();
					uint64_t nDiff = (nTickEnd - nTickStart);
					float fMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu()) * nDiff;
					int nKb = ((nDataEnd-nDataStart)>>10) + 1;
					int nCompressedKb = nKb;
					MicroProfilePrintf(MicroProfileWriteSocket, &Connection, "\n<!-- Sent %dkb in %.2fms-->\n\n",nKb, fMs);
					MicroProfileFlushSocket(Connection);
	#else
					MicroProfileCompressedSocketState CompressState;
					MicroProfileCompressedSocketStart(&CompressState, Connection);
					MicroProfileDumpHtml(MicroProfileCompressedWriteSocket, &CompressState, nFrames, pHost);
					S.nWebServerDataSent += CompressState.nSize;
					uint64_t nDataEnd = S.nWebServerDataSent;
					uint64_t nTickEnd = MP_TICK();
					uint64_t nDiff = (nTickEnd - nTickStart);
					float fMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu()) * nDiff;
					int nKb = ((nDataEnd-nDataStart)>>10) + 1;
					int nCompressedKb = ((CompressState.nCompressedSize)>>10) + 1;
					MicroProfilePrintf(MicroProfileCompressedWriteSocket, &CompressState, "\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
					MicroProfileCompressedSocketFinish(&CompressState);
					MicroProfileFlushSocket(Connection);
	#endif

	#if MICROPROFILE_DEBUG
					printf("\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
	#endif
				}
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
        S.bContextSwitchRunning = true;
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
#include <d3d11.h>
uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
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

uint32_t MicroProfileGpuFlip(void* pCtx)
{
	uint32_t nFrameTimeStamp = MicroProfileGpuInsertTimeStamp(pCtx);
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
					OutputDebugStringA("Query freq changing");
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
	return nFrameTimeStamp;
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
	HRESULT hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.GPU.pSyncQuery);
	MP_ASSERT(hr == S_OK);

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
	((ID3D11Query*)S.GPU.pSyncQuery)->Release();
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	MicroProfileD3D11Frame& Frame = S.GPU.m_QueryFrames[S.GPU.m_nQueryFrame];
	if (Frame.m_nRateQueryStarted)
	{
		ID3D11Query* pSyncQuery = (ID3D11Query*)S.GPU.pSyncQuery;
		ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)S.GPU.m_pDeviceContext;
		pContext->End(pSyncQuery);

		HRESULT hr;
		do
		{
			hr = pContext->GetData(pSyncQuery, pOutGpu, sizeof(*pOutGpu), 0);
		} while (hr == S_FALSE);
		*pOutCPU = MP_TICK();
		switch (hr)
		{
		case DXGI_ERROR_DEVICE_REMOVED:
		case DXGI_ERROR_INVALID_CALL:
		case E_INVALIDARG:
			MP_BREAK();
			return false;

		}
		MP_ASSERT(hr == S_OK);
		return 1;
	}
	return 0;
}

#elif defined(MICROPROFILE_GPU_TIMERS_D3D12)
#include <d3d12.h>
//#include <d3dx12.h>
uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	
	uint32_t nFrame = S.GPU.nFrame;
	uint32_t nQueryIndex = (S.GPU.nFrameCount.fetch_add(1) + S.GPU.nFrameStart) % MICROPROFILE_D3D_MAX_QUERIES;
	
	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)pContext;
	pCommandList->EndQuery(S.GPU.pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nQueryIndex);
	MP_ASSERT(nQueryIndex <= 0xffff);
	//uprintf("insert timestamp %d :: %d ... ctx %p\n", nQueryIndex, nFrame, pContext);
	return ((nFrame << 16) & 0xffff0000) | (nQueryIndex);
}

void MicroProfileGpuFetchRange(uint32_t nBegin, int32_t nCount, uint64_t nFrame)
{
	if (nCount <= 0)
		return;
	void* pData = 0;
	//uprintf("fetch [%d-%d]\n", nBegin, nBegin + nCount);
	D3D12_RANGE Range = { sizeof(uint64_t)*nBegin, sizeof(uint64_t)*(nBegin+nCount) };
	S.GPU.pBuffer->Map(0, &Range, &pData);
	memcpy(&S.GPU.nResults[nBegin], nBegin + (uint64_t*)pData, nCount * sizeof(uint64_t));
	for (int i = 0; i < nCount; ++i)
	{
		S.GPU.nQueryFrames[i + nBegin] = nFrame;
	}
	S.GPU.pBuffer->Unmap(0, 0);
}
void MicroProfileGpuWaitFence(uint64_t nFence)
{
	uint64_t nCompletedFrame = S.GPU.pFence->GetCompletedValue();
	//while(nCompletedFrame < nPending)
	//while(0 < nPending - nCompletedFrame)
	while (0 <= (int64_t)(nFence - nCompletedFrame))
	{
		MICROPROFILE_SCOPEI("Microprofile", "gpu-wait", 0);
		Sleep(20);//todo: use event.
		nCompletedFrame = S.GPU.pFence->GetCompletedValue();
	}

}

void MicroProfileGpuFetchResults(uint64_t nFrame)
{
//	MP_ASSERT(S.GPU.nFrame > nFrame);
	uint64_t nPending = S.GPU.nPendingFrame;
	//while(nPending <= nFrame)
	//while(0 <= nFrame - nPending)
	while (0 <= (int64_t)(nFrame - nPending))
	{
		MicroProfileGpuWaitFence(nPending);

		uint32_t nInternal = nPending % MICROPROFILE_D3D_INTERNAL_DELAY;
		uint32_t nBegin = S.GPU.Frames[nInternal].nBegin;
		uint32_t nCount = S.GPU.Frames[nInternal].nCount;
		MicroProfileGpuFetchRange(nBegin, (nBegin + nCount) > MICROPROFILE_D3D_MAX_QUERIES ? MICROPROFILE_D3D_MAX_QUERIES - nBegin : nCount, nPending);
		MicroProfileGpuFetchRange(0, (nBegin + nCount) - MICROPROFILE_D3D_MAX_QUERIES, nPending);
		nPending = ++S.GPU.nPendingFrame;
		MP_ASSERT(S.GPU.nFrame > nPending);
	}

}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nIndex)
{
	uint32_t nFrame = nIndex >> 16;
	uint32_t nQueryIndex = nIndex & 0xffff;
	uint32_t lala = S.GPU.nQueryFrames[nQueryIndex];
	MP_ASSERT((0xffff & lala) == nFrame);
	//uprintf("read TS [%d <- %lld]\n", nQueryIndex, S.GPU.nResults[nQueryIndex]);
	return S.GPU.nResults[nQueryIndex];
}

uint64_t MicroProfileTicksPerSecondGpu()
{
	return S.GPU.nFrequency;
}

uint32_t MicroProfileGpuFlip(void* pContext)
{
	uint32_t nFrameIndex = S.GPU.nFrame % MICROPROFILE_D3D_INTERNAL_DELAY;
	uint32_t nCount = 0, nStart = 0;

	ID3D12GraphicsCommandList* pCommandList = S.GPU.Frames[nFrameIndex].pCommandList;
	ID3D12CommandAllocator* pCommandAllocator = S.GPU.Frames[nFrameIndex].pCommandAllocator;
	pCommandAllocator->Reset();
	pCommandList->Reset(pCommandAllocator, nullptr);


	uint32_t nFrameTimeStamp = MicroProfileGpuInsertTimeStamp(pCommandList);
	if (S.GPU.nFrameCount)
	{

		nCount = S.GPU.nFrameCount.exchange(0);
		nStart = S.GPU.nFrameStart;
		S.GPU.nFrameStart = (S.GPU.nFrameStart + nCount) % MICROPROFILE_D3D_MAX_QUERIES;
		uint32_t nEnd = MicroProfileMin(nStart + nCount, (uint32_t)MICROPROFILE_D3D_MAX_QUERIES);
		MP_ASSERT(nStart != nEnd);
		uint32_t nSize = nEnd - nStart;
		pCommandList->ResolveQueryData(S.GPU.pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nStart, nEnd - nStart, S.GPU.pBuffer, nStart * sizeof(int64_t));
		if (nStart + nCount > MICROPROFILE_D3D_MAX_QUERIES)
		{
			pCommandList->ResolveQueryData(S.GPU.pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, nEnd + nStart - MICROPROFILE_D3D_MAX_QUERIES, S.GPU.pBuffer, 0);
		}
	} 
	pCommandList->Close();
	ID3D12CommandList* pList = pCommandList;
	S.GPU.pCommandQueue->ExecuteCommandLists(1, &pList);
	//uprintf("EXECUTE %p\n", pCommandList);
	S.GPU.pCommandQueue->Signal(S.GPU.pFence, S.GPU.nFrame);
	S.GPU.Frames[nFrameIndex].nBegin = nStart;
	S.GPU.Frames[nFrameIndex].nCount = nCount;
	S.GPU.nFrame++;
	//fetch from earlier frames

	MicroProfileGpuFetchResults(S.GPU.nFrame - MICROPROFILE_GPU_FRAME_DELAY);
	return nFrameTimeStamp;
}
void MicroProfileGpuInitD3D12(void* pDevice_, void* pCommandQueue_)
{
	ID3D12Device* pDevice = (ID3D12Device*)pDevice_;
	ID3D12CommandQueue* pCommandQueue = (ID3D12CommandQueue*)pCommandQueue_;
	memset(&S.GPU, 0, sizeof(S.GPU));
	S.GPU.pDevice = pDevice;
	S.GPU.pCommandQueue = pCommandQueue;
	pDevice->SetStablePowerState(TRUE);
	pCommandQueue->GetTimestampFrequency((uint64_t*)&S.GPU.nFrequency);
	
	HRESULT hr;
	D3D12_QUERY_HEAP_DESC QHDesc = {};
	QHDesc.Count = MICROPROFILE_D3D_MAX_QUERIES;
	QHDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	hr = pDevice->CreateQueryHeap(&QHDesc, IID_PPV_ARGS(&S.GPU.pHeap));
	MP_ASSERT(hr == S_OK);
	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask = 1;
	HeapProperties.VisibleNodeMask = 1;
	HeapProperties.Type = D3D12_HEAP_TYPE_READBACK;

	const size_t nResourceSize = MICROPROFILE_D3D_MAX_QUERIES * 8;
	
	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Width = nResourceSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = pDevice->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,	
		nullptr, 
		IID_PPV_ARGS(&S.GPU.pBuffer));
	MP_ASSERT(hr == S_OK);

	S.GPU.nFrame = 0;
	S.GPU.nPendingFrame = 0;
	pDevice->CreateFence(S.GPU.nPendingFrame, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&S.GPU.pFence));

	for(MicroProfileD3D12Frame& Frame : S.GPU.Frames)
	{
		hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Frame.pCommandAllocator));
		MP_ASSERT(hr == S_OK);
		hr = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Frame.pCommandAllocator, nullptr, IID_PPV_ARGS(&Frame.pCommandList));
		MP_ASSERT(hr == S_OK);
		hr = Frame.pCommandList->Close();
		MP_ASSERT(hr == S_OK);
	}	
}

void MicroProfileGpuShutdown()
{
	MicroProfileGpuWaitFence(S.GPU.nFrame-1);
	S.GPU.pHeap->Release();
	S.GPU.pBuffer->Release();
	S.GPU.pFence->Release();
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	HRESULT hr = S.GPU.pCommandQueue->GetClockCalibration((uint64_t*)pOutGpu, (uint64_t*)pOutCPU);
	MP_ASSERT(hr == S_OK);
	return 1;
}
#elif MICROPROFILE_GPU_TIMERS_GL
void MicroProfileGpuInitGL()
{
	S.GPU.GLTimerPos = 0;
	glGenQueries(MICROPROFILE_GL_MAX_QUERIES, &S.GPU.GLTimers[0]);		
}

uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
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

int MicroProfileGetGpuTickReference(int64_t* pOutCpu, int64_t* pOutGpu)
{
	int64_t nGpuTimeStamp;
	glGetInteger64v(GL_TIMESTAMP, &nGpuTimeStamp);
	if(nGpuTimeStamp)
	{
		*pOutCpu = MP_TICK();
		*pOutGpu = nGpuTimeStamp;
		#if 0 //debug test if timestamp diverges
		static int64_t nTicksPerSecondCpu = MicroProfileTicksPerSecondCpu();
		static int64_t nTicksPerSecondGpu = MicroProfileTicksPerSecondGpu();
		static int64_t nGpuStart = 0;
		static int64_t nCpuStart = 0;
		if(!nCpuStart)
		{
			nCpuStart = *pOutCpu;
			nGpuStart = *pOutGpu;
		}
		static int nCountDown = 100;
		if(0 == nCountDown--)
		{
			int64_t nCurCpu = *pOutCpu;
			int64_t nCurGpu = *pOutGpu;
			double fDistanceCpu = (nCurCpu - nCpuStart) / (double)nTicksPerSecondCpu;
			double fDistanceGpu = (nCurGpu - nGpuStart) / (double)nTicksPerSecondGpu;

			char buf[254];
			snprintf(buf, sizeof(buf)-1,"Distance %f %f diff %f\n", fDistanceCpu, fDistanceGpu, fDistanceCpu-fDistanceGpu);
			OutputDebugString(buf);
			nCountDown = 100;
		}
		#endif
		return 1;
	}
	return 0;
}


#endif

#undef S

#ifdef _WIN32
#pragma warning(pop)
#endif





#endif
#endif
#ifdef MICROPROFILE_EMBED_HTML
#include "microprofile_html.h"
#endif
