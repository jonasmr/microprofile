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
// Resource Usage:
// 	MicroProfile uses a few large allocations.
//	- One global instance of struct MicroProfile
// 	- One Large Allocation per thread that its tracking (MICROPROFILE_PER_THREAD_BUFFER_SIZE, default 2mb)
//  - One Large Allocation per gpu buffer used (MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE, default 128k)
//	- one thread for sending
//  - if context switch state is enabled, another thread plus a 128k buffer for storing cswitch data.
//  - two small variable size buffers. should not be more than a few kb.
//		- A state buffer for receiving websocket packets
//		- A state buffer for loading/saving settings.
//  - one small allocation for receiving packets from the sender
//
// To change how microprofile allocates memory, define these macros when compiling microprofile implementation
// 	MICROPROFILE_ALLOC
// 	MICROPROFILE_REALLOC
// 	MICROPROFILE_FREE


#ifndef MICROPROFILE_ENABLED
#define MICROPROFILE_ENABLED 1
#endif

#ifndef MICROPROFILE_ONCE
#define MICROPROFILE_ONCE

#include <stdint.h>
#if defined(_WIN32) && _MSC_VER == 1700
#define PRIx64 "llx"
#define PRIu64 "llu"
#define PRId64 "lld"
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
#define MICROPROFILE_ENTER(var) do{}while(0)
#define MICROPROFILE_ENTER_TOKEN(var) do{}while(0)
#define MICROPROFILE_ENTERI(group, name, color) do{}while(0)
#define MICROPROFILE_LEAVE() do{}while(0)
#define MICROPROFILE_GPU_ENTER(var) do{}while(0)
#define MICROPROFILE_GPU_ENTER_TOKEN(token) do{}while(0)
#define MICROPROFILE_GPU_ENTERI(group, name, color) do{}while(0)
#define MICROPROFILE_GPU_LEAVE() do{}while(0)
#define MICROPROFILE_GPU_ENTER_L(Log, var) do{}while(0)
#define MICROPROFILE_GPU_ENTER_TOKEN_L(Log, token) do{}while(0)
#define MICROPROFILE_GPU_ENTERI_L(Log, name, color) do{}while(0)
#define MICROPROFILE_GPU_LEAVE_L(Log) do{}while(0)
#define MICROPROFILE_GPU_INIT_QUEUE(QueueName) do {} while(0)
#define MICROPROFILE_GPU_GET_QUEUE(QueueName) do {} while(0)
#define MICROPROFILE_GPU_BEGIN(pContext, pLog) do {} while(0)
#define MICROPROFILE_GPU_SET_CONTEXT(pContext, pLog) do {} while(0)
#define MICROPROFILE_GPU_END(pLog) 0
#define MICROPROFILE_GPU_SUBMIT(Queue, Work) do {} while(0)
#define MICROPROFILE_THREADLOGGPURESET(a) do{}while(0)
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
#define MICROPROFILE_COUNTER_CONFIG(name, type, limit, flags)
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
#define MicroProfileOnThreadExit() do{}while(0)
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
#define MicroProfileSetEnableAllGroups(b) do{} while(0)
#define MicroProfileEnableCategory(a) do{} while(0)
#define MicroProfileDisableCategory(a) do{} while(0)
#define MicroProfileGetEnableAllGroups() false
#define MicroProfileSetForceMetaCounters(a)
#define MicroProfileGetForceMetaCounters() 0
#define MicroProfileEnableMetaCounter(c) do{}while(0)
#define MicroProfileDisableMetaCounter(c) do{}while(0)
#define MicroProfileDumpFile(html,csv,spikecpu,spikegpu) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)
#define MicroProfileStartContextSwitchTrace() do{}while(0)
#define MicroProfileDumpFile(html,csv,cpu, gpu) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)
#define MicroProfileGpuInsertTimeStamp(a) 1
#define MicroProfileGpuGetTimeStamp(a) 0
#define MicroProfileTicksPerSecondGpu() 1
#define MicroProfileGetGpuTickReference(a,b) 0
#define MicroProfileGpuInitD3D12(pDevice,nNodeCount,pCommandQueue) do{}while(0)
#define MicroProfileGpuInitD3D11(pDevice,pDeviceContext) do{}while(0)
#define MicroProfileGpuShutdown() do{}while(0)
#define MicroProfileGpuInitGL() do{}while(0)
#define MicroProfileSetCurrentNodeD3D12(nNode) do{}while(0)
#define MicroProfilePlatformMarkersGetEnabled() 0
#define MicroProfilePlatformMarkersSetEnabled(bEnabled) do{}while(0)
#define MicroProfileTickToMsMultiplierCpu() 1.f
#define MicroProfileTickToMsMultiplierGpu() 0.f
#define MicroProfileTicksPerSecondCpu() 1
#define MicroProfileTick() 0
#define MicroProfileEnabled() 0

#else

#include <stdint.h>

#ifndef MICROPROFILE_API
#define MICROPROFILE_API
#endif

#ifdef MICROPROFILE_PS4
#include "microprofile_ps4.h"
#endif
#ifdef MICROPROFILE_XBOXONE
#include "microprofile_xboxone.h"
#endif

#ifdef _WIN32
typedef uint32_t MicroProfileThreadIdType;
#else
#ifdef MICROPROFILE_THREADID_SIZE_4BYTE
typedef uint32_t MicroProfileThreadIdType;
#elif MICROPROFILE_THREADID_SIZE_8BYTE
typedef uint64_t MicroProfileThreadIdType;
#else
typedef uint64_t MicroProfileThreadIdType;
#endif
#endif


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
#define MICROPROFILE_ENTER(var) MicroProfileEnter(g_mp_##var)
#define MICROPROFILE_ENTER_TOKEN(var) MicroProfileEnter(token)
#define MICROPROFILE_ENTERI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeCpu);} MicroProfileEnter(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_LEAVE() MicroProfileLeave()
#define MICROPROFILE_GPU_ENTER(var) MicroProfileEnterGpu(g_mp_##var, MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_GPU_ENTER_TOKEN(token) MicroProfileEnterGpu(token, MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_GPU_ENTERI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeGpu);} MicroProfileEnterGpu(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_GPU_LEAVE() MicroProfileLeaveGpu(MicroProfileGetGlobaGpuThreadLog())
#define MICROPROFILE_GPU_ENTER_L(Log, var) MicroProfileEnterGpu(g_mp_##var, Log)
#define MICROPROFILE_GPU_ENTER_TOKEN_L(Log, token) MicroProfileEnterGpu(token, Log)
#define MICROPROFILE_GPU_ENTERI_L(Log, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeGpu);} MicroProfileEnterGpu(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), Log)
#define MICROPROFILE_GPU_LEAVE_L(Log) MicroProfileLeaveGpu(Log)
#define MICROPROFILE_GPU_INIT_QUEUE(QueueName) MicroProfileInitGpuQueue(QueueName)
#define MICROPROFILE_GPU_GET_QUEUE(QueueName) MicroProfileGetGpuQueue(QueueName)
#define MICROPROFILE_GPU_BEGIN(pContext, pLog) MicroProfileGpuBegin(pContext, pLog)
#define MICROPROFILE_GPU_SET_CONTEXT(pContext, pLog) MicroProfileGpuSetContext(pContext, pLog)
#define MICROPROFILE_GPU_END(pLog) MicroProfileGpuEnd(pLog)
#define MICROPROFILE_GPU_SUBMIT(Queue, Work) MicroProfileGpuSubmit(Queue, Work)
#define MICROPROFILE_THREADLOGGPURESET(a) MicroProfileThreadLogGpuReset(a)
#define MICROPROFILE_META_CPU(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeCpu)
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
#define MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(var, count) MicroProfileLocalCounterAddAtomic(&g_mp_local_counter##var, (count))
#define MICROPROFILE_COUNTER_LOCAL_SUB_ATOMIC(var, count) MicroProfileLocalCounterAddAtomic(&g_mp_local_counter##var, -(int64_t)(count))
#define MICROPROFILE_COUNTER_LOCAL_SET_ATOMIC(var, count) MicroProfileLocalCounterSetAtomic(&g_mp_local_counter##var, count)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD_ATOMIC(var) MicroProfileCounterAdd(g_mp_counter_token##var, MicroProfileLocalCounterSetAtomic(&g_mp_local_counter##var, 0))
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_SET_ATOMIC(var) MicroProfileCounterSet(g_mp_counter_token##var, MicroProfileLocalCounterSetAtomic(&g_mp_local_counter##var, 0))
#define MICROPROFILE_FORCEENABLECPUGROUP(s) MicroProfileForceEnableGroup(s, MicroProfileTokenTypeCpu)
#define MICROPROFILE_FORCEDISABLECPUGROUP(s) MicroProfileForceDisableGroup(s, MicroProfileTokenTypeCpu)
#define MICROPROFILE_FORCEENABLEGPUGROUP(s) MicroProfileForceEnableGroup(s, MicroProfileTokenTypeGpu)
#define MICROPROFILE_FORCEDISABLEGPUGROUP(s) MicroProfileForceDisableGroup(s, MicroProfileTokenTypeGpu)
#define MICROPROFILE_CONDITIONAL(expr) expr

#ifndef MICROPROFILE_PLATFORM_MARKERS
#define MICROPROFILE_PLATFORM_MARKERS 0
#endif

#if MICROPROFILE_PLATFORM_MARKERS
#define MICROPROFILE_PLATFORM_MARKERS_ENABLED S.nPlatformMarkersEnabled
#define MICROPROFILE_PLATFORM_MARKER_BEGIN(color,marker) MicroProfilePlatformMarkerBegin(color,marker)
#define MICROPROFILE_PLATFORM_MARKER_END() MicroProfilePlatformMarkerEnd()
#else
#define MICROPROFILE_PLATFORM_MARKER_BEGIN(color,marker) do{(void)color;(void)marker;}while(0)
#define MICROPROFILE_PLATFORM_MARKER_END() do{}while(0)
#define MICROPROFILE_PLATFORM_MARKERS_ENABLED 0
#endif




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




typedef enum MicroProfileTokenType_t
{
	MicroProfileTokenTypeCpu,
	MicroProfileTokenTypeGpu,
} MicroProfileTokenType;

struct MicroProfile;
struct MicroProfileThreadLogGpu;
struct MicroProfileScopeStateC;

#ifdef __cplusplus
#include <atomic>
extern "C" {
#endif

#define MICROPROFILE_INVALID_TOKEN (uint64_t)-1

MICROPROFILE_API void MicroProfileInit();
MICROPROFILE_API void MicroProfileShutdown();
MICROPROFILE_API MicroProfileToken MicroProfileFindToken(const char* sGroup, const char* sName);
MICROPROFILE_API MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token);
MICROPROFILE_API void MicroProfileGetTokenC(MicroProfileToken* pToken, const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token);
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
MICROPROFILE_API uint64_t MicroProfileEnterInternal(MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileLeaveInternal(MicroProfileToken nToken, uint64_t nTick);
MICROPROFILE_API void MicroProfileEnter(MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileLeave();
MICROPROFILE_API void MicroProfileEnterGpu(MicroProfileToken nToken, struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileLeaveGpu();
MICROPROFILE_API uint64_t MicroProfileGpuEnterInternal(struct MicroProfileThreadLogGpu* pLog, MicroProfileToken nToken);
MICROPROFILE_API void MicroProfileGpuLeaveInternal(struct MicroProfileThreadLogGpu* pLog, MicroProfileToken nToken, uint64_t nTick);
MICROPROFILE_API void MicroProfileGpuBegin(void* pContext, struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileGpuSetContext(void* pContext, struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API uint64_t MicroProfileGpuEnd(struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API struct MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAlloc();
MICROPROFILE_API void MicroProfileThreadLogGpuFree(struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileThreadLogGpuReset(struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API void MicroProfileGpuSubmit(int nQueue, uint64_t nWork);
MICROPROFILE_API int MicroProfileInitGpuQueue(const char* pQueueName);
MICROPROFILE_API int MicroProfileGetGpuQueue(const char* pQueueName);
MICROPROFILE_API void MicroProfileFlip(void* pGpuContext); //! call once per frame.
MICROPROFILE_API void MicroProfileToggleFrozen();
MICROPROFILE_API int MicroProfileIsFrozen();
MICROPROFILE_API int MicroProfileEnabled();
MICROPROFILE_API void MicroProfileForceEnableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API void MicroProfileForceDisableGroup(const char* pGroup, MicroProfileTokenType Type);
MICROPROFILE_API float MicroProfileGetTime(const char* pGroup, const char* pName);
MICROPROFILE_API int MicroProfilePlatformMarkersGetEnabled(); //enable platform markers. disables microprofile markers
MICROPROFILE_API void MicroProfilePlatformMarkersSetEnabled(int bEnabled); //enable platform markers. disables microprofile markers
MICROPROFILE_API void MicroProfileContextSwitchSearch(uint32_t* pContextSwitchStart, uint32_t* pContextSwitchEnd, uint64_t nBaseTicksCpu, uint64_t nBaseTicksEndCpu);
MICROPROFILE_API void MicroProfileOnThreadCreate(const char* pThreadName); //should be called from newly created threads
MICROPROFILE_API void MicroProfileOnThreadExit(); //call on exit to reuse log
MICROPROFILE_API void MicroProfileInitThreadLog();
MICROPROFILE_API void MicroProfileSetEnableAllGroups(int bEnable); 
MICROPROFILE_API void MicroProfileEnableCategory(const char* pCategory); 
MICROPROFILE_API void MicroProfileDisableCategory(const char* pCategory); 
MICROPROFILE_API int MicroProfileGetEnableAllGroups();
MICROPROFILE_API void MicroProfileSetForceMetaCounters(int bEnable); 
MICROPROFILE_API int MicroProfileGetForceMetaCounters();
MICROPROFILE_API void MicroProfileEnableMetaCounter(const char* pMet);
MICROPROFILE_API void MicroProfileDisableMetaCounter(const char* pMet);
MICROPROFILE_API void MicroProfileSetAggregateFrames(int frames);
MICROPROFILE_API int MicroProfileGetAggregateFrames();
MICROPROFILE_API int MicroProfileGetCurrentAggregateFrames();
MICROPROFILE_API struct MicroProfile* MicroProfileGet();
MICROPROFILE_API void MicroProfileGetRange(uint32_t nPut, uint32_t nGet, uint32_t nRange[2][2]);
MICROPROFILE_API void MicroProfileStartContextSwitchTrace();
MICROPROFILE_API void MicroProfileStopContextSwitchTrace();
MICROPROFILE_API int MicroProfileIsLocalThread(uint32_t nThreadId);
MICROPROFILE_API int MicroProfileFormatCounter(int eFormat, int64_t nCounter, char* pOut, uint32_t nBufferSize);
MICROPROFILE_API struct MicroProfileThreadLogGpu* MicroProfileGetGlobaGpuThreadLog();
MICROPROFILE_API int MicroProfileGetGlobaGpuQueue();
MICROPROFILE_API void MicroProfileRegisterGroup(const char* pGroup, const char* pCategory, uint32_t nColor);
#if MICROPROFILE_PLATFORM_MARKERS
MICROPROFILE_API void MicroProfilePlatformMarkerBegin(uint32_t nColor, const char * pMarker); //not implemented by microprofile.
MICROPROFILE_API void MicroProfilePlatformMarkerEnd();//not implemented by microprofile.
#endif

MICROPROFILE_API float MicroProfileTickToMsMultiplierCpu();
MICROPROFILE_API float MicroProfileTickToMsMultiplierGpu();
MICROPROFILE_API int64_t MicroProfileTicksPerSecondCpu();
MICROPROFILE_API uint64_t MicroProfileTick();

#ifdef __cplusplus
MICROPROFILE_API void MicroProfileLocalCounterAddAtomic(std::atomic<int64_t>* pCounter, int64_t nCount);
MICROPROFILE_API int64_t MicroProfileLocalCounterSetAtomic(std::atomic<int64_t>* pCounter, int64_t nCount);

}
#endif

#ifdef __cplusplus
struct MicroProfileThreadInfo
{
	//3 first members are used to sort. dont reorder
	uint32_t nIsLocal;
	MicroProfileThreadIdType pid;
	MicroProfileThreadIdType tid;
	//3 first members are used to sort. dont reorder

	const char* pThreadModule;
	const char* pProcessModule;
	MicroProfileThreadInfo()
		:nIsLocal(0)
		,pid(0)
		,tid(0)
		,pThreadModule("?")
		,pProcessModule("?")
	{
	}
	MicroProfileThreadInfo(uint32_t ThreadId, uint32_t ProcessId, uint32_t nIsLocal)
		:nIsLocal(nIsLocal)
		,pid(ProcessId)
		,tid(ThreadId)
		,pThreadModule("?")
		,pProcessModule("?")
	{
	}
	~MicroProfileThreadInfo() {}
};
MICROPROFILE_API MicroProfileThreadInfo MicroProfileGetThreadInfo(MicroProfileThreadIdType nThreadId);
MICROPROFILE_API uint32_t MicroProfileGetThreadInfoArray(MicroProfileThreadInfo** pThreadArray);
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

#if defined(MICROPROFILE_GPU_TIMERS_D3D12)
MICROPROFILE_API void MicroProfileGpuInitD3D12(void* pDevice,  uint32_t nNodeCount, void** pCommandQueues);
MICROPROFILE_API void MicroProfileGpuShutdown();
MICROPROFILE_API void MicroProfileSetCurrentNodeD3D12(uint32_t nNode);
#endif

MICROPROFILE_API void MicroProfileDumpFile(const char* pHtml, const char* pCsv, float fCpuSpike, float fGpuSpike);
MICROPROFILE_API uint32_t MicroProfileWebServerPort();

#if MICROPROFILE_GPU_TIMERS
MICROPROFILE_API uint32_t MicroProfileGpuInsertTimeStamp(void* pContext);
MICROPROFILE_API uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey);
MICROPROFILE_API uint64_t MicroProfileTicksPerSecondGpu();
MICROPROFILE_API int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu);
MICROPROFILE_API uint32_t MicroProfileGpuFlip(void*);
MICROPROFILE_API void MicroProfileGpuShutdown();
#else
#define MicroProfileGpuInsertTimeStamp(a) 1
#define MicroProfileGpuGetTimeStamp(a) 0
#define MicroProfileTicksPerSecondGpu() 1
#define MicroProfileGetGpuTickReference(a,b) 0
#define MicroProfileGpuFlip(a) 0
#define MicroProfileGpuShutdown() do{}while(0)

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
MICROPROFILE_API const char* MicroProfileGetThreadName(char* pzName);
#else
#define MicroProfileGetThreadName(a) "<implement MicroProfileGetThreadName to get threadnames>"
#endif



#ifdef __cplusplus
}
#endif



struct MicroProfileScopeStateC
{
	MicroProfileToken Token;
	int64_t nTick;
};

#ifdef __cplusplus
struct MicroProfileScopeHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileEnterInternal(nToken);
	}
	~MicroProfileScopeHandler()
	{		
		MicroProfileLeaveInternal(nToken, nTick);
	}
};

struct MicroProfileScopeGpuHandler
{
	MicroProfileToken nToken;
	MicroProfileThreadLogGpu* pLog;
	uint64_t nTick;
	MicroProfileScopeGpuHandler(MicroProfileToken Token, MicroProfileThreadLogGpu* pLog):nToken(Token), pLog(pLog)
	{
		nTick = MicroProfileGpuEnterInternal(pLog, nToken);
	}
	~MicroProfileScopeGpuHandler()
	{
		MicroProfileGpuLeaveInternal(pLog, nToken, nTick);
	}
};
#endif //__cplusplus
#endif //enabled

enum MicroProfileDrawMask
{
	MP_DRAW_OFF = 0x0,
	MP_DRAW_BARS = 0x1,
	MP_DRAW_DETAILED = 0x2,
	MP_DRAW_COUNTERS = 0x3,
	MP_DRAW_HIDDEN = 0x4,
	MP_DRAW_SIZE = 0x5,
};

enum MicroProfileDrawBarsMask
{
	MP_DRAW_TIMERS = 0x1,
	MP_DRAW_AVERAGE = 0x2,
	MP_DRAW_MAX = 0x4,
	MP_DRAW_MIN = 0x8,
	MP_DRAW_CALL_COUNT = 0x10,
	MP_DRAW_TIMERS_EXCLUSIVE = 0x20,
	MP_DRAW_AVERAGE_EXCLUSIVE = 0x40,
	MP_DRAW_MAX_EXCLUSIVE = 0x80,
	MP_DRAW_META_FIRST = 0x100,
	MP_DRAW_ALL = 0xffffffff,

};

enum MicroProfileCounterFormat
{
	MICROPROFILE_COUNTER_FORMAT_DEFAULT = 0,
	MICROPROFILE_COUNTER_FORMAT_BYTES = 1,
};

enum MicroProfileCounterFlags
{
	MICROPROFILE_COUNTER_FLAG_NONE = 0,
	MICROPROFILE_COUNTER_FLAG_DETAILED = 0x1,
	MICROPROFILE_COUNTER_FLAG_DETAILED_GRAPH = 0x2,
	//internal:
	MICROPROFILE_COUNTER_FLAG_INTERNAL_MASK = ~0x3,
	MICROPROFILE_COUNTER_FLAG_HAS_LIMIT = 0x4,
	MICROPROFILE_COUNTER_FLAG_CLOSED = 0x8,
	MICROPROFILE_COUNTER_FLAG_MANUAL_SWAP = 0x10,
	MICROPROFILE_COUNTER_FLAG_LEAF = 0x20,
};

#endif //once

//////////////////////////////////////////////////////////////////////////
// IMPL WALL
//////////////////////////////////////////////////////////////////////////
#if defined(MICROPROFILE_IMPL) && MICROPROFILE_ENABLED



#include <thread>
#include <mutex>
#include <atomic>


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
#define MICROPROFILE_WEBSOCKET_BUFFER_SIZE (10<<10)
#define MICROPROFILE_INVALID_TICK ((uint64_t)-1)
#define MICROPROFILE_GROUP_MASK_ALL 0xffffffffffff

#define MP_LOG_TICK_MASK  0x0000ffffffffffff
#define MP_LOG_INDEX_MASK 0x3fff000000000000
#define MP_LOG_BEGIN_MASK 0xc000000000000000
#define MP_LOG_GPU_EXTRA 0x3
#define MP_LOG_META 0x2
#define MP_LOG_ENTER 0x1
#define MP_LOG_LEAVE 0x0

enum
{
	MICROPROFILE_WEBSOCKET_DIRTY_MENU,
	MICROPROFILE_WEBSOCKET_DIRTY_ENABLED,
};


#ifndef MICROPROFILE_ALLOC //redefine all if overriding
#define MICROPROFILE_ALLOC(nSize, nAlign) MicroProfileAllocAligned(nSize, nAlign);
#define MICROPROFILE_REALLOC(p, s) realloc(p, s)
#define MICROPROFILE_FREE(p) free(p)
#endif

#define MICROPROFILE_ALLOC_OBJECT(T) (T*)MICROPROFILE_ALLOC(sizeof(T), alignof(T))


//from http://fugal.net/vim/rgbtxt.html
#define MP_SNOW 0xfffafa
#define MP_GHOSTWHITE 0xf8f8ff
#define MP_WHITESMOKE 0xf5f5f5
#define MP_GAINSBORO 0xdcdcdc
#define MP_FLORALWHITE 0xfffaf0
#define MP_OLDLACE 0xfdf5e6
#define MP_LINEN 0xfaf0e6
#define MP_ANTIQUEWHITE 0xfaebd7
#define MP_PAPAYAWHIP 0xffefd5
#define MP_BLANCHEDALMOND 0xffebcd
#define MP_BISQUE 0xffe4c4
#define MP_PEACHPUFF 0xffdab9
#define MP_NAVAJOWHITE 0xffdead
#define MP_MOCCASIN 0xffe4b5
#define MP_CORNSILK 0xfff8dc
#define MP_IVORY 0xfffff0
#define MP_LEMONCHIFFON 0xfffacd
#define MP_SEASHELL 0xfff5ee
#define MP_HONEYDEW 0xf0fff0
#define MP_MINTCREAM 0xf5fffa
#define MP_AZURE 0xf0ffff
#define MP_ALICEBLUE 0xf0f8ff
#define MP_LAVENDER 0xe6e6fa
#define MP_LAVENDERBLUSH 0xfff0f5
#define MP_MISTYROSE 0xffe4e1
#define MP_WHITE 0xffffff
#define MP_BLACK 0x000000
#define MP_DARKSLATEGRAY 0x2f4f4f
#define MP_DARKSLATEGREY 0x2f4f4f
#define MP_DIMGRAY 0x696969
#define MP_DIMGREY 0x696969
#define MP_SLATEGRAY 0x708090
#define MP_SLATEGREY 0x708090
#define MP_LIGHTSLATEGRAY 0x778899
#define MP_LIGHTSLATEGREY 0x778899
#define MP_GRAY 0xbebebe
#define MP_GREY 0xbebebe
#define MP_LIGHTGREY 0xd3d3d3
#define MP_LIGHTGRAY 0xd3d3d3
#define MP_MIDNIGHTBLUE 0x191970
#define MP_NAVY 0x000080
#define MP_NAVYBLUE 0x000080
#define MP_CORNFLOWERBLUE 0x6495ed
#define MP_DARKSLATEBLUE 0x483d8b
#define MP_SLATEBLUE 0x6a5acd
#define MP_MEDIUMSLATEBLUE 0x7b68ee
#define MP_LIGHTSLATEBLUE 0x8470ff
#define MP_MEDIUMBLUE 0x0000cd
#define MP_ROYALBLUE 0x4169e1
#define MP_BLUE 0x0000ff
#define MP_DODGERBLUE 0x1e90ff
#define MP_DEEPSKYBLUE 0x00bfff
#define MP_SKYBLUE 0x87ceeb
#define MP_LIGHTSKYBLUE 0x87cefa
#define MP_STEELBLUE 0x4682b4
#define MP_LIGHTSTEELBLUE 0xb0c4de
#define MP_LIGHTBLUE 0xadd8e6
#define MP_POWDERBLUE 0xb0e0e6
#define MP_PALETURQUOISE 0xafeeee
#define MP_DARKTURQUOISE 0x00ced1
#define MP_MEDIUMTURQUOISE 0x48d1cc
#define MP_TURQUOISE 0x40e0d0
#define MP_CYAN 0x00ffff
#define MP_LIGHTCYAN 0xe0ffff
#define MP_CADETBLUE 0x5f9ea0
#define MP_MEDIUMAQUAMARINE 0x66cdaa
#define MP_AQUAMARINE 0x7fffd4
#define MP_DARKGREEN 0x006400
#define MP_DARKOLIVEGREEN 0x556b2f
#define MP_DARKSEAGREEN 0x8fbc8f
#define MP_SEAGREEN 0x2e8b57
#define MP_MEDIUMSEAGREEN 0x3cb371
#define MP_LIGHTSEAGREEN 0x20b2aa
#define MP_PALEGREEN 0x98fb98
#define MP_SPRINGGREEN 0x00ff7f
#define MP_LAWNGREEN 0x7cfc00
#define MP_GREEN 0x00ff00
#define MP_CHARTREUSE 0x7fff00
#define MP_MEDIUMSPRINGGREEN 0x00fa9a
#define MP_GREENYELLOW 0xadff2f
#define MP_LIMEGREEN 0x32cd32
#define MP_YELLOWGREEN 0x9acd32
#define MP_FORESTGREEN 0x228b22
#define MP_OLIVEDRAB 0x6b8e23
#define MP_DARKKHAKI 0xbdb76b
#define MP_KHAKI 0xf0e68c
#define MP_PALEGOLDENROD 0xeee8aa
#define MP_LIGHTGOLDENRODYELLOW 0xfafad2
#define MP_LIGHTYELLOW 0xffffe0
#define MP_YELLOW 0xffff00
#define MP_GOLD 0xffd700
#define MP_LIGHTGOLDENROD 0xeedd82
#define MP_GOLDENROD 0xdaa520
#define MP_DARKGOLDENROD 0xb8860b
#define MP_ROSYBROWN 0xbc8f8f
#define MP_INDIANRED 0xcd5c5c
#define MP_SADDLEBROWN 0x8b4513
#define MP_SIENNA 0xa0522d
#define MP_PERU 0xcd853f
#define MP_BURLYWOOD 0xdeb887
#define MP_BEIGE 0xf5f5dc
#define MP_WHEAT 0xf5deb3
#define MP_SANDYBROWN 0xf4a460
#define MP_TAN 0xd2b48c
#define MP_CHOCOLATE 0xd2691e
#define MP_FIREBRICK 0xb22222
#define MP_BROWN 0xa52a2a
#define MP_DARKSALMON 0xe9967a
#define MP_SALMON 0xfa8072
#define MP_LIGHTSALMON 0xffa07a
#define MP_ORANGE 0xffa500
#define MP_DARKORANGE 0xff8c00
#define MP_CORAL 0xff7f50
#define MP_LIGHTCORAL 0xf08080
#define MP_TOMATO 0xff6347
#define MP_ORANGERED 0xff4500
#define MP_RED 0xff0000
#define MP_HOTPINK 0xff69b4
#define MP_DEEPPINK 0xff1493
#define MP_PINK 0xffc0cb
#define MP_LIGHTPINK 0xffb6c1
#define MP_PALEVIOLETRED 0xdb7093
#define MP_MAROON 0xb03060
#define MP_MEDIUMVIOLETRED 0xc71585
#define MP_VIOLETRED 0xd02090
#define MP_MAGENTA 0xff00ff
#define MP_VIOLET 0xee82ee
#define MP_PLUM 0xdda0dd
#define MP_ORCHID 0xda70d6
#define MP_MEDIUMORCHID 0xba55d3
#define MP_DARKORCHID 0x9932cc
#define MP_DARKVIOLET 0x9400d3
#define MP_BLUEVIOLET 0x8a2be2
#define MP_PURPLE 0xa020f0
#define MP_MEDIUMPURPLE 0x9370db
#define MP_THISTLE 0xd8bfd8
#define MP_SNOW1 0xfffafa
#define MP_SNOW2 0xeee9e9
#define MP_SNOW3 0xcdc9c9
#define MP_SNOW4 0x8b8989
#define MP_SEASHELL1 0xfff5ee
#define MP_SEASHELL2 0xeee5de
#define MP_SEASHELL3 0xcdc5bf
#define MP_SEASHELL4 0x8b8682
#define MP_ANTIQUEWHITE1 0xffefdb
#define MP_ANTIQUEWHITE2 0xeedfcc
#define MP_ANTIQUEWHITE3 0xcdc0b0
#define MP_ANTIQUEWHITE4 0x8b8378
#define MP_BISQUE1 0xffe4c4
#define MP_BISQUE2 0xeed5b7
#define MP_BISQUE3 0xcdb79e
#define MP_BISQUE4 0x8b7d6b
#define MP_PEACHPUFF1 0xffdab9
#define MP_PEACHPUFF2 0xeecbad
#define MP_PEACHPUFF3 0xcdaf95
#define MP_PEACHPUFF4 0x8b7765
#define MP_NAVAJOWHITE1 0xffdead
#define MP_NAVAJOWHITE2 0xeecfa1
#define MP_NAVAJOWHITE3 0xcdb38b
#define MP_NAVAJOWHITE4 0x8b795e
#define MP_LEMONCHIFFON1 0xfffacd
#define MP_LEMONCHIFFON2 0xeee9bf
#define MP_LEMONCHIFFON3 0xcdc9a5
#define MP_LEMONCHIFFON4 0x8b8970
#define MP_CORNSILK1 0xfff8dc
#define MP_CORNSILK2 0xeee8cd
#define MP_CORNSILK3 0xcdc8b1
#define MP_CORNSILK4 0x8b8878
#define MP_IVORY1 0xfffff0
#define MP_IVORY2 0xeeeee0
#define MP_IVORY3 0xcdcdc1
#define MP_IVORY4 0x8b8b83
#define MP_HONEYDEW1 0xf0fff0
#define MP_HONEYDEW2 0xe0eee0
#define MP_HONEYDEW3 0xc1cdc1
#define MP_HONEYDEW4 0x838b83
#define MP_LAVENDERBLUSH1 0xfff0f5
#define MP_LAVENDERBLUSH2 0xeee0e5
#define MP_LAVENDERBLUSH3 0xcdc1c5
#define MP_LAVENDERBLUSH4 0x8b8386
#define MP_MISTYROSE1 0xffe4e1
#define MP_MISTYROSE2 0xeed5d2
#define MP_MISTYROSE3 0xcdb7b5
#define MP_MISTYROSE4 0x8b7d7b
#define MP_AZURE1 0xf0ffff
#define MP_AZURE2 0xe0eeee
#define MP_AZURE3 0xc1cdcd
#define MP_AZURE4 0x838b8b
#define MP_SLATEBLUE1 0x836fff
#define MP_SLATEBLUE2 0x7a67ee
#define MP_SLATEBLUE3 0x6959cd
#define MP_SLATEBLUE4 0x473c8b
#define MP_ROYALBLUE1 0x4876ff
#define MP_ROYALBLUE2 0x436eee
#define MP_ROYALBLUE3 0x3a5fcd
#define MP_ROYALBLUE4 0x27408b
#define MP_BLUE1 0x0000ff
#define MP_BLUE2 0x0000ee
#define MP_BLUE3 0x0000cd
#define MP_BLUE4 0x00008b
#define MP_DODGERBLUE1 0x1e90ff
#define MP_DODGERBLUE2 0x1c86ee
#define MP_DODGERBLUE3 0x1874cd
#define MP_DODGERBLUE4 0x104e8b
#define MP_STEELBLUE1 0x63b8ff
#define MP_STEELBLUE2 0x5cacee
#define MP_STEELBLUE3 0x4f94cd
#define MP_STEELBLUE4 0x36648b
#define MP_DEEPSKYBLUE1 0x00bfff
#define MP_DEEPSKYBLUE2 0x00b2ee
#define MP_DEEPSKYBLUE3 0x009acd
#define MP_DEEPSKYBLUE4 0x00688b
#define MP_SKYBLUE1 0x87ceff
#define MP_SKYBLUE2 0x7ec0ee
#define MP_SKYBLUE3 0x6ca6cd
#define MP_SKYBLUE4 0x4a708b
#define MP_LIGHTSKYBLUE1 0xb0e2ff
#define MP_LIGHTSKYBLUE2 0xa4d3ee
#define MP_LIGHTSKYBLUE3 0x8db6cd
#define MP_LIGHTSKYBLUE4 0x607b8b
#define MP_SLATEGRAY1 0xc6e2ff
#define MP_SLATEGRAY2 0xb9d3ee
#define MP_SLATEGRAY3 0x9fb6cd
#define MP_SLATEGRAY4 0x6c7b8b
#define MP_LIGHTSTEELBLUE1 0xcae1ff
#define MP_LIGHTSTEELBLUE2 0xbcd2ee
#define MP_LIGHTSTEELBLUE3 0xa2b5cd
#define MP_LIGHTSTEELBLUE4 0x6e7b8b
#define MP_LIGHTBLUE1 0xbfefff
#define MP_LIGHTBLUE2 0xb2dfee
#define MP_LIGHTBLUE3 0x9ac0cd
#define MP_LIGHTBLUE4 0x68838b
#define MP_LIGHTCYAN1 0xe0ffff
#define MP_LIGHTCYAN2 0xd1eeee
#define MP_LIGHTCYAN3 0xb4cdcd
#define MP_LIGHTCYAN4 0x7a8b8b
#define MP_PALETURQUOISE1 0xbbffff
#define MP_PALETURQUOISE2 0xaeeeee
#define MP_PALETURQUOISE3 0x96cdcd
#define MP_PALETURQUOISE4 0x668b8b
#define MP_CADETBLUE1 0x98f5ff
#define MP_CADETBLUE2 0x8ee5ee
#define MP_CADETBLUE3 0x7ac5cd
#define MP_CADETBLUE4 0x53868b
#define MP_TURQUOISE1 0x00f5ff
#define MP_TURQUOISE2 0x00e5ee
#define MP_TURQUOISE3 0x00c5cd
#define MP_TURQUOISE4 0x00868b
#define MP_CYAN1 0x00ffff
#define MP_CYAN2 0x00eeee
#define MP_CYAN3 0x00cdcd
#define MP_CYAN4 0x008b8b
#define MP_DARKSLATEGRAY1 0x97ffff
#define MP_DARKSLATEGRAY2 0x8deeee
#define MP_DARKSLATEGRAY3 0x79cdcd
#define MP_DARKSLATEGRAY4 0x528b8b
#define MP_AQUAMARINE1 0x7fffd4
#define MP_AQUAMARINE2 0x76eec6
#define MP_AQUAMARINE3 0x66cdaa
#define MP_AQUAMARINE4 0x458b74
#define MP_DARKSEAGREEN1 0xc1ffc1
#define MP_DARKSEAGREEN2 0xb4eeb4
#define MP_DARKSEAGREEN3 0x9bcd9b
#define MP_DARKSEAGREEN4 0x698b69
#define MP_SEAGREEN1 0x54ff9f
#define MP_SEAGREEN2 0x4eee94
#define MP_SEAGREEN3 0x43cd80
#define MP_SEAGREEN4 0x2e8b57
#define MP_PALEGREEN1 0x9aff9a
#define MP_PALEGREEN2 0x90ee90
#define MP_PALEGREEN3 0x7ccd7c
#define MP_PALEGREEN4 0x548b54
#define MP_SPRINGGREEN1 0x00ff7f
#define MP_SPRINGGREEN2 0x00ee76
#define MP_SPRINGGREEN3 0x00cd66
#define MP_SPRINGGREEN4 0x008b45
#define MP_GREEN1 0x00ff00
#define MP_GREEN2 0x00ee00
#define MP_GREEN3 0x00cd00
#define MP_GREEN4 0x008b00
#define MP_CHARTREUSE1 0x7fff00
#define MP_CHARTREUSE2 0x76ee00
#define MP_CHARTREUSE3 0x66cd00
#define MP_CHARTREUSE4 0x458b00
#define MP_OLIVEDRAB1 0xc0ff3e
#define MP_OLIVEDRAB2 0xb3ee3a
#define MP_OLIVEDRAB3 0x9acd32
#define MP_OLIVEDRAB4 0x698b22
#define MP_DARKOLIVEGREEN1 0xcaff70
#define MP_DARKOLIVEGREEN2 0xbcee68
#define MP_DARKOLIVEGREEN3 0xa2cd5a
#define MP_DARKOLIVEGREEN4 0x6e8b3d
#define MP_KHAKI1 0xfff68f
#define MP_KHAKI2 0xeee685
#define MP_KHAKI3 0xcdc673
#define MP_KHAKI4 0x8b864e
#define MP_LIGHTGOLDENROD1 0xffec8b
#define MP_LIGHTGOLDENROD2 0xeedc82
#define MP_LIGHTGOLDENROD3 0xcdbe70
#define MP_LIGHTGOLDENROD4 0x8b814c
#define MP_LIGHTYELLOW1 0xffffe0
#define MP_LIGHTYELLOW2 0xeeeed1
#define MP_LIGHTYELLOW3 0xcdcdb4
#define MP_LIGHTYELLOW4 0x8b8b7a
#define MP_YELLOW1 0xffff00
#define MP_YELLOW2 0xeeee00
#define MP_YELLOW3 0xcdcd00
#define MP_YELLOW4 0x8b8b00
#define MP_GOLD1 0xffd700
#define MP_GOLD2 0xeec900
#define MP_GOLD3 0xcdad00
#define MP_GOLD4 0x8b7500
#define MP_GOLDENROD1 0xffc125
#define MP_GOLDENROD2 0xeeb422
#define MP_GOLDENROD3 0xcd9b1d
#define MP_GOLDENROD4 0x8b6914
#define MP_DARKGOLDENROD1 0xffb90f
#define MP_DARKGOLDENROD2 0xeead0e
#define MP_DARKGOLDENROD3 0xcd950c
#define MP_DARKGOLDENROD4 0x8b6508
#define MP_ROSYBROWN1 0xffc1c1
#define MP_ROSYBROWN2 0xeeb4b4
#define MP_ROSYBROWN3 0xcd9b9b
#define MP_ROSYBROWN4 0x8b6969
#define MP_INDIANRED1 0xff6a6a
#define MP_INDIANRED2 0xee6363
#define MP_INDIANRED3 0xcd5555
#define MP_INDIANRED4 0x8b3a3a
#define MP_SIENNA1 0xff8247
#define MP_SIENNA2 0xee7942
#define MP_SIENNA3 0xcd6839
#define MP_SIENNA4 0x8b4726
#define MP_BURLYWOOD1 0xffd39b
#define MP_BURLYWOOD2 0xeec591
#define MP_BURLYWOOD3 0xcdaa7d
#define MP_BURLYWOOD4 0x8b7355
#define MP_WHEAT1 0xffe7ba
#define MP_WHEAT2 0xeed8ae
#define MP_WHEAT3 0xcdba96
#define MP_WHEAT4 0x8b7e66
#define MP_TAN1 0xffa54f
#define MP_TAN2 0xee9a49
#define MP_TAN3 0xcd853f
#define MP_TAN4 0x8b5a2b
#define MP_CHOCOLATE1 0xff7f24
#define MP_CHOCOLATE2 0xee7621
#define MP_CHOCOLATE3 0xcd661d
#define MP_CHOCOLATE4 0x8b4513
#define MP_FIREBRICK1 0xff3030
#define MP_FIREBRICK2 0xee2c2c
#define MP_FIREBRICK3 0xcd2626
#define MP_FIREBRICK4 0x8b1a1a
#define MP_BROWN1 0xff4040
#define MP_BROWN2 0xee3b3b
#define MP_BROWN3 0xcd3333
#define MP_BROWN4 0x8b2323
#define MP_SALMON1 0xff8c69
#define MP_SALMON2 0xee8262
#define MP_SALMON3 0xcd7054
#define MP_SALMON4 0x8b4c39
#define MP_LIGHTSALMON1 0xffa07a
#define MP_LIGHTSALMON2 0xee9572
#define MP_LIGHTSALMON3 0xcd8162
#define MP_LIGHTSALMON4 0x8b5742
#define MP_ORANGE1 0xffa500
#define MP_ORANGE2 0xee9a00
#define MP_ORANGE3 0xcd8500
#define MP_ORANGE4 0x8b5a00
#define MP_DARKORANGE1 0xff7f00
#define MP_DARKORANGE2 0xee7600
#define MP_DARKORANGE3 0xcd6600
#define MP_DARKORANGE4 0x8b4500
#define MP_CORAL1 0xff7256
#define MP_CORAL2 0xee6a50
#define MP_CORAL3 0xcd5b45
#define MP_CORAL4 0x8b3e2f
#define MP_TOMATO1 0xff6347
#define MP_TOMATO2 0xee5c42
#define MP_TOMATO3 0xcd4f39
#define MP_TOMATO4 0x8b3626
#define MP_ORANGERED1 0xff4500
#define MP_ORANGERED2 0xee4000
#define MP_ORANGERED3 0xcd3700
#define MP_ORANGERED4 0x8b2500
#define MP_RED1 0xff0000
#define MP_RED2 0xee0000
#define MP_RED3 0xcd0000
#define MP_RED4 0x8b0000
#define MP_DEEPPINK1 0xff1493
#define MP_DEEPPINK2 0xee1289
#define MP_DEEPPINK3 0xcd1076
#define MP_DEEPPINK4 0x8b0a50
#define MP_HOTPINK1 0xff6eb4
#define MP_HOTPINK2 0xee6aa7
#define MP_HOTPINK3 0xcd6090
#define MP_HOTPINK4 0x8b3a62
#define MP_PINK1 0xffb5c5
#define MP_PINK2 0xeea9b8
#define MP_PINK3 0xcd919e
#define MP_PINK4 0x8b636c
#define MP_LIGHTPINK1 0xffaeb9
#define MP_LIGHTPINK2 0xeea2ad
#define MP_LIGHTPINK3 0xcd8c95
#define MP_LIGHTPINK4 0x8b5f65
#define MP_PALEVIOLETRED1 0xff82ab
#define MP_PALEVIOLETRED2 0xee799f
#define MP_PALEVIOLETRED3 0xcd6889
#define MP_PALEVIOLETRED4 0x8b475d
#define MP_MAROON1 0xff34b3
#define MP_MAROON2 0xee30a7
#define MP_MAROON3 0xcd2990
#define MP_MAROON4 0x8b1c62
#define MP_VIOLETRED1 0xff3e96
#define MP_VIOLETRED2 0xee3a8c
#define MP_VIOLETRED3 0xcd3278
#define MP_VIOLETRED4 0x8b2252
#define MP_MAGENTA1 0xff00ff
#define MP_MAGENTA2 0xee00ee
#define MP_MAGENTA3 0xcd00cd
#define MP_MAGENTA4 0x8b008b
#define MP_ORCHID1 0xff83fa
#define MP_ORCHID2 0xee7ae9
#define MP_ORCHID3 0xcd69c9
#define MP_ORCHID4 0x8b4789
#define MP_PLUM1 0xffbbff
#define MP_PLUM2 0xeeaeee
#define MP_PLUM3 0xcd96cd
#define MP_PLUM4 0x8b668b
#define MP_MEDIUMORCHID1 0xe066ff
#define MP_MEDIUMORCHID2 0xd15fee
#define MP_MEDIUMORCHID3 0xb452cd
#define MP_MEDIUMORCHID4 0x7a378b
#define MP_DARKORCHID1 0xbf3eff
#define MP_DARKORCHID2 0xb23aee
#define MP_DARKORCHID3 0x9a32cd
#define MP_DARKORCHID4 0x68228b
#define MP_PURPLE1 0x9b30ff
#define MP_PURPLE2 0x912cee
#define MP_PURPLE3 0x7d26cd
#define MP_PURPLE4 0x551a8b
#define MP_MEDIUMPURPLE1 0xab82ff
#define MP_MEDIUMPURPLE2 0x9f79ee
#define MP_MEDIUMPURPLE3 0x8968cd
#define MP_MEDIUMPURPLE4 0x5d478b
#define MP_THISTLE1 0xffe1ff
#define MP_THISTLE2 0xeed2ee
#define MP_THISTLE3 0xcdb5cd
#define MP_THISTLE4 0x8b7b8b
#define MP_GRAY0 0x000000
#define MP_GREY0 0x000000
#define MP_GRAY1 0x030303
#define MP_GREY1 0x030303
#define MP_GRAY2 0x050505
#define MP_GREY2 0x050505
#define MP_GRAY3 0x080808
#define MP_GREY3 0x080808
#define MP_GRAY4 0x0a0a0a
#define MP_GREY4 0x0a0a0a
#define MP_GRAY5 0x0d0d0d
#define MP_GREY5 0x0d0d0d
#define MP_GRAY6 0x0f0f0f
#define MP_GREY6 0x0f0f0f
#define MP_GRAY7 0x121212
#define MP_GREY7 0x121212
#define MP_GRAY8 0x141414
#define MP_GREY8 0x141414
#define MP_GRAY9 0x171717
#define MP_GREY9 0x171717
#define MP_GRAY10 0x1a1a1a
#define MP_GREY10 0x1a1a1a
#define MP_GRAY11 0x1c1c1c
#define MP_GREY11 0x1c1c1c
#define MP_GRAY12 0x1f1f1f
#define MP_GREY12 0x1f1f1f
#define MP_GRAY13 0x212121
#define MP_GREY13 0x212121
#define MP_GRAY14 0x242424
#define MP_GREY14 0x242424
#define MP_GRAY15 0x262626
#define MP_GREY15 0x262626
#define MP_GRAY16 0x292929
#define MP_GREY16 0x292929
#define MP_GRAY17 0x2b2b2b
#define MP_GREY17 0x2b2b2b
#define MP_GRAY18 0x2e2e2e
#define MP_GREY18 0x2e2e2e
#define MP_GRAY19 0x303030
#define MP_GREY19 0x303030
#define MP_GRAY20 0x333333
#define MP_GREY20 0x333333
#define MP_GRAY21 0x363636
#define MP_GREY21 0x363636
#define MP_GRAY22 0x383838
#define MP_GREY22 0x383838
#define MP_GRAY23 0x3b3b3b
#define MP_GREY23 0x3b3b3b
#define MP_GRAY24 0x3d3d3d
#define MP_GREY24 0x3d3d3d
#define MP_GRAY25 0x404040
#define MP_GREY25 0x404040
#define MP_GRAY26 0x424242
#define MP_GREY26 0x424242
#define MP_GRAY27 0x454545
#define MP_GREY27 0x454545
#define MP_GRAY28 0x474747
#define MP_GREY28 0x474747
#define MP_GRAY29 0x4a4a4a
#define MP_GREY29 0x4a4a4a
#define MP_GRAY30 0x4d4d4d
#define MP_GREY30 0x4d4d4d
#define MP_GRAY31 0x4f4f4f
#define MP_GREY31 0x4f4f4f
#define MP_GRAY32 0x525252
#define MP_GREY32 0x525252
#define MP_GRAY33 0x545454
#define MP_GREY33 0x545454
#define MP_GRAY34 0x575757
#define MP_GREY34 0x575757
#define MP_GRAY35 0x595959
#define MP_GREY35 0x595959
#define MP_GRAY36 0x5c5c5c
#define MP_GREY36 0x5c5c5c
#define MP_GRAY37 0x5e5e5e
#define MP_GREY37 0x5e5e5e
#define MP_GRAY38 0x616161
#define MP_GREY38 0x616161
#define MP_GRAY39 0x636363
#define MP_GREY39 0x636363
#define MP_GRAY40 0x666666
#define MP_GREY40 0x666666
#define MP_GRAY41 0x696969
#define MP_GREY41 0x696969
#define MP_GRAY42 0x6b6b6b
#define MP_GREY42 0x6b6b6b
#define MP_GRAY43 0x6e6e6e
#define MP_GREY43 0x6e6e6e
#define MP_GRAY44 0x707070
#define MP_GREY44 0x707070
#define MP_GRAY45 0x737373
#define MP_GREY45 0x737373
#define MP_GRAY46 0x757575
#define MP_GREY46 0x757575
#define MP_GRAY47 0x787878
#define MP_GREY47 0x787878
#define MP_GRAY48 0x7a7a7a
#define MP_GREY48 0x7a7a7a
#define MP_GRAY49 0x7d7d7d
#define MP_GREY49 0x7d7d7d
#define MP_GRAY50 0x7f7f7f
#define MP_GREY50 0x7f7f7f
#define MP_GRAY51 0x828282
#define MP_GREY51 0x828282
#define MP_GRAY52 0x858585
#define MP_GREY52 0x858585
#define MP_GRAY53 0x878787
#define MP_GREY53 0x878787
#define MP_GRAY54 0x8a8a8a
#define MP_GREY54 0x8a8a8a
#define MP_GRAY55 0x8c8c8c
#define MP_GREY55 0x8c8c8c
#define MP_GRAY56 0x8f8f8f
#define MP_GREY56 0x8f8f8f
#define MP_GRAY57 0x919191
#define MP_GREY57 0x919191
#define MP_GRAY58 0x949494
#define MP_GREY58 0x949494
#define MP_GRAY59 0x969696
#define MP_GREY59 0x969696
#define MP_GRAY60 0x999999
#define MP_GREY60 0x999999
#define MP_GRAY61 0x9c9c9c
#define MP_GREY61 0x9c9c9c
#define MP_GRAY62 0x9e9e9e
#define MP_GREY62 0x9e9e9e
#define MP_GRAY63 0xa1a1a1
#define MP_GREY63 0xa1a1a1
#define MP_GRAY64 0xa3a3a3
#define MP_GREY64 0xa3a3a3
#define MP_GRAY65 0xa6a6a6
#define MP_GREY65 0xa6a6a6
#define MP_GRAY66 0xa8a8a8
#define MP_GREY66 0xa8a8a8
#define MP_GRAY67 0xababab
#define MP_GREY67 0xababab
#define MP_GRAY68 0xadadad
#define MP_GREY68 0xadadad
#define MP_GRAY69 0xb0b0b0
#define MP_GREY69 0xb0b0b0
#define MP_GRAY70 0xb3b3b3
#define MP_GREY70 0xb3b3b3
#define MP_GRAY71 0xb5b5b5
#define MP_GREY71 0xb5b5b5
#define MP_GRAY72 0xb8b8b8
#define MP_GREY72 0xb8b8b8
#define MP_GRAY73 0xbababa
#define MP_GREY73 0xbababa
#define MP_GRAY74 0xbdbdbd
#define MP_GREY74 0xbdbdbd
#define MP_GRAY75 0xbfbfbf
#define MP_GREY75 0xbfbfbf
#define MP_GRAY76 0xc2c2c2
#define MP_GREY76 0xc2c2c2
#define MP_GRAY77 0xc4c4c4
#define MP_GREY77 0xc4c4c4
#define MP_GRAY78 0xc7c7c7
#define MP_GREY78 0xc7c7c7
#define MP_GRAY79 0xc9c9c9
#define MP_GREY79 0xc9c9c9
#define MP_GRAY80 0xcccccc
#define MP_GREY80 0xcccccc
#define MP_GRAY81 0xcfcfcf
#define MP_GREY81 0xcfcfcf
#define MP_GRAY82 0xd1d1d1
#define MP_GREY82 0xd1d1d1
#define MP_GRAY83 0xd4d4d4
#define MP_GREY83 0xd4d4d4
#define MP_GRAY84 0xd6d6d6
#define MP_GREY84 0xd6d6d6
#define MP_GRAY85 0xd9d9d9
#define MP_GREY85 0xd9d9d9
#define MP_GRAY86 0xdbdbdb
#define MP_GREY86 0xdbdbdb
#define MP_GRAY87 0xdedede
#define MP_GREY87 0xdedede
#define MP_GRAY88 0xe0e0e0
#define MP_GREY88 0xe0e0e0
#define MP_GRAY89 0xe3e3e3
#define MP_GREY89 0xe3e3e3
#define MP_GRAY90 0xe5e5e5
#define MP_GREY90 0xe5e5e5
#define MP_GRAY91 0xe8e8e8
#define MP_GREY91 0xe8e8e8
#define MP_GRAY92 0xebebeb
#define MP_GREY92 0xebebeb
#define MP_GRAY93 0xededed
#define MP_GREY93 0xededed
#define MP_GRAY94 0xf0f0f0
#define MP_GREY94 0xf0f0f0
#define MP_GRAY95 0xf2f2f2
#define MP_GREY95 0xf2f2f2
#define MP_GRAY96 0xf5f5f5
#define MP_GREY96 0xf5f5f5
#define MP_GRAY97 0xf7f7f7
#define MP_GREY97 0xf7f7f7
#define MP_GRAY98 0xfafafa
#define MP_GREY98 0xfafafa
#define MP_GRAY99 0xfcfcfc
#define MP_GREY99 0xfcfcfc
#define MP_GRAY100 0xffffff
#define MP_GREY100 0xffffff
#define MP_DARKGREY 0xa9a9a9
#define MP_DARKGRAY 0xa9a9a9
#define MP_DARKBLUE 0x00008b
#define MP_DARKCYAN 0x008b8b
#define MP_DARKMAGENTA 0x8b008b
#define MP_DARKRED 0x8b0000
#define MP_LIGHTGREEN 0x90ee90





typedef uint64_t MicroProfileLogEntry;


void MicroProfileSleep(uint32_t nMs);
template<typename T>
T MicroProfileMin(T a, T b);
template<typename T>
T MicroProfileMax(T a, T b);
template<typename T>
T MicroProfileClamp(T a, T min_, T max_);
int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond);
float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond);
uint64_t MicroProfileLogType(MicroProfileLogEntry Index);
uint64_t MicroProfileLogTimerIndex(MicroProfileLogEntry Index);
MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick);
int64_t MicroProfileLogTickDifference(MicroProfileLogEntry Start, MicroProfileLogEntry End);
int64_t MicroProfileLogSetTick(MicroProfileLogEntry e, int64_t nTick);
uint16_t MicroProfileGetTimerIndex(MicroProfileToken t);// { return (t & 0xffff); }
uint64_t MicroProfileGetGroupMask(MicroProfileToken t);// { return ((t >> 16)&MICROPROFILE_GROUP_MASK_ALL); }
MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint16_t nTimer);// { return (nGroupMask << 16) | nTimer; }



//////////////////////////////////////////////////////////////////////////
// platform IMPL
void* MicroProfileAllocAligned(size_t nSize, size_t nAlign);

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
	if (nTicksPerSecond == 0)
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
typedef uint64_t MicroProfileThreadIdType;

void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
	void* p; 
	posix_memalign(&p, nAlign, nSize); 
	return p;
}

#elif defined(_WIN32)
int64_t MicroProfileGetTick();
#define MP_TICK() MicroProfileGetTick()
#define MP_BREAK() __debugbreak()
#define MP_THREAD_LOCAL __declspec(thread)
#define MP_STRCASECMP _stricmp
#define MP_GETCURRENTTHREADID() GetCurrentThreadId()
void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
	return _aligned_malloc(nSize, nAlign);
}

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
typedef uint64_t MicroProfileThreadIdType;
#endif



#ifdef MICROPROFILE_PS4
#define MICROPROFILE_PS4_DECL
#include "microprofile_ps4.h"
#endif

#ifdef MICROPROFILE_XBOXONE
#define MICROPROFILE_XBOXONE_DECL
#include "microprofile_xboxone.h"
#endif


#ifndef MP_GETCURRENTTHREADID 
#define MP_GETCURRENTTHREADID() 0
typedef uint32_t MicroProfileThreadIdType;
static void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
	void* p; 
	posix_memalign(&p, nAlign, nSize); 
	return p;
}

#endif


#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)


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
	char pNameExt[MICROPROFILE_NAME_MAX_LEN];
	uint32_t nNameLen;
	uint32_t nColor;
	int nWSNext;
	bool bGraph;
	bool bWSEnabled;
	MicroProfileTokenType Type;

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
	MicroProfileThreadIdType nThreadOut;
	MicroProfileThreadIdType nThreadIn;
	int64_t nCpu : 8;
	int64_t nTicks : 56;
};


struct MicroProfileFrameState
{
	int64_t nFrameStartCpu;
	int64_t nFrameStartGpu;
	uint64_t nFrameId;
	uint32_t nLogStart[MICROPROFILE_MAX_THREADS];
};

struct MicroProfileThreadLog
{
	MicroProfileLogEntry	Log[MICROPROFILE_BUFFER_SIZE];

	std::atomic<uint32_t>	nPut;
	std::atomic<uint32_t>	nGet;
	uint32_t				nStackPut;
	uint32_t				nStackScope;
	MicroProfileScopeStateC ScopeState[MICROPROFILE_STACK_MAX];


	uint32_t 				nActive;
	uint32_t 				nGpu;
	MicroProfileThreadIdType			nThreadId;
	uint32_t 				nLogIndex;

	MicroProfileLogEntry 	nStackLogEntry[MICROPROFILE_STACK_MAX];
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


struct MicroProfileWebSocketBuffer
{
	char Header[20];
	char Buffer[MICROPROFILE_WEBSOCKET_BUFFER_SIZE];
	uint32_t nPut;
	MpSocket Socket;


	char SendBuffer[MICROPROFILE_WEBSOCKET_BUFFER_SIZE];
	std::atomic<uint32_t> nSendPut;
	std::atomic<uint32_t> nSendGet;
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

	uint32_t				nStackScope;
	MicroProfileScopeStateC ScopeState[MICROPROFILE_STACK_MAX];
};


struct MicroProfileGpuTimerState;
#if MICROPROFILE_GPU_TIMERS_D3D11
struct MicroProfileD3D11Frame
{
	uint32_t m_nQueryStart;
	uint32_t m_nQueryCountMax;
	std::atomic<uint32_t> m_nQueryCount;
	uint32_t m_nRateQueryStarted;
	void* m_pRateQuery;
};

struct MicroProfileGpuTimerState
{
	uint32_t bInitialized;
	void* m_pDevice;
	void* m_pImmediateContext;
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

#define MICROPROFILE_D3D_MAX_NODE_COUNT 4
#define MICROPROFILE_D3D_INTERNAL_DELAY 8

#define MP_NODE_MASK_ALL(n) ((1u<<(n))-1u)
#define MP_NODE_MASK_ONE(n) (1u<<(n))

struct MicroProfileD3D12Frame
{
	uint32_t nBegin;
	uint32_t nCount;
	uint32_t nNode;
	ID3D12GraphicsCommandList* pCommandList[MICROPROFILE_D3D_MAX_NODE_COUNT];
	ID3D12CommandAllocator* pCommandAllocator;
};
struct MicroProfileGpuTimerState
{
	ID3D12Device* pDevice;
	uint32_t nNodeCount;
	uint32_t nCurrentNode;

	uint64_t nFrame;
	uint64_t nPendingFrame;

	uint32_t nFrameStart;
	std::atomic<uint32_t> nFrameCount;
	int64_t nFrequency;
	ID3D12Resource* pBuffer;

	struct
	{
		ID3D12CommandQueue* pCommandQueue;
		ID3D12QueryHeap* pHeap;
		ID3D12Fence* pFence;
	}
	NodeState[MICROPROFILE_D3D_MAX_NODE_COUNT];

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
	uint32_t nFrozen;
	uint32_t nWasFrozen;
	uint32_t nPlatformMarkersEnabled;

	uint64_t nForceGroup;
	uint32_t nForceEnable;
	uint32_t nForceMetaCounters;

	uint64_t nForceGroupUI;
	uint64_t nActiveGroupWanted;
	uint32_t nStartEnabled;
	uint32_t nAllThreadsWanted;

	uint32_t nOverflow;

	uint64_t nGroupMask;
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
	uint32_t 				nFrameNext;
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

	int 						WebSocketTimers;
	uint32_t 					nWebSocketDirty;
	MpSocket 					WebSockets[1];
	int64_t 					WebSocketFrameLast[1];
	uint32_t					nNumWebSockets;
	uint32_t 					nSocketFail; // for error propagation.


	MicroProfileThread 			WebSocketSendThread;
	bool 	 					WebSocketThreadRunning;

	uint32_t 					WSCategoriesSent;
	uint32_t 					WSGroupsSent;
	uint32_t 					WSTimersSent;
	uint32_t 					WSCountersSent;
	MicroProfileWebSocketBuffer WSBuf;
	char* 						pJsonSettings;
	bool 						bJsonSettingsReadOnly;
	uint32_t 					nJsonSettingsPending;
	uint32_t 					nJsonSettingsBufferSize;
	uint32_t 					nWSWasConnected;
	uint32_t 					nMicroProfileShutdown;
	uint32_t					nWSViewMode;

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

	MicroProfileGpuTimerState* 	pGPU;

};

inline uint64_t MicroProfileLogType(MicroProfileLogEntry Index)
{
	return ((MP_LOG_BEGIN_MASK & Index) >> 62) & 0x3;
}

inline uint64_t MicroProfileLogTimerIndex(MicroProfileLogEntry Index)
{
	return (0x3fff & (Index >> 48));
}

inline MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick)
{
	MicroProfileLogEntry Entry = (nBegin << 62) | ((0x3fff & nToken) << 48) | (MP_LOG_TICK_MASK&nTick);
	uint64_t t = MicroProfileLogType(Entry);
	uint64_t nTimerIndex = MicroProfileLogTimerIndex(Entry);
	MP_ASSERT(t == nBegin);
	MP_ASSERT(nTimerIndex == (nToken & 0x3fff));
	return Entry;

}

inline int64_t MicroProfileLogTickDifference(MicroProfileLogEntry Start, MicroProfileLogEntry End)
{
	uint64_t nStart = Start;
	uint64_t nEnd = End;
	int64_t nDifference = ((nEnd << 16) - (nStart << 16));
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

inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t)
{ 
	return (t & 0xffff); 
}
inline uint64_t MicroProfileGetGroupMask(MicroProfileToken t)
{
	return ((t >> 16)&MICROPROFILE_GROUP_MASK_ALL); 
}
inline MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint16_t nTimer)
{
	return (nGroupMask << 16) | nTimer;
}

template<typename T>
T MicroProfileMin(T a, T b)
{
	return a < b ? a : b;
}

template<typename T>
T MicroProfileMax(T a, T b)
{
	return a > b ? a : b;
}
template<typename T>
T MicroProfileClamp(T a, T min_, T max_)
{
	return MicroProfileMin(max_, MicroProfileMax(min_, a));
}

inline int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond)
{
	return (int64_t)(fMs*0.001f*nTicksPerSecond);
}

inline float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond)
{
	return 1000.f / nTicksPerSecond;
}
float MicroProfileTickToMsMultiplierCpu()
{
	return MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
}

float MicroProfileTickToMsMultiplierGpu()
{
	return MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
}
uint16_t MicroProfileGetGroupIndex(MicroProfileToken t)
{
	return (uint16_t)MicroProfileGet()->TimerToGroup[MicroProfileGetTimerIndex(t)];
}

uint64_t MicroProfileTick()
{
	return MP_TICK();
}


#ifdef _WIN32
#include <windows.h>
#define snprintf _snprintf

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4100)
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

#if 1

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
    *pThread = MICROPROFILE_ALLOC_OBJECT(std::thread);
    new (*pThread) std::thread(Func, nullptr);
}
inline void MicroProfileThreadJoin(MicroProfileThread* pThread)
{
    (*pThread)->join();
    (*pThread)->~thread();
	MICROPROFILE_FREE(*pThread);
    *pThread = 0;
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


MICROPROFILE_DEFINE(g_MicroProfileFlip, "MicroProfile", "MicroProfileFlip", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileThreadLoop, "MicroProfile", "ThreadLoop", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileClear, "MicroProfile", "Clear", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileAccumulate, "MicroProfile", "Accumulate", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchSearch,"MicroProfile", "ContextSwitchSearch", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileGpuSubmit, "MicroProfile", "MicroProfileGpuSubmit", MP_HOTPINK2);

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
void* MicroProfileSocketSenderThread(void*);

void MicroProfileInit()
{
	static bool bOnce = true;
	if(!bOnce)
	{
		return;
	}

	std::recursive_mutex& mutex = MicroProfileMutex();
	bool bUseLock = g_bUseLock;
	if(bUseLock)
		mutex.lock();
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
		S.nFrozen = 0;
		S.nWasFrozen = 0;
		S.nForceGroup = 0;
		S.nActiveGroupWanted = 0;
		S.nStartEnabled = 0;
		S.nAllThreadsWanted = 1;
		S.nAggregateFlip = 0;
		S.nTotalTimers = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
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
		S.WebSocketTimers = -1;
		S.nSocketFail = 0;


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

		S.pJsonSettings = 0;
		S.nJsonSettingsPending = 0;
		S.nJsonSettingsBufferSize = 0;
		S.nWSWasConnected = 0;
	}
	if(bUseLock)
	{
		mutex.unlock();
	}
}

void MicroProfileShutdown()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	if(S.pJsonSettings)
	{
		MICROPROFILE_FREE(S.pJsonSettings);
		S.pJsonSettings = 0;
		S.nJsonSettingsBufferSize = 0;
	}
	S.nMicroProfileShutdown = 1;
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
void MicroProfileSetThreadLog(MicroProfileThreadLog* pLog)
{
	g_MicroProfileThreadLogThreadLocal = pLog;
}
#endif

MicroProfileThreadLog* MicroProfileGetThreadLog2()
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
	if (!pLog)
	{
		MicroProfileInitThreadLog();
		pLog = MicroProfileGetThreadLog();
	}
	return pLog;
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

MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName)
{
	MicroProfileScopeLock L(MicroProfileMutex());
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
		pLog = MICROPROFILE_ALLOC_OBJECT(MicroProfileThreadLog);
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
	char Buffer[64];
	g_bUseLock = true;
	MicroProfileInit();
	MP_ASSERT(MicroProfileGetThreadLog() == 0);
	MicroProfileThreadLog* pLog = MicroProfileCreateThreadLog(pThreadName ? pThreadName : MicroProfileGetThreadName(Buffer));
	(void)Buffer;
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
		pLog = MICROPROFILE_ALLOC_OBJECT(MicroProfileThreadLogGpu);
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
	if(S.nStartEnabled)
	{
		S.nActiveGroupWanted |= (1ll << (uint64_t)S.nGroupCount);
	}
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
	snprintf(&S.TimerInfo[nTimerIndex].pNameExt[0], sizeof(S.TimerInfo[nTimerIndex].pNameExt)-1, "%s %s", S.GroupInfo[nGroupIndex].pName, pName);
	S.TimerInfo[nTimerIndex].pName[nLen] = '\0';
	S.TimerInfo[nTimerIndex].nNameLen = nLen;
	S.TimerInfo[nTimerIndex].nColor = nColor&0xffffff;
	S.TimerInfo[nTimerIndex].nGroupIndex = nGroupIndex;
	S.TimerInfo[nTimerIndex].nTimerIndex = nTimerIndex;
	S.TimerInfo[nTimerIndex].nWSNext = -1;
	S.TimerInfo[nTimerIndex].bWSEnabled = false;
	S.TimerInfo[nTimerIndex].Type = Type;
	S.TimerToGroup[nTimerIndex] = nGroupIndex;
	return nToken;
}
void MicroProfileGetTokenC(MicroProfileToken* pToken, const char* pGroup, const char* pName, uint32_t nColor, MicroProfileTokenType Type)
{
	if(*pToken == MICROPROFILE_INVALID_TOKEN)
	{
		MicroProfileInit();
		MicroProfileScopeLock L(MicroProfileMutex());
		if(*pToken == MICROPROFILE_INVALID_TOKEN)
		{
			*pToken = MicroProfileGetToken(pGroup, pName, nColor, Type);
		}	
	}
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
	S.CounterInfo[nResult].nFlags |= MICROPROFILE_COUNTER_FLAG_LEAF;

	MP_ASSERT((int)nResult >= 0);
	return nResult;
}

inline void MicroProfileLogPut(MicroProfileLogEntry LE, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPut+1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nGet = pLog->nGet.load(std::memory_order_relaxed);
	uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;
	MP_ASSERT(nDistance < MICROPROFILE_BUFFER_SIZE);
	uint32_t nStackPut = pLog->nStackPut;
	if (nDistance < nStackPut + 2)
	{
		S.nOverflow = 100;
	}
	else
	{
		pLog->Log[nPut] = LE;
		pLog->nPut.store(nNextPos, std::memory_order_release);
	}
}

inline uint64_t MicroProfileLogPutEnter(MicroProfileToken nToken_, uint64_t nTick, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_ENTER, nToken_, nTick);
	uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPut + 1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
	uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;
	MP_ASSERT(nDistance < MICROPROFILE_BUFFER_SIZE);
	uint32_t nStackPut = (pLog->nStackPut);
	if (nDistance < nStackPut+4) //2 for ring buffer, 2 for the actual entries
	{
		S.nOverflow = 100;
		return MICROPROFILE_INVALID_TICK;
	}
	else
	{
		pLog->nStackPut = nStackPut + 1;
		pLog->Log[nPut] = LE;
		pLog->nPut.store(nNextPos, std::memory_order_release);
		return nTick;
	}
}
inline void MicroProfileLogPutLeave(MicroProfileToken nToken_, uint64_t nTick, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_LEAVE, nToken_, nTick);
	uint32_t nPos = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPos + 1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nStackPut = --(pLog->nStackPut);
	uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
	MP_ASSERT(nStackPut < MICROPROFILE_STACK_MAX);
	MP_ASSERT(nNextPos != nGet); //should never happen
	pLog->Log[nPos] = LE;
	pLog->nPut.store(nNextPos, std::memory_order_release);
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


uint64_t MicroProfileEnterInternal(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		uint64_t nTick = MP_TICK();
		if(MICROPROFILE_PLATFORM_MARKERS_ENABLED)
		{
			uint32_t idx = MicroProfileGetTimerIndex(nToken_);
			MicroProfileTimerInfo& TI = S.TimerInfo[idx];
			MICROPROFILE_PLATFORM_MARKER_BEGIN(TI.nColor, TI.pNameExt);
			return nTick;
		}
		else
		{
			return MicroProfileLogPutEnter(nToken_, nTick, MicroProfileGetThreadLog2());
		}
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

void MicroProfileLocalCounterAddAtomic(std::atomic<int64_t>* pCounter, int64_t nCount)
{
	pCounter->fetch_add(nCount);
}
int64_t MicroProfileLocalCounterSetAtomic(std::atomic<int64_t>* pCounter, int64_t nCount)
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

void MicroProfileLeaveInternal(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(MICROPROFILE_INVALID_TICK != nTickStart)
	{
		if(MICROPROFILE_PLATFORM_MARKERS_ENABLED)
		{
			MICROPROFILE_PLATFORM_MARKER_END();
		}
		else
		{
			uint64_t nTick = MP_TICK();
			MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
			MicroProfileLogPutLeave(nToken_, nTick, pLog);
		}
	}
}


void MicroProfileEnter(MicroProfileToken nToken)
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope < MICROPROFILE_STACK_MAX); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[pLog->nStackScope++];
	pScopeState->Token = nToken;
	pScopeState->nTick = MicroProfileEnterInternal(nToken);
}
void MicroProfileLeave()
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope > 0); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[--pLog->nStackScope];
	MicroProfileLeaveInternal(pScopeState->Token, pScopeState->nTick);
}


void MicroProfileEnterGpu(MicroProfileToken nToken, MicroProfileThreadLogGpu* pLog)
{
	MP_ASSERT(pLog->nStackScope < MICROPROFILE_STACK_MAX); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[pLog->nStackScope++];
	pScopeState->Token = nToken;
	pScopeState->nTick = MicroProfileGpuEnterInternal(pLog, nToken);
}
void MicroProfileLeaveGpu(MicroProfileThreadLogGpu* pLog)
{
	MP_ASSERT(pLog->nStackScope > 0); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[--pLog->nStackScope];
	MicroProfileGpuLeaveInternal(pLog, pScopeState->Token, pScopeState->nTick);
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

uint64_t MicroProfileGpuEnterInternal(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}

		MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd		
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		MicroProfileLogPutGpu(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		MicroProfileLogPutGpu(pLog->nLogIndex, MP_TICK(), MP_LOG_GPU_EXTRA, pGpuLog);
		return 1;
	}
	return 0;
}

void MicroProfileGpuLeaveInternal(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(nTickStart)
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}

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
	if(S.nActiveGroup || pContextSwitch->nTicks <= S.nPauseTicks)
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

void MicroProfileToggleFrozen()
{
	S.nFrozen = !S.nFrozen;
}

int MicroProfileIsFrozen()
{
	return S.nFrozen != 0?1:0;
}
int MicroProfileEnabled()
{
	return S.nActiveGroup != 0;
}
	
static void MicroProfileFlipEnabled()
{
	uint64_t nNewActiveGroup = 0;
	uint64_t nActiveBefore = S.nActiveGroup;
	nNewActiveGroup = S.nActiveGroupWanted;
	nNewActiveGroup |= S.nForceGroup;
	nNewActiveGroup |= S.nForceGroupUI;
	if(S.nActiveGroup != nNewActiveGroup)
		S.nActiveGroup = nNewActiveGroup;
	uint32_t nNewActiveBars = 0;
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
	if(S.nFrozen)
	{
		S.nActiveBars = 0;
		S.nActiveGroup = 0;
	}

#if MICROPROFILE_DEBUG
	if(nActiveBefore != S.nActiveGroup)
	{
		printf("new active group is %08llx %08llx\n", S.nActiveGroup, S.nGroupMask);
	}
#else
	(void)nActiveBefore;
#endif

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
		if(!S.WebSocketThreadRunning)
		{
			MicroProfileThreadStart(&S.WebSocketSendThread, MicroProfileSocketSenderThread);
			S.WebSocketThreadRunning = 1;
		}
	}

	int nLoop = 0;
	do
	{
		if(MicroProfileWebServerUpdate())
		{	
			S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; //hide spike from dumping webpage
		}
		if(nLoop++)
		{
			MicroProfileSleep(100);
			if((nLoop % 10) == 0)
			{
				#if MICROPROFILE_DEBUG
				printf("microprofile frozen %d\n", nLoop);
				#endif
			}
		}
	}while(S.nFrozen);

	uint32_t nAggregateClear = S.nAggregateClear || S.nAutoClearFrames, nAggregateFlip = 0;

	if(S.nAutoClearFrames)
	{
		nAggregateClear = 1;
		nAggregateFlip = 1;
		S.nAutoClearFrames -= 1;
	}

	bool nRunning = S.nActiveGroup != 0;
	if(nRunning)
	{
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

		MicroProfileGpuBegin(pContext, S.pGpuGlobal);

		uint32_t nGpuTimeStamp = MicroProfileGpuFlip(pContext);



		uint64_t nFrameIdx = S.nFramePutIndex++;
		S.nFramePut = (S.nFramePut+1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.Frames[S.nFramePut].nFrameId = nFrameIdx;
		MP_ASSERT((S.nFramePutIndex % MICROPROFILE_MAX_FRAME_HISTORY) == S.nFramePut);
		S.nFrameCurrent = (S.nFramePut + MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.nFrameCurrentIndex++;
		uint32_t nFrameNext = (S.nFrameCurrent+1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.nFrameNext = nFrameNext;

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
		uint32_t nPutStart[MICROPROFILE_MAX_THREADS];
		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			if(!pLog)
			{
				pFramePut->nLogStart[i] = 0;
				nPutStart[i] = (uint32_t)-1;
			}
			else
			{
				uint32_t nPut = pLog->nPut.load(std::memory_order_acquire);
				pFramePut->nLogStart[i] = nPut;
				nPutStart[i] = nPut;
			}
		}

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
					
					uint64_t* pStackLog = &pLog->nStackLogEntry[0];
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
								pStackLog[nStackPos] = LE;
								nStackPos++;
								pChildTickStack[nStackPos] = 0;

							}
							else if(MP_LOG_META == nType)
							{
								if(nStackPos)
								{
									int64_t nMetaIndex = MicroProfileLogTimerIndex(LE);
									int64_t nMetaCount = MicroProfileLogGetTick(LE);
									MP_ASSERT(nMetaIndex < MICROPROFILE_META_MAX);
									MicroProfileLogEntry LEPrev = pStackLog[nStackPos-1];
									int64_t nCounter = MicroProfileLogTimerIndex(LEPrev);
									S.MetaCounters[nMetaIndex].nCounters[nCounter] += nMetaCount;
								}
							}
							else if(MP_LOG_LEAVE == nType)
							{
								int nTimer = MicroProfileLogTimerIndex(LE);
								uint8_t nGroup = pTimerToGroup[nTimer];
								MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
								MP_ASSERT(nStackPos);
								{									
									int64_t nTickStart = pStackLog[nStackPos-1];
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
							else
							{
								//MP_BREAK();
							}
						}
					}
					for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
					{
						pLog->nGroupTicks[j] += nGroupTicks[j];
						pFrameGroup[j] += nGroupTicks[j];
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


		if(S.nAggregateFlip <= ++S.nAggregateFlipCount)
		{
			nAggregateFlip = 1;
			if(S.nAggregateFlip) // if 0 accumulate indefinitely
			{
				nAggregateClear = 1;
			}
		}

		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			uint32_t nNewGet = pFrameNext->nLogStart[i];

			if(pLog && nNewGet != (uint32_t)-1)
			{
				pLog->nGet.store(nNewGet);
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


	MicroProfileFlipEnabled();
}

void MicroProfileSetEnableAllGroups(int bEnable)
{
	if(bEnable)
	{
		S.nActiveGroupWanted = S.nGroupMask;
		S.nStartEnabled = 1;
	}
	else
	{
		S.nActiveGroupWanted = 0;
		S.nStartEnabled = 0;
	}
}
void MicroProfileEnableCategory(const char* pCategory, int bEnabled)
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

int MicroProfileGetEnableAllGroups()
{
	return S.nGroupMask == S.nActiveGroupWanted?1:0;
}

void MicroProfileSetForceMetaCounters(int bForce)
{
	S.nForceMetaCounters = bForce ? 1 : 0;
}

int MicroProfileGetForceMetaCounters()
{
	return 0 != S.nForceMetaCounters?1:0;
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

int MicroProfilePlatformMarkersGetEnabled()
{
	return S.nPlatformMarkersEnabled != 0 ? 1 : 0;
}
void MicroProfilePlatformMarkersSetEnabled(int bEnabled)
{
	S.nPlatformMarkersEnabled = bEnabled ? 1 : 0;
}



void MicroProfileContextSwitchSearch(uint32_t* pContextSwitchStart, uint32_t* pContextSwitchEnd, uint64_t nBaseTicksCpu, uint64_t nBaseTicksEndCpu)
{
	MICROPROFILE_SCOPE(g_MicroProfileContextSwitchSearch);
	uint32_t nContextSwitchPut = S.nContextSwitchPut;
	uint64_t nContextSwitchStart, nContextSwitchEnd;
	nContextSwitchStart = nContextSwitchEnd = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;		
	int64_t nSearchEnd = nBaseTicksEndCpu + MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());
	int64_t nSearchBegin = nBaseTicksCpu - MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());
	int64_t nMax = INT64_MIN;
	int64_t nMin = INT64_MAX;
	for(uint32_t i = 0; i < MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE; ++i)
	{
		uint32_t nIndex = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - (i+1)) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
		MicroProfileContextSwitch& CS = S.ContextSwitch[nIndex];
		if (nMax < CS.nTicks)
			nMax = CS.nTicks;
		if (nMin > CS.nTicks && CS.nTicks != 0)
			nMin = CS.nTicks;
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
		pOut[0] = '0';
		pOut[1] = '\0';
		return 1;
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
		if(nCounter < 0) // handle INT_MIN
		{
			nCounter = -(nCounter+1);
		}
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
		const char* pExt[] = { "b","kb","mb","gb","tb","pb", "eb","zb", "yb" };
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

extern const char* g_MicroProfileHtmlLive_begin[];
extern size_t g_MicroProfileHtmlLive_begin_sizes[];
extern size_t g_MicroProfileHtmlLive_begin_count;
extern const char* g_MicroProfileHtmlLive_end[];
extern size_t g_MicroProfileHtmlLive_end_sizes[];
extern size_t g_MicroProfileHtmlLive_end_count;

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
void MicroProfileDumpCsv(MicroProfileWriteCallback CB, void* Handle)
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

void MicroProfileDumpHtmlLive(MicroProfileWriteCallback CB, void* Handle)
{
	for(size_t i = 0; i < g_MicroProfileHtmlLive_begin_count; ++i)
	{
		CB(Handle, g_MicroProfileHtmlLive_begin_sizes[i]-1, g_MicroProfileHtmlLive_begin[i]);
	}
	for(size_t i = 0; i < g_MicroProfileHtmlLive_end_count; ++i)
	{
		CB(Handle, g_MicroProfileHtmlLive_end_sizes[i]-1, g_MicroProfileHtmlLive_end[i]);
	}

}
void MicroProfileDumpHtml(MicroProfileWriteCallback CB, void* Handle, uint64_t nMaxFrames, const char* pHost, uint64_t nStartFrameId = (uint64_t)-1)
{
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
			MicroProfileThreadIdType ThreadId = S.Pool[i]->nThreadId;
			if(!ThreadId)
			{
				ThreadId = (MicroProfileThreadIdType)-1;
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


	for(int i = 0; i < (int)S.nNumCounters; ++i)
	{
		if(0 != (S.CounterInfo[i].nFlags & MICROPROFILE_COUNTER_FLAG_DETAILED))
		{
			int64_t nCounterMax = S.nCounterMax[i];
			int64_t nCounterMin = S.nCounterMin[i];
			uint32_t nBaseIndex = S.nCounterHistoryPut;
			MicroProfilePrintf(CB, Handle, "\nvar CounterHistoryArray%d =[", i);
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				uint32_t nHistoryIndex = (nBaseIndex + j) % MICROPROFILE_GRAPH_HISTORY;
				int64_t nValue = MicroProfileClamp(S.nCounterHistory[nHistoryIndex][i], nCounterMin, nCounterMax);
				MicroProfilePrintf(CB, Handle, "%lld,", nValue);
			}
			MicroProfilePrintf(CB, Handle, "];\n");

			int64_t nCounterHeightBase = nCounterMax;
			int64_t nCounterOffset = 0;
			if(nCounterMin < 0)
			{
				nCounterHeightBase = nCounterMax - nCounterMin;
				nCounterOffset = -nCounterMin;
			}
			double fRcp = nCounterHeightBase? (1.0 / nCounterHeightBase) : 0;

			MicroProfilePrintf(CB, Handle, "\nvar CounterHistoryArrayPrc%d =[", i);
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				uint32_t nHistoryIndex = (nBaseIndex + j) % MICROPROFILE_GRAPH_HISTORY;
				int64_t nValue = MicroProfileClamp(S.nCounterHistory[nHistoryIndex][i], nCounterMin, nCounterMax);
				float fPrc = (nValue+nCounterOffset) * fRcp;
				MicroProfilePrintf(CB, Handle, "%f,",fPrc);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
			MicroProfilePrintf(CB, Handle, "var CounterHistory%d = MakeCounterHistory(%d, CounterHistoryArray%d, CounterHistoryArrayPrc%d)\n", i, i, i, i);
		}
		else
		{
			MicroProfilePrintf(CB, Handle, "var CounterHistory%d;\n", i);
		}
	}	

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
		MicroProfilePrintf(CB, Handle, "MakeCounter(%d, %d, %d, %d, %d, '%s', %lld, %lld, %lld, '%s', %lld, '%s', %f, %f, %d, CounterHistory%d),\n", 
			i,
			S.CounterInfo[i].nParent,
			S.CounterInfo[i].nSibling,
 	 		S.CounterInfo[i].nFirstChild,
 	 		S.CounterInfo[i].nLevel,
			S.CounterInfo[i].pName,
			nCounter,
			S.nCounterMin[i],
			S.nCounterMax[i],
			Formatted,
			nLimit,
			FormattedLimit,
			fCounterPrc,
			fBoxPrc,
			S.CounterInfo[i].eFormat == MICROPROFILE_COUNTER_FORMAT_BYTES ? 1 : 0, 
			i
			);
	}
	MicroProfilePrintf(CB, Handle, "];\n\n");

	uint32_t nNumFrames = 0;
	uint32_t nFirstFrame = (uint32_t)-1;
	if(nStartFrameId != (uint64_t)-1)
	{
		//search for the frane
		for(uint32_t i = 0; i < MICROPROFILE_MAX_FRAME_HISTORY; ++i)
		{
			if(S.Frames[i].nFrameId == nStartFrameId)
			{
				nFirstFrame = i;
				break;
			}
		}
		if(nFirstFrame != (uint32_t)-1)
		{
			uint32_t nLastFrame = S.nFrameCurrent;
			uint32_t nDistance = (MICROPROFILE_MAX_FRAME_HISTORY + nFirstFrame - nLastFrame) % MICROPROFILE_MAX_FRAME_HISTORY;
			nNumFrames = MicroProfileMin(nDistance, (uint32_t)nMaxFrames);
		}
	}

	if(nNumFrames == 0)
	{
		nNumFrames = (MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 3); //leave a few to not overwrite
		nNumFrames = MicroProfileMin(nNumFrames, (uint32_t)nMaxFrames);
		nFirstFrame = (S.nFrameCurrent + MICROPROFILE_MAX_FRAME_HISTORY - nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	}

	uint32_t nLastFrame = (nFirstFrame + nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	MP_ASSERT(nFirstFrame < MICROPROFILE_MAX_FRAME_HISTORY);
	MP_ASSERT(nLastFrame  < MICROPROFILE_MAX_FRAME_HISTORY);
	const int64_t nTickStart = S.Frames[nFirstFrame].nFrameStartCpu;
	const int64_t nTickEnd = S.Frames[nLastFrame].nFrameStartCpu;
	int64_t nTickStartGpu = S.Frames[nFirstFrame].nFrameStartGpu;


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
					nLogType = MicroProfileLogType(pLog->Log[k]);
					fToMs = nLogType == MP_LOG_GPU_EXTRA ? fToMsCpu : fToMsBase;
					nStartTick = nLogType == MP_LOG_GPU_EXTRA ? nTickStart : nStartTickBase;
					fTime = nLogType == MP_LOG_META ? 0.f : MicroProfileLogTickDifference(nStartTick, pLog->Log[k]) * fToMs;
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


	MicroProfilePrintf(CB, Handle, "var CSwitchThreads = {");


	MicroProfileThreadInfo* pThreadInfo = nullptr;
	uint32_t nNumThreads = MicroProfileGetThreadInfoArray(&pThreadInfo);
	for (uint32_t i = 0; i < nNumThreads; ++i)
	{
		const char* p1 = pThreadInfo[i].pThreadModule ? pThreadInfo[i].pThreadModule : "?";
		const char* p2 = pThreadInfo[i].pProcessModule ? pThreadInfo[i].pProcessModule : "?";

		MicroProfilePrintf(CB, Handle, "%" PRId64 ":{\'tid\':%" PRId64 ",\'pid\':%" PRId64 ",\'t\':\'%s\',\'p\':\'%s\'},\n",
			(uint64_t)pThreadInfo[i].tid,
			(uint64_t)pThreadInfo[i].tid,
			(uint64_t)pThreadInfo[i].pid,
			p1, p2
			);
	}

	MicroProfilePrintf(CB, Handle, "};\n");

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
			MicroProfileDumpCsv(MicroProfileWriteFile, F);
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
		S.ListenerSocket = (MpSocket)-1;
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
enum MicroProfileGetCommand
{
	EMICROPROFILE_GET_COMMAND_DUMP,
	EMICROPROFILE_GET_COMMAND_DUMP_RANGE,
	EMICROPROFILE_GET_COMMAND_LIVE,
	EMICROPROFILE_GET_COMMAND_UNKNOWN,
};
struct MicroProfileParseGetResult
{
	uint64_t nFrames;
	uint64_t nFrameStart;
};
MicroProfileGetCommand MicroProfileParseGet(const char* pGet, MicroProfileParseGetResult* pResult)
{
	if(0 == strlen(pGet))
	{
		return EMICROPROFILE_GET_COMMAND_LIVE;
	}
	const char* pStart = pGet;
	if(*pStart == 'b' || *pStart == 'p')
	{
		S.nWSWasConnected = 1; //do not load default when url has one specified.
		return EMICROPROFILE_GET_COMMAND_LIVE;
	}
	if(*pStart == 'r') // range
	{
		//very very manual parsing
		if('/' != *++pStart)
			return EMICROPROFILE_GET_COMMAND_UNKNOWN;
		++pStart;
		
		char* pEnd = nullptr;
		uint64_t nFrameStart = strtoll(pStart, &pEnd, 10);
		if(pEnd == pStart || *pEnd != '/' || *pEnd == '\0')
		{
			return EMICROPROFILE_GET_COMMAND_UNKNOWN;
		}
		pStart = pEnd + 1;
		
		uint64_t nFrameEnd = strtoll(pStart, &pEnd, 10);

		if(pEnd == pStart || nFrameEnd <= nFrameStart)
		{
			return EMICROPROFILE_GET_COMMAND_UNKNOWN;
		}
		pResult->nFrames = nFrameEnd - nFrameStart;
		pResult->nFrameStart = nFrameStart;
		return EMICROPROFILE_GET_COMMAND_DUMP_RANGE;
	}
	while(*pGet != '\0')
	{
		if(*pGet < '0' || *pGet > '9')
			return EMICROPROFILE_GET_COMMAND_UNKNOWN;
		pGet++;
	}
	int nFrames = atoi(pStart);
	pResult->nFrameStart = (uint64_t)-1;
	if(nFrames)
	{
		pResult->nFrames = nFrames;
	}
	else
	{
		pResult->nFrames = MICROPROFILE_WEBSERVER_MAXFRAMES;
	}
	return EMICROPROFILE_GET_COMMAND_DUMP;
}




void MicroProfileBase64Encode(char* pOut, const uint8_t* pIn, uint32_t nLen)
{
	static const char* CODES ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	//..straight from wikipedia.
	int b;
	char* o = pOut;
	for(uint32_t i = 0; i < nLen; i += 3)
	{
		b = (pIn[i] & 0xfc) >> 2;
		*o++ = CODES[b];
		b = (pIn[i] & 0x3) << 4;
		if(i + 1 < nLen)
		{
			b |= (pIn[i + 1] & 0xF0) >> 4;
			*o++ = CODES[b];
            b = (pIn[i + 1] & 0x0F) << 2;
			if (i + 2 < nLen)
			{
				b |= (pIn[i + 2] & 0xC0) >> 6;
				*o++ = CODES[b];
				b = pIn[i + 2] & 0x3F;
				*o++ = CODES[b];
			} 
			else
			{
				*o++ = CODES[b];
				*o++ = '=';
			}
		}
		else
		{
			*o++ = CODES[b];
			*o++ = '=';
			*o++ = '=';
		}
	}
}

//begin: SHA-1 in C
//ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
//SHA-1 in C
//By Steve Reid <steve@edmweb.com>
//100% Public Domain

typedef struct {
	uint32_t state[5];
	uint32_t count[2];
	unsigned char buffer[64];
} MicroProfile_SHA1_CTX;
#include <string.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

static void MicroProfile_SHA1_Transform(uint32_t[5], const unsigned char[64]);

#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define blk0(i) (block->l[i] = htonl(block->l[i]))
#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] \
    ^block->l[(i+2)&15]^block->l[i&15],1))

#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);


// Hash a single 512-bit block. This is the core of the algorithm. 

static void MicroProfile_SHA1_Transform(uint32_t state[5], const unsigned char buffer[64])
{
    uint32_t a, b, c, d, e;
    typedef union {
	unsigned char c[64];
	uint32_t l[16];
    } CHAR64LONG16;
    CHAR64LONG16 *block;

    block = (CHAR64LONG16 *) buffer;
    // Copy context->state[] to working vars
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    // 4 rounds of 20 operations each. Loop unrolled. 
    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
    // Add the working vars back into context.state[] 
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    // Wipe variables 
    a = b = c = d = e = 0;
}


void MicroProfile_SHA1_Init(MicroProfile_SHA1_CTX *context)
{
    // SHA1 initialization constants 
    context->state[0] = 0x67452301;
    context->state[1] = 0xEFCDAB89;
    context->state[2] = 0x98BADCFE;
    context->state[3] = 0x10325476;
    context->state[4] = 0xC3D2E1F0;
    context->count[0] = context->count[1] = 0;
}


// Run your data through this. 

void MicroProfile_SHA1_Update(MicroProfile_SHA1_CTX *context, const unsigned char *data, unsigned int len)
{
    unsigned int i, j;

    j = (context->count[0] >> 3) & 63;
    if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
    context->count[1] += (len >> 29);
    i = 64 - j;
    while (len >= i) {
		memcpy(&context->buffer[j], data, i);
		MicroProfile_SHA1_Transform(context->state, context->buffer);
		data += i;
		len -= i;
		i = 64;
		j = 0;
    }

    memcpy(&context->buffer[j], data, len);
}


// Add padding and return the message digest.

void MicroProfile_SHA1_Final(unsigned char digest[20], MicroProfile_SHA1_CTX *context)
{
    uint32_t i, j;
    unsigned char finalcount[8];

    for (i = 0; i < 8; i++) {
        finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)]
         >> ((3-(i & 3)) * 8) ) & 255);  // Endian independent
    }
    MicroProfile_SHA1_Update(context, (unsigned char *) "\200", 1);
    while ((context->count[0] & 504) != 448) {
		MicroProfile_SHA1_Update(context, (unsigned char *) "\0", 1);
    }
    MicroProfile_SHA1_Update(context, finalcount, 8);  // Should cause a SHA1Transform()
    for (i = 0; i < 20; i++) {
		digest[i] = (unsigned char)((context->state[i>>2] >> ((3-(i & 3)) * 8) ) & 255);
    }
    // Wipe variables
    i = j = 0;
    memset(context->buffer, 0, 64);
    memset(context->state, 0, 20);
    memset(context->count, 0, 8);
    memset(&finalcount, 0, 8);
}


#undef rol
#undef blk0
#undef blk
#undef R0
#undef R1
#undef R2
#undef R3
#undef R4

//end: SHA-1 in C


void MicroProfileWebSocketSendState(MpSocket C);
void MicroProfileWebSocketSendEnabled(MpSocket C);
void WSPrintStart(MpSocket C);
void WSPrintf(const char* pFmt, ...);
void WSPrintEnd();
void WSFlush();
bool MicroProfileWebSocketReceive(MpSocket C);


enum
{
	TYPE_NONE = 0,
	TYPE_TIMER = 1,
	TYPE_GROUP = 2,
	TYPE_CATEGORY = 3,
	TYPE_SETTING = 4, 
	TYPE_COUNTER = 5, 
};

enum
{
	SETTING_FORCE_ENABLE = 0,
	SETTING_CONTEXT_SWITCH_TRACE = 1,
	SETTING_PLATFORM_MARKERS = 2, 
};

enum
{
	MSG_TIMER_TREE=1,
	MSG_ENABLED = 2,
	MSG_FRAME = 3,
	MSG_LOADSETTINGS = 4, 
	MSG_PRESETS = 5, 
	MSG_CURRENTSETTINGS = 6, 
	MSG_COUNTERS = 7,
};

enum
{
	VIEW_GRAPH = 0,
	VIEW_BAR = 1,
	VIEW_COUNTERS = 2,
	VIEW_SINGLE= 3

};
void MicroProfileSocketDumpState()
{
	fd_set Read, Write, Error;
	FD_ZERO(&Read);
	FD_ZERO(&Write);
	FD_ZERO(&Error);
	MpSocket LastSocket = 1;
	for(uint32_t i = 0; i < S.nNumWebSockets; ++i)
	{
		LastSocket = MicroProfileMax(LastSocket, S.WebSockets[i]+1);
		FD_SET(S.WebSockets[i], &Read);
		FD_SET(S.WebSockets[i], &Write);
		FD_SET(S.WebSockets[i], &Error);
	}
	timeval tv;
	tv.tv_sec = 0;
    tv.tv_usec = 0;

    if(-1 == select(LastSocket, &Read, &Write, &Error, &tv))
    {
    	MP_ASSERT(0);
    }
	for(uint32_t i = 0; i < S.nNumWebSockets; i++)
	{
		MpSocket s = S.WebSockets[i];
		printf("%" PRId64 " ", (uint64_t)s);

		if(FD_ISSET(s, &Error))
		{
			printf("e");
		}
		else
		{
			printf("_");

		}
		if(FD_ISSET(s, &Read))
		{
			printf("r");
		}
		else
		{
			printf(" ");
		}
		if(FD_ISSET(s, &Write))
		{
			printf("w");
		}
		else
		{
			printf(" ");
		}
	}
	printf("\n");
	for(uint32_t i = 1; i < S.nNumWebSockets; i++)
	{
#ifndef _WINDOWS ///windowssocket
		MpSocket s = S.WebSockets[i];
		int error_code;
		socklen_t error_code_size = sizeof(error_code);
		int r = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size);
		MP_ASSERT(r >= 0);
		if (error_code != 0) {
		    /* socket has a non zero error status */
		    fprintf(stderr, "socket error: %d %s\n", (int)s, strerror(error_code));
		    MP_ASSERT(0);
		}
#endif

	}

}

void MicroProfileSleep(uint32_t nMs)
{
#ifdef _WIN32
	Sleep(nMs);
#else
	usleep(nMs * 1000);
#endif
}

bool MicroProfileSocketSend2(MpSocket Connection, const void* pMessage, int nLen);
void* MicroProfileSocketSenderThread(void*)
{
	MicroProfileOnThreadCreate("MicroProfileSocketSenderThread");
	while(!S.nMicroProfileShutdown)
	{
		if(S.nSocketFail)
		{
			MicroProfileSleep(100);
			continue;
		}
		MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSocketSendLoop", MP_GREEN4);
		uint32_t nEnd = MICROPROFILE_WEBSOCKET_BUFFER_SIZE;
		uint32_t nGet = S.WSBuf.nSendGet.load();
		uint32_t nPut = S.WSBuf.nSendPut.load();
		uint32_t nSendStart = 0;
		uint32_t nSendAmount = 0;
		if(nGet > nPut)
		{
			nSendStart = nGet;
			nSendAmount = nEnd - nGet;
		}
		else if(nGet < nPut)
		{
			nSendStart = nGet;
			nSendAmount = nPut - nGet;
		}


		if(nSendAmount)
		{
			if(!MicroProfileSocketSend2(S.WebSockets[0], &S.WSBuf.SendBuffer[nSendStart], nSendAmount))
			{
				S.nSocketFail = 1;
			}
			else
			{
				S.WSBuf.nSendGet.store( (nGet + nSendAmount) % MICROPROFILE_WEBSOCKET_BUFFER_SIZE);
			}
		}
		else
		{
			MicroProfileSleep(20);
		}

	}
	return 0;
}

void MicroProfileSocketSend(MpSocket Connection, const void* pMessage, int nLen)
{
	if(S.nSocketFail || nLen <= 0)
	{
		return;
	}
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSocketSend", MP_GREEN4);
	while(nLen != 0)
	{
		MP_ASSERT(nLen > 0);
		uint32_t nEnd = MICROPROFILE_WEBSOCKET_BUFFER_SIZE;
		uint32_t nGet = S.WSBuf.nSendGet.load();
		uint32_t nPut = S.WSBuf.nSendPut.load();
		uint32_t nAmount = 0;
		if(nPut < nGet)
		{
			nAmount = nGet - nPut - 1;
		}
		else
		{
			if(nGet == 0)
			{
				nAmount = nEnd - nPut - 1;
			}
			else
			{
				nAmount = nEnd - nPut;
			}
		}
		MP_ASSERT((int)nAmount >= 0);
		nAmount = MicroProfileMin(nLen, (int)nAmount);
		if(nAmount)
		{
			memcpy(&S.WSBuf.SendBuffer[nPut], pMessage, nAmount);
			pMessage = (void*) ((char*)pMessage + nAmount);
			nLen -= nAmount;
			S.WSBuf.nSendPut.store( (nPut+nAmount) % MICROPROFILE_WEBSOCKET_BUFFER_SIZE);
		}
		else
		{
			MicroProfileSleep(20);
		}



	}
}

bool MicroProfileSocketSend2(MpSocket Connection, const void* pMessage, int nLen)
{
	if(S.nSocketFail || nLen <= 0)
	{
		return false;
	}
	//MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSocketSend2", 0);
#ifndef _WIN32 
	int error_code;
	socklen_t error_code_size = sizeof(error_code);
	getsockopt(Connection, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
	if(error_code != 0)
	{
    	return false;
	}
#endif

	int s = 0;
	s = send(Connection, (const char*)pMessage, nLen, 0);
#ifdef _WIN32
	if(s == SOCKET_ERROR)
	{
		return false;
	}
#endif
	if(s < 0)
	{
		return false;
	}
	MP_ASSERT(s == nLen);
	return true;
}

uint32_t MicroProfileWebSocketIdPack(uint32_t type, uint32_t element)
{
	MP_ASSERT(type < 255);
	MP_ASSERT(element < 0xffffff);
	return type << 24 | element;
}
void MicroProfileWebSocketIdUnpack(uint32_t nPacked, uint32_t& type, uint32_t& element)
{
	type = (nPacked >> 24) & 0xff;
	element = nPacked & 0xffffff;
}


struct MicroProfileWebSocketHeader0
{
	union
	{
		struct
		{
			uint8_t opcode  : 4;
			uint8_t RSV3 : 1;
			uint8_t RSV2 : 1;
			uint8_t RSV1 : 1;
			uint8_t FIN  : 1;
		};
		uint8_t v;
	};
};

struct MicroProfileWebSocketHeader1
{
	union
	{
		struct
		{
			uint8_t payload : 7;
			uint8_t MASK  : 1;
		};
		uint8_t v;
	};
};


bool MicroProfileWebSocketSend(MpSocket Connection, const char* pMessage, uint64_t nLen)
{
	MicroProfileWebSocketHeader0 h0;
	MicroProfileWebSocketHeader1 h1;
	h0.v = 0;
	h1.v = 0;
	h0.opcode = 1;
	h0.FIN = 1;
	uint32_t nExtraSizeBytes = 0;
	uint8_t nExtraSize[8];
	if(nLen > 125)
	{
		if(nLen > 0xffff)
		{
			nExtraSizeBytes = 8;
			h1.payload = 127;
		}
		else
		{
			h1.payload = 126;
			nExtraSizeBytes = 2;
		}
		uint64_t nCount = nLen;
		for(uint32_t i = 0; i < nExtraSizeBytes; ++i)
		{
			nExtraSize[nExtraSizeBytes-i-1] = nCount & 0xff;
			nCount >>= 8;
		}

		uint32_t nSize = 0;
		for(uint32_t i = 0; i < nExtraSizeBytes; i++)
		{
			nSize <<= 8;
			nSize += nExtraSize[i];			
		}
		MP_ASSERT(nSize == nLen); // verify
	}
	else
	{
		h1.payload = nLen;
	}
	MP_ASSERT(pMessage == &S.WSBuf.Buffer[0]); //space for header is preallocated here
	MP_ASSERT(nExtraSizeBytes < 18);
	char* pTmp = (char*)(pMessage - nExtraSizeBytes - 2);
	memcpy(pTmp+2, &nExtraSize[0], nExtraSizeBytes);
	pTmp[1] = *(char*)&h1;
	pTmp[0] = *(char*)&h0;
	//MicroProfileSocketSend(Connection, pTmp, nExtraSizeBytes + 2 + nLen);
	#if 1
	MicroProfileSocketSend(Connection, &h0, 1);
	MicroProfileSocketSend(Connection, &h1, 1);
	if(nExtraSizeBytes)
	{
		MicroProfileSocketSend(Connection, &nExtraSize[0], nExtraSizeBytes);
	}
	MicroProfileSocketSend(Connection, pMessage, nLen);
	#endif
	return true;

}

void MicroProfileWebSocketClearTimers()
{
	while(S.WebSocketTimers != -1)
	{
		int nNext = S.TimerInfo[S.WebSocketTimers].nWSNext;
		S.TimerInfo[S.WebSocketTimers].bWSEnabled = false;
		S.TimerInfo[S.WebSocketTimers].nWSNext = -1;
		S.WebSocketTimers = nNext;
	}

	S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
}
void MicroProfileToggleWebSocketToggleTimer(uint32_t nTimer)
{
	if(nTimer < S.nTotalTimers)
	{
		auto& TI = S.TimerInfo[nTimer];
		int* pPrev = &S.WebSocketTimers;
		while(*pPrev != -1 && *pPrev != (int)nTimer)
		{
			MP_ASSERT(*pPrev < (int)S.nTotalTimers && *pPrev >= 0);
			pPrev = &S.TimerInfo[*pPrev].nWSNext;
		}
		if(TI.bWSEnabled)
		{
			MP_ASSERT(*pPrev == (int)nTimer);
			*pPrev = TI.nWSNext;
			TI.nWSNext = -1;
			TI.bWSEnabled = false;
		}
		else
		{
			MP_ASSERT(*pPrev == -1);
			TI.nWSNext = -1;
			*pPrev = (int)nTimer;
			TI.bWSEnabled = true;
		}
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
	}


}
bool MicroProfileWebSocketTimerEnabled(uint32_t nTimer)
{
	if(nTimer < S.nTotalTimers)
	{
		return S.TimerInfo[nTimer].bWSEnabled;
	}
	return false;
}
void MicroProfileToggleGroup(uint32_t nGroup)
{
	if(nGroup < S.nGroupCount)
	{
		S.nActiveGroupWanted ^= (1ll << nGroup);
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;

	}
}
bool MicroProfileGroupEnabled(uint32_t nGroup)
{
	if(nGroup < S.nGroupCount)
	{
		return 0 != (S.nActiveGroupWanted & (1ll << nGroup));
	}
	return false;
}
bool MicroProfileCategoryEnabled(uint32_t nCategory)
{
	if(nCategory < S.nCategoryCount)
	{
		uint64_t nGroupMask = S.CategoryInfo[nCategory].nGroupMask;
		return nGroupMask == (nGroupMask & S.nActiveGroupWanted);
	}
	return false;

}
void MicroProfileToggleCategory(uint32_t nCategory)
{
	if(nCategory < S.nCategoryCount)
	{
		uint64_t nGroupMask = S.CategoryInfo[nCategory].nGroupMask;
		if(nGroupMask != (nGroupMask & S.nActiveGroupWanted))
		{
			S.nActiveGroupWanted |= nGroupMask;
		}
		else
		{
			S.nActiveGroupWanted &= ~nGroupMask;
		}
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
	}

}
void MicroProfileWebSocketCommand(uint32_t nCommand)
{
	uint32_t nType, nElement;
	MicroProfileWebSocketIdUnpack(nCommand, nType, nElement);
	switch(nType)
	{
	case TYPE_NONE:
		break;
	case TYPE_SETTING:
		switch(nElement)
		{
		case SETTING_FORCE_ENABLE:
			MicroProfileSetEnableAllGroups(!MicroProfileGetEnableAllGroups());
			break;
		case SETTING_CONTEXT_SWITCH_TRACE:
		    if(!S.bContextSwitchRunning)
		    {
		    	MicroProfileStartContextSwitchTrace();
		    }
		    else
		    {
		    	MicroProfileStopContextSwitchTrace();
		    }
			break;
		case SETTING_PLATFORM_MARKERS:
			MicroProfilePlatformMarkersSetEnabled(!MicroProfilePlatformMarkersGetEnabled());
			break;
		}
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
		break;
	case TYPE_TIMER:
		MicroProfileToggleWebSocketToggleTimer(nElement);
		break;
	case TYPE_GROUP:
		MicroProfileToggleGroup(nElement);
		break;
	case TYPE_CATEGORY:
		MicroProfileToggleCategory(nElement);
		break;
	default:
		printf("unknown type %d\n", nType);

	}
}
#define MICROPROFILE_PRESET_HEADER_MAGIC2 0x28586813
#define MICROPROFILE_PRESET_HEADER_VERSION2 0x00000200

struct MicroProfileSettingsFileHeader
{
	uint32_t nMagic;
	uint32_t nVersion;
	uint32_t nNumHeaders;
	uint32_t nHeadersOffset;
	uint32_t nMaxJsonSize;
	uint32_t nMaxNameSize;
};
struct MicroProfileSettingsHeader
{
	uint32_t nJsonOffset;
	uint32_t nJsonSize;
	uint32_t nNameOffset;
	uint32_t nNameSize;
};

typedef bool (*MicroProfileOnSettings)(const char* pName, uint32_t nNameLen, const char* pJson, uint32_t nJson);


#ifndef MICROPROFILE_SETTINGS_FILE_PATH
#define MICROPROFILE_SETTINGS_FILE_PATH ""
#endif
#define MICROPROFILE_SETTINGS_FILE MICROPROFILE_SETTINGS_FILE_PATH "mppresets.cfg"
#define MICROPROFILE_SETTINGS_FILE_BUILTIN MICROPROFILE_SETTINGS_FILE_PATH "mppresets.builtin.cfg"
#define MICROPROFILE_SETTINGS_FILE_TEMP MICROPROFILE_SETTINGS_FILE ".tmp"

template<typename T>
void MicroProfileParseSettings(const char* pFileName, T CB)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());


	FILE* F = fopen(pFileName, "rb");
	if(!F)
	{
		return;
	}
	long nFileSize = 0;
	fseek(F, 0, SEEK_END);
	nFileSize = ftell(F);
	if(nFileSize > (32<<10))
	{
		printf("trying to load a >32kb settings file on the stack. this should never happen!\n");
		MP_BREAK();
	}
	char* pFile = (char*)alloca(nFileSize+1);
	fseek(F, 0, SEEK_SET);
	if(1 != fread(pFile, nFileSize, 1, F))
	{
		printf("failed to read settings file\n");
		fclose(F);
		return;
	}
	fclose(F);
	pFile[nFileSize] = '\0';

	char* pPos = pFile;
	char* pEnd = pFile + nFileSize;

	while(pPos != pEnd)
	{
		const char* pName = 0;
		int nNameLen = 0;
		const char* pJson = 0;
		int nJsonLen = 0;
		int Failed = 0;

		auto SkipWhite = [&](char* pPos, const char* pEnd)
		{
			while(pPos != pEnd)
			{
				if(isspace(*pPos))
				{
					pPos++;
				}
				else if('#' == *pPos)
				{
					while(pPos != pEnd && *pPos != '\n')
					{
						++pPos;
					}
				}
				else
				{
					break;
				}
			}
			return pPos;
		};

		auto ParseName = [&](char* pPos, char* pEnd, const char** ppName, int* pLen)
		{
			pPos = SkipWhite(pPos, pEnd);
			int nLen = 0;
			*ppName = pPos;

			while(pPos != pEnd && (isalpha(*pPos) || isdigit(*pPos)))
			{
				nLen++;
				pPos++;
			}
			*pLen = nLen;
			if(pPos == pEnd || !isspace(*pPos))
			{
				Failed = 1;
				return pEnd;
			}
			*pPos++ = '\0';
			return pPos;
		};

		auto ParseJson = [&](char* pPos, char* pEnd, const char** pJson, int* pLen) -> char*
		{
			pPos = SkipWhite(pPos, pEnd);
			if(*pPos != '{' || pPos == pEnd)
			{
				Failed = 1;
				return pPos;
			}
			*pJson = pPos++;
			int nLen = 1;
			int nDepth = 1;
			while(pPos != pEnd && nDepth != 0)
			{
				nLen++;
				char nChar = *pPos++;
				if(nChar == '{')
				{
					nDepth++;
				}
				else if(nChar == '}')
				{
					nDepth--;
				}
			}
			if(pPos == pEnd || !isspace(*pPos))
			{
				Failed = 1;
				return pEnd;
			}
			*pLen = nLen;
			*pPos++ = '\0';		
			return pPos;
		};

		pPos = ParseName(pPos, pEnd, &pName, &nNameLen);
		pPos = ParseJson(pPos, pEnd, &pJson, &nJsonLen);
		if(Failed)
		{
			break;
		}
		if(!CB(pName, nNameLen, pJson, nJsonLen))
		{
			break;
		}
	}
}

bool MicroProfileSavePresets(const char* pSettingsName, const char* pJsonSettings)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());

	FILE* F = fopen(MICROPROFILE_SETTINGS_FILE_TEMP, "w");
	if(!F)
	{
		return false;
	}

	bool bWritten = false;

	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE,
		[&](const char* pName, uint32_t nNameSize, const char* pJson, uint32_t nJsonSize) -> bool
		{
			fwrite(pName, nNameSize, 1, F);
			fputc(' ', F);
			if(0 != MP_STRCASECMP(pSettingsName, pName))
			{
				fwrite(pJson, nJsonSize, 1, F);
			}
			else
			{
				bWritten = true;
				fwrite(pJsonSettings, strlen(pJsonSettings), 1, F);
			}
			fputc('\n', F);
			return true;
		}
	);
	if(!bWritten)
	{
		fwrite(pSettingsName, strlen(pSettingsName), 1, F);
		fputc(' ', F);
		fwrite(pJsonSettings, strlen(pJsonSettings), 1, F);
		fputc('\n', F);
	}
	fflush(F);
	fclose(F);
#ifdef MICROPROFILE_MOVE_FILE
	MICROPROFILE_MOVE_FILE(MICROPROFILE_SETTINGS_FILE_TEMP, MICROPROFILE_SETTINGS_FILE);
#elif defined(_WIN32)
	MoveFileEx(MICROPROFILE_SETTINGS_FILE_TEMP, MICROPROFILE_SETTINGS_FILE, MOVEFILE_REPLACE_EXISTING);
#else
	rename(MICROPROFILE_SETTINGS_FILE_TEMP, MICROPROFILE_SETTINGS_FILE);
#endif
	return false;
}


void MicroProfileWebSocketSendPresets(MpSocket Connection)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());
	WSPrintStart(Connection);
	WSPrintf("{\"k\":\"%d\",\"v\":{", MSG_PRESETS);
	WSPrintf("\"p\":[\"Default\"");
	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE,
		[](const char* pName, uint32_t nNameLen, const char* pJson, uint32_t nJsonLen)
		{
			WSPrintf(",\"%s\"", pName);
			return true;
		}
	);
	WSPrintf("],\"r\":[");
	bool bFirst = true;
	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE_BUILTIN,
		[&bFirst](const char* pName, uint32_t nNameLen, const char* pJson, uint32_t nJsonLen)
		{
			WSPrintf("%c\"%s\"", bFirst ? ' ': ',', pName);
			bFirst = false;
			return true;
		}
	);
	WSPrintf("]}}");
	WSFlush();
	WSPrintEnd();
}

void MicroProfileLoadPresets(const char* pSettingsName, bool bReadOnlyPreset)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());	
	const char* pPresetFile = bReadOnlyPreset ? MICROPROFILE_SETTINGS_FILE_BUILTIN : MICROPROFILE_SETTINGS_FILE;
	MicroProfileParseSettings(pPresetFile,
		[=](const char* pName, uint32_t l0, const char* pJson, uint32_t l1)
		{
			if(0 == MP_STRCASECMP(pName, pSettingsName))
			{
				uint32_t nLen = (uint32_t)strlen(pJson)+1;
				if(nLen > S.nJsonSettingsBufferSize)
				{
					S.pJsonSettings = (char*)MICROPROFILE_REALLOC(S.pJsonSettings, nLen);
					S.nJsonSettingsBufferSize = nLen;
				}
				memcpy(S.pJsonSettings, pJson, nLen);
				S.nJsonSettingsPending = 1;
				S.bJsonSettingsReadOnly = bReadOnlyPreset ? 1 : 0;
				return false;
			}
			return true;
		}
	);
}






bool MicroProfileWebSocketReceive(MpSocket Connection)
{

     //  0                   1                   2                   3
     //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     // +-+-+-+-+-------+-+-------------+-------------------------------+
     // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     // | |1|2|3|       |K|             |                               |
     // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	int r;
	uint64_t nSize;
	uint64_t nSizeBytes = 0;
	uint8_t Mask[4];
	static unsigned char* Bytes = 0;
	static char BytesAllocated = 0;
	MicroProfileWebSocketHeader0 h0;
	MicroProfileWebSocketHeader1 h1;
	static_assert(sizeof(h0) == 1, "");
	static_assert(sizeof(h1) == 1, "");
	r = recv(Connection, (char*)&h0, 1, 0);
	if(1 != r)
		goto fail;
	r = recv(Connection, (char*)&h1, 1, 0);
	if(1 != r)
		goto fail;

	if(h0.v == 0x88)
	{
		goto fail;
	}

	if(h0.RSV1 != 0 || h0.RSV2 != 0 || h0.RSV3 != 0)
		goto fail;

	nSize = h1.payload;
	nSizeBytes = 0;
	switch(nSize)
	{
		case 126: 
			nSizeBytes = 2;
			break;
		case 127:
			nSizeBytes = 8;
			break;
		default:
			break;
	}
	if(nSizeBytes)
	{
		nSize = 0;
        uint64_t MessageLength = 0;

		uint8_t BytesMessage[8];
		r = recv(Connection, (char*)&BytesMessage[0], nSizeBytes, 0);
		if(nSizeBytes != r)
			goto fail;
		for(uint32_t i = 0; i < nSizeBytes; i++)
		{
			nSize <<= 8;
			nSize += BytesMessage[i];			
		}

        for(uint32_t i = 0; i < nSizeBytes; i++)
            MessageLength |= BytesMessage[i] << ((nSizeBytes - 1 - i) * 8);
        MP_ASSERT(MessageLength == nSize);
	}

	if(h1.MASK)
	{
		recv(Connection, (char*)&Mask[0], 4, 0);
	}


	if(nSize+1 > BytesAllocated)
	{
		Bytes = (unsigned char*)MICROPROFILE_REALLOC(Bytes, nSize+1);
		BytesAllocated = nSize + 1;
	}
	recv(Connection, (char*)Bytes, nSize, 0);
	for(uint32_t i = 0; i < nSize; ++i)
		Bytes[i] ^= Mask[i&3];

	Bytes[nSize] = '\0';
	switch(Bytes[0])
	{
		case 'a':
			{
				S.nAggregateFlip = strtoll((const char*)&Bytes[1], nullptr, 10);
			}
			break;
		case 's':
			{
				char* pJson = strchr((char*)Bytes, ',');
				if(pJson && *pJson != '\0')
				{
					*pJson = '\0';
					MicroProfileSavePresets((const char*)Bytes+1, (const char*)pJson+1);
				}
				break;
			}

		case 'l':
			{
				MicroProfileLoadPresets((const char*)Bytes+1, 0);
				break;
			}
		case 'm':
			{
				MicroProfileLoadPresets((const char*)Bytes+1, 1);
				break;
			}
		case 'd':
			{
				MicroProfileWebSocketClearTimers();
				S.nActiveGroupWanted = 0;
				S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
			}
		case 'c':
			{
				char* pStr = (char*)Bytes + 1;
				char* pEnd = pStr + nSize - 1;
				uint32_t Message = strtol(pStr, &pEnd, 10);
				MicroProfileWebSocketCommand(Message);
			}
			break;
		case 'f':
			MicroProfileToggleFrozen();
			break;
		case 'v':
			S.nWSViewMode = (int)Bytes[1]-'0';
			break;
		case 'r':
			printf("got clear message\n");
			S.nAggregateClear = 1;
			break;
		case 'x':
			MicroProfileWebSocketClearTimers();
			break;
		default:
			printf("got unknown message size %lld: '%s'\n", (long long)nSize, Bytes);
	}
	return true;

fail:
	return false;

}
void MicroProfileWebSocketSendPresets(MpSocket Connection);


void MicroProfileWebSocketHandshake(MpSocket Connection, char* pWebSocketKey)
{
	//reset web socket buffer
	S.WSBuf.nSendPut.store(0);
	S.WSBuf.nSendGet.store(0);
	S.nSocketFail = 0;
	//if(!S.WebSocketThreadRunning)
	//{
 //       MicroProfileThreadStart(&S.WebSocketSendThread, MicroProfileSocketSenderThread);
	//	S.WebSocketThreadRunning = 1;
	//}

	const char* pGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char* pHandShake = "HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n" 
	"Connection: Upgrade\r\n"
	"Sec-WebSocket-Accept: ";

	char EncodeBuffer[512];
	int nLen = snprintf(EncodeBuffer, sizeof(EncodeBuffer)-1, "%s%s", pWebSocketKey, pGUID);
	//printf("encode buffer is '%s' %d, %d\n", EncodeBuffer, nLen, (int)strlen(EncodeBuffer));

	uint8_t sha[20];
	MicroProfile_SHA1_CTX ctx;
    MicroProfile_SHA1_Init(&ctx);
 	MicroProfile_SHA1_Update(&ctx, (unsigned char*)EncodeBuffer, nLen);
	MicroProfile_SHA1_Final((unsigned char*)&sha[0], &ctx);
	char HashOut[(2+sizeof(sha)/3)*4];
	memset(&HashOut[0], 0, sizeof(HashOut));
	MicroProfileBase64Encode(&HashOut[0], &sha[0], sizeof(sha));

	char Reply[11024];
	nLen = snprintf(Reply, sizeof(Reply)-1, "%s%s\r\n\r\n", pHandShake, HashOut);;
	MP_ASSERT(nLen >= 0);
	MicroProfileSocketSend(Connection, Reply, nLen);
	S.WebSockets[S.nNumWebSockets++] = Connection;


	S.WSCategoriesSent = 0;
	S.WSGroupsSent = 0;
	S.WSTimersSent = 0;
	S.WSCountersSent = 0;
	S.nJsonSettingsPending = 0;

	MicroProfileWebSocketSendState(Connection);
	MicroProfileWebSocketSendPresets(Connection);
	if(!S.nWSWasConnected)
	{
		S.nWSWasConnected = 1;
		MicroProfileLoadPresets("Default", 0);
	}
	else
	{
		if(S.pJsonSettings)
		{
			WSPrintStart(Connection);
			WSPrintf("{\"k\":\"%d\",\"ro\":%d,\"v\":%s}", MSG_CURRENTSETTINGS, S.bJsonSettingsReadOnly ? 1 : 0, S.pJsonSettings);
			WSFlush();
			WSPrintEnd();
		}
	}

}

void MicroProfileWebSocketSendCounters()
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileWebSocketSendCounters", MP_GREEN4);
	if(S.nWSViewMode == VIEW_COUNTERS)
	{
		WSPrintf("{\"k\":\"%d\",\"v\":[", MSG_COUNTERS);
		for(uint32_t i = 0; i < S.nNumCounters; ++i)
		{
			uint64_t nCounter = S.Counters[i].load();
			WSPrintf("%c%lld", i == 0 ? ' ' : ',', nCounter);
		}
		WSPrintf("]}");
		WSFlush();
	}
}

void MicroProfileWebSocketSendFrame(MpSocket Connection)
{
	if(S.nFrameCurrent != S.WebSocketFrameLast[0] || S.nFrozen)
	{
		MicroProfileWebSocketSendState(Connection);
		MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileWebSocketSendFrame", MP_GREEN4);
		WSPrintStart(Connection);
		float fTickToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		float fTickToMsGpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
		MicroProfileFrameState* pFrameCurrent = &S.Frames[S.nFrameCurrent];
		MicroProfileFrameState* pFrameNext = &S.Frames[S.nFrameNext];

		uint64_t nFrameTicks = pFrameNext->nFrameStartCpu - pFrameCurrent->nFrameStartCpu;
		uint64_t nFrame = pFrameCurrent->nFrameId;
		double fTime = nFrameTicks * fTickToMsCpu;
		WSPrintf("{\"k\":\"%d\",\"v\":{\"t\":%f,\"f\":%lld,\"a\":%d,\"fr\":%d,\"m\":%d", MSG_FRAME, fTime, nFrame, MicroProfileGetCurrentAggregateFrames(), S.nFrozen, S.nWSViewMode);
		if(S.nFrameCurrent != S.WebSocketFrameLast[0])
		{
			WSPrintf(",\"x\":{");
			int nTimer = S.WebSocketTimers;
			while(-1 != nTimer)
			{
				MicroProfileTimerInfo& TI = S.TimerInfo[nTimer];
				float fTickToMs = TI.Type == MicroProfileTokenTypeGpu ? fTickToMsGpu : fTickToMsCpu;
				uint32_t id = MicroProfileWebSocketIdPack(TYPE_TIMER, nTimer);
				fTime = fTickToMs * S.Frame[nTimer].nTicks;
				float fCount = (float)S.Frame[nTimer].nCount;
				float fTimeExcl = fTickToMs * S.FrameExclusive[nTimer];
				if(0 == (S.nActiveGroup & (1llu << TI.nGroupIndex)))
				{
					fTime = fCount = fTimeExcl = 0.f;
				}
				nTimer = TI.nWSNext;
				WSPrintf("\"%d\":[%f,%f,%f]%c", id, fTime, fTimeExcl, fCount, nTimer == -1 ? ' ' : ',');
			}
			WSPrintf("}");
		}
		WSPrintf("}}");
		WSFlush();
		MicroProfileWebSocketSendCounters();
		WSPrintEnd();
		S.WebSocketFrameLast[0] = S.nFrameCurrent;
	}
}





void MicroProfileWebSocketFrame()
{
	if (!S.nNumWebSockets)
	{
		return;
	}
	MICROPROFILE_SCOPEI("MicroProfile", "Websocket-update", MP_GREEN4);
	fd_set Read, Write, Error;
	FD_ZERO(&Read);
	FD_ZERO(&Write);
	FD_ZERO(&Error);
	MpSocket LastSocket = 1;
	for(uint32_t i = 0; i < S.nNumWebSockets; ++i)
	{
		LastSocket = MicroProfileMax(LastSocket, S.WebSockets[i]+1);
		FD_SET(S.WebSockets[i], &Read);
		FD_SET(S.WebSockets[i], &Write);
		FD_SET(S.WebSockets[i], &Error);
	}
	timeval tv;
	tv.tv_sec = 0;
    tv.tv_usec = 0;

    if(-1 == select(LastSocket, &Read, &Write, &Error, &tv))
    {
    	MP_ASSERT(0);
    }
	for(uint32_t i = 0; i < S.nNumWebSockets;)
	{
		MpSocket s = S.WebSockets[i];
		bool bConnected = true;
		if(FD_ISSET(s, &Error))
		{
			MP_ASSERT(0); // todo, remove & fix.
		}
		if(FD_ISSET(s, &Read))
		{
			bConnected = MicroProfileWebSocketReceive(s);
		}
		if(FD_ISSET(s, &Write))
		{
			if(S.nJsonSettingsPending)
			{
				WSPrintStart(s);
				WSPrintf("{\"k\":\"%d\",\"ro\":%d,\"v\":%s}", MSG_LOADSETTINGS, S.bJsonSettingsReadOnly ? 1 : 0, S.pJsonSettings);
				WSFlush();
				WSPrintEnd();
				S.nJsonSettingsPending = 0;
			}
			if(S.nWebSocketDirty)
			{
				MicroProfileFlipEnabled();
				MicroProfileWebSocketSendEnabled(s);
				S.nWebSocketDirty = 0;
			}	
			MicroProfileWebSocketSendFrame(s);	
		}
		if(S.nSocketFail)
		{
			bConnected = false;
		}
		S.nSocketFail = 0;

		if(!bConnected)
		{
			#if MICROPROFILE_DEBUG
			printf("removing socket %lld\n", (uint64_t)s);
			#endif

#ifndef _WIN32
        	shutdown(S.WebSockets[i], SHUT_WR);
#else
			shutdown(S.WebSockets[i], 1);
#endif
            char tmp[128];
            int r = 1;
            while(r > 0)
            {
                r = recv(S.WebSockets[i], tmp, sizeof(tmp), 0);
            }
            #ifdef _WIN32
			closesocket(S.WebSockets[i]);
			#else
			close(S.WebSockets[i]);
			#endif

			--S.nNumWebSockets;
			S.WebSockets[i] = S.WebSockets[S.nNumWebSockets];
			#if MICROPROFILE_DEBUG
			printf("done removing\n");
			#endif
		}
		else
		{
			++i;
		}
	}
	if(S.nWasFrozen)
	{
		S.nWasFrozen--;
	}
}

void WSPrintStart(MpSocket C)
{
	MP_ASSERT(S.WSBuf.Socket == 0);
	MP_ASSERT(S.WSBuf.nPut == 0);
	S.WSBuf.Socket = C;
}

void WSPrintf(const char* pFmt, ...) 
{
	va_list args;
	va_start (args, pFmt);

	MP_ASSERT(S.WSBuf.nPut < MICROPROFILE_WEBSOCKET_BUFFER_SIZE);
	uint32_t nSpace = MICROPROFILE_WEBSOCKET_BUFFER_SIZE - S.WSBuf.nPut - 1;
	char* pPut = &S.WSBuf.Buffer[S.WSBuf.nPut];
#ifdef _WIN32
	int nSize = (int)vsnprintf_s(pPut, nSpace, nSpace, pFmt, args);
#else
	int nSize = (int)vsnprintf(pPut, nSpace, pFmt, args);
#endif
	va_end(args);
	MP_ASSERT(nSize>=0);
	S.WSBuf.nPut += nSize;
}


void WSPrintEnd()
{
	MP_ASSERT(S.WSBuf.nPut == 0);
	S.WSBuf.Socket = 0;
}

void WSFlush()
{
	MP_ASSERT(S.WSBuf.Socket != 0);
	MP_ASSERT(S.WSBuf.nPut != 0);
	MicroProfileWebSocketSend(S.WSBuf.Socket, &S.WSBuf.Buffer[0], S.WSBuf.nPut);
	S.WSBuf.nPut = 0;
}
void MicroProfileWebSocketSendEnabledMessage(uint32_t id, int bEnabled)
{
	WSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"e\":%d}}", MSG_ENABLED, id, bEnabled?1:0);
	WSFlush();

}
void MicroProfileWebSocketSendEnabled(MpSocket C)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Websocket-SendEnabled", MP_GREEN4);
	WSPrintStart(C);
	for(uint32_t i = 0; i < S.nCategoryCount; ++i)
	{
		MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_CATEGORY, i), MicroProfileCategoryEnabled(i));
	}

	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_GROUP, i), MicroProfileGroupEnabled(i));
	}
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_TIMER, i), MicroProfileWebSocketTimerEnabled(i));
	}
	MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_FORCE_ENABLE), MicroProfileGetEnableAllGroups());
	MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_CONTEXT_SWITCH_TRACE), S.bContextSwitchRunning);
	MicroProfileWebSocketSendEnabledMessage(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_PLATFORM_MARKERS), MicroProfilePlatformMarkersGetEnabled());


	WSPrintEnd();
}
void MicroProfileWebSocketSendEntry(uint32_t id, uint32_t parent, const char* pName, int nEnabled, uint32_t nColor)
{
	WSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"pid\":%d,", MSG_TIMER_TREE, id, parent);
	WSPrintf("\"name\":\"%s\",", pName);
	WSPrintf("\"e\":%d,", nEnabled);	
	WSPrintf("\"color\":\"#%02x%02x%02x\"", MICROPROFILE_UNPACK_RED(nColor) & 0xff, MICROPROFILE_UNPACK_GREEN(nColor) & 0xff, MICROPROFILE_UNPACK_BLUE(nColor) & 0xff);
	WSPrintf("}}");
	WSFlush();
}

void MicroProfileWebSocketSendCounterEntry(uint32_t id, uint32_t parent, const char* pName, int64_t nLimit, int nFormat)
{
	WSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"pid\":%d,", MSG_TIMER_TREE, id, parent);
	WSPrintf("\"name\":\"%s\",", pName);
	WSPrintf("\"limit\":%d,", nLimit);	
	WSPrintf("\"format\":%d", nFormat);	
	WSPrintf("}}");
	WSFlush();
}

void MicroProfileWebSocketSendState(MpSocket C)
{
	if(S.WSCategoriesSent != S.nCategoryCount || S.WSGroupsSent != S.nGroupCount || S.WSTimersSent != S.nTotalTimers || S.WSCountersSent != S.nNumCounters)
	{
		WSPrintStart(C);
		uint32_t root = MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_FORCE_ENABLE);
		MicroProfileWebSocketSendEntry(root, 0, "All", MicroProfileGetEnableAllGroups(), (uint32_t)-1);
		for(uint32_t i = S.WSCategoriesSent; i < S.nCategoryCount; ++i)
		{
			MicroProfileCategory& CI = S.CategoryInfo[i];
			uint32_t id = MicroProfileWebSocketIdPack(TYPE_CATEGORY, i);
			uint32_t parent = root;
			MicroProfileWebSocketSendEntry(id, parent, CI.pName, MicroProfileCategoryEnabled(i), 0xffffffff);
		}

		for(uint32_t i = S.WSGroupsSent; i < S.nGroupCount; ++i)
		{
			MicroProfileGroupInfo& GI = S.GroupInfo[i];
			uint32_t id = MicroProfileWebSocketIdPack(TYPE_GROUP, i);
			uint32_t parent = MicroProfileWebSocketIdPack(TYPE_CATEGORY, GI.nCategory);
			MicroProfileWebSocketSendEntry(id, parent, GI.pName, MicroProfileGroupEnabled(i), GI.nColor);
		}

		for(uint32_t i = S.WSTimersSent; i < S.nTotalTimers; ++i)
		{
			MicroProfileTimerInfo& TI = S.TimerInfo[i];
			uint32_t id = MicroProfileWebSocketIdPack(TYPE_TIMER, i);
			uint32_t parent = MicroProfileWebSocketIdPack(TYPE_GROUP, TI.nGroupIndex);
			MicroProfileWebSocketSendEntry(id, parent, TI.pName, MicroProfileWebSocketTimerEnabled(i), TI.nColor);
		}

		for(uint32_t i = S.WSCountersSent; i < S.nNumCounters; ++i)
		{
			MicroProfileCounterInfo& CI = S.CounterInfo[i];
			uint32_t id = MicroProfileWebSocketIdPack(TYPE_COUNTER, i);			
			uint32_t parent = CI.nParent == (uint32_t)-1 ? 0 : MicroProfileWebSocketIdPack(TYPE_COUNTER, CI.nParent);
			MicroProfileWebSocketSendCounterEntry(id, parent, CI.pName, CI.nLimit, CI.eFormat);
		}
#if MICROPROFILE_CONTEXT_SWITCH_TRACE
		MicroProfileWebSocketSendEntry(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_CONTEXT_SWITCH_TRACE), 0, "Context Switch Trace", S.bContextSwitchRunning, (uint32_t)-1);
#endif
#if MICROPROFILE_PLATFORM_MARKERS
		MicroProfileWebSocketSendEntry(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_PLATFORM_MARKERS), 0, "Platform Markers", S.bContextSwitchRunning, (uint32_t)-1);
#endif
		WSPrintEnd();

		S.WSCategoriesSent = S.nCategoryCount;
		S.WSGroupsSent = S.nGroupCount;
		S.WSTimersSent = S.nTotalTimers;
		S.WSCountersSent = S.nNumCounters;
	}
}



bool MicroProfileWebServerUpdate()
{
	MICROPROFILE_SCOPEI("MicroProfile", "Webserver-update", MP_GREEN4);
	MpSocket Connection = accept(S.ListenerSocket, 0, 0);
	bool bServed = false;
	MicroProfileWebSocketFrame();
	if(!MP_INVALID_SOCKET(Connection))
	{
		std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
		char Req[8192];
		int nReceived = recv(Connection, Req, sizeof(Req)-1, 0);
		if(nReceived > 0)
		{
			Req[nReceived] = '\0';
			#if MICROPROFILE_DEBUG
			printf("req received\n%s", Req);
			#endif
#if MICROPROFILE_MINIZ
			//Expires: Tue, 01 Jan 2199 16:00:00 GMT\r\n
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Encoding: deflate\r\n\r\n"
#else
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
#endif
			char* pHttp = strstr(Req, "HTTP/");
			char* pGet = strstr(Req, "GET /");
			char* pHost = strstr(Req, "Host: ");
			char* pWebSocketKey = strstr(Req, "Sec-WebSocket-Key: ");
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

			if(pWebSocketKey)
			{
				if(S.nNumWebSockets) // only allow 1
				{
					return false;
				}
				pWebSocketKey += sizeof("Sec-WebSocket-Key: ")-1;
				Terminate(pWebSocketKey);
				MicroProfileWebSocketHandshake(Connection, pWebSocketKey);
				return false;
			}

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
				MicroProfileParseGetResult R;
				auto P = MicroProfileParseGet(pGet, &R);
				switch(P)
				{
					case EMICROPROFILE_GET_COMMAND_LIVE:
					{
							MicroProfileSetNonBlocking(Connection, 0);
							uint64_t nTickStart = MP_TICK();
							send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER)-1, 0);
							uint64_t nDataStart = S.nWebServerDataSent;
							S.WebServerPut = 0;
			#if 0 == MICROPROFILE_MINIZ
							MicroProfileDumpHtmlLive(MicroProfileWriteSocket, &Connection);
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
							MicroProfileDumpHtmlLive(MicroProfileCompressedWriteSocket, &CompressState);
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
			#else
							(void)nCompressedKb;
			#endif
					}
					break;
					case EMICROPROFILE_GET_COMMAND_DUMP_RANGE:
					case EMICROPROFILE_GET_COMMAND_DUMP:
					{
						{
							MicroProfileSetNonBlocking(Connection, 0);
							uint64_t nTickStart = MP_TICK();
							send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER)-1, 0);
							uint64_t nDataStart = S.nWebServerDataSent;
							S.WebServerPut = 0;
			#if 0 == MICROPROFILE_MINIZ
							MicroProfileDumpHtml(MicroProfileWriteSocket, &Connection, R.nFrames, pHost, R.nFrameStart);
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


							MicroProfileDumpHtml(MicroProfileCompressedWriteSocket, &CompressState, R.nFrames, pHost, R.nFrameStart);



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
			#else
							(void)nCompressedKb;
			#endif
						}
					}
					break;
					case EMICROPROFILE_GET_COMMAND_UNKNOWN:
					{
						#if MICROPROFILE_DEBUG
						printf("unknown get command %s\n", pGet);
						#endif
					}
					break;

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
int MicroProfileIsLocalThread(uint32_t nThreadId);


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

typedef VOID (WINAPI *EventCallback)(PEVENT_TRACE);
typedef ULONG (WINAPI *BufferCallback)(PEVENT_TRACE_LOGFILE);
bool MicroProfileStartWin32Trace(EventCallback EvtCb, BufferCallback BufferCB)
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
	sessionProperties.EnableFlags = EVENT_TRACE_FLAG_CSWITCH | EVENT_TRACE_FLAG_PROCESS;
	sessionProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	sessionProperties.MaximumFileSize = 0;
	sessionProperties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	sessionProperties.LogFileNameOffset = 0;

	StopTrace(NULL, KERNEL_LOGGER_NAME, &sessionProperties);
	status = StartTrace((PTRACEHANDLE)&SessionHandle, KERNEL_LOGGER_NAME, &sessionProperties);

	if (ERROR_SUCCESS != status)
	{
		return false;
	}

	EVENT_TRACE_LOGFILE log;
	ZeroMemory(&log, sizeof(log));

	log.LoggerName = KERNEL_LOGGER_NAME;
	log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
	log.EventCallback = EvtCb;
	log.BufferCallback = BufferCB;

	TRACEHANDLE hLog = OpenTrace(&log);
	ProcessTrace(&hLog, 1, 0, 0);
	CloseTrace(hLog);
	MicroProfileContextSwitchShutdownTrace();
	return true;


}

#include <winternl.h>
#include <psapi.h>
#include <tlhelp32.h>
#define ThreadQuerySetWin32StartAddress 9
typedef LONG    NTSTATUS;
typedef NTSTATUS(WINAPI *pNtQIT)(HANDLE, LONG, PVOID, ULONG, PULONG);
#define STATUS_SUCCESS    ((NTSTATUS)0x000 00000L)
#define ThreadQuerySetWin32StartAddress 9


struct MicroProfileWin32ContextSwitchShared
{
	std::atomic<int64_t> nPut;
	std::atomic<int64_t> nGet;
	std::atomic<int64_t> nQuit;
	std::atomic<int64_t> nTickTrace;
	std::atomic<int64_t> nTickProgram;
	enum {
		BUFFER_SIZE = (2 << 20) / sizeof(MicroProfileContextSwitch),
	};
	MicroProfileContextSwitch Buffer[BUFFER_SIZE];
};

struct MicroProfileWin32ThreadInfo
{
	struct Process
	{
		uint32_t pid;
		uint32_t nNumModules;
		uint32_t nModuleStart;
		const char* pProcessModule;
	};
	struct Module
	{
		int64_t nBase;
		int64_t nEnd;
		const char* pName;
	};
	enum {
		MAX_PROCESSES = 5 * 1024,
		MAX_THREADS = 20 * 1024,
		MAX_MODULES = 20 * 1024,
		MAX_STRINGS = 16 * 1024,		
		MAX_CHARS = 128 * 1024,
	};
	uint32_t nNumProcesses;
	uint32_t nNumThreads;
	uint32_t nStringOffset;
	uint32_t nNumStrings;
	uint32_t nNumModules;
	Process P[MAX_PROCESSES];
	Module M[MAX_MODULES];
	MicroProfileThreadInfo T[MAX_THREADS];
	const char* pStrings[MAX_STRINGS];	
	char StringData[MAX_CHARS];
};

static MicroProfileWin32ThreadInfo g_ThreadInfo;

const char* MicroProfileWin32ThreadInfoAddString(const char* pString)
{
	size_t nLen = strlen(pString);
	uint32_t nHash = *(uint32_t*)pString;
	nHash ^= (nHash >> 16);
	enum
	{
		MAX_SEARCH = 256,
	};
	for (uint32_t i = 0; i < MAX_SEARCH; ++i)
	{
		uint32_t idx = (i + nHash) % MicroProfileWin32ThreadInfo::MAX_STRINGS;
		if (0 == g_ThreadInfo.pStrings[idx])
		{
			g_ThreadInfo.pStrings[idx] = &g_ThreadInfo.StringData[g_ThreadInfo.nStringOffset];
			memcpy(&g_ThreadInfo.StringData[g_ThreadInfo.nStringOffset], pString, nLen + 1);
			g_ThreadInfo.nStringOffset += (uint32_t)(nLen + 1);
			return g_ThreadInfo.pStrings[idx];
		}
		if(0 == strcmp(g_ThreadInfo.pStrings[idx], pString))
		{
			return g_ThreadInfo.pStrings[idx];
		}
	}
	return "internal hash table fail: should never happen";
}
void MicroProfileWin32ExtractModules(MicroProfileWin32ThreadInfo::Process& P)
{
	HANDLE hModuleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, P.pid);
	MODULEENTRY32 me;
	if (Module32First(hModuleSnapshot, &me))
	{
		do
		{
			if (g_ThreadInfo.nNumModules < MicroProfileWin32ThreadInfo::MAX_MODULES)
			{
				auto& M = g_ThreadInfo.M[g_ThreadInfo.nNumModules++];
				P.nNumModules++;
				intptr_t nBase = (intptr_t)me.modBaseAddr;
				intptr_t nEnd = nBase + me.modBaseSize;
				M.nBase = nBase;
				M.nEnd = nEnd;
				M.pName = MicroProfileWin32ThreadInfoAddString(&me.szModule[0]);
			}
		} while (Module32Next(hModuleSnapshot, &me));
	}
	if (hModuleSnapshot)
		CloseHandle(hModuleSnapshot);


}
void MicroProfileWin32InitThreadInfo2()
{
	memset(&g_ThreadInfo, 0, sizeof(g_ThreadInfo));
	float fToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	PROCESSENTRY32 pe32;
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	pe32.dwSize = sizeof(PROCESSENTRY32);
	{
		int64_t nTickStart = MP_TICK();
		if (Process32First(hSnap, &pe32))
		{
			do
			{

				MicroProfileWin32ThreadInfo::Process P;
				P.pid = pe32.th32ProcessID;
				P.pProcessModule = MicroProfileWin32ThreadInfoAddString(pe32.szExeFile);
				g_ThreadInfo.P[g_ThreadInfo.nNumProcesses++] = P;
			} while (Process32Next(hSnap, &pe32) && g_ThreadInfo.nNumProcesses < MicroProfileWin32ThreadInfo::MAX_PROCESSES);
		}
#if MICROPROFILE_DEBUG
		int64_t nTicksEnd = MP_TICK();
		float fMs = fToMsCpu * (nTicksEnd - nTickStart);
		printf("Process iteration %6.2fms processes %d\n", fMs, g_ThreadInfo.nNumProcesses);
#endif
	}
	{
		int64_t nTickStart = MP_TICK();
		for (uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
		{
			uint32_t pid = g_ThreadInfo.P[i].pid;
			g_ThreadInfo.P[i].nModuleStart = g_ThreadInfo.nNumModules;
			g_ThreadInfo.P[i].nNumModules = 0;
			MicroProfileWin32ExtractModules(g_ThreadInfo.P[i]);
		}
#if MICROPROFILE_DEBUG
		int64_t nTicksEnd = MP_TICK();
		float fMs = fToMsCpu * (nTicksEnd - nTickStart);
		printf("Module iteration %6.2fms NumModules %d\n", fMs, g_ThreadInfo.nNumModules);
#endif
	}


	pNtQIT NtQueryInformationThread = (pNtQIT)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
	intptr_t dwStartAddress;
	ULONG olen;
	uint32_t nThreadsTested = 0;
	uint32_t nThreadsSucceeded = 0;
	uint32_t nThreadsSucceeded2 = 0;

	if (Thread32First(hSnap, &te32))
	{
		int64_t nTickStart = MP_TICK();
		do
		{
			nThreadsTested++;
			const char* pModule = "?";
			HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
			if (hThread)
			{

				NTSTATUS ntStatus = NtQueryInformationThread(hThread, (THREADINFOCLASS)ThreadQuerySetWin32StartAddress, &dwStartAddress, sizeof(dwStartAddress), &olen);
				if (0 == ntStatus)
				{
					bool bFound = false;
					uint32_t nProcessIndex = (uint32_t)-1;					
					for (uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
					{
						if (g_ThreadInfo.P[i].pid == te32.th32OwnerProcessID)
						{
							nProcessIndex = i;
							break;
						}
					}
					if (nProcessIndex != (uint32_t)-1)
					{
						uint32_t nModuleStart = g_ThreadInfo.P[nProcessIndex].nModuleStart;
						uint32_t nNumModules = g_ThreadInfo.P[nProcessIndex].nNumModules;
						for (uint32_t i = 0; i < nNumModules; ++i)
						{
							auto& M = g_ThreadInfo.M[nModuleStart + i];
							if (M.nBase <= dwStartAddress && M.nEnd >= dwStartAddress)
							{
								pModule = M.pName;
							}
						}
					}
				}
			}
			if (hThread)
				CloseHandle(hThread);
			{
				MicroProfileThreadInfo T;
				T.pid = te32.th32OwnerProcessID;
				T.tid = te32.th32ThreadID;
				const char* pProcess = "unknown";
				for (uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
				{
					if (g_ThreadInfo.P[i].pid == T.pid)
					{
						pProcess = g_ThreadInfo.P[i].pProcessModule;
						break;
					}
				}
				T.pProcessModule = pProcess;
				T.pThreadModule = MicroProfileWin32ThreadInfoAddString(pModule);
				T.nIsLocal = GetCurrentProcessId() == T.pid ? 1 : 0;
				nThreadsSucceeded++;
				g_ThreadInfo.T[g_ThreadInfo.nNumThreads++] = T;
			}



		} while (Thread32Next(hSnap, &te32) && g_ThreadInfo.nNumThreads < MicroProfileWin32ThreadInfo::MAX_THREADS);

#if MICROPROFILE_DEBUG
		int64_t nTickEnd = MP_TICK();
		float fMs = fToMsCpu * (nTickEnd - nTickStart);
		printf("Thread iteration %6.2fms Threads %d\n", fMs, g_ThreadInfo.nNumThreads);
#endif

	}
}

void MicroProfileWin32UpdateThreadInfo()
{
	static int nWasRunning = 1;
	static int nOnce = 0;
	int nRunning = S.nActiveGroup != 0;

	if ((0 == nRunning && 1 == nWasRunning) || nOnce == 0)
	{
		nOnce = 1;
		MicroProfileWin32InitThreadInfo2();
	}
	nWasRunning = nRunning;

}

const char* MicroProfileThreadNameFromId(MicroProfileThreadIdType nThreadId)
{
	MicroProfileWin32UpdateThreadInfo();
	static char result[1024];
	for (uint32_t i = 0; i < g_ThreadInfo.nNumThreads; ++i)
	{
		if (g_ThreadInfo.T[i].tid == nThreadId)
		{
			sprintf_s(result, "p:%s t:%s", g_ThreadInfo.T[i].pProcessModule, g_ThreadInfo.T[i].pThreadModule);
			return result;
		}
	}
	sprintf_s(result, "?");
	return result;
}

#define MICROPROFILE_FILEMAPPING "microprofile-shared"
#ifdef MICROPROFILE_WIN32_COLLECTOR
#define MICROPROFILE_WIN32_CSWITCH_TIMEOUT 15 //seconds to wait before collector exits
static MicroProfileWin32ContextSwitchShared* g_pShared = 0;
VOID WINAPI MicroProfileContextSwitchCallbackCollector(PEVENT_TRACE pEvent)
{
	static int64_t nPackets = 0;
	static int64_t nSkips = 0;
	if (pEvent->Header.Guid == g_MicroProfileThreadClassGuid)
	{
		if (pEvent->Header.Class.Type == 36)
		{
			MicroProfileSCSwitch* pCSwitch = (MicroProfileSCSwitch*)pEvent->MofData;
			if ((pCSwitch->NewThreadId != 0) || (pCSwitch->OldThreadId != 0))
			{
				MicroProfileContextSwitch Switch;
				Switch.nThreadOut = pCSwitch->OldThreadId;
				Switch.nThreadIn = pCSwitch->NewThreadId;
				Switch.nCpu = pEvent->BufferContext.ProcessorNumber;
				Switch.nTicks = pEvent->Header.TimeStamp.QuadPart;
				int64_t nPut = g_pShared->nPut.load(std::memory_order_relaxed);
				int64_t nGet = g_pShared->nGet.load(std::memory_order_relaxed);
				nPackets++;
				if(nPut - nGet < MicroProfileWin32ContextSwitchShared::BUFFER_SIZE)
				{
					g_pShared->Buffer[nPut%MicroProfileWin32ContextSwitchShared::BUFFER_SIZE] = Switch;
					g_pShared->nPut.store(nPut + 1, std::memory_order_release);
					nSkips = 0;
				}
				else
				{
					nSkips++;
				}
			}
		}
	}
	if(0 == (nPackets%(4<<10)))
	{
		int64_t nTickTrace = MP_TICK();
		g_pShared->nTickTrace.store(nTickTrace);
		int64_t nTickProgram = g_pShared->nTickProgram.load();
		float fTickToMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		float fTime = fabs(fTickToMs * (nTickTrace - nTickProgram));
		printf("\rRead %" PRId64 " CSwitch Packets, Skips %" PRId64 " Time difference %6.3fms         ", nPackets, nSkips, fTime);
		fflush(stdout);
		if (fTime > MICROPROFILE_WIN32_CSWITCH_TIMEOUT * 1000)
		{
			g_pShared->nQuit.store(1);
		}

	}
}

ULONG WINAPI MicroProfileBufferCallbackCollector(PEVENT_TRACE_LOGFILE Buffer)
{
	return (g_pShared->nQuit.load()) ? FALSE : TRUE;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		return 1;
	}
	printf("using file '%s'\n", argv[1]);
	HANDLE hMemory = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, argv[1]);
	if (hMemory == NULL)
	{
		return 1;
	}
	g_pShared = (MicroProfileWin32ContextSwitchShared*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MicroProfileWin32ContextSwitchShared));
	
	if (g_pShared != NULL)
	{
		MicroProfileStartWin32Trace(MicroProfileContextSwitchCallbackCollector, MicroProfileBufferCallbackCollector);
		UnmapViewOfFile(g_pShared);
	}

	CloseHandle(hMemory);
	return 0;
}
#endif

void* MicroProfileTraceThread(void* unused)
{
	MicroProfileOnThreadCreate("ContextSwitchThread");
	MicroProfileContextSwitchShutdownTrace();
	if(!MicroProfileStartWin32Trace(MicroProfileContextSwitchCallback, MicroProfileBufferCallback))
	{
		MicroProfileContextSwitchShutdownTrace();
		//not running as admin. try and start other process.
		MicroProfileWin32ContextSwitchShared* pShared = 0;
		char Filename[512];
		time_t t = time(NULL);
		_snprintf_s(Filename, sizeof(Filename), "%s_%d", MICROPROFILE_FILEMAPPING, (int)t);

		HANDLE hMemory = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MicroProfileWin32ContextSwitchShared), Filename);
		if (hMemory != NULL)
		{
			pShared = (MicroProfileWin32ContextSwitchShared*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MicroProfileWin32ContextSwitchShared));
			if (pShared != NULL)
			{
#ifdef _M_IX86
#define CSWITCH_EXE "microprofile-win32-cswitch_x86.exe"
#else
#define CSWITCH_EXE "microprofile-win32-cswitch_x64.exe"
#endif
				pShared->nTickProgram.store(MP_TICK());
				pShared->nTickTrace.store(MP_TICK());
				HINSTANCE Instance = ShellExecuteA(NULL, "runas", CSWITCH_EXE, Filename, "", SW_SHOWMINNOACTIVE);
				int64_t nInstance = (int64_t)Instance;
				if(nInstance >= 32)
				{
					int64_t nPut, nGet;
					while(!S.bContextSwitchStop)
					{
						nPut = pShared->nPut.load(std::memory_order_acquire);
						nGet = pShared->nGet.load(std::memory_order_relaxed);
						if (nPut == nGet)
						{
							Sleep(20);
						}
						else
						{
							for (int64_t i = nGet; i != nPut; i++)
							{
								MicroProfileContextSwitchPut(&pShared->Buffer[i%MicroProfileWin32ContextSwitchShared::BUFFER_SIZE]);
							}
							pShared->nGet.store(nPut, std::memory_order_release);
							pShared->nTickProgram.store(MP_TICK());
						}
					} 
					pShared->nQuit.store(1);
					
				}
			}
			UnmapViewOfFile(pShared);
		}
		CloseHandle(hMemory);
	}
	S.bContextSwitchRunning = false;
	return 0;
}


MicroProfileThreadInfo MicroProfileGetThreadInfo(MicroProfileThreadIdType nThreadId)
{
	MicroProfileWin32UpdateThreadInfo();

	for (uint32_t i = 0; i < g_ThreadInfo.nNumThreads; ++i)
	{
		if (g_ThreadInfo.T[i].tid == nThreadId)
		{
			return g_ThreadInfo.T[i];
		}
	}
	MicroProfileThreadInfo TI((uint32_t)nThreadId, 0, 0);
	return TI;
}
uint32_t MicroProfileGetThreadInfoArray(MicroProfileThreadInfo** pThreadArray)
{
	MicroProfileWin32InitThreadInfo2();
	*pThreadArray = &g_ThreadInfo.T[0];
	return g_ThreadInfo.nNumThreads;
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


MicroProfileThreadInfo MicroProfileGetThreadInfo(MicroProfileThreadIdType nThreadId)
{
	MicroProfileThreadInfo TI((uint32_t)nThreadId, 0, 0);
	return TI;
}
uint32_t MicroProfileGetThreadInfoArray(MicroProfileThreadInfo** pThreadArray)
{
	*pThreadArray = 0;
	return 0;
}

#endif
#else

MicroProfileThreadInfo MicroProfileGetThreadInfo(MicroProfileThreadIdType nThreadId)
{
	MicroProfileThreadInfo TI((uint32_t)nThreadId, 0, 0);
	return TI;
}
uint32_t MicroProfileGetThreadInfoArray(MicroProfileThreadInfo** pThreadArray)
{
	*pThreadArray = 0;
	return 0;
}
void MicroProfileStopContextSwitchTrace(){}
void MicroProfileStartContextSwitchTrace(){}

#endif




#if MICROPROFILE_GPU_TIMERS_D3D11
uint32_t MicroProfileGpuInsertTimeStamp(void* pContext_)
{
	MicroProfileD3D11Frame& Frame = S.pGPU->m_QueryFrames[S.pGPU->m_nQueryFrame];
	uint32_t nStart = Frame.m_nQueryStart;
	if(Frame.m_nRateQueryStarted)
	{
		uint32_t nIndex = (uint32_t)-1;
		do
		{
			nIndex = Frame.m_nQueryCount.load();
			if(nIndex + 1 >= Frame.m_nQueryCountMax)
			{
				return (uint32_t)-1;
			}
		}while(!Frame.m_nQueryCount.compare_exchange_weak(nIndex, nIndex+1));
		nIndex += nStart;
		uint32_t nQueryIndex = nIndex % MICROPROFILE_D3D_MAX_QUERIES;

		ID3D11Query* pQuery = (ID3D11Query*)S.pGPU->m_pQueries[nQueryIndex];
		ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)pContext_;
		pContext->End(pQuery);
		return nQueryIndex;
	}
	return (uint32_t)-1;
}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nIndex)
{
	if(nIndex == (uint32_t)-1)
	{
		return (uint64_t)-1;
	}
	int64_t nResult = S.pGPU->m_nQueryResults[nIndex];
	MP_ASSERT(nResult != -1);
	return nResult;	
}

bool MicroProfileGpuGetData(void* pQuery, void* pData, uint32_t nDataSize)
{
	HRESULT hr;
	do
	{
		hr = ((ID3D11DeviceContext*)S.pGPU->m_pImmediateContext)->GetData((ID3D11Query*)pQuery, pData, nDataSize, 0);
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
	return S.pGPU->m_nQueryFrequency;
}

uint32_t MicroProfileGpuFlip(void* pDeviceContext_)
{
	ID3D11DeviceContext* pDeviceContext = (ID3D11DeviceContext*)pDeviceContext_;
	uint32_t nFrameTimeStamp = MicroProfileGpuInsertTimeStamp(pDeviceContext);
	MicroProfileD3D11Frame& CurrentFrame = S.pGPU->m_QueryFrames[S.pGPU->m_nQueryFrame];
	ID3D11DeviceContext* pImmediateContext = (ID3D11DeviceContext*)S.pGPU->m_pImmediateContext;
	if(CurrentFrame.m_nRateQueryStarted)
	{
		pImmediateContext->End((ID3D11Query*)CurrentFrame.m_pRateQuery);
	}
	uint32_t nNextFrame = (S.pGPU->m_nQueryFrame + 1) % MICROPROFILE_GPU_FRAME_DELAY;
	S.pGPU->m_nQueryPut = (CurrentFrame.m_nQueryStart + CurrentFrame.m_nQueryCount) % MICROPROFILE_D3D_MAX_QUERIES;
	MicroProfileD3D11Frame& OldFrame = S.pGPU->m_QueryFrames[nNextFrame];
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
			if(S.pGPU->m_nQueryFrequency != (int64_t)Result.nFrequency)
			{
				if(S.pGPU->m_nQueryFrequency)
				{
					OutputDebugStringA("Query freq changing");
				}
				S.pGPU->m_nQueryFrequency = Result.nFrequency;
			}
			uint32_t nStart = OldFrame.m_nQueryStart;
			uint32_t nCount = OldFrame.m_nQueryCount;
			for(uint32_t i = 0; i < nCount; ++i)
			{
				uint32_t nIndex = (i + nStart) % MICROPROFILE_D3D_MAX_QUERIES;

				if(!MicroProfileGpuGetData(S.pGPU->m_pQueries[nIndex], &S.pGPU->m_nQueryResults[nIndex], sizeof(uint64_t)))
				{
					S.pGPU->m_nQueryResults[nIndex] = -1;
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
				S.pGPU->m_nQueryResults[nIndex] = -1;
			}
		}
		S.pGPU->m_nQueryGet = (OldFrame.m_nQueryStart + OldFrame.m_nQueryCount) % MICROPROFILE_D3D_MAX_QUERIES;
	}

	S.pGPU->m_nQueryFrame = nNextFrame;
	MicroProfileD3D11Frame& NextFrame = S.pGPU->m_QueryFrames[nNextFrame];
	pImmediateContext->Begin((ID3D11Query*)NextFrame.m_pRateQuery);
	NextFrame.m_nQueryStart = S.pGPU->m_nQueryPut;
	NextFrame.m_nQueryCount = 0;
	if(S.pGPU->m_nQueryPut >= S.pGPU->m_nQueryGet)
	{
		NextFrame.m_nQueryCountMax = (MICROPROFILE_D3D_MAX_QUERIES - S.pGPU->m_nQueryPut) + S.pGPU->m_nQueryGet;
	}
	else
	{
		NextFrame.m_nQueryCountMax = S.pGPU->m_nQueryGet - S.pGPU->m_nQueryPut - 1;
	}
	if(NextFrame.m_nQueryCountMax)
		NextFrame.m_nQueryCountMax -= 1;
	NextFrame.m_nRateQueryStarted = 1;
	return nFrameTimeStamp;
}

void MicroProfileGpuInitD3D11(void* pDevice_, void* pImmediateContext)
{
	ID3D11Device* pDevice = (ID3D11Device*)pDevice_;
	S.pGPU = MICROPROFILE_ALLOC_OBJECT(MicroProfileGpuTimerState);
	S.pGPU->m_pImmediateContext = pImmediateContext;

	D3D11_QUERY_DESC Desc;
	Desc.MiscFlags = 0;
	Desc.Query = D3D11_QUERY_TIMESTAMP;
	for(uint32_t i = 0; i < MICROPROFILE_D3D_MAX_QUERIES; ++i)
	{
		HRESULT hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.pGPU->m_pQueries[i]);
		MP_ASSERT(hr == S_OK);
		S.pGPU->m_nQueryResults[i] = -1;
	}
	HRESULT hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.pGPU->pSyncQuery);
	MP_ASSERT(hr == S_OK);

	S.pGPU->m_nQueryPut = 0;
	S.pGPU->m_nQueryGet = 0;
	S.pGPU->m_nQueryFrame = 0;
	S.pGPU->m_nQueryFrequency = 0;
	Desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY; ++i)
	{
		S.pGPU->m_QueryFrames[i].m_nQueryStart = 0;
		S.pGPU->m_QueryFrames[i].m_nQueryCount = 0;
		S.pGPU->m_QueryFrames[i].m_nRateQueryStarted = 0;
		hr = pDevice->CreateQuery(&Desc, (ID3D11Query**)&S.pGPU->m_QueryFrames[i].m_pRateQuery);
		MP_ASSERT(hr == S_OK);
	}
}


void MicroProfileGpuShutdown()
{
	for(uint32_t i = 0; i < MICROPROFILE_D3D_MAX_QUERIES; ++i)
	{
		if(S.pGPU->m_pQueries[i])
		{
			ID3D11Query* pQuery = (ID3D11Query*)S.pGPU->m_pQueries[i];
			pQuery->Release();
			S.pGPU->m_pQueries[i] = 0;
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY; ++i)
	{
		if(S.pGPU->m_QueryFrames[i].m_pRateQuery)
		{
			ID3D11Query* pQuery = (ID3D11Query*)S.pGPU->m_QueryFrames[i].m_pRateQuery;
			pQuery->Release();
			S.pGPU->m_QueryFrames[i].m_pRateQuery = 0;
		}
	}
	if(S.pGPU->pSyncQuery)
	{
		ID3D11Query* pSyncQuery = (ID3D11Query*)S.pGPU->pSyncQuery;
		pSyncQuery->Release();
		S.pGPU->pSyncQuery = 0;
	}

	MICROPROFILE_FREE(S.pGPU);
	S.pGPU = 0;
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	MicroProfileD3D11Frame& Frame = S.pGPU->m_QueryFrames[S.pGPU->m_nQueryFrame];
	if (Frame.m_nRateQueryStarted)
	{
		ID3D11Query* pSyncQuery = (ID3D11Query*)S.pGPU->pSyncQuery;
		ID3D11DeviceContext* pImmediateContext = (ID3D11DeviceContext*)S.pGPU->m_pImmediateContext;
		pImmediateContext->End(pSyncQuery);

		HRESULT hr;
		do
		{
			hr = pImmediateContext->GetData(pSyncQuery, pOutGpu, sizeof(*pOutGpu), 0);
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
uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	uint32_t nNode = S.pGPU->nCurrentNode;
	uint32_t nFrame = S.pGPU->nFrame;
	uint32_t nQueryIndex = (S.pGPU->nFrameCount.fetch_add(1) + S.pGPU->nFrameStart) % MICROPROFILE_D3D_MAX_QUERIES;
	
	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)pContext;
	pCommandList->EndQuery(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nQueryIndex);
	MP_ASSERT(nQueryIndex <= 0xffff);
	//uprintf("insert timestamp %d :: %d ... ctx %p\n", nQueryIndex, nFrame, pContext);
	return ((nFrame << 16) & 0xffff0000) | (nQueryIndex);
}

void MicroProfileGpuFetchRange(uint32_t nBegin, int32_t nCount, uint64_t nFrame, int64_t nTimestampOffset)
{
	if (nCount <= 0)
		return;
	void* pData = 0;
	//uprintf("fetch [%d-%d]\n", nBegin, nBegin + nCount);
	D3D12_RANGE Range = { sizeof(uint64_t)*nBegin, sizeof(uint64_t)*(nBegin+nCount) };
	S.pGPU->pBuffer->Map(0, &Range, &pData);
	memcpy(&S.pGPU->nResults[nBegin], nBegin + (uint64_t*)pData, nCount * sizeof(uint64_t));
	for (int i = 0; i < nCount; ++i)
	{
		S.pGPU->nQueryFrames[i + nBegin] = nFrame;
		S.pGPU->nResults[i + nBegin] -= nTimestampOffset;
	}
	S.pGPU->pBuffer->Unmap(0, 0);
}
void MicroProfileGpuWaitFence(uint32_t nNode, uint64_t nFence)
{
	uint64_t nCompletedFrame = S.pGPU->NodeState[nNode].pFence->GetCompletedValue();
	//while(nCompletedFrame < nPending)
	//while(0 < nPending - nCompletedFrame)
	while (0 < (int64_t)(nFence - nCompletedFrame))
	{
		MICROPROFILE_SCOPEI("Microprofile", "gpu-wait", MP_GREEN4);
		Sleep(20);//todo: use event.
		nCompletedFrame = S.pGPU->NodeState[nNode].pFence->GetCompletedValue();
	}

}

void MicroProfileGpuFetchResults(uint64_t nFrame)
{
	uint64_t nPending = S.pGPU->nPendingFrame;
	//while(nPending <= nFrame)
	//while(0 <= nFrame - nPending)
	while (0 <= (int64_t)(nFrame - nPending))
	{
		uint32_t nInternal = nPending % MICROPROFILE_D3D_INTERNAL_DELAY;
		uint32_t nNode = S.pGPU->Frames[nInternal].nNode;
		MicroProfileGpuWaitFence(nNode, nPending);
		int64_t nTimestampOffset = 0;
		if (nNode != 0)
		{
			// Adjust timestamp queries from GPU x to be in GPU 0's frame of reference
			HRESULT hr;
			int64_t nCPU0, nGPU0;
			hr = S.pGPU->NodeState[0].pCommandQueue->GetClockCalibration((uint64_t *)&nGPU0, (uint64_t*)&nCPU0);
			MP_ASSERT(hr == S_OK);
			int64_t nCPUx, nGPUx;
			hr = S.pGPU->NodeState[nNode].pCommandQueue->GetClockCalibration((uint64_t *)&nGPUx, (uint64_t*)&nCPUx);
			MP_ASSERT(hr == S_OK);
			int64_t nFreqCPU = MicroProfileTicksPerSecondCpu();
			int64_t nElapsedCPU = nCPUx - nCPU0;
			int64_t nElapsedGPU = S.pGPU->nFrequency * nElapsedCPU / nFreqCPU;
			nTimestampOffset = nGPUx - nGPU0 - nElapsedGPU;
		}

		uint32_t nBegin = S.pGPU->Frames[nInternal].nBegin;
		uint32_t nCount = S.pGPU->Frames[nInternal].nCount;
		MicroProfileGpuFetchRange(nBegin, (nBegin + nCount) > MICROPROFILE_D3D_MAX_QUERIES ? MICROPROFILE_D3D_MAX_QUERIES - nBegin : nCount, nPending, nTimestampOffset);
		MicroProfileGpuFetchRange(0, (nBegin + nCount) - MICROPROFILE_D3D_MAX_QUERIES, nPending, nTimestampOffset);

		nPending = ++S.pGPU->nPendingFrame;
		MP_ASSERT(S.pGPU->nFrame > nPending);
	}

}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nIndex)
{
	uint32_t nFrame = nIndex >> 16;
	uint32_t nQueryIndex = nIndex & 0xffff;
	uint32_t lala = S.pGPU->nQueryFrames[nQueryIndex];
	MP_ASSERT((0xffff & lala) == nFrame);
	//uprintf("read TS [%d <- %lld]\n", nQueryIndex, S.pGPU->nResults[nQueryIndex]);
	return S.pGPU->nResults[nQueryIndex];
}

uint64_t MicroProfileTicksPerSecondGpu()
{
	return S.pGPU->nFrequency;
}

uint32_t MicroProfileGpuFlip(void* pContext)
{
	uint32_t nNode = S.pGPU->nCurrentNode;
	uint32_t nFrameIndex = S.pGPU->nFrame % MICROPROFILE_D3D_INTERNAL_DELAY;
	uint32_t nCount = 0, nStart = 0;

	ID3D12GraphicsCommandList* pCommandList = S.pGPU->Frames[nFrameIndex].pCommandList[nNode];
	ID3D12CommandAllocator* pCommandAllocator = S.pGPU->Frames[nFrameIndex].pCommandAllocator;
	pCommandAllocator->Reset();
	pCommandList->Reset(pCommandAllocator, nullptr);


	uint32_t nFrameTimeStamp = MicroProfileGpuInsertTimeStamp(pCommandList);
	if (S.pGPU->nFrameCount)
	{

		nCount = S.pGPU->nFrameCount.exchange(0);
		nStart = S.pGPU->nFrameStart;
		S.pGPU->nFrameStart = (S.pGPU->nFrameStart + nCount) % MICROPROFILE_D3D_MAX_QUERIES;
		uint32_t nEnd = MicroProfileMin(nStart + nCount, (uint32_t)MICROPROFILE_D3D_MAX_QUERIES);
		MP_ASSERT(nStart != nEnd);
		uint32_t nSize = nEnd - nStart;
		pCommandList->ResolveQueryData(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nStart, nEnd - nStart, S.pGPU->pBuffer, nStart * sizeof(int64_t));
		if (nStart + nCount > MICROPROFILE_D3D_MAX_QUERIES)
		{
			pCommandList->ResolveQueryData(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, nEnd + nStart - MICROPROFILE_D3D_MAX_QUERIES, S.pGPU->pBuffer, 0);
		}
	} 
	pCommandList->Close();
	ID3D12CommandList* pList = pCommandList;
	S.pGPU->NodeState[nNode].pCommandQueue->ExecuteCommandLists(1, &pList);
	//uprintf("EXECUTE %p\n", pCommandList);
	S.pGPU->NodeState[nNode].pCommandQueue->Signal(S.pGPU->NodeState[nNode].pFence, S.pGPU->nFrame);
	S.pGPU->Frames[nFrameIndex].nBegin = nStart;
	S.pGPU->Frames[nFrameIndex].nCount = nCount;
	S.pGPU->Frames[nFrameIndex].nNode = nNode;

	S.pGPU->nFrame++;
	//fetch from earlier frames

	MicroProfileGpuFetchResults(S.pGPU->nFrame - MICROPROFILE_GPU_FRAME_DELAY);
	return nFrameTimeStamp;
}
void MicroProfileGpuInitD3D12(void* pDevice_, uint32_t nNodeCount, void** pCommandQueues_)
{
	ID3D12Device* pDevice = (ID3D12Device*)pDevice_;
	S.pGPU = MICROPROFILE_ALLOC_OBJECT(MicroProfileGpuTimerState);
	memset(S.pGPU, 0, sizeof(*S.pGPU));
	S.pGPU->pDevice = pDevice;
	S.pGPU->nNodeCount = nNodeCount;
	MP_ASSERT(S.pGPU->nNodeCount <= MICROPROFILE_D3D_MAX_NODE_COUNT);

	for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
	{
		S.pGPU->NodeState[nNode].pCommandQueue = (ID3D12CommandQueue *)pCommandQueues_[nNode];
		if (nNode == 0)
		{
			S.pGPU->NodeState[nNode].pCommandQueue->GetTimestampFrequency((uint64_t*)&(S.pGPU->nFrequency));
			MP_ASSERT(S.pGPU->nFrequency);
		}
		else
		{
			// Don't support GPUs with different timer frequencies for now
			int64_t nFrequency;
			S.pGPU->NodeState[nNode].pCommandQueue->GetTimestampFrequency((uint64_t*)&nFrequency);
			MP_ASSERT(nFrequency == S.pGPU->nFrequency);
		}

		D3D12_QUERY_HEAP_DESC QHDesc;
		QHDesc.Count = MICROPROFILE_D3D_MAX_QUERIES;
		QHDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
		QHDesc.NodeMask = MP_NODE_MASK_ONE(nNode);
		HRESULT	hr = pDevice->CreateQueryHeap(&QHDesc, IID_PPV_ARGS(&S.pGPU->NodeState[nNode].pHeap));
		MP_ASSERT(hr == S_OK);

		pDevice->CreateFence(S.pGPU->nPendingFrame, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&S.pGPU->NodeState[nNode].pFence));
	}



	HRESULT hr;
	D3D12_HEAP_PROPERTIES HeapProperties;
	HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProperties.CreationNodeMask = 0;
	HeapProperties.VisibleNodeMask = MP_NODE_MASK_ALL(S.pGPU->nNodeCount);
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
		IID_PPV_ARGS(&S.pGPU->pBuffer));
	MP_ASSERT(hr == S_OK);

	S.pGPU->nFrame = 0;
	S.pGPU->nPendingFrame = 0;

	for(MicroProfileD3D12Frame& Frame : S.pGPU->Frames)
	{
		hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Frame.pCommandAllocator));
		MP_ASSERT(hr == S_OK);
		for (uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
		{
			hr = pDevice->CreateCommandList(MP_NODE_MASK_ONE(nNode), D3D12_COMMAND_LIST_TYPE_DIRECT, Frame.pCommandAllocator, nullptr, IID_PPV_ARGS(&Frame.pCommandList[nNode]));
			MP_ASSERT(hr == S_OK);
			hr = Frame.pCommandList[nNode]->Close();
			MP_ASSERT(hr == S_OK);
		}
	}
}

void MicroProfileGpuShutdown()
{
	for(uint32_t nNode=0; nNode < S.pGPU->nNodeCount; ++nNode)
	{
		MicroProfileGpuWaitFence(nNode, S.pGPU->nFrame - 1);
	}
	for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
	{
		S.pGPU->NodeState[nNode].pHeap->Release();
		S.pGPU->NodeState[nNode].pFence->Release();
	}
	S.pGPU->pBuffer->Release();
	for(MicroProfileD3D12Frame& Frame : S.pGPU->Frames)
	{
		Frame.pCommandAllocator->Release();
		for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
		{
			Frame.pCommandList[nNode]->Release();
		}
	}


	MICROPROFILE_FREE(S.pGPU);
	S.pGPU = 0;

}
void MicroProfileSetCurrentNodeD3D12(uint32_t nNode)
{
	S.pGPU->nCurrentNode = nNode;
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	HRESULT hr = S.pGPU->NodeState[0].pCommandQueue->GetClockCalibration((uint64_t*)pOutGpu, (uint64_t*)pOutCPU);
	MP_ASSERT(hr == S_OK);
	return 1;
}
#elif MICROPROFILE_GPU_TIMERS_GL
void MicroProfileGpuInitGL()
{
	S.pGPU->GLTimerPos = 0;
	glGenQueries(MICROPROFILE_GL_MAX_QUERIES, &S.pGPU->GLTimers[0]);		
}

uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	uint32_t nIndex = (S.pGPU->GLTimerPos+1)%MICROPROFILE_GL_MAX_QUERIES;
	glQueryCounter(S.pGPU->GLTimers[nIndex], GL_TIMESTAMP);
	S.pGPU->GLTimerPos = nIndex;
	return nIndex;
}
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey)
{
	uint64_t result;
	glGetQueryObjectui64v(S.pGPU->GLTimers[nKey], GL_QUERY_RESULT, &result);
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

#ifdef MICROPROFILE_PS4
#define MICROPROFILE_PS4_IMPL
#include "microprofile_ps4.h"
#endif
#ifdef MICROPROFILE_XBOXONE
#define MICROPROFILE_XBOXONE_IMPL
#include "microprofile_xboxone.h"
#endif

#endif //#if defined(MICROPROFILE_IMPL) && MICROPROFILE_ENABLED

#ifdef MICROPROFILE_EMBED_HTML
#include "microprofile_html.h"
#endif
