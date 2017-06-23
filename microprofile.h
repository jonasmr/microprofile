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

#ifdef MICROPROFILE_USE_CONFIG
#include "microprofile.config.h"
#endif

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
#define MICROPROFILE_TIMELINE_TOKEN(token) do{}while(0)
#define MICROPROFILE_TIMELINE_ENTER(token, color, name) do{}while(0)
#define MICROPROFILE_TIMELINE_ENTERF(token, color, fmt, ...) do{}while(0)
#define MICROPROFILE_TIMELINE_LEAVE(token) do{}while(0)
#define MICROPROFILE_TIMELINE_ENTER_STATIC(color, name) do{}while(0)
#define MICROPROFILE_TIMELINE_LEAVE_STATIC(name) do{}while(0)
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
#define MICROPROFILE_COUNTER_CONFIG_ONCE(name, type, limit, flags)
#define MICROPROFILE_DECLARE_LOCAL_COUNTER(var) 
#define MICROPROFILE_DEFINE_LOCAL_COUNTER(var, name) 
#define MICROPROFILE_DECLARE_LOCAL_ATOMIC_COUNTER(var) 
#define MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(var, name) 
#define MICROPROFILE_COUNTER_LOCAL_ADD(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SUB(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SET(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD(var) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_SET(var) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SUB_ATOMIC(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_SET_ATOMIC(var, count) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD_ATOMIC(var) do{}while(0)
#define MICROPROFILE_COUNTER_LOCAL_UPDATE_SET_ATOMIC(var) do{}while(0)

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
#define MicroProfileDumpFileImmediately(html,csv,gfcontext) do{} while(0)
#define MicroProfileWebServerPort() ((uint32_t)-1)
#define MicroProfileStartContextSwitchTrace() do{}while(0)
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
#define MICROPROFILE_SCOPEGPU_TOKEN(token) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token, MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_SCOPEGPU(var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var, MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_SCOPEGPUI(name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken("GPU", name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_SCOPEGPU_TOKEN_L(Log, token) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(token, Log)
#define MICROPROFILE_SCOPEGPU_L(Log, var) MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var, Log)
#define MICROPROFILE_SCOPEGPUI_L(Log, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken("GPU", name, color,  MicroProfileTokenTypeGpu); MicroProfileScopeGpuHandler MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), Log)
#define MICROPROFILE_ENTER(var) MicroProfileEnter(g_mp_##var)
#define MICROPROFILE_ENTER_TOKEN(var) MicroProfileEnter(token)
#define MICROPROFILE_ENTERI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeCpu);} MicroProfileEnter(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_LEAVE() MicroProfileLeave()
#define MICROPROFILE_GPU_ENTER(var) MicroProfileEnterGpu(g_mp_##var, MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_GPU_ENTER_TOKEN(token) MicroProfileEnterGpu(token, MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_GPU_ENTERI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MICROPROFILE_INVALID_TOKEN; if(MICROPROFILE_INVALID_TOKEN == MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__)){MicroProfileGetTokenC(&MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), group, name, color, MicroProfileTokenTypeGpu);} MicroProfileEnterGpu(MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__), MicroProfileGetGlobalGpuThreadLog())
#define MICROPROFILE_GPU_LEAVE() MicroProfileLeaveGpu(MicroProfileGetGlobalGpuThreadLog())
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
#define MICROPROFILE_TIMELINE_TOKEN(token) uint32_t token = 0
#define MICROPROFILE_TIMELINE_ENTER(token, color, name) token = MicroProfileTimelineEnter(color, name)
#define MICROPROFILE_TIMELINE_ENTERF(token, color, fmt, ...) token = MicroProfileTimelineEnterf(color, fmt, ##__VA_ARGS__)
#define MICROPROFILE_TIMELINE_LEAVE(token) do{if(token){MicroProfileTimelineLeave(token);}}while(0)
 // use only with static string literals
#define MICROPROFILE_TIMELINE_ENTER_STATIC(color, name) MicroProfileTimelineEnterStatic(color, name)
 // use only with static string literals
#define MICROPROFILE_TIMELINE_LEAVE_STATIC(name) MicroProfileTimelineLeaveStatic(name)
#define MICROPROFILE_THREADLOGGPURESET(a) MicroProfileThreadLogGpuReset(a)
#define MICROPROFILE_META_CPU(name, count) do{}while(0)//static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__) = MicroProfileGetMetaToken(name); MicroProfileMetaUpdate(MICROPROFILE_TOKEN_PASTE(g_mp_meta,__LINE__), count, MicroProfileTokenTypeCpu)
#define MICROPROFILE_COUNTER_ADD(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterAdd(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), count)
#define MICROPROFILE_COUNTER_SUB(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterAdd(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), -(int64_t)count)
#define MICROPROFILE_COUNTER_SET(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter_set,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterSet(MICROPROFILE_TOKEN_PASTE(g_mp_counter_set,__LINE__), count)
#define MICROPROFILE_COUNTER_SET_INT32_PTR(name, ptr) MicroProfileCounterSetPtr(name, ptr, sizeof(int32_t))
#define MICROPROFILE_COUNTER_SET_INT64_PTR(name, ptr) MicroProfileCounterSetPtr(name, ptr, sizeof(int64_t))
#define MICROPROFILE_COUNTER_CLEAR_PTR(name) MicroProfileCounterSetPtr(name, 0, 0)
#define MICROPROFILE_COUNTER_SET_LIMIT(name, count) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__) = MicroProfileGetCounterToken(name); MicroProfileCounterSetLimit(MICROPROFILE_TOKEN_PASTE(g_mp_counter,__LINE__), count)
#define MICROPROFILE_COUNTER_CONFIG(name, type, limit, flags) MicroProfileCounterConfig(name, type, limit, flags)
#define MICROPROFILE_COUNTER_CONFIG_ONCE(name, type, limit, flags) do{static bool MICROPROFILE_TOKEN_PASTE(g_mponce,__LINE__) = false; if(!MICROPROFILE_TOKEN_PASTE(g_mponce,__LINE__)){MICROPROFILE_TOKEN_PASTE(g_mponce,__LINE__) = true; MicroProfileCounterConfig(name,type,limit,flags);}}while(0)
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

// #ifndef MICROPROFILE_META_MAX
// #define MICROPROFILE_META_MAX 8
// #endif

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

#ifndef MICROPROFILE_TIMELINE_MAX_ENTRIES
#define MICROPROFILE_TIMELINE_MAX_ENTRIES (4<<10)
#endif 

#ifndef MICROPROFILE_MAX_STRING
#define MICROPROFILE_MAX_STRING 128
#endif


#ifndef MICROPROFILE_TIMELINE_MAX_TOKENS
#define MICROPROFILE_TIMELINE_MAX_TOKENS 64
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

#ifndef MICROPROFILE_MAX_GROUPS
#define MICROPROFILE_MAX_GROUPS 128 //must be multiple of 32
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

#define MICROPROFILE_INVALID_TOKEN ((uint64_t)-1)

MICROPROFILE_API void MicroProfileInit();
MICROPROFILE_API void MicroProfileShutdown();
MICROPROFILE_API MicroProfileToken MicroProfileFindToken(const char* sGroup, const char* sName);
MICROPROFILE_API MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token);
MICROPROFILE_API void MicroProfileGetTokenC(MicroProfileToken* pToken, const char* sGroup, const char* sName, uint32_t nColor, MicroProfileTokenType Token);
// MICROPROFILE_API MicroProfileToken MicroProfileGetMetaToken(const char* pName);
MICROPROFILE_API MicroProfileToken MicroProfileGetCounterToken(const char* pName);
// MICROPROFILE_API void MicroProfileMetaUpdate(MicroProfileToken, int nCount, MicroProfileTokenType eTokenType);
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
MICROPROFILE_API void MicroProfileLeaveGpu(struct MicroProfileThreadLogGpu* pLog);
MICROPROFILE_API uint32_t MicroProfileTimelineEnterInternal(uint32_t nColor, const char* pStr, int nStrLen, int bIsStaticString);
MICROPROFILE_API uint32_t MicroProfileTimelineEnter(uint32_t nColor, const char* pStr);
MICROPROFILE_API uint32_t MicroProfileTimelineEnterf(uint32_t nColor, const char* pStr, ...);
MICROPROFILE_API void MicroProfileTimelineLeave(uint32_t id);
MICROPROFILE_API void MicroProfileTimelineEnterStatic(uint32_t nColor, const char* pStr);
MICROPROFILE_API void MicroProfileTimelineLeaveStatic(const char* pStr);
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
MICROPROFILE_API struct MicroProfileThreadLogGpu* MicroProfileGetGlobalGpuThreadLog();
MICROPROFILE_API int MicroProfileGetGlobalGpuQueue();
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

#if defined(MICROPROFILE_GPU_TIMERS_VULKAN)
#include <vulkan/vulkan.h>
void MicroProfileGpuInitVulkan(VkDevice* pDevices, VkPhysicalDevice* pPhysicalDevices, VkQueue* pQueues, uint32_t* QueueFamily, uint32_t nNodeCount);
MICROPROFILE_API void MicroProfileGpuShutdown();
MICROPROFILE_API void MicroProfileSetCurrentNodeVulkan(uint32_t nNode);
#endif


MICROPROFILE_API void MicroProfileDumpFile(const char* pHtml, const char* pCsv, float fCpuSpike, float fGpuSpike);
MICROPROFILE_API void MicroProfileDumpFileImmediately(const char* pHtml, const char* pCsv, void* pGpuContext);
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


