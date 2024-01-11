#define MICROPROFILE_IMPL
#include "microprofile.h"
#if MICROPROFILE_ENABLED

#define BREAK_SKIP() __builtin_trap()

#if MICROPROFILE_GPU_TIMERS && MICROPROFILE_GPU_TIMER_CALLBACKS
static MicroProfileGpuInsertTimeStamp_CB MicroProfileGpuInsertTimeStamp = 0;
static MicroProfileGpuGetTimeStamp_CB MicroProfileGpuGetTimeStamp = 0;
static MicroProfileTicksPerSecondGpu_CB MicroProfileTicksPerSecondGpu = 0;
static MicroProfileGetGpuTickReference_CB MicroProfileGetGpuTickReference = 0;
static MicroProfileGpuFlip_CB MicroProfileGpuFlip = 0;
static MicroProfileGpuShutdown_CB MicroProfileGpuShutdown = 0;

void MicroProfileGpuSetCallbacks(MicroProfileGpuInsertTimeStamp_CB InsertTimeStamp,
								 MicroProfileGpuGetTimeStamp_CB GetTimeStamp,
								 MicroProfileTicksPerSecondGpu_CB TicksPerSecond,
								 MicroProfileGetGpuTickReference_CB GetTickReference,
								 MicroProfileGpuFlip_CB Flip,
								 MicroProfileGpuShutdown_CB Shutdown)
{
	MicroProfileGpuInsertTimeStamp = InsertTimeStamp;
	MicroProfileGpuGetTimeStamp = GetTimeStamp;
	MicroProfileTicksPerSecondGpu = TicksPerSecond;
	MicroProfileGetGpuTickReference = GetTickReference;
	MicroProfileGpuFlip = Flip;
	MicroProfileGpuShutdown = Shutdown;
}
#endif

#ifdef _WIN32
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <malloc.h>
#endif

#ifdef _WIN32
#define MICROPROFILE_MAX_PATH MAX_PATH
#else
#define MICROPROFILE_MAX_PATH 1024
#endif


#include <atomic>
#include <ctype.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <thread>

#if defined(MICROPROFILE_SYSTEM_STB)
#include <stb_sprintf.h>
#else
#define STB_SPRINTF_IMPLEMENTATION
#include "stb/stb_sprintf.h"
#endif

#if defined(_WIN32) && _MSC_VER == 1700
#define PRIx64 "llx"
#define PRIu64 "llu"
#define PRId64 "lld"
#else
#include <inttypes.h>
#endif

#define MICROPROFILE_MAX_COUNTERS 512
#define MICROPROFILE_MAX_COUNTER_NAME_CHARS (MICROPROFILE_MAX_COUNTERS * 16)
#define MICROPROFILE_MAX_GROUP_INTS (MICROPROFILE_MAX_GROUPS / 32)
#define MICROPROFILE_MAX_CATEGORIES 16
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_BUFFER_SIZE) / sizeof(MicroProfileLogEntry))
#define MICROPROFILE_GPU_BUFFER_SIZE ((MICROPROFILE_PER_THREAD_GPU_BUFFER_SIZE) / sizeof(MicroProfileLogEntry))
#define MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS 256
#define MICROPROFILE_WEBSOCKET_BUFFER_SIZE (64 << 10)
#define MICROPROFILE_INVALID_TICK ((uint64_t)-1)
#define MICROPROFILE_DROPPED_TICK ((uint64_t)-2)
#define MICROPROFILE_INVALID_FRAME ((uint32_t)-1)
#define MICROPROFILE_GROUP_MASK_ALL 0xffffffff
#define MICROPROFILE_MAX_PATCH_ERRORS 32
#define MICROPROFILE_MAX_MODULE_EXEC_REGIONS 16

#define MP_LOG_TICK_MASK 0x0000ffffffffffff
#define MP_LOG_INDEX_MASK 0x3fff000000000000
#define MP_LOG_BEGIN_MASK 0xc000000000000000
#define MP_LOG_CSTR_MASK 0xe000000000000000
#define MP_LOG_CSTR_BIT 0x2000000000000000
#define MP_LOG_PAYLOAD_PTR_MASK (~(MP_LOG_BEGIN_MASK | MP_LOG_CSTR_BIT))

#define MP_LOG_ENTER_LEAVE_MASK 0x8000000000000000

#define MP_LOG_LEAVE 0x0
#define MP_LOG_ENTER 0x1
#define MP_LOG_EXTENDED 0x2
#define MP_LOG_EXTENDED_NO_DATA 0x3

//#define MP_LOG_EXTRA_DATA 0x3

static_assert(0 == (MICROPROFILE_MAX_GROUPS % 32), "MICROPROFILE_MAX_GROUPS must be divisible by 32");

enum EMicroProfileTokenExtended
{
	ETOKEN_GPU_CPU_TIMESTAMP = 0x3fff,
	ETOKEN_GPU_CPU_SOURCE_THREAD = 0x3ffe,
	ETOKEN_META_MARKER = 0x3ffd,
	ETOKEN_CUSTOM_NAME = 0x3ffc,
	ETOKEN_CUSTOM_COLOR = 0x3ffb,
	ETOKEN_CUSTOM_ID = 0x3ffa,
	ETOKEN_CSTR_PTR = 0x2000, // note, matches MP_LOG_CSTR_BIT
	ETOKEN_MAX = 0x2000,
};

enum
{
	MICROPROFILE_WEBSOCKET_DIRTY_MENU,
	MICROPROFILE_WEBSOCKET_DIRTY_ENABLED,
};

#ifndef MICROPROFILE_ALLOC // redefine all if overriding
#define MICROPROFILE_ALLOC(nSize, nAlign) MicroProfileAllocAligned(nSize, nAlign);
#define MICROPROFILE_REALLOC(p, s) realloc(p, s)
#define MICROPROFILE_FREE(p) MicroProfileFreeAligned(p)
#define MICROPROFILE_FREE_NON_ALIGNED(p) free(p)
#endif

#define MP_ALLOC(nSize, nAlign) MicroProfileAllocInternal(nSize, nAlign)
#define MP_REALLOC(p, s) MicroProfileReallocInternal(p, s)
#define MP_FREE(p) MicroProfileFreeInternal(p)
#define MP_ALLOC_OBJECT(T) (T*)MP_ALLOC(sizeof(T), alignof(T))

#ifndef MICROPROFILE_DEBUG
#define MICROPROFILE_DEBUG 0
#endif

typedef uint64_t MicroProfileLogEntry;

void MicroProfileSleep(uint32_t nMs);
template <typename T>
T MicroProfileMin(T a, T b);
template <typename T>
T MicroProfileMax(T a, T b);
template <typename T>
T MicroProfileClamp(T a, T min_, T max_);
int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond);
float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond);
uint32_t MicroProfileLogGetType(MicroProfileLogEntry Index);
uint64_t MicroProfileLogGetTimerIndex(MicroProfileLogEntry Index);
MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick);
int64_t MicroProfileLogTickDifference(MicroProfileLogEntry Start, MicroProfileLogEntry End);
int64_t MicroProfileLogSetTick(MicroProfileLogEntry e, int64_t nTick);
uint16_t MicroProfileGetTimerIndex(MicroProfileToken t);
uint32_t MicroProfileGetGroupMask(MicroProfileToken t);
MicroProfileToken MicroProfileMakeToken(uint64_t nGroupMask, uint32_t nGroupIndex, uint16_t nTimer);
bool MicroProfileAnyGroupActive();
void MicroProfileWriteFile(void* Handle, size_t nSize, const char* pData);

// defer implementation
#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)
void IntentionallyNotDefinedFunction__(); // DO NOT DEFINE THIS
template <typename T>
struct MicroProfileExitScope
{
	T lambda;
	MicroProfileExitScope(T lambda)
		: lambda(lambda)
	{
	}
	~MicroProfileExitScope()
	{
		lambda();
	}

	MicroProfileExitScope(const MicroProfileExitScope& rhs)
		: lambda(rhs.lambda)
	{
		IntentionallyNotDefinedFunction__(); // this is here to ensure the compiler does not create duplicate copies
	}

  private:
	MicroProfileExitScope& operator=(const MicroProfileExitScope&);
};

class MicroProfileExitScopeHelp
{
  public:
	template <typename T>
	MicroProfileExitScope<T> operator+(T t)
	{
		return t;
	}
};
#define defer const auto& CONCAT(defer__, __LINE__) = MicroProfileExitScopeHelp() + [&]()

//////////////////////////////////////////////////////////////////////////
// platform IMPL
void* MicroProfileAllocInternal(size_t nSize, size_t nAlign);
void MicroProfileFreeInternal(void* pPtr);
void* MicroProfileReallocInternal(void* pPtr, size_t nSize);

void* MicroProfileAllocAligned(size_t nSize, size_t nAlign);
void MicroProfileFreeAligned(void* pMem);

#if defined(__APPLE__)
#include <TargetConditionals.h>
#include <libkern/OSAtomic.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>

#if TARGET_OS_IPHONE
#define MICROPROFILE_IOS
#endif

#define MP_TICK() mach_absolute_time()
inline int64_t MicroProfileTicksPerSecondCpu_()
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

int64_t MicroProfileTicksPerSecondCpu()
{
	return MicroProfileTicksPerSecondCpu_();
}
#define MicroProfileTicksPerSecondCpu MicroProfileTicksPerSecondCpu_

inline uint64_t MicroProfileGetCurrentThreadId()
{
	uint64_t tid;
	pthread_threadid_np(pthread_self(), &tid);
	return tid;
}

#include <stdlib.h>

#define MP_BREAK() __builtin_trap()
#define MP_THREAD_LOCAL __thread
#define MP_STRCASECMP strcasecmp
#define MP_GETCURRENTTHREADID() MicroProfileGetCurrentThreadId()
#define MP_STRCASESTR strcasestr
#define MP_THREAD_LOCAL __thread
#define MP_NOINLINE __attribute__((noinline))

void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
	void* p;
	int result = posix_memalign(&p, nAlign, nSize);
	if(result != 0)
	{
		return nullptr;
	}
	return p;
}

void MicroProfileFreeAligned(void* pMem)
{
	free(pMem);
}

#elif defined(_WIN32)
#include <Shlwapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
int64_t MicroProfileGetTick();
#define MP_TICK() MicroProfileGetTick()
#define MP_BREAK() __debugbreak()
#define MP_THREAD_LOCAL __declspec(thread)
#define MP_STRCASECMP _stricmp
#define MP_GETCURRENTTHREADID() GetCurrentThreadId()
#define MP_STRCASESTR StrStrI
#define MP_THREAD_LOCAL __declspec(thread)
#define MP_NOINLINE __declspec(noinline)

void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
	return _aligned_malloc(nSize, nAlign);
}

void MicroProfileFreeAligned(void* pMem)
{
	_aligned_free(pMem);
}

#else

#ifndef MICROPROFILE_CUSTOM_PLATFORM
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
inline int64_t MicroProfileTicksPerSecondCpu_()
{
	return 1000000000ll;
}

int64_t MicroProfileTicksPerSecondCpu()
{
	return MicroProfileTicksPerSecondCpu_();
}
#define MicroProfileTicksPerSecondCpu MicroProfileTicksPerSecondCpu_

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
#define MP_GETCURRENTTHREADID() (uint64_t) pthread_self()
#define MP_STRCASESTR strcasestr
#define MP_THREAD_LOCAL __thread
#define MP_NOINLINE __attribute__((noinline))

void* MicroProfileAllocAligned(size_t nSize, size_t nAlign)
{
#if defined(__linux__)
	void* p;
	int result = posix_memalign(&p, nAlign, nSize);
	if(result != 0)
	{
		return nullptr;
	}
	return p;
#else
	return memalign(nAlign, nSize);
#endif
}
void MicroProfileFreeAligned(void* pMem)
{
	free(pMem);
}
#endif

#endif

#ifdef MICROPROFILE_PS4
#define MICROPROFILE_PS4_DECL
#include "microprofile_ps4.h"
#endif

#ifdef MICROPROFILE_XBOXONE
#define MICROPROFILE_XBOXONE_DECL
#include "microprofile_xboxone.h"
#else
#ifdef _WIN32
#include <d3d11_1.h>
#endif
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

#define MP_ASSERT(a)                                                                                                                                                                                   \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
		if(!(a))                                                                                                                                                                                       \
		{                                                                                                                                                                                              \
			MP_BREAK();                                                                                                                                                                                \
		}                                                                                                                                                                                              \
	} while(0)

#ifdef _WIN32
#include <basetsd.h>
typedef UINT_PTR MpSocket;
#else
typedef int MpSocket;
#endif

#ifndef _WIN32
typedef pthread_t MicroProfileThread;
#elif defined(_WIN32)
#if _MSC_VER == 1900
typedef void* HANDLE;
#endif

typedef HANDLE MicroProfileThread;
#else
typedef std::thread* MicroProfileThread;
#endif

#if MICROPROFILE_DYNAMIC_INSTRUMENT
struct MicroProfileSymbolDesc;
void MicroProfileSymbolQueryFunctions(MpSocket Connection, const char* pFilter);
void MicroProfileInstrumentFunction(void* pFunction, const char* pModuleName, const char* pFunctionName, uint32_t nColor);
bool MicroProfileSymbolInitialize(bool bStartLoad, const char* pModuleName = 0);
MicroProfileSymbolDesc* MicroProfileSymbolFindFuction(void* pAddress);
void MicroProfileInstrumentFunctionsCalled(void* pFunction, const char* pModuleName, const char* pFunctionName);
void MicroProfileSymbolQuerySendResult(MpSocket Connection);
void MicroProfileSymbolSendFunctionNames(MpSocket Connection);
void MicroProfileSymbolSendErrors(MpSocket Connection);
const char* MicroProfileSymbolModuleGetString(uint32_t nIndex);
void MicroProfileInstrumentWithoutSymbols(const char** pModules, const char** pSymbols, uint32_t nNumSymbols);
void MicroProfileSymbolUpdateModuleList();
#endif

struct MicroProfileFunctionQuery;

// hash table functions & declarations
struct MicroProfileHashTable;
struct MicroProfileHashTableIterator;
typedef bool (*MicroProfileHashCompareFunction)(uint64_t l, uint64_t r);
typedef uint64_t (*MicroProfileHashFunction)(uint64_t p);
uint64_t MicroProfileHashTableHashString(uint64_t pString);
bool MicroProfileHashTableCompareString(uint64_t L, uint64_t R);
void MicroProfileHashTableInit(MicroProfileHashTable* pTable, uint32_t nInitialSize, uint32_t nSearchLimit, MicroProfileHashCompareFunction CompareFunc, MicroProfileHashFunction HashFunc);
void MicroProfileHashTableDestroy(MicroProfileHashTable* pTable);
uint64_t MicroProfileHashTableHash(MicroProfileHashTable* pTable, uint64_t K);
bool MicroProfileHashTableSet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t Value);
MicroProfileHashTableIterator MicroProfileGetHashTableIteratorBegin(MicroProfileHashTable* HashTable);
MicroProfileHashTableIterator MicroProfileGetHashTableIteratorEnd(MicroProfileHashTable* HashTable);

struct MicroProfileTimer
{
	uint64_t nTicks;
	uint32_t nCount;
};

struct MicroProfileCategory
{
	char pName[MICROPROFILE_NAME_MAX_LEN];
	uint32_t nGroupMask[MICROPROFILE_MAX_GROUP_INTS];
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
	uint32_t Flags;
};

struct MicroProfileCounterInfo
{
	int nParent;
	int nSibling;
	int nFirstChild;
	uint16_t nNameLen;
	uint8_t nLevel;
	const char* pName;
	uint32_t nFlags;
	int64_t nLimit;
	MicroProfileCounterFormat eFormat;
	std::atomic<int64_t> ExternalAtomic;
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
	uint64_t nFrameStartCpu;
	uint64_t nFrameStartGpu;
	uint64_t nFrameId;
	uint32_t nGpuPending;
	uint32_t nLogStart[MICROPROFILE_MAX_THREADS];
	uint32_t nLogStartTimeline;
	uint32_t nTimelineFrameMax;
	int32_t nHistoryTimeline;
};


// All frame counter data stored. Used to store the time for all counters/groups for every frame.
// Must be enabled with MicroProfileEnableFrameCounterExtraData()
// Will allocate sizeof(MicroProfileFrameExtraCounterData) * MICROPROFILE_MAX_FRAME_HISTORY bytes
struct MicroProfileFrameExtraCounterData
{
	uint16_t NumTimers;
	uint16_t NumGroups;
	uint64_t Timers[MICROPROFILE_MAX_TIMERS];
	uint64_t Groups[MICROPROFILE_MAX_GROUPS];
};

struct MicroProfileCsvConfig
{
	enum CsvConfigState
	{
		INACTIVE = 0 ,
		CONFIG,
		ACTIVE,
	};
	CsvConfigState State;
	uint32_t NumTimers;
	uint32_t NumGroups;
	uint32_t NumCounters;
	uint32_t MaxTimers;
	uint32_t MaxGroups;
	uint32_t MaxCounters;
	uint32_t TotalElements;
	uint16_t* TimerIndices;
	uint16_t* GroupIndices;
	uint16_t* CounterIndices;
	uint64_t* FrameData;
	const char** pTimerNames;
	const char** pGroupNames;
	const char** pCounterNames;
	uint32_t Flags;

};

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4200) // zero-sized struct
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4244) // possible loss of data
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4091)
#pragma warning(disable : 4189) // local variable is initialized but not referenced. (for defer local variables)
#endif

struct MicroProfileStringBlock
{
	enum
	{
		DEFAULT_SIZE = 8192,
	};
	MicroProfileStringBlock* pNext;
	uint32_t nUsed;
	uint32_t nSize;
	char Memory[];
};

struct MicroProfileHashTableEntry
{
	uint64_t Key;
	uint64_t Hash;
	uintptr_t Value;
};

struct MicroProfileHashTable
{
	MicroProfileHashTableEntry* pEntries;
	uint32_t nUsed;
	uint32_t nAllocated;
	uint32_t nSearchLimit;
	uint32_t nLim;
	MicroProfileHashCompareFunction CompareFunc;
	MicroProfileHashFunction HashFunc;
};

struct MicroProfileHashTableIterator
{
	MicroProfileHashTableIterator(uint32_t nIndex, MicroProfileHashTable* pTable)
		: nIndex(nIndex)
		, pTable(pTable)
	{
	}
	MicroProfileHashTableIterator(const MicroProfileHashTableIterator& other)
		: nIndex(other.nIndex)
		, pTable(other.pTable)
	{
	}

	uint32_t nIndex;
	MicroProfileHashTable* pTable;

	void AssertValid()
	{
		MP_ASSERT(nIndex < pTable->nAllocated);
	}

	MicroProfileHashTableEntry& operator*()
	{
		AssertValid();
		return pTable->pEntries[nIndex];
	}
	MicroProfileHashTableEntry* operator->()
	{
		AssertValid();
		return &pTable->pEntries[nIndex];
	}
	bool operator==(const MicroProfileHashTableIterator& rhs)
	{
		return nIndex == rhs.nIndex && pTable == rhs.pTable;
	}
	bool operator!=(const MicroProfileHashTableIterator& rhs)
	{
		return nIndex != rhs.nIndex || pTable != rhs.pTable;
	}

	void SkipInvalid()
	{
		while(nIndex < pTable->nAllocated && pTable->pEntries[nIndex].Hash == 0)
			nIndex++;
	}
	MicroProfileHashTableIterator operator++()
	{
		AssertValid();
		nIndex++;
		SkipInvalid();
		return *this;
	}
	MicroProfileHashTableIterator operator++(int)
	{
		MicroProfileHashTableIterator tmp = *this;
		++(*this);
		return tmp;
	}
};

struct MicroProfileStrings
{
	MicroProfileHashTable HashTable;
	MicroProfileStringBlock* pFirst;
	MicroProfileStringBlock* pLast;
};

struct MicroProfileThreadLog
{

	std::atomic<uint32_t> nPut;
	std::atomic<uint32_t> nGet;

	MicroProfileLogEntry Log[MICROPROFILE_BUFFER_SIZE];

	uint32_t nStackPut;
	uint32_t nStackScope;
	MicroProfileScopeStateC ScopeState[MICROPROFILE_STACK_MAX];

	uint32_t nActive;
	uint32_t nGpu;
	MicroProfileThreadIdType nThreadId;
	uint32_t nLogIndex;
	uint32_t nCustomId;
	uint32_t nIdleFrames;

	MicroProfileLogEntry nStackLogEntry[MICROPROFILE_STACK_MAX];
	uint64_t nChildTickStack[MICROPROFILE_STACK_MAX + 1];
	int32_t nStackPos;

	uint8_t nGroupStackPos[MICROPROFILE_MAX_GROUPS];
	uint64_t nGroupTicks[MICROPROFILE_MAX_GROUPS];
	uint64_t nAggregateGroupTicks[MICROPROFILE_MAX_GROUPS];
	enum
	{
		THREAD_MAX_LEN = 64,
	};
	char ThreadName[64];
	int nFreeListNext;
};

struct MicroProfileWebSocketBuffer
{
	char* pBufferAllocation;
	char* pBuffer;
	uint32_t nBufferSize;
	uint32_t nPut;
	MpSocket Socket;

	char SendBuffer[MICROPROFILE_WEBSOCKET_BUFFER_SIZE];
	std::atomic<uint32_t> nSendPut;
	std::atomic<uint32_t> nSendGet;
};

typedef void (*MicroProfileHookFunc)(int x);

struct MicroProfilePatchError
{
	unsigned char Code[32];
	char Message[256];
	int AlreadyInstrumented;
	int nCodeSize;
};

// linear, per-frame per-thread gpu log
struct MicroProfileThreadLogGpu
{
	MicroProfileLogEntry Log[MICROPROFILE_GPU_BUFFER_SIZE];
	uint32_t nPut;
	uint32_t nStart;
	uint32_t nId;
	void* pContext;
	uint32_t nAllocated;

	uint32_t nStackScope;
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
#elif MICROPROFILE_GPU_TIMERS_D3D12
#include <d3d12.h>

#ifndef MICROPROFILE_D3D_MAX_QUERIES
#define MICROPROFILE_D3D_MAX_QUERIES (32 << 10)
#endif

#define MICROPROFILE_D3D_MAX_NODE_COUNT 4
#define MICROPROFILE_D3D_INTERNAL_DELAY 8

#define MP_NODE_MASK_ALL(n) ((1u << (n)) - 1u)
#define MP_NODE_MASK_ONE(n) (1u << (n))

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
	} NodeState[MICROPROFILE_D3D_MAX_NODE_COUNT];

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

// enum {
// 	MICROPROFILE_SYMBOLSTATE_DEFAULT = 0,
// 	MICROPROFILE_SYMBOLSTATE_LOADING = 1,
// 	MICROPROFILE_SYMBOLSTATE_DONE = 2,
// };

struct MicroProfileSymbolState
{
	std::atomic<int> nModuleLoadsFinished;
	std::atomic<int> nModuleLoadsRequested;
	std::atomic<int64_t> nSymbolsLoaded;
};

struct MicroProfileSymbolModuleRegion
{
	intptr_t nBegin;
	intptr_t nEnd;
};
struct MicroProfileSymbolModule
{
	uint32_t nMatchOffset;
	uint32_t nStringOffset;
	const char* pBaseString;
	const char* pTrimmedString;
	MicroProfileSymbolModuleRegion Regions[MICROPROFILE_MAX_MODULE_EXEC_REGIONS];
	int nNumExecutableRegions;
	intptr_t nProgress;
	intptr_t nProgressTarget;
	struct MicroProfileSymbolBlock* pSymbolBlock;

	int64_t nSymbols;
	std::atomic<int64_t> nSymbolsLoaded;
	std::atomic<int> nModuleLoadRequested;
	std::atomic<int> nModuleLoadFinished;
};

struct MicroProfile
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nCategoryCount;
	uint32_t nAggregateClear;
	uint32_t nAggregateFlip;
	uint32_t nAggregateFlipCount;
	uint32_t nAggregateFrames;

	uint64_t nFlipStartTick;
	uint64_t nAggregateFlipTick;

	uint32_t nDisplay;
	uint32_t nBars;
	uint32_t nActiveGroups[MICROPROFILE_MAX_GROUP_INTS];
	bool AnyActive;
	uint32_t nFrozen;
	uint32_t nWasFrozen;
	uint32_t nPlatformMarkersEnabled;

	uint32_t nForceEnable;

	uint32_t nForceGroups[MICROPROFILE_MAX_GROUP_INTS];
	uint32_t nActiveGroupsWanted[MICROPROFILE_MAX_GROUP_INTS];
	uint32_t nGroupMask[MICROPROFILE_MAX_GROUP_INTS];

	uint32_t nStartEnabled;
	uint32_t nAllThreadsWanted;

	uint32_t nOverflow;

	uint32_t nMaxGroupSize;
	uint32_t nDumpFileNextFrame;
	uint32_t nDumpFileCountDown;
	uint32_t nDumpSpikeMask;
	uint32_t nAutoClearFrames;

	float fDumpCpuSpike;
	float fDumpGpuSpike;
	char HtmlDumpPath[512];
	char CsvDumpPath[512];
	uint32_t DumpFrameCount;

	int64_t nPauseTicks;
	std::atomic<int64_t> nContextSwitchStalledTick;
	int64_t nContextSwitchLastPushed;
	int64_t nContextSwitchLastIndexPushed;

	float fReferenceTime;
	float fRcpReferenceTime;

	MicroProfileCategory CategoryInfo[MICROPROFILE_MAX_CATEGORIES];
	MicroProfileGroupInfo GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo TimerInfo[MICROPROFILE_MAX_TIMERS];
	uint32_t TimerToGroup[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer AccumTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t AccumMaxTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t AccumMinTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t AccumTimersExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t AccumMaxTimersExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer Frame[MICROPROFILE_MAX_TIMERS];
	uint64_t FrameExclusive[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t AggregateMax[MICROPROFILE_MAX_TIMERS];
	uint64_t AggregateMin[MICROPROFILE_MAX_TIMERS];
	uint64_t AggregateExclusive[MICROPROFILE_MAX_TIMERS];
	uint64_t AggregateMaxExclusive[MICROPROFILE_MAX_TIMERS];

	uint32_t FrameGroupThreadValid[MICROPROFILE_MAX_THREADS / 32 + 1];
	struct GroupTime
	{
		uint64_t nTicks;
		uint64_t nTicksExclusive;
		uint32_t nCount;
	};

	GroupTime FrameGroupThread[MICROPROFILE_MAX_THREADS][MICROPROFILE_MAX_GROUPS];
	GroupTime FrameGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t AccumGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t AccumGroupMax[MICROPROFILE_MAX_GROUPS];

	uint64_t AggregateGroup[MICROPROFILE_MAX_GROUPS];
	uint64_t AggregateGroupMax[MICROPROFILE_MAX_GROUPS];

	MicroProfileGraphState Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t nGraphPut;

	uint32_t nThreadActive[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLog* Pool[MICROPROFILE_MAX_THREADS];
	MicroProfileThreadLogGpu* PoolGpu[MICROPROFILE_MAX_THREADS];

	MicroProfileThreadLog TimelineLog;
	uint32_t TimelineTokenFrameEnter[MICROPROFILE_TIMELINE_MAX_TOKENS];
	uint32_t TimelineTokenFrameLeave[MICROPROFILE_TIMELINE_MAX_TOKENS];
	uint32_t TimelineToken[MICROPROFILE_TIMELINE_MAX_TOKENS];
	const char* TimelineTokenStaticString[MICROPROFILE_TIMELINE_MAX_TOKENS];

	uint32_t nTimelineFrameMax;
	MicroProfileFrameExtraCounterData* FrameExtraCounterData;
	MicroProfileCsvConfig CsvConfig;

	uint32_t nNumLogs;
	uint32_t nNumLogsGpu;
	uint32_t nMemUsage;
	int nFreeListHead;

	MicroProfileToken nTokenNegativeCpu;
	MicroProfileToken nTokenNegativeGpu;
	uint32_t nTimerNegativeCpuIndex;
	uint32_t nTimerNegativeGpuIndex;

	uint32_t nFrameCurrent;
	uint32_t nFrameCurrentIndex;
	uint32_t nFramePut;
	uint32_t nFrameNext;
	uint64_t nFramePutIndex;

	MicroProfileFrameState Frames[MICROPROFILE_MAX_FRAME_HISTORY];

	uint64_t nFlipTicks;
	uint64_t nFlipAggregate;
	uint64_t nFlipMax;
	uint64_t nFlipAggregateDisplay;
	uint64_t nFlipMaxDisplay;

	MicroProfileThread ContextSwitchThread;
	bool bContextSwitchRunning;
	bool bContextSwitchStop;
	bool bContextSwitchAllThreads;
	bool bContextSwitchNoBars;
	uint32_t nContextSwitchUsage;
	uint32_t nContextSwitchLastPut;

	int64_t nContextSwitchHoverTickIn;
	int64_t nContextSwitchHoverTickOut;
	uint32_t nContextSwitchHoverThread;
	uint32_t nContextSwitchHoverThreadBefore;
	uint32_t nContextSwitchHoverThreadAfter;
	uint8_t nContextSwitchHoverCpu;
	uint8_t nContextSwitchHoverCpuNext;

	uint32_t CoreCount;
	uint8_t CoreEfficiencyClass[MICROPROFILE_MAX_CPU_CORES];

	uint32_t nContextSwitchPut;
	MicroProfileContextSwitch ContextSwitch[MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE];

	MpSocket ListenerSocket;
	uint32_t nWebServerPort;

	char WebServerBuffer[MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE];
	uint32_t WebServerPut;

	uint64_t nWebServerDataSent;

	int WebSocketTimers;
	uint32_t nWebSocketDirty;
	MpSocket WebSockets[1];
	int64_t WebSocketFrameLast[1];
	uint32_t nNumWebSockets;
	uint32_t nSocketFail; // for error propagation.

	MicroProfileThread WebSocketSendThread;
	bool WebSocketThreadRunning;
	bool WebSocketThreadJoined;

	uint32_t WSCategoriesSent;
	uint32_t WSGroupsSent;
	uint32_t WSTimersSent;
	uint32_t WSCountersSent;
	MicroProfileWebSocketBuffer WSBuf;
	char* pJsonSettings;
	bool bJsonSettingsReadOnly;
	uint32_t nJsonSettingsPending;
	uint32_t nJsonSettingsBufferSize;
	uint32_t nWSWasConnected;
	uint32_t nMicroProfileShutdown;
	uint32_t nWSViewMode;

	char CounterNames[MICROPROFILE_MAX_COUNTER_NAME_CHARS];
	MicroProfileCounterInfo CounterInfo[MICROPROFILE_MAX_COUNTERS];
	MicroProfileCounterSource CounterSource[MICROPROFILE_MAX_COUNTERS];
	uint32_t nNumCounters;
	uint32_t nCounterNamePos;
	std::atomic<int64_t> Counters[MICROPROFILE_MAX_COUNTERS];
#if MICROPROFILE_COUNTER_HISTORY // uses 1kb per allocated counter. 512kb for default counter count
	uint32_t nCounterHistoryPut;
	int64_t nCounterHistory[MICROPROFILE_GRAPH_HISTORY][MICROPROFILE_MAX_COUNTERS]; // flipped to make swapping cheap, drawing more expensive.
	int64_t nCounterMax[MICROPROFILE_MAX_COUNTERS];
	int64_t nCounterMin[MICROPROFILE_MAX_COUNTERS];
#endif

	MicroProfileThread AutoFlipThread;
	std::atomic<uint32_t> nAutoFlipDelay;
	std::atomic<uint32_t> nAutoFlipStop;

	MicroProfileStrings Strings;
	MicroProfileToken CounterToken_MicroProfile;
	MicroProfileToken CounterToken_StringBlock;
	MicroProfileToken CounterToken_StringBlock_Count;
	MicroProfileToken CounterToken_StringBlock_Waste;
	MicroProfileToken CounterToken_StringBlock_Strings;
	MicroProfileToken CounterToken_StringBlock_Memory;

	MicroProfileToken CounterToken_Alloc;
	MicroProfileToken CounterToken_Alloc_Memory;
	MicroProfileToken CounterToken_Alloc_Count;

#if MICROPROFILE_DYNAMIC_INSTRUMENT
	uint32_t DynamicTokenIndex;
	MicroProfileToken DynamicTokens[MICROPROFILE_MAX_DYNAMIC_TOKENS];
	void* FunctionsInstrumented[MICROPROFILE_MAX_DYNAMIC_TOKENS];
	const char* FunctionsInstrumentedName[MICROPROFILE_MAX_DYNAMIC_TOKENS];
	const char* FunctionsInstrumentedModuleNames[MICROPROFILE_MAX_DYNAMIC_TOKENS];
	const char* FunctionsInstrumentedUnmangled[MICROPROFILE_MAX_DYNAMIC_TOKENS];
	uint32_t WSFunctionsInstrumentedSent;
	MicroProfileSymbolState SymbolState;

	MicroProfileSymbolModule SymbolModules[MICROPROFILE_INSTRUMENT_MAX_MODULES];
	char SymbolModuleNameBuffer[MICROPROFILE_INSTRUMENT_MAX_MODULE_CHARS];
	int SymbolModuleNameOffset;
	int SymbolNumModules;
	int WSSymbolModulesSent;
	std::atomic<int> nSymbolsDirty;

	MicroProfileFunctionQuery* pPendingQuery;
	MicroProfileFunctionQuery* pFinishedQuery;
	MicroProfileFunctionQuery* pQueryFreeList;
	uint32_t nQueryProcessed;
	uint32_t nNumQueryFree;
	uint32_t nNumQueryAllocated;

	int SymbolThreadRunning;
	int SymbolThreadFinished;
	MicroProfileThread SymbolThread;
	int nNumPatchErrors;
	MicroProfilePatchError PatchErrors[MICROPROFILE_MAX_PATCH_ERRORS];
	int nNumPatchErrorFunctions;
	const char* PatchErrorFunctionNames[MICROPROFILE_MAX_PATCH_ERRORS];
#endif

	int GpuQueue;
	MicroProfileThreadLogGpu* pGpuGlobal;
	MicroProfileGpuTimerState* pGPU;
};

inline uint32_t MicroProfileLogGetType(MicroProfileLogEntry Index)
{
	return ((MP_LOG_BEGIN_MASK & Index) >> 62) & 0x3;
}

inline uint64_t MicroProfileLogGetTimerIndex(MicroProfileLogEntry Index)
{
	return (0x3fff & (Index >> 48));
}
uint32_t MicroProfileLogGetDataSize(MicroProfileLogEntry Index)
{
	if(MicroProfileLogGetType(Index) == MP_LOG_EXTENDED)
		return 0xffff & (Index >> 32);
	else
		return 0;
}

inline EMicroProfileTokenExtended MicroProfileLogGetExtendedToken(MicroProfileLogEntry Index)
{
	return (EMicroProfileTokenExtended)(0x3fff & (Index >> 48));
}

inline uint32_t MicroProfileLogGetExtendedDataSize(MicroProfileLogEntry Index)
{
	return (uint32_t)(0xffff & (Index >> 32));
}

inline uint32_t MicroProfileLogGetExtendedPayload(MicroProfileLogEntry Index)
{
	return (uint32_t)(0xffffffff & Index);
}

inline uint64_t MicroProfileLogGetExtendedPayloadNoData(MicroProfileLogEntry Index)
{
	return (uint64_t)(MP_LOG_TICK_MASK & Index);
}

inline void* MicroProfileLogGetExtendedPayloadNoDataPtr(MicroProfileLogEntry Index)
{
	return (void*)(MP_LOG_PAYLOAD_PTR_MASK & Index);
}

MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick);
MicroProfileLogEntry MicroProfileMakeLogExtended(EMicroProfileTokenExtended eTokenExt, uint32_t nDataSizeQWords, uint32_t nPayload);
MicroProfileLogEntry MicroProfileMakeLogExtendedNoData(EMicroProfileTokenExtended eTokenExt, uint64_t nTick);

inline MicroProfileLogEntry MicroProfileMakeLogIndex(uint64_t nBegin, MicroProfileToken nToken, int64_t nTick)
{
	MicroProfileLogEntry Entry = (nBegin << 62) | ((0x3fff & nToken) << 48) | (MP_LOG_TICK_MASK & nTick);
	uint32_t t = MicroProfileLogGetType(Entry);
	uint64_t nTimerIndex = MicroProfileLogGetTimerIndex(Entry);
	MP_ASSERT(t == nBegin);
	MP_ASSERT(nTimerIndex == (nToken & 0x3fff));
	return Entry;
}

// extended data, with the option to store 0xfffe * 8 bytes after
inline MicroProfileLogEntry MicroProfileMakeLogExtended(EMicroProfileTokenExtended eTokenExt, uint32_t nDataSizeQWords, uint32_t nPayload)
{
	MP_ASSERT(nDataSizeQWords < 0xffff);
	MicroProfileLogEntry Entry = (((uint64_t)MP_LOG_EXTENDED) << 62) | ((0x3fff & (uint64_t)eTokenExt) << 48) | ((0xffff & (uint64_t)nDataSizeQWords) << 32) | nPayload;

	MP_ASSERT(MicroProfileLogGetExtendedToken(Entry) == eTokenExt);
	MP_ASSERT(MicroProfileLogGetExtendedDataSize(Entry) == nDataSizeQWords);
	MP_ASSERT(MicroProfileLogGetExtendedPayload(Entry) == nPayload);

	return Entry;
}
// extended with no data, but instead 48 bits payload
inline MicroProfileLogEntry MicroProfileMakeLogExtendedNoData(EMicroProfileTokenExtended eTokenExt, uint64_t nPayload)
{
	MicroProfileLogEntry Entry = (((uint64_t)MP_LOG_EXTENDED_NO_DATA) << 62) | ((0x3fff & (uint64_t)eTokenExt) << 48) | (MP_LOG_TICK_MASK & nPayload);

	MP_ASSERT(MicroProfileLogGetExtendedToken(Entry) == eTokenExt);
	MP_ASSERT(MicroProfileLogGetExtendedPayloadNoData(Entry) == nPayload);

	return Entry;
}

// extended with no data, but instead 61 bits payload. used to store a pointer.
inline MicroProfileLogEntry MicroProfileMakeLogExtendedNoDataPtr(uint64_t nPayload)
{
	uint64_t hest = ETOKEN_CSTR_PTR;
	MicroProfileLogEntry Entry = (((uint64_t)MP_LOG_EXTENDED_NO_DATA) << 62) | (hest << 48) | (MP_LOG_PAYLOAD_PTR_MASK & nPayload);
	uint64_t v0 = (MP_LOG_PAYLOAD_PTR_MASK & nPayload);
	uint64_t v1 = (uint64_t)MicroProfileLogGetExtendedPayloadNoDataPtr(Entry);

	MP_ASSERT(v0 == v1);
	return Entry;
}

inline uint32_t MicroProfileGetQWordSize(uint32_t nDataSize)
{
	uint32_t nSize = (nDataSize + 7) / 8;
	MP_ASSERT(nSize < 0xffff); // won't pack...
	return nSize;
}

namespace
{
struct MicroProfilePayloadPack
{
	union
	{
		struct
		{
#if MICROPROFILE_BIG_ENDIAN /// NOT implemented.
			char h;
			char message[7];
#else
			char message[7];
			char h;
#endif
		};
		uint64_t LogEntry;
	};
};
}; // namespace

inline int64_t MicroProfileLogTickDifference(MicroProfileLogEntry Start, MicroProfileLogEntry End)
{
	int64_t nStart = Start;
	int64_t nEnd = End;
	int64_t nDifference = ((nEnd << 16) - (nStart << 16));
	return nDifference >> 16;
}
inline int64_t MicroProfileLogTickMax(MicroProfileLogEntry A, MicroProfileLogEntry B)
{
	int64_t Diff = MicroProfileLogTickDifference(A, B);
	if(Diff < 0)
	{
		return A;
	}
	else
	{
		return B;
	}
}

inline int64_t MicroProfileLogTickMin(MicroProfileLogEntry A, MicroProfileLogEntry B)
{
	int64_t Diff = MicroProfileLogTickDifference(A, B);
	if(Diff < 0)
	{
		return B;
	}
	else
	{
		return A;
	}
}
inline int64_t MicroProfileLogTickClamp(uint64_t T, uint64_t min, uint64_t max)
{
	return MicroProfileLogTickMin(MicroProfileLogTickMax(T, min), max);
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
inline uint32_t MicroProfileGetGroupMask(MicroProfileToken t)
{
	return (uint32_t)((t >> 16) & MICROPROFILE_GROUP_MASK_ALL);
}
inline uint32_t MicroProfileGetGroupMaskIndex(MicroProfileToken t)
{
	return (uint32_t)(t >> 48);
}

inline MicroProfileToken MicroProfileMakeToken(uint32_t nGroupMask, uint16_t nGroupIndex, uint16_t nTimer)
{
	uint64_t token = ((uint64_t)nGroupIndex << 48llu) | ((uint64_t)nGroupMask << 16llu) | nTimer;
	if(0 != (token & MP_LOG_CSTR_MASK))
	{
		MP_BREAK(); // should never happen
	}
	return token;
}

template <typename T>
T MicroProfileMin(T a, T b)
{
	return a < b ? a : b;
}

template <typename T>
T MicroProfileMax(T a, T b)
{
	return a > b ? a : b;
}
template <typename T>
T MicroProfileClamp(T a, T min_, T max_)
{
	return MicroProfileMin(max_, MicroProfileMax(min_, a));
}

inline int64_t MicroProfileMsToTick(float fMs, int64_t nTicksPerSecond)
{
	return (int64_t)(fMs * 0.001f * nTicksPerSecond);
}

inline float MicroProfileTickToMsMultiplier(int64_t nTicksPerSecond)
{
	return 1000.f / (nTicksPerSecond ? nTicksPerSecond : 1);
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
#define fopen microprofile_fopen_helper

FILE* microprofile_fopen_helper(const char* filename, const char* mode)
{
	FILE* F = 0;
	if(0 == fopen_s(&F, filename, mode))
	{
		return F;
	}
	return 0;
}

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

#ifndef _WIN32
typedef pthread_t MicroProfileThread;
void MicroProfileThreadStart(MicroProfileThread* pThread, MicroProfileThreadFunc Func)
{
	pthread_attr_t Attr;
	int r = pthread_attr_init(&Attr);
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
DWORD __stdcall ThreadTrampoline(void* pFunc)
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
	*pThread = MP_ALLOC_OBJECT(std::thread);
	new(*pThread) std::thread(Func, nullptr);
}
inline void MicroProfileThreadJoin(MicroProfileThread* pThread)
{
	(*pThread)->join();
	(*pThread)->~thread();
	MP_FREE(*pThread);
	*pThread = 0;
}
#endif
#endif

#if MICROPROFILE_WEBSERVER

#ifdef _WIN32
#define MP_INVALID_SOCKET(f) (f == INVALID_SOCKET)
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#define MP_INVALID_SOCKET(f) (f < 0)
#endif

void MicroProfileWebServerStart();
void MicroProfileWebServerStop();
void MicroProfileWebServerJoin();
bool MicroProfileWebServerUpdate();
void MicroProfileDumpToFile();

#else

#define MicroProfileWebServerStart()                                                                                                                                                                   \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
#define MicroProfileWebServerStop()                                                                                                                                                                    \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
#define MicroProfileWebServerJoin()                                                                                                                                                                    \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
#define MicroProfileWebServerUpdate() false
#define MicroProfileDumpToFile()                                                                                                                                                                       \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
#endif

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#if MICROPROFILE_DEBUG
#ifdef _WIN32
void uprintf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char buffer[1024];
	stbsp_vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	OutputDebugStringA(buffer);
	va_end(args);
}
#else
#define uprintf(...) printf(__VA_ARGS__)
#endif
#else
#define uprintf(...)                                                                                                                                                                                   \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
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
MICROPROFILE_DEFINE(g_MicroProfileContextSwitchSearch, "MicroProfile", "ContextSwitchSearch", MP_GREEN4);
MICROPROFILE_DEFINE(g_MicroProfileGpuSubmit, "MicroProfile", "MicroProfileGpuSubmit", MP_HOTPINK2);
MICROPROFILE_DEFINE(g_MicroProfileSendLoop, "MicroProfile", "MicroProfileSocketSendLoop", MP_GREEN4);
MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(g_MicroProfileBytesPerFlip, "microprofile/bytesperflip");

void MicroProfileHashTableInit(MicroProfileHashTable* pTable, uint32_t nInitialSize, MicroProfileHashCompareFunction CompareFunc, MicroProfileHashFunction HashFunc);
void MicroProfileHashTableDestroy(MicroProfileHashTable* pTable);
uint64_t MicroProfileHashTableHash(MicroProfileHashTable* pTable, uint64_t K);
void MicroProfileHashTableGrow(MicroProfileHashTable* pTable);

bool MicroProfileHashTableSet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t Value, uint64_t H, bool bAllowGrow);
bool MicroProfileHashTableGet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t* pValue);
bool MicroProfileHashTableRemove(MicroProfileHashTable* pTable, uint64_t Key);

bool MicroProfileHashTableSetString(MicroProfileHashTable* pTable, const char* pKey, const char* pValue);
bool MicroProfileHashTableGetString(MicroProfileHashTable* pTable, const char* pKey, const char** pValue);
bool MicroProfileHashTableRemoveString(MicroProfileHashTable* pTable, const char* pKey);
enum
{
	ESTRINGINTERN_LOWERCASE = 1,
};
const char* MicroProfileStringIntern(const char* pStr);
const char* MicroProfileStringInternLower(const char* pStr);
const char* MicroProfileStringIntern(const char* pStr, uint32_t nLen, uint32_t nInternalFlags = 0);

void MicroProfileStringsInit(MicroProfileStrings* pStrings);
void MicroProfileStringsDestroy(MicroProfileStrings* pStrings);

MicroProfileToken MicroProfileCounterTokenInit(int nParent);
void MicroProfileCounterTokenInitName(MicroProfileToken nToken, const char* pName);
void MicroProfileCounterConfigInternal(MicroProfileToken, uint32_t eFormat, int64_t nLimit, uint32_t nFlags);
uint16_t MicroProfileFindGroup(const char* pGroup);

inline std::recursive_mutex& MicroProfileMutex()
{
	static std::recursive_mutex Mutex;
	return Mutex;
}
std::recursive_mutex& MicroProfileGetMutex()
{
	return MicroProfileMutex();
}

inline std::recursive_mutex& MicroProfileTimelineMutex()
{
	static std::recursive_mutex Mutex;
	return Mutex;
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
		bOnce = false;
		memset(&S, 0, sizeof(S));

		MicroProfileStringsInit(&S.Strings);

		// these strings are used for counter names inside the string
		S.CounterToken_MicroProfile = MicroProfileCounterTokenInit(-1);
		S.CounterToken_StringBlock = MicroProfileCounterTokenInit(S.CounterToken_MicroProfile);
		S.CounterToken_StringBlock_Count = MicroProfileCounterTokenInit(S.CounterToken_StringBlock);
		S.CounterToken_StringBlock_Waste = MicroProfileCounterTokenInit(S.CounterToken_StringBlock);
		S.CounterToken_StringBlock_Strings = MicroProfileCounterTokenInit(S.CounterToken_StringBlock);
		S.CounterToken_StringBlock_Memory = MicroProfileCounterTokenInit(S.CounterToken_StringBlock);

		S.CounterToken_Alloc = MicroProfileCounterTokenInit(S.CounterToken_MicroProfile);
		S.CounterToken_Alloc_Memory = MicroProfileCounterTokenInit(S.CounterToken_Alloc);
		S.CounterToken_Alloc_Count = MicroProfileCounterTokenInit(S.CounterToken_Alloc);

		MicroProfileCounterTokenInitName(S.CounterToken_MicroProfile, "microprofile");
		MicroProfileCounterTokenInitName(S.CounterToken_StringBlock, "stringblock");
		MicroProfileCounterTokenInitName(S.CounterToken_StringBlock_Count, "count");
		MicroProfileCounterTokenInitName(S.CounterToken_StringBlock_Waste, "waste");
		MicroProfileCounterTokenInitName(S.CounterToken_StringBlock_Strings, "strings");
		MicroProfileCounterTokenInitName(S.CounterToken_StringBlock_Memory, "memory");

		MicroProfileCounterTokenInitName(S.CounterToken_Alloc, "alloc");
		MicroProfileCounterTokenInitName(S.CounterToken_Alloc_Memory, "memory");
		MicroProfileCounterTokenInitName(S.CounterToken_Alloc_Count, "count");

		S.nMemUsage += sizeof(S);
		for(int i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
		{
			S.GroupInfo[i].pName[0] = '\0';
		}
		for(int i = 0; i < MICROPROFILE_MAX_CATEGORIES; ++i)
		{
			S.CategoryInfo[i].pName[0] = '\0';
			memset(S.CategoryInfo[i].nGroupMask, 0, sizeof(S.CategoryInfo[i].nGroupMask));
		}
		memcpy(&S.CategoryInfo[0].pName[0], "default", sizeof("default"));
		S.nCategoryCount = 1;
		for(int i = 0; i < MICROPROFILE_MAX_TIMERS; ++i)
		{
			S.TimerInfo[i].pName[0] = '\0';
		}
		S.nGroupCount = 0;
		S.nFlipStartTick = MP_TICK();
		S.nContextSwitchStalledTick = MP_TICK();
		S.nAggregateFlipTick = MP_TICK();
		memset(S.nActiveGroups, 0, sizeof(S.nActiveGroups));
		S.nFrozen = 0;
		S.nWasFrozen = 0;
		memset(S.nForceGroups, 0, sizeof(S.nForceGroups));
		memset(S.nActiveGroupsWanted, 0, sizeof(S.nActiveGroupsWanted));
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
			S.Frames[i].nFrameStartGpu = MICROPROFILE_INVALID_TICK;
		}
		S.nWebServerPort = MICROPROFILE_WEBSERVER_PORT; // Use defined value as default port
		S.nWebServerDataSent = (uint64_t)-1;
		S.WebSocketTimers = -1;
		S.nSocketFail = 0;

		S.DumpFrameCount = MICROPROFILE_WEBSERVER_DEFAULT_FRAMES;

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

		for(uint32_t i = 0; i < MICROPROFILE_TIMELINE_MAX_TOKENS; ++i)
		{
			S.TimelineTokenFrameEnter[i] = MICROPROFILE_INVALID_FRAME;
			S.TimelineTokenFrameLeave[i] = MICROPROFILE_INVALID_FRAME;
			S.TimelineTokenStaticString[i] = nullptr;
			S.TimelineToken[i] = 0;
		}
		S.nTokenNegativeCpu = MicroProfileGetToken("__NEGATIVE_CPU", "NEGATIVE_CPU", MP_PURPLE, MicroProfileTokenTypeCpu, 0);
		S.nTokenNegativeGpu = MicroProfileGetToken("__NEGATIVE_GPU", "NEGATIVE_GPU", MP_PURPLE, MicroProfileTokenTypeGpu, 0);
		S.nTimerNegativeCpuIndex = MicroProfileGetTimerIndex(S.nTokenNegativeCpu);
		S.nTimerNegativeGpuIndex = MicroProfileGetTimerIndex(S.nTokenNegativeGpu);
	}
#if MICROPROFILE_FRAME_EXTRA_DATA
	S.FrameExtraCounterData = (MicroProfileFrameExtraCounterData*)1;
#endif
	MicroProfileCounterConfigInternal(S.CounterToken_Alloc_Memory, MICROPROFILE_COUNTER_FORMAT_BYTES, 0, MICROPROFILE_COUNTER_FLAG_DETAILED);
	MICROPROFILE_COUNTER_CONFIG("MicroProfile/ThreadLog/Memory", MICROPROFILE_COUNTER_FORMAT_BYTES, 0, MICROPROFILE_COUNTER_FLAG_DETAILED);

	if(bUseLock)
	{
		mutex.unlock();
	}
}

void MicroProfileJoinContextSwitchTrace();

void MicroProfileShutdown()
{
	{
		std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
		S.nMicroProfileShutdown = 1;
		MicroProfileStopContextSwitchTrace();
	}
	MicroProfileWebServerJoin();
	MicroProfileJoinContextSwitchTrace();
	{

		std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
		if(S.pJsonSettings)
		{
			MP_FREE(S.pJsonSettings);
			S.pJsonSettings = 0;
			S.nJsonSettingsBufferSize = 0;
		}
		if(S.pGPU)
		{
			MicroProfileGpuShutdown();
		}
		MicroProfileHashTableDestroy(&S.Strings.HashTable);
		MicroProfileStringsDestroy(&S.Strings);
		MICROPROFILE_FREE_NON_ALIGNED(S.WSBuf.pBufferAllocation);

		MicroProfileFreeGpuQueue(S.GpuQueue);
		MicroProfileThreadLogGpuFree(S.pGpuGlobal);

		for(uint32_t i = 0; i < S.nNumLogs; ++i)
		{
#if MICROPROFILE_ASSERT_LOG_FREED
			MP_ASSERT(S.Pool[i]->nActive != 1);
#endif
			MP_FREE(S.Pool[i]);
		}

		for(uint32_t i = 0; i < S.nNumLogsGpu; ++i)
		{
#if MICROPROFILE_ASSERT_LOG_FREED
			MP_ASSERT(!S.PoolGpu[i]->nAllocated);
#endif
			MP_FREE(S.PoolGpu[i]);
		}
	}
}

static void* MicroProfileAutoFlipThread(void*)
{
	MicroProfileOnThreadCreate("AutoFlipThread");
	while(0 == S.nAutoFlipStop.load())
	{
		MICROPROFILE_SCOPEI("MICROPROFILE", "AutoFlipThread", 0);
		MicroProfileSleep(S.nAutoFlipDelay);
		MicroProfileFlip(0);
	}
	MicroProfileOnThreadExit();
	return 0;
}

void MicroProfileStartAutoFlip(uint32_t nMsDelay)
{
	S.nAutoFlipDelay = nMsDelay;
	S.nAutoFlipStop.store(0);
	MicroProfileThreadStart(&S.AutoFlipThread, MicroProfileAutoFlipThread);
}
void MicroProfileStopAutoFlip()
{
	S.nAutoFlipStop.store(1);
	MicroProfileThreadJoin(&S.AutoFlipThread);
}

void MicroProfileEnableFrameExtraCounterData()
{
	// should not be called at the same time as MicroProfileFlip.
	if(!S.FrameExtraCounterData)
	{
		S.FrameExtraCounterData = (MicroProfileFrameExtraCounterData*)1;
	}
}

void MicroProfileCsvConfigEnd()
{
	MP_ASSERT(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG);
	S.CsvConfig.State = MicroProfileCsvConfig::ACTIVE;
}
void MicroProfileCsvConfigBegin(uint32_t MaxTimers, uint32_t MaxGroups, uint32_t MaxCounters, uint32_t Flags)
{
	MP_ASSERT(S.CsvConfig.State == MicroProfileCsvConfig::INACTIVE); // right now, only support being configured once.
	uint32_t TotalElements = MaxTimers + MaxGroups + MaxCounters;
	uint32_t BaseSize = (sizeof(MicroProfileCsvConfig) + 7) & 7;
	uint32_t TimerIndexSize = sizeof(uint16_t) * MaxTimers;
	uint32_t GroupIndexSize = sizeof(uint16_t) * MaxGroups;
	uint32_t CounterIndexSize = sizeof(uint16_t) * MaxCounters;
	uint32_t FrameBlockSize = TotalElements * sizeof(uint64_t); 
	uint32_t FrameDataSize = FrameBlockSize * MICROPROFILE_MAX_FRAME_HISTORY; 
	S.CsvConfig.NumTimers = 0;
	S.CsvConfig.NumGroups = 0;
	S.CsvConfig.NumCounters = 0;
	S.CsvConfig.MaxTimers = MaxTimers;
	S.CsvConfig.MaxGroups = MaxGroups;
	S.CsvConfig.MaxCounters = MaxCounters;
	S.CsvConfig.TotalElements = TotalElements;
	S.CsvConfig.TimerIndices = (uint16_t*)MicroProfileAllocInternal(TimerIndexSize, alignof(uint16_t));
	S.CsvConfig.pTimerNames = (const char**)MicroProfileAllocInternal(MaxTimers * sizeof(const char*), alignof(const char*));
	memset(S.CsvConfig.pTimerNames, 0, MaxTimers * sizeof(const char*));
	for(uint32_t i = 0; i < TimerIndexSize; ++i)
		S.CsvConfig.TimerIndices[i] = UINT16_MAX;
	S.CsvConfig.pGroupNames = (const char**)MicroProfileAllocInternal(MaxGroups * sizeof(const char*), alignof(const char*));
	memset(S.CsvConfig.pGroupNames, 0, MaxGroups * sizeof(const char*));
	S.CsvConfig.GroupIndices = (uint16_t*)MicroProfileAllocInternal(GroupIndexSize, alignof(uint16_t));
	for(uint32_t i = 0; i < GroupIndexSize; ++i)
		S.CsvConfig.GroupIndices[i] = UINT16_MAX;
	S.CsvConfig.pCounterNames = (const char**)MicroProfileAllocInternal(MaxCounters * sizeof(const char*), alignof(const char*));
	memset(S.CsvConfig.pCounterNames, 0, MaxCounters * sizeof(const char*));
	S.CsvConfig.CounterIndices = (uint16_t*)MicroProfileAllocInternal(CounterIndexSize, alignof(uint16_t));
	for(uint32_t i = 0; i < CounterIndexSize; ++i)
		S.CsvConfig.CounterIndices[i] = UINT16_MAX;
	S.CsvConfig.FrameData = (uint64_t*)MicroProfileAllocInternal(FrameDataSize, alignof(uint64_t));
	memset(S.CsvConfig.FrameData, 0, FrameDataSize);
	S.CsvConfig.State = MicroProfileCsvConfig::CONFIG;
	S.CsvConfig.Flags = Flags;
}
void MicroProfileCsvConfigAddTimer(const char* Group, const char* Timer, const char* Name)
{
	MP_ASSERT(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG);
	if(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG && S.CsvConfig.NumTimers < S.CsvConfig.MaxTimers)
	{
		MicroProfileToken ret = MicroProfileFindToken(Group, Timer);
		if(ret != MICROPROFILE_INVALID_TOKEN)
		{
			S.CsvConfig.pTimerNames[S.CsvConfig.NumTimers] = Name;
			S.CsvConfig.TimerIndices[S.CsvConfig.NumTimers++] = MicroProfileGetTimerIndex(ret);
		}
	}
}
void MicroProfileCsvConfigAddGroup(const char* Group, const char* Name)
{
	MP_ASSERT(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG);
	if(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG && S.CsvConfig.NumGroups < S.CsvConfig.MaxGroups)
	{
		uint16_t Index = MicroProfileFindGroup(Group);
		MP_ASSERT(UINT16_MAX != Index);
		if(UINT16_MAX != Index)
		{
			S.CsvConfig.pGroupNames[S.CsvConfig.NumGroups] = Name;
			S.CsvConfig.GroupIndices[S.CsvConfig.NumGroups++] = Index;
		}
	}

}
void MicroProfileCsvConfigAddCounter(const char* CounterName, const char* Name)
{
	MP_ASSERT(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG);
	if(S.CsvConfig.State == MicroProfileCsvConfig::CONFIG && S.CsvConfig.NumCounters < S.CsvConfig.MaxCounters)
	{
		MicroProfileToken Token = MicroProfileGetCounterToken(CounterName, MICROPROFILE_COUNTER_TOKEN_DONT_CREATE);
		if(MICROPROFILE_INVALID_TOKEN != Token)
		{
			MP_ASSERT(Token < UINT16_MAX);
			S.CsvConfig.pCounterNames[S.CsvConfig.NumCounters] = Name;
			S.CsvConfig.CounterIndices[S.CsvConfig.NumCounters++] = (uint16_t)Token;
		}
	}
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
	if(!pLog)
	{
		MicroProfileInitThreadLog();
		pLog = MicroProfileGetThreadLog();
	}
	return pLog;
}

struct MicroProfileScopeLock
{
	bool bUseLock;
	int nUnlock;
	std::recursive_mutex& m;
	MicroProfileScopeLock(std::recursive_mutex& m)
		: bUseLock(g_bUseLock)
		, nUnlock(0)
		, m(m)
	{
		if(bUseLock)
			m.lock();
	}
	~MicroProfileScopeLock()
	{
		MP_ASSERT(nUnlock == 0);
		if(bUseLock)
			m.unlock();
	}
	void Unlock()
	{
		MP_ASSERT(bUseLock);
		m.unlock();
		nUnlock++;
	}
	void Lock()
	{
		m.lock();
		nUnlock--;
	}
};

void MicroProfileLogReset(MicroProfileThreadLog* pLog);
void MicroProfileLogClearInternal(MicroProfileThreadLog* pLog);

MicroProfileThreadLog* MicroProfileCreateThreadLog(const char* pName)
{
	MicroProfileScopeLock L(MicroProfileMutex());

	if(S.nNumLogs == MICROPROFILE_MAX_THREADS && S.nFreeListHead == -1)
	{
		uprintf("recycling thread logs\n");
		// reuse the oldest.
		MicroProfileThreadLog* pOldest = 0;
		uint32_t nIdleFrames = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			MicroProfileThreadLog* pLog = S.Pool[i];
			uprintf("tlactive %p, %d.  idle:%d\n", pLog, pLog->nActive, pLog->nIdleFrames);
			if(pLog->nActive == 2)
			{
				if(pLog->nIdleFrames >= nIdleFrames)
				{
					nIdleFrames = pLog->nIdleFrames;
					pOldest = pLog;
				}
			}
		}
		MP_ASSERT(pOldest);
		MicroProfileLogReset(pOldest);
	}

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
		MICROPROFILE_COUNTER_ADD("MicroProfile/ThreadLog/Allocated", 1);
		MICROPROFILE_COUNTER_ADD("MicroProfile/ThreadLog/Memory", sizeof(MicroProfileThreadLog));
		pLog = MP_ALLOC_OBJECT(MicroProfileThreadLog);
		MicroProfileLogClearInternal(pLog);
		S.nMemUsage += sizeof(MicroProfileThreadLog);
		pLog->nLogIndex = S.nNumLogs;
		MP_ASSERT(S.nNumLogs < MICROPROFILE_MAX_THREADS);
		S.Pool[S.nNumLogs++] = pLog;
	}
	int len = 0;
	if(pName)
	{
		len = (int)strlen(pName);
		int maxlen = sizeof(pLog->ThreadName) - 1;
		len = len < maxlen ? len : maxlen;
		memcpy(&pLog->ThreadName[0], pName, len);
	}
	else
	{
		len = snprintf(&pLog->ThreadName[0], sizeof(pLog->ThreadName) - 1, "TID:[%" PRId64 "]", (int64_t)MP_GETCURRENTTHREADID());
	}
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
	pLog->nStackScope = 0;
}

MicroProfileThreadLogGpu* MicroProfileThreadLogGpuAllocInternal()
{
	MicroProfileThreadLogGpu* pLog = 0;
	for(uint32_t i = 0; i < S.nNumLogsGpu; ++i)
	{
		MicroProfileThreadLogGpu* pNextLog = S.PoolGpu[i];
		if(pNextLog && !pNextLog->nAllocated)
		{
			pLog = pNextLog;
			break;
		}
	}
	if(!pLog)
	{
		pLog = MP_ALLOC_OBJECT(MicroProfileThreadLogGpu);
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
	for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; i++)
	{
		MicroProfileThreadLog* pLog = S.Pool[i];
		if(pLog && pLog->nGpu && pLog->nActive && 0 == MP_STRCASECMP(pQueueName, pLog->ThreadName))
		{
			return i;
		}
	}
	MP_ASSERT(0); // call MicroProfileInitGpuQueue
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
	MP_ASSERT(0); // call MicroProfileInitGpuQueue
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

void MicroProfileFreeGpuQueue(int nQueue)
{
	MicroProfileThreadLog* pLog = S.Pool[nQueue];
	if(pLog)
	{
		MP_ASSERT(pLog->nActive == 1);
		pLog->nActive = 2;
	}
}

MicroProfileThreadLogGpu* MicroProfileGetGlobalGpuThreadLog()
{
	return S.pGpuGlobal;
}

MICROPROFILE_API int MicroProfileGetGlobalGpuQueue()
{
	return S.GpuQueue;
}
void MicroProfileLogClearInternal(MicroProfileThreadLog* pLog)
{
	// can't clear atomics..
	void* pStart = (void*)&pLog->Log[0];
	void* pEnd = (void*)(pLog + 1);
	memset(pStart, 0, (uintptr_t)pEnd - (uintptr_t)pStart);
	pLog->nPut.store(0);
	pLog->nGet.store(0);
}
void MicroProfileLogReset(MicroProfileThreadLog* pLog)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());

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
	MicroProfileLogClearInternal(pLog);
	pLog->nFreeListNext = S.nFreeListHead;
	S.nFreeListHead = nLogIndex;
	for(int i = 0; i < MICROPROFILE_MAX_FRAME_HISTORY; ++i)
	{
		S.Frames[i].nLogStart[nLogIndex] = 0;
	}
}

void MicroProfileOnThreadExit()
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
	if(pLog)
	{
		MP_ASSERT(pLog->nActive == 1);
		pLog->nActive = 2;
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
uint16_t MicroProfileFindGroup(const char* pGroup)
{
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		if(!MP_STRCASECMP(pGroup, S.GroupInfo[i].pName))
		{
			return i;
		}
	}
	return UINT16_MAX;
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
	if(nLen > MICROPROFILE_NAME_MAX_LEN - 1)
		nLen = MICROPROFILE_NAME_MAX_LEN - 1;
	memcpy(&S.GroupInfo[S.nGroupCount].pName[0], pGroup, nLen);
	S.GroupInfo[S.nGroupCount].pName[nLen] = '\0';
	S.GroupInfo[S.nGroupCount].nNameLen = nLen;
	S.GroupInfo[S.nGroupCount].nNumTimers = 0;
	S.GroupInfo[S.nGroupCount].nGroupIndex = S.nGroupCount;
	S.GroupInfo[S.nGroupCount].Type = Type;
	S.GroupInfo[S.nGroupCount].nMaxTimerNameLen = 0;
	S.GroupInfo[S.nGroupCount].nColor = 0x42;
	S.GroupInfo[S.nGroupCount].nCategory = 0;
	uint32_t nIndex = S.nGroupCount / 32;
	uint32_t nBit = S.nGroupCount % 32;
	{
		S.CategoryInfo[0].nGroupMask[nIndex] |= (1 << nBit);
	}
	if(S.nStartEnabled)
	{
		S.nActiveGroupsWanted[nIndex] |= (1ll << nBit);
		S.nActiveGroups[nIndex] |= (1ll << nBit);
		S.AnyActive = true;
	}
	nGroupIndex = S.nGroupCount++;
	S.nGroupMask[nIndex] |= (1 << nBit);
	MP_ASSERT(S.nGroupCount < MICROPROFILE_MAX_GROUPS);
	return nGroupIndex;
}

void MicroProfileRegisterGroup(const char* pGroup, const char* pCategory, uint32_t nColor)
{
	MicroProfileScopeLock L(MicroProfileMutex());

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
		if(nLen > MICROPROFILE_NAME_MAX_LEN - 1)
			nLen = MICROPROFILE_NAME_MAX_LEN - 1;
		memcpy(&S.CategoryInfo[nCategoryIndex].pName[0], pCategory, nLen);
		S.CategoryInfo[nCategoryIndex].pName[nLen] = '\0';
	}
	uint16_t nGroup = MicroProfileGetGroup(pGroup, 0 != MP_STRCASECMP(pGroup, "gpu") ? MicroProfileTokenTypeCpu : MicroProfileTokenTypeGpu);
	S.GroupInfo[nGroup].nColor = nColor;
	if(nCategoryIndex >= 0)
	{
		uint32_t nIndex = nGroup / 32;
		uint32_t nBit = nGroup % 32;
		nBit = (1 << nBit);
		uint32_t nOldCategory = S.GroupInfo[nGroup].nCategory;
		S.CategoryInfo[nOldCategory].nGroupMask[nIndex] &= ~nBit;
		S.CategoryInfo[nCategoryIndex].nGroupMask[nIndex] |= nBit;
		S.GroupInfo[nGroup].nCategory = nCategoryIndex;
	}
}

MicroProfileToken MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor, MicroProfileTokenType Type, uint32_t Flags)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	MicroProfileToken ret = MicroProfileFindToken(pGroup, pName);
	if(ret != MICROPROFILE_INVALID_TOKEN)
	{
		int idx = MicroProfileGetTimerIndex(ret);
		MP_ASSERT(S.TimerInfo[idx].Flags == Flags);
		return ret;
	}
	uint16_t nGroupIndex = MicroProfileGetGroup(pGroup, Type);
	uint16_t nTimerIndex = (uint16_t)(S.nTotalTimers++);
	MP_ASSERT(nTimerIndex < MICROPROFILE_MAX_TIMERS);

	uint32_t nBitIndex = nGroupIndex / 32;
	uint32_t nBit = nGroupIndex % 32;
	uint32_t nGroupMask = 1ll << nBit;
	MicroProfileToken nToken = MicroProfileMakeToken(nGroupMask, (uint16_t)nBitIndex, nTimerIndex);
	S.GroupInfo[nGroupIndex].nNumTimers++;
	S.GroupInfo[nGroupIndex].nMaxTimerNameLen = MicroProfileMax(S.GroupInfo[nGroupIndex].nMaxTimerNameLen, (uint32_t)strlen(pName));
	MP_ASSERT(S.GroupInfo[nGroupIndex].Type == Type); // dont mix cpu & gpu timers in the same group
	S.nMaxGroupSize = MicroProfileMax(S.nMaxGroupSize, S.GroupInfo[nGroupIndex].nNumTimers);
	S.TimerInfo[nTimerIndex].nToken = nToken;
	uint32_t nLen = (uint32_t)strlen(pName);
	if(nLen > MICROPROFILE_NAME_MAX_LEN - 1)
		nLen = MICROPROFILE_NAME_MAX_LEN - 1;
	memcpy(&S.TimerInfo[nTimerIndex].pName, pName, nLen);
	snprintf(&S.TimerInfo[nTimerIndex].pNameExt[0], sizeof(S.TimerInfo[nTimerIndex].pNameExt) - 1, "%s %s", S.GroupInfo[nGroupIndex].pName, pName);
	S.TimerInfo[nTimerIndex].pName[nLen] = '\0';
	S.TimerInfo[nTimerIndex].nNameLen = nLen;
	S.TimerInfo[nTimerIndex].nColor = nColor & 0xffffff;
	S.TimerInfo[nTimerIndex].nGroupIndex = nGroupIndex;
	S.TimerInfo[nTimerIndex].nTimerIndex = nTimerIndex;
	S.TimerInfo[nTimerIndex].nWSNext = -1;
	S.TimerInfo[nTimerIndex].bWSEnabled = false;
	S.TimerInfo[nTimerIndex].Type = Type;
	S.TimerInfo[nTimerIndex].Flags = Flags;
	// printf("*** TOKEN %08d %s\\%s .. flags %08x\n", nTimerIndex, pGroup, pName, Flags);
	S.TimerToGroup[nTimerIndex] = nGroupIndex;
	return nToken;
}

// MicroProfileToken MicroProfileGetCStrToken(const char* pCStr)
//{
//	return MicroProfileMakeTokenCStr(pCStr);
//}

void MicroProfileGetTokenC(MicroProfileToken* pToken, const char* pGroup, const char* pName, uint32_t nColor, MicroProfileTokenType Type, uint32_t flags)
{
	if(*pToken == MICROPROFILE_INVALID_TOKEN)
	{
		MicroProfileInit();
		MicroProfileScopeLock L(MicroProfileMutex());
		if(*pToken == MICROPROFILE_INVALID_TOKEN)
		{
			*pToken = MicroProfileGetToken(pGroup, pName, nColor, Type, flags);
		}
	}
}

const char* MicroProfileNextName(const char* pName, char* pNameOut, uint32_t* nSubNameLen)
{
	int nMaxLen = MICROPROFILE_NAME_MAX_LEN - 1;
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
	} while(nCounter >= 0);
	int nOffset = 0;
	while(nIndex >= 0 && nOffset < (int)sizeof(Buffer) - 2)
	{
		uint32_t nLen = S.CounterInfo[nNodes[nIndex]].nNameLen + nOffset; // < sizeof(Buffer)-1
		nLen = MicroProfileMin((uint32_t)(sizeof(Buffer) - 2 - nOffset), nLen);
		memcpy(&Buffer[nOffset], S.CounterInfo[nNodes[nIndex]].pName, nLen);

		nOffset += S.CounterInfo[nNodes[nIndex]].nNameLen + 1;
		if(nIndex)
		{
			Buffer[nOffset++] = '/';
		}
		nIndex--;
	}
	return &Buffer[0];
}

// S.CounterTokenStringBlock = MicroProfileCounterTokenInit(S.CounterTokenMicroProfile);
MicroProfileToken MicroProfileCounterTokenInit(int nParent)
{
	MicroProfileToken nResult = S.nNumCounters++;
	S.CounterInfo[nResult].nParent = nParent;
	S.CounterInfo[nResult].nSibling = -1;
	S.CounterInfo[nResult].nFirstChild = -1;
	S.CounterInfo[nResult].nFlags = 0;
	S.CounterInfo[nResult].eFormat = MICROPROFILE_COUNTER_FORMAT_DEFAULT;
	S.CounterInfo[nResult].nLimit = 0;
	S.CounterInfo[nResult].ExternalAtomic = 0;
	S.CounterSource[nResult].pSource = 0;
	S.CounterSource[nResult].nSourceSize = 0;
	S.CounterInfo[nResult].nNameLen = 0;
	S.CounterInfo[nResult].pName = nullptr;
	if(nParent >= 0)
	{
		MP_ASSERT(nParent < (int)S.nNumCounters);
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
void MicroProfileCounterTokenInitName(MicroProfileToken nToken, const char* pName)
{
	MP_ASSERT(0 == S.CounterInfo[nToken].pName);
	S.CounterInfo[nToken].nNameLen = (uint16_t)strlen(pName);
	S.CounterInfo[nToken].pName = MicroProfileStringInternLower(pName);
}

MicroProfileToken MicroProfileGetCounterTokenByParent(int nParent, const char* pName, uint32_t nFlags)
{
	for(uint32_t i = 0; i < S.nNumCounters; ++i)
	{
		if(nParent == S.CounterInfo[i].nParent && S.CounterInfo[i].pName == pName)
		{
			return i;
		}
	}
	if(0 != (MICROPROFILE_COUNTER_TOKEN_DONT_CREATE & nFlags))
		return MICROPROFILE_INVALID_TOKEN;
	MicroProfileToken nResult = MicroProfileCounterTokenInit(nParent);
	MicroProfileCounterTokenInitName(nResult, pName);
	return nResult;
}

// by passing in last token/parent, and a non-changing static string, 
// we can quickly return in case the parent is the same as before.
// Note that this doesn't support paths, but instead must be called once per level in the tree
// String must be preinterned.
MicroProfileToken MicroProfileCounterTokenTree(MicroProfileToken* LastToken, MicroProfileToken CurrentParent, const char* pString)
{
	MicroProfileToken Token = *LastToken;
	if (Token != MICROPROFILE_INVALID_TOKEN)
	{
		if (S.CounterInfo[Token].pName == pString && S.CounterInfo[Token].nParent == CurrentParent)
		{
			return Token;
		}
	}
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	Token = MicroProfileGetCounterTokenByParent(CurrentParent, pString, 0);
	*LastToken = Token;
	return Token;
}

const char* MicroProfileCounterString(const char* pString)
{
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	return MicroProfileStringInternLower(pString);
}
	
// Same as above, but works with non-static strings. always takes a lock, and does a search, so expect this to be not cheap
MicroProfileToken MicroProfileCounterTokenTreeDynamic(MicroProfileToken* LastToken, MicroProfileToken Parent, const char* pString)
{
	(void)LastToken;
	MicroProfileInit();
	MicroProfileScopeLock L(MicroProfileMutex());
	const char* pSubNameLower = MicroProfileStringInternLower(pString);
	return MicroProfileGetCounterTokenByParent(Parent, pSubNameLower, 0);
}

MicroProfileToken MicroProfileGetCounterToken(const char* pName, uint32_t CounterFlag)
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
		const char* pSubNameLower = MicroProfileStringInternLower(SubName);
		nResult = MicroProfileGetCounterTokenByParent(nResult, pSubNameLower, CounterFlag);
		if(MICROPROFILE_INVALID_TOKEN == nResult)
			return nResult;

	} while(pName != 0);
	S.CounterInfo[nResult].nFlags |= MICROPROFILE_COUNTER_FLAG_LEAF;

	MP_ASSERT((int)nResult >= 0);
	return nResult;
}

inline void MicroProfileLogPut(MicroProfileLogEntry LE, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0);		   // this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive == 1); // Dont put after calling thread exit
	uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPut + 1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nGet = pLog->nGet.load(std::memory_order_relaxed);
	uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;
	MP_ASSERT(nDistance < MICROPROFILE_BUFFER_SIZE);
	uint32_t nStackPut = pLog->nStackPut;
	if(nDistance < nStackPut + 2)
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
	MP_ASSERT(pLog != 0);		   // this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive == 1); // Dont put after calling thread exit
	uint32_t nStackPut = pLog->nStackPut;
	if(nStackPut < MICROPROFILE_STACK_MAX)
	{
		uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_ENTER, nToken_, nTick);
		uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nNextPos = (nPut + 1) % MICROPROFILE_BUFFER_SIZE;
		uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
		uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;
		MP_ASSERT(nDistance < MICROPROFILE_BUFFER_SIZE);
		if(nDistance < nStackPut + 4) // 2 for ring buffer, 2 for the actual entries
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
	else
	{
		S.nOverflow = 100;
		pLog->nStackPut = nStackPut + 1;
		return MICROPROFILE_DROPPED_TICK;
	}
}

inline uint64_t MicroProfileLogPutEnterCStr(const char* pStr, uint64_t nTick, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0);		   // this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive == 1); // Dont put after calling thread exit
	uint32_t nStackPut = pLog->nStackPut;
	if(nStackPut < MICROPROFILE_STACK_MAX)
	{
		uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_ENTER, ETOKEN_CSTR_PTR, nTick);
		uint64_t LEStr = MicroProfileMakeLogExtendedNoDataPtr((uint64_t)pStr);

		MP_ASSERT(ETOKEN_CSTR_PTR == MicroProfileLogGetTimerIndex(LE));

		uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nNextPos = (nPut + 2) % MICROPROFILE_BUFFER_SIZE;
		uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
		uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;
		MP_ASSERT(nDistance < MICROPROFILE_BUFFER_SIZE);
		if(nDistance < nStackPut + 6) // 2 for ring buffer, 4 for the actual entries
		{
			S.nOverflow = 100;
			return MICROPROFILE_INVALID_TICK;
		}
		else
		{
			pLog->nStackPut = nStackPut + 1;
			pLog->Log[nPut + 0] = LE;
			pLog->Log[(nPut + 1) % MICROPROFILE_BUFFER_SIZE] = LEStr;
			pLog->nPut.store(nNextPos, std::memory_order_release);
			return nTick;
		}
	}
	else
	{
		S.nOverflow = 100;
		pLog->nStackPut = nStackPut + 1;
		return MICROPROFILE_DROPPED_TICK;
	}
}
inline void MicroProfileLogPutLeaveCStr(const char* pStr, uint64_t nTick, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); // this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	MP_ASSERT(pLog->nStackPut != 0);
	uint32_t nStackPut = --(pLog->nStackPut);
	MP_ASSERT(nStackPut < 0xf0000000);
	if(nStackPut < MICROPROFILE_STACK_MAX)
	{
		uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_LEAVE, ETOKEN_CSTR_PTR, nTick);
		uint64_t LEStr = MicroProfileMakeLogExtendedNoDataPtr((uint64_t)pStr);
		MP_ASSERT(ETOKEN_CSTR_PTR == MicroProfileLogGetTimerIndex(LE));

		uint32_t nPos = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nNextPos = (nPos + 2) % MICROPROFILE_BUFFER_SIZE;

		uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
		MP_ASSERT(nStackPut < MICROPROFILE_STACK_MAX);
		MP_ASSERT(nNextPos != nGet); // should never happen
		pLog->Log[nPos + 0] = LE;
		pLog->Log[(nPos + 1) % MICROPROFILE_BUFFER_SIZE] = LEStr;

		pLog->nPut.store(nNextPos, std::memory_order_release);
	}
}

inline void MicroProfileLogPutLeave(MicroProfileToken nToken_, uint64_t nTick, MicroProfileThreadLog* pLog)
{
	MP_ASSERT(pLog != 0); // this assert is hit if MicroProfileOnCreateThread is not called
	MP_ASSERT(pLog->nActive);
	MP_ASSERT(pLog->nStackPut != 0);
	uint32_t nStackPut = --(pLog->nStackPut);
	if(nStackPut < MICROPROFILE_STACK_MAX)
	{
		uint64_t LE = MicroProfileMakeLogIndex(MP_LOG_LEAVE, nToken_, nTick);
		uint32_t nPos = pLog->nPut.load(std::memory_order_relaxed);
		uint32_t nNextPos = (nPos + 1) % MICROPROFILE_BUFFER_SIZE;

		uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
		MP_ASSERT(nStackPut < MICROPROFILE_STACK_MAX);
		MP_ASSERT(nNextPos != nGet); // should never happen
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
		pLog->nPut = nPos + 1;
	}
}

inline void MicroProfileLogPutGpuTimer(MicroProfileToken nToken_, uint64_t nTick, uint64_t nBegin, MicroProfileThreadLogGpu* pLog)
{
	MicroProfileLogPutGpu(MicroProfileMakeLogIndex(nBegin, nToken_, nTick), pLog);
}

inline void MicroProfileLogPutGpuExtended(EMicroProfileTokenExtended eTokenExt, uint32_t nDataSizeQWords, uint32_t nPayload, MicroProfileThreadLogGpu* pLog)
{
	MicroProfileLogEntry LE = MicroProfileMakeLogExtended(eTokenExt, nDataSizeQWords, nPayload);
	MicroProfileLogPutGpu(LE, pLog);
}

inline void MicroProfileLogPutGpuExtendedNoData(EMicroProfileTokenExtended eTokenExt, uint64_t nPayload, MicroProfileThreadLogGpu* pLog)
{
	MicroProfileLogEntry LE = MicroProfileMakeLogExtendedNoData(eTokenExt, nPayload);
	MicroProfileLogPutGpu(LE, pLog);
}

uint32_t MicroProfileGroupTokenActive(MicroProfileToken nToken_)
{
	uint32_t nMask = MicroProfileGetGroupMask(nToken_);
	uint32_t nIndex = MicroProfileGetGroupMaskIndex(nToken_);
	return 0 != (S.nActiveGroups[nIndex] & nMask);
}

uint64_t MicroProfileEnterInternal(MicroProfileToken nToken_)
{
	if(MicroProfileGroupTokenActive(nToken_))
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

uint64_t MicroProfileEnterInternalCStr(const char* pStr)
{
	if(S.AnyActive)
	{
		uint64_t nTick = MP_TICK();
		if(MICROPROFILE_PLATFORM_MARKERS_ENABLED)
		{
			MICROPROFILE_PLATFORM_MARKER_BEGIN(0, pStr);
			return nTick;
		}
		else
		{
			return MicroProfileLogPutEnterCStr(pStr, nTick, MicroProfileGetThreadLog2());
		}
	}
	return MICROPROFILE_INVALID_TICK;
}

void MicroProfileTimelineLeave(uint32_t id)
{
	if(!id)
		return;
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileTimelineMutex());
	MicroProfileThreadLog* pLog = &S.TimelineLog;
	uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPut + 1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
	uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;

	{
		uint32_t nFrameStart = S.TimelineTokenFrameEnter[id % MICROPROFILE_TIMELINE_MAX_TOKENS];
		uint32_t nFrameCurrent = S.nFrameCurrent;
		if(nFrameCurrent < nFrameStart)
			nFrameCurrent += MICROPROFILE_MAX_FRAME_HISTORY;
		uint32_t nFrameDistance = (nFrameCurrent - nFrameStart) % MICROPROFILE_MAX_FRAME_HISTORY;

		S.TimelineTokenFrameEnter[id % MICROPROFILE_TIMELINE_MAX_TOKENS] = MICROPROFILE_INVALID_FRAME;
		S.TimelineTokenFrameLeave[id % MICROPROFILE_TIMELINE_MAX_TOKENS] = nFrameCurrent;

		S.TimelineToken[id % MICROPROFILE_TIMELINE_MAX_TOKENS] = 0;
		S.nTimelineFrameMax = MicroProfileMax(S.nTimelineFrameMax, nFrameDistance);
	}

	if(nDistance < 2 + 4)
	{
		S.nOverflow = 100;
	}
	else
	{
		uint64_t LEEnter = MicroProfileMakeLogIndex(MP_LOG_LEAVE, ETOKEN_CUSTOM_NAME, MP_TICK());
		uint64_t LEId = MicroProfileMakeLogExtended(ETOKEN_CUSTOM_ID, 0, id);

		pLog->Log[nPut++] = LEEnter;
		nPut %= MICROPROFILE_BUFFER_SIZE;
		pLog->Log[nPut++] = LEId;
		nPut %= MICROPROFILE_BUFFER_SIZE;
		pLog->nPut.store(nPut);
	}
}

void MicroProfileTimelineEnterStatic(uint32_t nColor, const char* pStr)
{
	if (!S.AnyActive)
		return;
	uint32_t nToken = MicroProfileTimelineEnterInternal(nColor, pStr, (uint32_t)strlen(pStr), true);
	(void)nToken;
}
void MicroProfileTimelineLeaveStatic(const char* pStr)
{
	if (!S.AnyActive)
		return;

	for(uint32_t i = 0; i < MICROPROFILE_TIMELINE_MAX_TOKENS; ++i)
	{
		if(S.TimelineTokenStaticString[i] && 0 == MP_STRCASECMP(pStr, S.TimelineTokenStaticString[i]))
		{
			MicroProfileTimelineLeave(S.TimelineToken[i]);
		}
	}
}

uint32_t MicroProfileTimelineEnterInternal(uint32_t nColor, const char* pStr, uint32_t nStrLen, int bIsStaticString)
{
	if (!S.AnyActive)
		return 0;
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileTimelineMutex());
	MicroProfileThreadLog* pLog = &S.TimelineLog;
	MP_ASSERT(pStr[nStrLen] == '\0');
	nStrLen += 1;
	uint32_t nStringQwords = MicroProfileGetQWordSize(nStrLen);
	uint32_t nNumMessages = nStringQwords;

	uint32_t nPut = pLog->nPut.load(std::memory_order_relaxed);
	uint32_t nNextPos = (nPut + 1) % MICROPROFILE_BUFFER_SIZE;
	uint32_t nGet = pLog->nGet.load(std::memory_order_acquire);
	uint32_t nDistance = (nGet - nNextPos) % MICROPROFILE_BUFFER_SIZE;

	if(nDistance < nNumMessages + 7)
	{
		S.nOverflow = 100;
		return 0;
	}
	else
	{

		uint32_t token = pLog->nCustomId;
		uint32_t nFrameLeave = S.TimelineTokenFrameLeave[token % MICROPROFILE_TIMELINE_MAX_TOKENS];
		uint32_t nFrameEnter = S.TimelineTokenFrameEnter[token % MICROPROFILE_TIMELINE_MAX_TOKENS];
		uint32_t nCounter = 0;
		uint32_t nFrameCurrent = S.nFrameCurrent;
		{

			/// dont reuse tokens until their leave command has been dead for the maximum amount of frames we can generate a capture for.
			while(token == 0 || nFrameEnter != MICROPROFILE_INVALID_FRAME || (nFrameCurrent - nFrameLeave < MICROPROFILE_MAX_FRAME_HISTORY + 3 && nFrameLeave != MICROPROFILE_INVALID_FRAME))
			{
				token = (uint32_t)pLog->nCustomId++;
				nFrameLeave = S.TimelineTokenFrameLeave[token % MICROPROFILE_TIMELINE_MAX_TOKENS];
				nFrameEnter = S.TimelineTokenFrameEnter[token % MICROPROFILE_TIMELINE_MAX_TOKENS];
				if(++nCounter == MICROPROFILE_TIMELINE_MAX_TOKENS)
				{
					// MP_BREAK();
					return 0;
				}
			}
			S.TimelineTokenFrameEnter[token % MICROPROFILE_TIMELINE_MAX_TOKENS] = S.nFrameCurrent;
		}
		if(bIsStaticString)
		{
			S.TimelineTokenStaticString[token % MICROPROFILE_TIMELINE_MAX_TOKENS] = pStr;
		}
		else
		{
			S.TimelineTokenStaticString[token % MICROPROFILE_TIMELINE_MAX_TOKENS] = nullptr;
		}
		S.TimelineToken[token % MICROPROFILE_TIMELINE_MAX_TOKENS] = token;

		uint64_t LEEnter = MicroProfileMakeLogIndex(MP_LOG_ENTER, ETOKEN_CUSTOM_NAME, MP_TICK());
		uint64_t LEColor = MicroProfileMakeLogExtended(ETOKEN_CUSTOM_COLOR, 0, nColor);
		uint64_t LEId = MicroProfileMakeLogExtended(ETOKEN_CUSTOM_ID, nStringQwords, token);

		pLog->Log[nPut++] = LEEnter;
		nPut %= MICROPROFILE_BUFFER_SIZE;
		pLog->Log[nPut++] = LEColor;
		nPut %= MICROPROFILE_BUFFER_SIZE;
		pLog->Log[nPut++] = LEId;
		nPut %= MICROPROFILE_BUFFER_SIZE;

		// copy if we dont wrap
		if(nPut + nStringQwords <= MICROPROFILE_BUFFER_SIZE)
		{
			memcpy(&pLog->Log[nPut], pStr, nStrLen + 1);
			nPut += nStringQwords;
		}
		else
		{
			int nCharsLeft = (int)nStrLen;
			while(nCharsLeft > 0)
			{
				int nCount = MicroProfileMin(nCharsLeft, 8);
				memcpy(&pLog->Log[nPut++], pStr, nCount);
				// uint64_t LEPayload = MicroProfileMakeLogPayload(pStr, nCount);
				// pLog->Log[nPut++] = LEPayload; nPut %= MICROPROFILE_BUFFER_SIZE;
				pStr += nCount;
				nCharsLeft -= nCount;
			}
		}
		pLog->nPut.store(nPut);
		return token;
	}
}

uint32_t MicroProfileTimelineEnter(uint32_t nColor, const char* pStr)
{
	return MicroProfileTimelineEnterInternal(nColor, pStr, (uint32_t)strlen(pStr), false);
}

uint32_t MicroProfileTimelineEnterf(uint32_t nColor, const char* pStr, ...)
{
	if (!S.AnyActive)
		return 0;
	char buffer[MICROPROFILE_MAX_STRING + 1];
	va_list args;
	va_start(args, pStr);
#ifdef _WIN32
	size_t size = vsprintf_s(buffer, pStr, args);
#else
	size_t size = vsnprintf(buffer, sizeof(buffer) - 1, pStr, args);
#endif
	va_end(args);
	MP_ASSERT(size < sizeof(buffer));
	buffer[size] = '\0';
	return MicroProfileTimelineEnterInternal(nColor, buffer, (uint32_t)size, false);
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

void MicroProfileLocalCounterAddAtomic(MicroProfileToken nToken, int64_t nCount)
{
	std::atomic<int64_t>* pCounter = &S.CounterInfo[nToken].ExternalAtomic;
	pCounter->fetch_add(nCount);
}
int64_t MicroProfileLocalCounterSetAtomic(MicroProfileToken nToken, int64_t nCount)
{

	std::atomic<int64_t>* pCounter = &S.CounterInfo[nToken].ExternalAtomic;
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

void MicroProfileCounterConfigInternal(MicroProfileToken nToken, uint32_t eFormat, int64_t nLimit, uint32_t nFlags)
{
	S.CounterInfo[nToken].eFormat = (MicroProfileCounterFormat)eFormat;
	S.CounterInfo[nToken].nLimit = nLimit;
	S.CounterInfo[nToken].nFlags |= (nFlags & ~MICROPROFILE_COUNTER_FLAG_INTERNAL_MASK);
}

void MicroProfileCounterConfig(const char* pName, uint32_t eFormat, int64_t nLimit, uint32_t nFlags)
{
	MicroProfileToken nToken = MicroProfileGetCounterToken(pName, 0);
	MicroProfileCounterConfigInternal(nToken, eFormat, nLimit, nFlags);
}

void MicroProfileCounterSetPtr(const char* pCounterName, void* pSource, uint32_t nSize)
{
	MicroProfileToken nToken = MicroProfileGetCounterToken(pCounterName, 0);
	S.CounterSource[nToken].pSource = pSource;
	S.CounterSource[nToken].nSourceSize = nSize;
}

inline void MicroProfileFetchCounter(uint32_t i)
{
	switch(S.CounterSource[i].nSourceSize)
	{
	case sizeof(int32_t):
		S.Counters[i] = *(int32_t*)S.CounterSource[i].pSource;
		break;
	case sizeof(int64_t):
		S.Counters[i] = *(int64_t*)S.CounterSource[i].pSource;
		break;
	default:
		break;
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

void MicroProfileLeaveInternalCStr(const char* pStr, uint64_t nTickStart)
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
			MicroProfileLogPutLeaveCStr(pStr, nTick, pLog);
		}
	}
}

void MicroProfileEnter(MicroProfileToken nToken)
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope < MICROPROFILE_STACK_MAX); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	uint32_t nStackPos = pLog->nStackScope++;
	if(nStackPos < MICROPROFILE_STACK_MAX)
	{
		MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[nStackPos];
		pScopeState->Token = nToken;
		pScopeState->nTick = MicroProfileEnterInternal(nToken);
	}
	else
	{
		S.nOverflow = 100;
	}
}
void MicroProfileLeave()
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope > 0); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	uint32_t nStackPos = --pLog->nStackScope;
	if(nStackPos < MICROPROFILE_STACK_MAX)
	{
		MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[nStackPos];
		MicroProfileLeaveInternal(pScopeState->Token, pScopeState->nTick);
	}
	else
	{
		S.nOverflow = 100;
	}
}

void MicroProfileEnterGpu(MicroProfileToken nToken, MicroProfileThreadLogGpu* pLog)
{
	// MP_ASSERT(pLog->nStackScope < MICROPROFILE_STACK_MAX); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	uint32_t nStackPos = pLog->nStackScope++;
	if(nStackPos < MICROPROFILE_STACK_MAX)
	{
		MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[nStackPos];
		pScopeState->Token = nToken;
		pScopeState->nTick = MicroProfileGpuEnterInternal(pLog, nToken);
	}
	else
	{
		S.nOverflow = 100;
	}
}
void MicroProfileLeaveGpu(MicroProfileThreadLogGpu* pLog)
{
	uint32_t nStackPos = --pLog->nStackScope;
	if(nStackPos < MICROPROFILE_STACK_MAX)
	{
		MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[nStackPos];
		MicroProfileGpuLeaveInternal(pLog, pScopeState->Token, pScopeState->nTick);
	}
}

void MicroProfileEnterNegative()
{
	MicroProfileEnter(S.nTokenNegativeCpu);
}

void MicroProfileEnterNegativeGpu(MicroProfileThreadLogGpu* pLog)
{
	MicroProfileEnterGpu(S.nTokenNegativeGpu, pLog);
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
	MP_ASSERT(nCount < (int64_t)MICROPROFILE_GPU_BUFFER_SIZE);
	nStart++;
	for(int32_t i = 0; i < nCount; ++i)
	{
		MP_ASSERT(nStart < MICROPROFILE_GPU_BUFFER_SIZE);
		MicroProfileLogEntry LE = pGpuLog->Log[nStart++];
		MicroProfileLogPut(LE, pQueueLog);
	}
}

uint64_t MicroProfileGpuEnterInternal(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_)
{
	if(MicroProfileGroupTokenActive(nToken_))
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}

		MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		MicroProfileLogPutGpuTimer(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();

		MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), pGpuLog);
		MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, pGpuLog);
		// MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
		// uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		// MicroProfileLogPutGpu(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
		// MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		// MicroProfileLogPutGpu(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), MP_LOG_EXTRA_DATA, pGpuLog);
		// MicroProfileLogPutGpu(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, MP_LOG_EXTRA_DATA, pGpuLog);

		return 1;
	}
	return 0;
}

uint64_t MicroProfileGpuEnterInternalCStr(MicroProfileThreadLogGpu* pGpuLog, const char* pStr)
{
	MP_BREAK(); // not implemented
	return 0;
	// if(S.AnyGpuActive)
	// {
	// 	if(!MicroProfileGetThreadLog())
	// 	{
	// 		MicroProfileInitThreadLog();
	// 	}

	// 	MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
	// 	uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
	// 	MicroProfileLogPutGpuTimer(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
	// 	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();

	// 	MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), pGpuLog);
	// 	MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, pGpuLog);
	// 	// MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
	// 	// uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
	// 	// MicroProfileLogPutGpu(nToken_, nTimer, MP_LOG_ENTER, pGpuLog);
	// 	// MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
	// 	// MicroProfileLogPutGpu(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), MP_LOG_EXTRA_DATA, pGpuLog);
	// 	// MicroProfileLogPutGpu(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, MP_LOG_EXTRA_DATA, pGpuLog);

	// 	return 1;
	// }
	// return 0;
}

void MicroProfileGpuLeaveInternal(MicroProfileThreadLogGpu* pGpuLog, MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(nTickStart)
	{
		if(!MicroProfileGetThreadLog())
		{
			MicroProfileInitThreadLog();
		}

		MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
		uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
		MicroProfileLogPutGpuTimer(nToken_, nTimer, MP_LOG_LEAVE, pGpuLog);
		MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
		MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), pGpuLog);
		MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, pGpuLog);
	}
}

void MicroProfileGpuLeaveInternalCStr(MicroProfileThreadLogGpu* pGpuLog, uint64_t nTickStart)
{
	MP_BREAK(); // not implemented
	return;
	// if(nTickStart)
	// {
	// 	if(!MicroProfileGetThreadLog())
	// 	{
	// 		MicroProfileInitThreadLog();
	// 	}

	// 	MP_ASSERT(pGpuLog->pContext != (void*)-1); // must be called between GpuBegin/GpuEnd
	// 	uint64_t nTimer = MicroProfileGpuInsertTimeStamp(pGpuLog->pContext);
	// 	MicroProfileLogPutGpuTimer(nToken_, nTimer, MP_LOG_LEAVE, pGpuLog);
	// 	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog();
	// 	MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_TIMESTAMP, MP_TICK(), pGpuLog);
	// 	MicroProfileLogPutGpuExtendedNoData(ETOKEN_GPU_CPU_SOURCE_THREAD, pLog->nLogIndex, pGpuLog);
	// }
}

void MicroProfileContextSwitchPut(MicroProfileContextSwitch* pContextSwitch)
{
	if(0 == S.nPauseTicks || (S.nPauseTicks - pContextSwitch->nTicks) > 0)
	{
		uint32_t nPut = S.nContextSwitchPut;
		S.ContextSwitch[nPut] = *pContextSwitch;
		S.nContextSwitchPut = (S.nContextSwitchPut + 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
		//if(S.nContextSwitchPut < nPut)
		//{
		//	float fMsDelay = MicroProfileTickToMsMultiplierCpu() * ((int64_t)S.nFlipStartTick - pContextSwitch->nTicks);
		//	uprintf("context switch wrap .. %7.3fms\n", fMsDelay);
		//}
		//if(S.nContextSwitchPut % 1024 == 0)
		//{
		//	float fMsDelay = MicroProfileTickToMsMultiplierCpu() * ((int64_t)S.nFlipStartTick - pContextSwitch->nTicks);
		//	uprintf("cswitch tick %x ... %7.3fms\n", S.nContextSwitchPut, fMsDelay);
		//}
		S.nContextSwitchLastPushed = pContextSwitch->nTicks;
	}
	else
	{
		S.nContextSwitchStalledTick = MP_TICK();
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
	return S.nFrozen != 0 ? 1 : 0;
}
int MicroProfileEnabled()
{
	return MicroProfileAnyGroupActive();
}
void* MicroProfileAllocInternal(size_t nSize, size_t nAlign)
{
	nAlign = MicroProfileMax(4 * sizeof(uint32_t), nAlign);
	nSize += nAlign;
	intptr_t nPtr = (intptr_t)MICROPROFILE_ALLOC(nSize, nAlign);
	nPtr += nAlign;
	uint32_t* pVal = (uint32_t*)nPtr;
	MP_ASSERT(nSize < 0xffffffff);
	MP_ASSERT(nAlign < 0xffffffff);
	pVal[-1] = (uint32_t)nSize;
	pVal[-2] = (uint32_t)nAlign;
	pVal[-3] = (uint32_t)0x28586813;
	MicroProfileCounterAdd(S.CounterToken_Alloc_Memory, nSize);
	MicroProfileCounterAdd(S.CounterToken_Alloc_Count, 1);
	return (void*)nPtr;
}
void MicroProfileFreeInternal(void* pPtr)
{
	intptr_t p = (intptr_t)pPtr;
	uint32_t* p4 = (uint32_t*)pPtr;
	uint32_t nSize = p4[-1];
	uint32_t nAlign = p4[-2];
	uint32_t nMagic = p4[-3];
	MP_ASSERT(nMagic == 0x28586813);
	MICROPROFILE_FREE((void*)(p - nAlign));
	MicroProfileCounterAdd(S.CounterToken_Alloc_Memory, -(int)nSize);
	MicroProfileCounterAdd(S.CounterToken_Alloc_Count, -1);
}
void* MicroProfileReallocInternal(void* pPtr, size_t nSize)
{
	intptr_t p = (intptr_t)pPtr;
	uint32_t nAlignBase;

	if(p)
	{
		uint32_t* p4 = (uint32_t*)pPtr;
		uint32_t nSizeBase = p4[-1];
		nAlignBase = p4[-2];
		uint32_t nMagicBase = p4[-3];
		MP_ASSERT(nMagicBase == 0x28586813);

		MicroProfileCounterAdd(S.CounterToken_Alloc_Memory, nSize - nSizeBase);
	}
	else
	{
		nAlignBase = 4 * sizeof(uint32_t);
		MicroProfileCounterAdd(S.CounterToken_Alloc_Memory, nSize + nAlignBase);
		MicroProfileCounterAdd(S.CounterToken_Alloc_Count, 1);
	}

	nSize += nAlignBase;
	MP_ASSERT(nAlignBase >= 4 * sizeof(uint32_t));
	if(p)
	{
		p = (intptr_t)MICROPROFILE_REALLOC((void*)(p - nAlignBase), nSize);
	}
	else
	{
		p = (intptr_t)MICROPROFILE_REALLOC((void*)(p), nSize);
	}
	p += nAlignBase;
	uint32_t* pVal = (uint32_t*)p;
	MP_ASSERT(nSize < 0xffffffff);
	MP_ASSERT(nAlignBase < 0xffffffff);
	pVal[-1] = (uint32_t)nSize;
	pVal[-2] = (uint32_t)nAlignBase;
	pVal[-3] = (uint32_t)0x28586813;
	return (void*)p;
}

static void MicroProfileFlipEnabled()
{
	if(S.nFrozen)
	{
		memset(S.nActiveGroups, 0, sizeof(S.nActiveGroups));
		S.AnyActive = false;
	}
	else
	{
		bool AnyActive = false;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			uint32_t nNew = S.nActiveGroupsWanted[i];
			nNew |= S.nForceGroups[i];
			if(nNew)
				AnyActive = true;
			if(S.nActiveGroups[i] != nNew)
			{
				S.nActiveGroups[i] = nNew;
			}
		}
		S.AnyActive = AnyActive;
	}
}

void MicroProfileFlip(void* pContext, uint32_t FlipFlag)
{
	MicroProfileFlip_CB(pContext, nullptr, FlipFlag);
}

#define MICROPROFILE_TICK_VALIDATE_FRAME_TIME 0

void MicroProfileFlip_CB(void* pContext, MicroProfileOnFreeze FreezeCB, uint32_t FlipFlag)
{
	MICROPROFILE_COUNTER_LOCAL_UPDATE_SET_ATOMIC(g_MicroProfileBytesPerFlip);
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
			S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; // hide spike from dumping webpage
		}
		else
		{
			S.nDumpFileCountDown--;
		}
	}
#if MICROPROFILE_WEBSERVER
	if(MICROPROFILE_FLIP_FLAG_START_WEBSERVER == (MICROPROFILE_FLIP_FLAG_START_WEBSERVER & FlipFlag) && S.nWebServerDataSent == (uint64_t)-1)
	{
		MicroProfileWebServerStart();
		S.nWebServerDataSent = 0;
		if(!S.WebSocketThreadRunning)
		{
			S.WebSocketThreadRunning = 1;
			MicroProfileThreadStart(&S.WebSocketSendThread, MicroProfileSocketSenderThread);
		}
	}
#endif

	int nLoop = 0;
	do
	{
#if MICROPROFILE_WEBSERVER
		if(MicroProfileWebServerUpdate())
		{
			S.nAutoClearFrames = MICROPROFILE_GPU_FRAME_DELAY + 3; // hide spike from dumping webpage
		}
#endif
		if(nLoop++)
		{
			MicroProfileSleep(100);
			if((nLoop % 10) == 0)
			{
				uprintf("microprofile frozen %d\n", nLoop);
			}
		}
	} while(S.nFrozen);

	uint32_t nAggregateClear = S.nAggregateClear || S.nAutoClearFrames, nAggregateFlip = 0;

	if(S.nAutoClearFrames)
	{
		nAggregateClear = 1;
		nAggregateFlip = 1;
		S.nAutoClearFrames -= 1;
	}

	bool nRunning = MicroProfileAnyGroupActive();
	if(nRunning)
	{
		S.nFlipStartTick = MP_TICK();
		int64_t nGpuWork = MicroProfileGpuEnd(S.pGpuGlobal);
		MicroProfileGpuSubmit(S.GpuQueue, nGpuWork);
		MicroProfileThreadLogGpuReset(S.pGpuGlobal);
		for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
		{
			if(S.PoolGpu[i])
			{
				S.PoolGpu[i]->nPut = 0;
			}
		}

		MicroProfileGpuBegin(pContext, S.pGpuGlobal);

		uint32_t nGpuTimeStamp = MicroProfileGpuFlip(pContext);

		uint64_t nFrameIdx = S.nFramePutIndex++;
		S.nFramePut = (S.nFramePut + 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.Frames[S.nFramePut].nFrameId = nFrameIdx;
		MP_ASSERT((S.nFramePutIndex % MICROPROFILE_MAX_FRAME_HISTORY) == S.nFramePut);
		S.nFrameCurrent = (S.nFramePut + MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 1) % MICROPROFILE_MAX_FRAME_HISTORY;
		S.nFrameCurrentIndex++;
		uint32_t nFrameNext = (S.nFrameCurrent + 1) % MICROPROFILE_MAX_FRAME_HISTORY;
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
		const int64_t nTickStartFrame = pFrameCurrent->nFrameStartCpu;
		const int64_t nTickEndFrame = pFrameNext->nFrameStartCpu;

		pFrameCurrent->nGpuPending = 0;
		pFramePut->nGpuPending = 1;

		pFramePut->nFrameStartCpu = MP_TICK();

		pFramePut->nFrameStartGpu = nGpuTimeStamp;
		{
			const float fDumpTimeThreshold = 1000.f * 60 * 60 * 24 * 365.f; // if time above this, then we're handling uninitialized counters
			int nDumpNextFrame = 0;
			float fTimeGpu = 0.f;
			if(pFrameNext->nFrameStartGpu != MICROPROFILE_INVALID_TICK)
			{

				uint64_t nTickCurrent = pFrameCurrent->nFrameStartGpu;
				uint64_t nTickNext = pFrameNext->nFrameStartGpu = MicroProfileGpuGetTimeStamp((uint32_t)pFrameNext->nFrameStartGpu);
				nTickCurrent = MicroProfileLogTickMin(nTickCurrent, nTickNext);
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

		const uint64_t nTickEndFrameGpu_ = pFrameNext->nFrameStartGpu;
		const uint64_t nTickStartFrameGpu_ = pFrameCurrent->nFrameStartGpu;
		const bool bGpuFrameInvalid = nTickEndFrameGpu_ == MICROPROFILE_INVALID_TICK || nTickStartFrameGpu_ == MICROPROFILE_INVALID_TICK;
		const uint64_t nTickEndFrameGpu = bGpuFrameInvalid ? 1 : nTickEndFrameGpu_;
		const uint64_t nTickStartFrameGpu = bGpuFrameInvalid ? 2 : nTickStartFrameGpu_;


		MicroProfileFrameExtraCounterData* ExtraData = S.FrameExtraCounterData;
		bool UsingExtraData = false;
		if(ExtraData)
		{
			if((intptr_t)ExtraData == 1)
			{
				size_t Bytes = sizeof(MicroProfileFrameExtraCounterData) * MICROPROFILE_MAX_FRAME_HISTORY;
				printf(" allocating %d bytes %f\n", (int)Bytes, Bytes / (1024.0 * 1024.0));
				ExtraData = S.FrameExtraCounterData = (MicroProfileFrameExtraCounterData*)MicroProfileAllocInternal(Bytes, alignof(uint64_t));
				memset(ExtraData, 0, Bytes);
			}
			ExtraData = ExtraData + S.nFrameCurrent;
			UsingExtraData = true;
		}
#define MP_ASSERT_LE_WRAP(l, g) MP_ASSERT(uint64_t(g - l) < 0x8000000000000000)

		{
			MP_ASSERT_LE_WRAP(nTickStartFrame, nTickEndFrame);
			uint64_t nTick = nTickEndFrame - nTickStartFrame;
			S.nFlipTicks = nTick;
			S.nFlipAggregate += nTick;
			S.nFlipMax = MicroProfileMax(S.nFlipMax, nTick);
		}

		uint32_t* pTimerToGroup = &S.TimerToGroup[0];
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
				if(!pLog->nGpu)
				{
					uint32_t nStart = pFrameCurrent->nLogStart[i];
					while(nStart != nPut)
					{
						int64_t LE = pLog->Log[nStart];
						int64_t nDifference = MicroProfileLogTickDifference(LE, nTickEndFrame);
						uint32_t Ext = MicroProfileLogGetType(LE);
						if(nDifference > 0 || 0 != (0x2 & Ext))
						{
							nStart = (nStart + 1) % MICROPROFILE_BUFFER_SIZE;
						}
						else
						{
							break;
						}
					}
					pFrameNext->nLogStart[i] = nStart;
				}
			}
		}
		{
			pFramePut->nLogStartTimeline = S.TimelineLog.nPut.load(std::memory_order_acquire);

			uint32_t nFrameCurrent = S.nFrameCurrent;
			uint32_t nTimelineFrameDeltaMax = S.nTimelineFrameMax;
			for(uint32_t i = 0; i != MICROPROFILE_TIMELINE_MAX_TOKENS; ++i)
			{
				uint32_t nFrameStart = S.TimelineTokenFrameEnter[i];
				if(nFrameStart != MICROPROFILE_INVALID_FRAME)
				{
					uint32_t nCur = nFrameCurrent;
					if(nCur < nFrameStart)
						nCur += MICROPROFILE_MAX_FRAME_HISTORY;
					if(nCur >= nFrameStart)
					{
						uint32_t D = nCur - nFrameStart;
						nTimelineFrameDeltaMax = MicroProfileMax(nTimelineFrameDeltaMax, D);
					}
				}
			}
			pFramePut->nTimelineFrameMax = nTimelineFrameDeltaMax;
			S.nTimelineFrameMax = 0;
		}
		{
			for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
			{
				MicroProfileThreadLog* pLog = S.Pool[i];
				if(!pLog)
					continue;
				if(pLog->nGpu)
				{
					uint32_t nPut = pFrameNext->nLogStart[i];
					uint32_t nGet = pFrameCurrent->nLogStart[i];
					uint32_t nRange[2][2] = {
						{ 0, 0 },
						{ 0, 0 },
					};
					MicroProfileGetRange(nPut, nGet, nRange);
					for(uint32_t j = 0; j < 2; ++j)
					{
						uint32_t nStart = nRange[j][0];
						uint32_t nEnd = nRange[j][1];
						for(uint32_t k = nStart; k < nEnd; ++k)
						{
							MicroProfileLogEntry L = pLog->Log[k];
							if(MicroProfileLogGetType(L) < MP_LOG_EXTENDED)
							{
								pLog->Log[k] = MicroProfileLogSetTick(L, MicroProfileGpuGetTimeStamp((uint32_t)MicroProfileLogGetTick(L)));
							}
							k += MicroProfileLogGetDataSize(L);
						}
					}
				}
			}
		}

		{
			MicroProfile::GroupTime* pFrameGroup = &S.FrameGroup[0];
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
					pFrameGroup[i].nTicks = 0;
					pFrameGroup[i].nTicksExclusive = 0;
					pFrameGroup[i].nCount = 0;
				}
			}
			{
				MICROPROFILE_SCOPE(g_MicroProfileThreadLoop);
				memset(S.FrameGroupThreadValid, 0, sizeof(S.FrameGroupThreadValid));

				int64_t nNegativeStack[MICROPROFILE_STACK_MAX];

				for(uint32_t idx_thread = 0; idx_thread < MICROPROFILE_MAX_THREADS; ++idx_thread)
				{
					MicroProfileThreadLog* pLog = S.Pool[idx_thread];
					if(!pLog)
						continue;
					bool bGpu = pLog->nGpu != 0;
					int64_t nTickStartLog = bGpu ? nTickStartFrameGpu : nTickStartFrame;
					int64_t nTickEndLog = bGpu ? nTickEndFrameGpu : nTickEndFrame;

					float fToMs = bGpu ? MicroProfileTickToMsMultiplierGpu() : MicroProfileTickToMsMultiplierCpu();
					float fFrameTime = fToMs * (nTickEndLog - nTickStartLog);

					MicroProfile::GroupTime* pFrameGroupThread = &S.FrameGroupThread[idx_thread][0];

					const uint32_t nTimerNegative = bGpu ? S.nTimerNegativeGpuIndex : S.nTimerNegativeCpuIndex;

					uint32_t nPut = pFrameNext->nLogStart[idx_thread];
					uint32_t nGet = pFrameCurrent->nLogStart[idx_thread];
					uint32_t nRange[2][2] = {
						{ 0, 0 },
						{ 0, 0 },
					};
					MicroProfileGetRange(nPut, nGet, nRange);
					if(nPut != nGet)
					{
						S.FrameGroupThreadValid[idx_thread / 32] |= 1 << (idx_thread % 32);
						memset(pFrameGroupThread, 0, sizeof(S.FrameGroupThread[idx_thread]));
					}

					uint64_t* pStackLog = &pLog->nStackLogEntry[0];
					uint64_t* pChildTickStack = &pLog->nChildTickStack[1];
					int32_t nStackPos = pLog->nStackPos;
					for(int32_t i = 0; i < nStackPos; ++i)
					{
						nNegativeStack[i] = 0;
					}
					uint8_t TimerStackPos[MICROPROFILE_MAX_TIMERS];
					uint8_t GroupStackPos[MICROPROFILE_MAX_GROUPS];
					memset(TimerStackPos, 0, sizeof(TimerStackPos));
					memset(GroupStackPos, 0, sizeof(GroupStackPos));

					// restore group and timer stack pos.
					for(int32_t i = 0; i < nStackPos; ++i)
					{
						uint64_t nTimer = MicroProfileLogGetTimerIndex(pStackLog[i]);
						uint32_t nGroup = pTimerToGroup[nTimer];
						MP_ASSERT(nTimer < MICROPROFILE_MAX_TIMERS);
						MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
						TimerStackPos[nTimer]++;
						GroupStackPos[nGroup]++;
					}

					for(uint32_t j = 0; j < 2; ++j)
					{
						uint32_t nStart = nRange[j][0];
						uint32_t nEnd = nRange[j][1];
						for(uint32_t k = nStart; k < nEnd; ++k)
						{
							MicroProfileLogEntry LE = pLog->Log[k];
							uint32_t nType = MicroProfileLogGetType(LE);

							switch(nType)
							{
							case MP_LOG_ENTER:
							{
								uint64_t nTimer = MicroProfileLogGetTimerIndex(LE);
								if(nTimer != ETOKEN_CSTR_PTR)
								{
									MP_ASSERT(nTimer < S.nTotalTimers);
									uint32_t nGroup = pTimerToGroup[nTimer];
									MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
									MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);


									// When we aggretate the total time, we have to count if the timers & groups are layered, to avoid summing them twice when calculating the total time.
									// Averages become nonsense regardless.
									TimerStackPos[nTimer]++;
									GroupStackPos[nGroup]++;

									pStackLog[nStackPos] = LE;

									nNegativeStack[nStackPos] = 0;
									pChildTickStack[nStackPos] = 0;
									nStackPos++;
								}
								break;
							}
							case MP_LOG_LEAVE:
							{
								uint64_t nTimer = MicroProfileLogGetTimerIndex(LE);
								if(nTimer != ETOKEN_CSTR_PTR)
								{
									MP_ASSERT(nTimer < S.nTotalTimers);
									uint32_t nGroup = pTimerToGroup[nTimer];
									MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
									MP_ASSERT(nStackPos);
									uint64_t nTicks;
									bool bGroupRoot = 0 == GroupStackPos[nGroup] || 0 == --GroupStackPos[nGroup];
									bool bTimerRoot = 0 == TimerStackPos[nTimer] || 0 == --TimerStackPos[nTimer];
									{
										nStackPos--;
										uint64_t nTickStart = MicroProfileLogTickClamp(pStackLog[nStackPos], nTickStartLog, nTickEndLog);
										uint64_t nClamped = MicroProfileLogTickClamp(LE, nTickStartLog, nTickEndLog);
										nTicks = MicroProfileLogTickDifference(nTickStart, nClamped);
										MP_ASSERT(nTicks < 0x8000000000000000);

										uint64_t nChildTicks = pChildTickStack[nStackPos];

										MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
										if(nStackPos)
										{
											pChildTickStack[nStackPos - 1] += nTicks;
										}
										MP_ASSERT(nTicks >= nChildTicks);
										uint64_t nTicksExclusive = (nTicks - nChildTicks);
										S.FrameExclusive[nTimer] += nTicksExclusive;
										pFrameGroupThread[nGroup].nTicksExclusive += nTicksExclusive;
										if(bTimerRoot) // dont count this if its below another instance of the same timer.
										{
											S.Frame[nTimer].nTicks += nTicks;
											S.Frame[nTimer].nCount += 1; 
											MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
											if (bGroupRoot)
											{
												pFrameGroupThread[nGroup].nTicks += nTicks;
												pFrameGroupThread[nGroup].nCount += 1;
											}
										}
										
									}
								}
								break;
							}
							case MP_LOG_EXTENDED:
							{
								k += MicroProfileLogGetDataSize(LE);
								break;
							}
							case MP_LOG_EXTENDED_NO_DATA:
								break;
							}
						}
					}

					for(int32_t i = nStackPos - 1; i >= 0; --i)
					{

						MicroProfileLogEntry LE = pStackLog[i];
						uint64_t nTickStart = MicroProfileLogTickClamp(LE, nTickStartLog, nTickEndLog);
						uint64_t nTicks = MicroProfileLogTickDifference(nTickStart, nTickEndLog);
						int64_t nChildTicks = pChildTickStack[i];
						pChildTickStack[i] = 0; // consume..

						MP_ASSERT(i < MICROPROFILE_STACK_MAX && i >= 0);
						if(i)
						{
							pChildTickStack[i - 1] += nTicks;
						}
						MP_ASSERT(nTicks >= (uint64_t)nChildTicks);

						uint32_t nTimer = (uint32_t)MicroProfileLogGetTimerIndex(LE);
						uint32_t nGroup = pTimerToGroup[nTimer];

						bool bGroupRoot = 0 == GroupStackPos[nGroup] || 0 == --GroupStackPos[nGroup];
						bool bTimerRoot = 0 == TimerStackPos[nTimer] || 0 == --TimerStackPos[nTimer];

						uint64_t nTicksExclusive = (nTicks - nChildTicks);
						S.FrameExclusive[nTimer] += nTicksExclusive;
						pFrameGroupThread[nGroup].nTicksExclusive += nTicksExclusive;
						if(bTimerRoot)
						{
							S.Frame[nTimer].nTicks += nTicks;
							S.Frame[nTimer].nCount += 1;

							MP_ASSERT(nGroup < MICROPROFILE_MAX_GROUPS);
							if (bGroupRoot)
							{
								pFrameGroupThread[nGroup].nTicks += nTicks;
								pFrameGroupThread[nGroup].nCount += 1;
							}
						}

					}
					#ifdef MP_ASSERT
					for(uint8_t& g : GroupStackPos)
					{
						MP_ASSERT(g == 0);
					}
					for(uint8_t& g : TimerStackPos)
					{
						MP_ASSERT(g == 0);
					}
					#endif
					
					pLog->nStackPos = nStackPos;
					for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
					{
						pLog->nGroupTicks[j] += pFrameGroupThread[j].nTicks;

						if((S.FrameGroupThreadValid[idx_thread / 32] & (1 << (idx_thread % 32))) != 0)
						{
							pFrameGroup[j].nTicks += pFrameGroupThread[j].nTicks;
							pFrameGroup[j].nTicksExclusive += pFrameGroupThread[j].nTicksExclusive;
							pFrameGroup[j].nCount += pFrameGroupThread[j].nCount;
						}
					}

					if(pLog->nPut == pLog->nGet && pLog->nActive == 2)
					{
						pLog->nIdleFrames++;
					}
					else
					{
						pLog->nIdleFrames = 0;
					}
					if(pLog->nActive == 2 && pLog->nIdleFrames > MICROPROFILE_THREAD_LOG_FRAMES_REUSE)
					{
						MicroProfileLogReset(pLog);
					}
				}
			}
			{
				MICROPROFILE_SCOPE(g_MicroProfileAccumulate);
				uint64_t* ExtraPut = nullptr;
				if(UsingExtraData)
				{
					ExtraPut = &ExtraData->Timers[0];
					ExtraData->NumTimers = S.nTotalTimers;
				}

				for(uint32_t i = 0; i < S.nTotalTimers; ++i)
				{
					S.AccumTimers[i].nTicks += S.Frame[i].nTicks;
					S.AccumTimers[i].nCount += S.Frame[i].nCount;
					S.AccumMaxTimers[i] = MicroProfileMax(S.AccumMaxTimers[i], S.Frame[i].nTicks);
					S.AccumMinTimers[i] = MicroProfileMin(S.AccumMinTimers[i], S.Frame[i].nTicks);
					S.AccumTimersExclusive[i] += S.FrameExclusive[i];
					S.AccumMaxTimersExclusive[i] = MicroProfileMax(S.AccumMaxTimersExclusive[i], S.FrameExclusive[i]);
					if(ExtraPut)
						*ExtraPut++ = S.Frame[i].nTicks;					
				}
				ExtraPut = nullptr;
				if(UsingExtraData)
				{
					ExtraPut = &ExtraData->Groups[0];
					ExtraData->NumGroups = S.nGroupCount;
				}

				for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
				{
					S.AccumGroup[i] += pFrameGroup[i].nTicks;
					S.AccumGroupMax[i] = MicroProfileMax(S.AccumGroupMax[i], pFrameGroup[i].nTicks);
					if(ExtraPut)
						*ExtraPut++ = pFrameGroup[i].nTicks;
				}
				if(S.CsvConfig.State == MicroProfileCsvConfig::ACTIVE)
				{
					uint32_t FrameIndex = S.nFrameCurrent % MICROPROFILE_MAX_FRAME_HISTORY;
					uint64_t* FrameData = S.CsvConfig.FrameData + S.CsvConfig.TotalElements * FrameIndex;
					{
						uint16_t* TimerIndices = S.CsvConfig.TimerIndices;
						for(uint32_t i = 0; i < S.CsvConfig.NumTimers; ++i)
						{
							uint16_t Index = TimerIndices[i];
							if(Index != UINT16_MAX)
							{
								*FrameData = S.Frame[Index].nTicks;
							}else{
								*FrameData = 0;
							}
							FrameData++;
						}
					}
					{
						uint16_t* GroupIndices = S.CsvConfig.GroupIndices;
						for(uint32_t i = 0; i < S.CsvConfig.NumGroups; ++i)
						{
							uint16_t Index = GroupIndices[i];
							if(Index != UINT16_MAX)
							{
								*FrameData = pFrameGroup[Index].nTicks;
							}else{
								*FrameData = 0;
							}
							FrameData++;
						}
					}
					{
						uint16_t* CounterIndices = S.CsvConfig.CounterIndices;
						for(uint32_t i = 0; i < S.CsvConfig.NumCounters; ++i)
						{
							uint16_t Index = CounterIndices[i];
							if(Index != UINT16_MAX)
							{
								*FrameData = S.Counters[Index].load();
							}else{
								*FrameData = 0;
							}
							FrameData++;
						}
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
			S.nGraphPut = (S.nGraphPut + 1) % MICROPROFILE_GRAPH_HISTORY;
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
		if(pFrameNext->nLogStartTimeline != (uint32_t)-1)
		{
			S.TimelineLog.nGet.store(pFrameNext->nLogStartTimeline);
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
		S.nCounterHistoryPut = (S.nCounterHistoryPut + 1) % MICROPROFILE_GRAPH_HISTORY;
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
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			S.nActiveGroupsWanted[i] = S.nGroupMask[i];
		}
		S.nStartEnabled = 1;
		MicroProfileFlipEnabled();
	}
	else
	{
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			S.nActiveGroupsWanted[i] = 0;
		}
		S.nStartEnabled = 0;
		MicroProfileFlipEnabled();
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
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
			{
				S.nActiveGroupsWanted[i] |= S.CategoryInfo[nCategoryIndex].nGroupMask[i];
			}
		}
		else
		{
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
			{
				S.nActiveGroupsWanted[i] &= ~S.CategoryInfo[nCategoryIndex].nGroupMask[i];
			}
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
	return 0 == memcmp(S.nGroupMask, S.nActiveGroupsWanted, sizeof(S.nGroupMask));
}

void MicroProfileSetForceMetaCounters(int bForce)
{
}

int MicroProfileGetForceMetaCounters()
{

	return 0;
}

void MicroProfileEnableMetaCounter(const char* pMeta)
{
}

void MicroProfileDisableMetaCounter(const char* pMeta)
{
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
	uint32_t nIndex = nGroup / 32;
	uint32_t nBit = nGroup % 32;
	S.nForceGroups[nIndex] |= (1ll << nBit);
}

void MicroProfileForceDisableGroup(const char* pGroup, MicroProfileTokenType Type)
{
	MicroProfileInit();
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	uint16_t nGroup = MicroProfileGetGroup(pGroup, Type);
	uint32_t nIndex = nGroup / 32;
	uint32_t nBit = nGroup % 32;

	S.nForceGroups[nIndex] &= ~(1ll << nBit);
}

void MicroProfileCalcAllTimers(
	float* pTimers, float* pAverage, float* pMax, float* pMin, float* pCallAverage, float* pExclusive, float* pAverageExclusive, float* pMaxExclusive, float* pTotal, uint32_t nSize)
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
		pTimers[nIdx + 1] = fPrc;
		pAverage[nIdx] = fAverageMs;
		pAverage[nIdx + 1] = fAveragePrc;
		pMax[nIdx] = fMaxMs;
		pMax[nIdx + 1] = fMaxPrc;
		pMin[nIdx] = fMinMs;
		pMin[nIdx + 1] = fMinPrc;
		pCallAverage[nIdx] = fCallAverageMs;
		pCallAverage[nIdx + 1] = fCallAveragePrc;
		pExclusive[nIdx] = fMsExclusive;
		pExclusive[nIdx + 1] = fPrcExclusive;
		pAverageExclusive[nIdx] = fAverageMsExclusive;
		pAverageExclusive[nIdx + 1] = fAveragePrcExclusive;
		pMaxExclusive[nIdx] = fMaxMsExclusive;
		pMaxExclusive[nIdx + 1] = fMaxPrcExclusive;
		pTotal[nIdx] = fTotalMs;
		pTotal[nIdx + 1] = 0.f;
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

#define MICROPROFILE_CONTEXT_SWITCH_SEARCH_DEBUG MICROPROFILE_DEBUG

void MicroProfileContextSwitchSearch(uint32_t* pContextSwitchStart, uint32_t* pContextSwitchEnd, uint64_t nBaseTicksCpu, uint64_t nBaseTicksEndCpu)
{
	MICROPROFILE_SCOPE(g_MicroProfileContextSwitchSearch);
	uint32_t nContextSwitchPut = S.nContextSwitchPut;
	uint64_t nContextSwitchStart, nContextSwitchEnd;
	nContextSwitchStart = nContextSwitchEnd = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
	int64_t nSearchEnd = nBaseTicksEndCpu + MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());
	int64_t nSearchBegin = nBaseTicksCpu - MicroProfileMsToTick(30.f, MicroProfileTicksPerSecondCpu());

#if MICROPROFILE_CONTEXT_SWITCH_SEARCH_DEBUG
	int64_t lp = S.nContextSwitchLastPushed;
	uprintf("cswitch-search\n");
	uprintf("Begin %" PRId64 " End %" PRId64 " Last %" PRId64 "\n", nSearchBegin, nSearchEnd, lp);

	float fToMs = MicroProfileTickToMsMultiplierCpu();
	uprintf("E    %6.2fms\n", fToMs * (nSearchEnd - nSearchBegin));
	uprintf("LAST %6.2fms\n", fToMs * (lp - nSearchBegin));
#endif

	int64_t nMax = INT64_MIN;
	int64_t nMin = INT64_MAX;
	for(uint32_t i = 0; i < MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE; ++i)
	{
		uint32_t nIndex = (nContextSwitchPut + MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE - (i + 1)) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE;
		MicroProfileContextSwitch& CS = S.ContextSwitch[nIndex];
		if(nMax < CS.nTicks)
			nMax = CS.nTicks;
		if(nMin > CS.nTicks && CS.nTicks != 0)
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

#if MICROPROFILE_CONTEXT_SWITCH_SEARCH_DEBUG
	{
		uprintf("contextswitch start %" PRId64 " %" PRId64 "\n", nContextSwitchStart, nContextSwitchEnd);

		MicroProfileContextSwitch& CS0 = S.ContextSwitch[0];
		int64_t nMax = CS0.nTicks;
		int64_t nMin = CS0.nTicks;
		int64_t nBegin = 0;
		int64_t nEnd = 0;
		int nRanges = 0;
		for(uint32_t i = 0; i < MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE; i += 1024)
		{
			int64_t MinTick = INT64_MAX;
			int64_t MaxTick = INT64_MIN;
			for(int j = 0; j < 1024; ++j)
			{
				MicroProfileContextSwitch& CS = S.ContextSwitch[i + j];
				int64_t nTicks = CS.nTicks;
				MinTick = MicroProfileMin(nTicks, MinTick);
				MaxTick = MicroProfileMax(nTicks, MaxTick);
			}

			uprintf("XX range [%5" PRIx64 ":%5" PRIx64 "] :: [%6.2f:%6.2f]                 [%p :: %p] .. ref %p\n",
					i,
					i + 1024,
					fToMs * (MinTick - nSearchBegin),
					fToMs * (MaxTick - nSearchBegin),
					(void*)MinTick,
					(void*)MaxTick,
					(void*)nSearchBegin

			);
		}
		uprintf("\n\n");

		for(uint32_t i = 0; i < MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE; ++i)
		{
			MicroProfileContextSwitch& CS = S.ContextSwitch[i];
			int64_t nTicks = CS.nTicks;
			float fMs = (nTicks - nMax) * fToMs;
			if(fMs < 0 || fMs > 50)
			{
				// dump range here
				uprintf("range [%5" PRId64 ":%5" PRId64 "] :: [%6.2f:%6.2f]                 [%p :: %p] .. ref %p\n",
						nBegin,
						nEnd,
						fToMs * (nMin - nSearchBegin),
						fToMs * (nMax - nSearchBegin),
						(void*)nMin,
						(void*)nMax,
						(void*)nSearchBegin

				);

				nEnd = nBegin = i;
				nMax = nMin = CS.nTicks;
				nRanges++;
			}
			else
			{
				nEnd = i;
				nMax = MicroProfileMax(nTicks, nMax);
			}
		}
	}

	lp = S.nContextSwitchLastPushed;
	uprintf("E    %6.2fms\n", fToMs * (nSearchEnd - nSearchBegin));
	uprintf("LP2 %6.2fms\n", fToMs * (lp - nSearchBegin));
#endif
}

int MicroProfileFormatCounter(int eFormat, int64_t nCounter, char* pOut, uint32_t nBufferSize)
{
	if(!nCounter)
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
			nCounter = -(nCounter + 1);
		}
	}

	switch(eFormat)
	{
	case MICROPROFILE_COUNTER_FORMAT_DEFAULT:
	{
		int nSeperate = 0;
		while(nCounter)
		{
			if(nSeperate)
			{
				*pTmp++ = '.';
			}
			nSeperate = 1;
			for(uint32_t i = 0; nCounter && i < 3; ++i)
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
		while(pTmp > pOut) // reverse string
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
		const char* pExt[] = { "b", "kb", "mb", "gb", "tb", "pb", "eb", "zb", "yb" };
		size_t nNumExt = sizeof(pExt) / sizeof(pExt[0]);
		int64_t nShift = 0;
		int64_t nDivisor = 1;
		int64_t nCountShifted = nCounter >> 10;
		while(nCountShifted)
		{
			nDivisor <<= 10;
			nCountShifted >>= 10;
			nShift++;
		}
		MP_ASSERT(nShift < (int64_t)nNumExt);
		if(nShift)
		{
			nLen = snprintf(pOut, nBufferSize - 1, "%c%3.2f%s", nNegative ? '-' : ' ', (double)nCounter / nDivisor, pExt[nShift]);
		}
		else
		{
			nLen = snprintf(pOut, nBufferSize - 1, "%c%" PRId64 "%s", nNegative ? '-' : ' ', nCounter, pExt[nShift]);
		}
	}
	break;
	}
	pBase[nLen] = '\0';

	return nLen;
}

bool MicroProfileAnyGroupActive()
{
	for (uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
	{
		if (S.nActiveGroups[i] != 0)
			return true;
	}
	return false;
}
bool MicroProfileGroupActive(uint32_t nGroupIndex)
{
	MP_ASSERT(nGroupIndex < MICROPROFILE_MAX_GROUPS);
	uint32_t nIndex = nGroupIndex / 32;
	uint32_t nBit = nGroupIndex % 32;
	return ((S.nActiveGroups[nIndex] >> nBit) & 1) == 1;
}


void MicroProfileToggleGroup(uint32_t nGroup)
{
	if (nGroup < S.nGroupCount)
	{
		uint32_t nIndex = nGroup / 32;
		uint32_t nBit = nGroup % 32;
		S.nActiveGroupsWanted[nIndex] ^= (1ll << nBit);
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
	}
}
bool MicroProfileGroupEnabled(uint32_t nGroup)
{
	if (nGroup < S.nGroupCount)
	{
		uint32_t nIndex = nGroup / 32;
		uint32_t nBit = nGroup % 32;
		return 0 != (S.nActiveGroupsWanted[nIndex] & (1ll << nBit));
	}
	return false;
}
bool MicroProfileCategoryEnabled(uint32_t nCategory)
{
	if (nCategory < S.nCategoryCount)
	{
		for (uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			if (S.CategoryInfo[nCategory].nGroupMask[i] != (S.CategoryInfo[nCategory].nGroupMask[i] & S.nActiveGroupsWanted[i]))
			{
				return false;
			}
		}
		return true;
	}
	return false;
}
void MicroProfileToggleCategory(uint32_t nCategory)
{
	if (nCategory < S.nCategoryCount)
	{
		bool bAllSet = true;
		for (uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			bAllSet = bAllSet && S.CategoryInfo[nCategory].nGroupMask[i] == (S.CategoryInfo[nCategory].nGroupMask[i] & S.nActiveGroupsWanted[i]);
		}
		for (uint32_t i = 0; i < MICROPROFILE_MAX_GROUP_INTS; ++i)
		{
			if (bAllSet)
			{
				S.nActiveGroupsWanted[i] &= ~S.CategoryInfo[nCategory].nGroupMask[i];
			}
			else
			{
				S.nActiveGroupsWanted[i] |= S.CategoryInfo[nCategory].nGroupMask[i];
			}
		}
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
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

typedef void (*MicroProfileWriteCallback)(void* Handle, size_t size, const char* pData);

uint32_t MicroProfileWebServerPort()
{
	return S.nWebServerPort;
}

void MicroProfileSetWebServerPort(uint32_t nPort)
{
	if(S.nWebServerPort != nPort)
	{
		MicroProfileWebServerJoin();
		MicroProfileWebServerStop();
		S.nWebServerPort = nPort;
		S.nWebServerDataSent = (uint64_t)-1; // Will cause the web server and its thread to be restarted next time MicroProfileFlip() is called.
	}
}

void MicroProfileDumpFileImmediately(const char* pHtml, const char* pCsv, void* pGpuContext, uint32_t FrameCount)
{
	for(uint32_t i = 0; i < 2; ++i)
	{
		MicroProfileFlip(pGpuContext);
	}
	for(uint32_t i = 0; i < MICROPROFILE_GPU_FRAME_DELAY + 1; ++i)
	{
		MicroProfileFlip(pGpuContext);
	}

	uint32_t nDumpMask = 0;
	if(pHtml)
	{


		size_t nLen = strlen(pHtml);
		if(nLen > sizeof(S.HtmlDumpPath) - 1)
		{
			return;
		}
		const size_t ExtSize = sizeof(".html")-1;
		if(nLen > ExtSize && 0 == memcmp(".html", pHtml + nLen - ExtSize, ExtSize))
			nLen -= ExtSize;
		memcpy(S.HtmlDumpPath, pHtml, nLen);
		S.HtmlDumpPath[nLen] = '\0';


		nDumpMask |= 1;
	}
	if(pCsv)
	{
		size_t nLen = strlen(pCsv);
		if(nLen > sizeof(S.CsvDumpPath) - 1)
		{
			return;
		}
		const size_t ExtSize = sizeof(".csv")-1;
		if(nLen > ExtSize && 0 == memcmp(".csv", pCsv + nLen - ExtSize, ExtSize))
			nLen -= ExtSize;
		memcpy(S.CsvDumpPath, pCsv, nLen + 1);
		S.CsvDumpPath[nLen] = '\0';

		nDumpMask |= 2;
	}
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	S.nDumpFileNextFrame = nDumpMask;
	S.nDumpSpikeMask = 0;
	S.nDumpFileCountDown = 0;
	S.DumpFrameCount = FrameCount;

	MicroProfileDumpToFile();
}
void MicroProfileDumpFile(const char* pHtml, const char* pCsv, float fCpuSpike, float fGpuSpike, uint32_t FrameCount)
{
	S.fDumpCpuSpike = fCpuSpike;
	S.fDumpGpuSpike = fGpuSpike;
	S.DumpFrameCount = FrameCount;
	uint32_t nDumpMask = 0;
	if(pHtml)
	{
		size_t nLen = strlen(pHtml);
		if(nLen > sizeof(S.HtmlDumpPath) - 1)
		{
			return;
		}
		const size_t ExtSize = sizeof(".html")-1;
		if(nLen > ExtSize && 0 == memcmp(".html", pHtml + nLen - ExtSize, ExtSize))
			nLen -= ExtSize;
		memcpy(S.HtmlDumpPath, pHtml, nLen);
		S.HtmlDumpPath[nLen] = '\0';

		nDumpMask |= 1;
	}
	if(pCsv)
	{
		size_t nLen = strlen(pCsv);
		if(nLen > sizeof(S.CsvDumpPath) - 1)
		{
			return;
		}
		const size_t ExtSize = sizeof(".csv")-1;
		if(nLen > ExtSize && 0 == memcmp(".csv", pCsv + nLen - ExtSize, ExtSize))
			nLen -= ExtSize;
		memcpy(S.CsvDumpPath, pCsv, nLen);
		S.CsvDumpPath[nLen] = '\0';

		nDumpMask |= 2;
	}
	if(fCpuSpike > 0.f || fGpuSpike > 0.f)
	{
		S.nDumpFileNextFrame = 0;
		S.nDumpSpikeMask = nDumpMask;
	}
	else
	{
		std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
		S.nDumpFileNextFrame = nDumpMask;
		S.nDumpSpikeMask = 0;
		S.nDumpFileCountDown = 0;

		MicroProfileDumpToFile();
	}
}

struct MicroProfilePrintfArgs
{
	MicroProfileWriteCallback CB;
	void* Handle;
};

char* MicroProfilePrintfCallback(const char* buf, void* user, int len)
{
	MicroProfilePrintfArgs* A = (MicroProfilePrintfArgs*)user;
	(A->CB)(A->Handle, len, buf);
	return const_cast<char*>(buf);
};

void MicroProfilePrintf(MicroProfileWriteCallback CB, void* Handle, const char* pFmt, ...)
{
	va_list args;
	va_start(args, pFmt);
	MicroProfilePrintfArgs A;
	A.CB = CB;
	A.Handle = Handle;
	char Buffer[STB_SPRINTF_MIN];
	int size = stbsp_vsprintfcb(MicroProfilePrintfCallback, (void*)&A, Buffer, pFmt, args);
	(void)size;
	va_end(args);
}

void MicroProfileGetFramesToDump(uint64_t nStartFrameId, uint32_t nMaxFrames, uint32_t& nFirstFrame, uint32_t& nLastFrame, uint32_t& nNumFrames)
{
	nFirstFrame = (uint32_t)-1;
	nNumFrames = 0;

	if(nStartFrameId != (uint64_t)-1)
	{
		// search for the frane
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
			nLastFrame = S.nFrameCurrent;
			uint32_t nDistance = (MICROPROFILE_MAX_FRAME_HISTORY + nFirstFrame - nLastFrame) % MICROPROFILE_MAX_FRAME_HISTORY;
			nNumFrames = MicroProfileMin(nDistance, (uint32_t)nMaxFrames);
		}
	}

	if(nNumFrames == 0)
	{
		nNumFrames = (MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 3); // leave a few to not overwrite
		nNumFrames = MicroProfileMin(nNumFrames, (uint32_t)nMaxFrames);
		nFirstFrame = (S.nFrameCurrent + MICROPROFILE_MAX_FRAME_HISTORY - nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	}

	nLastFrame = (nFirstFrame + nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
}

#define printf(...) MicroProfilePrintf(CB, Handle, __VA_ARGS__)

void MicroProfileDumpCsvWithConfig(MicroProfileWriteCallback CB, void* Handle, uint32_t nFirstFrame, uint32_t nLastFrame, uint32_t nNumFrames)
{
	uint32_t NumTimers = S.CsvConfig.NumTimers;
	uint32_t NumGroups = S.CsvConfig.NumGroups;
	uint32_t NumCounters = S.CsvConfig.NumCounters;
	uint16_t* TimerIndices = S.CsvConfig.TimerIndices;
	uint16_t* GroupIndices = S.CsvConfig.GroupIndices;
	uint64_t* FrameData = S.CsvConfig.FrameData;
	uint16_t* CounterIndices = S.CsvConfig.CounterIndices;
	uint32_t TotalElements = S.CsvConfig.TotalElements;
	uint32_t Offset = 0;
	bool UseFrameTime = 0 != (MICROPROFILE_CSV_FLAG_FRAME_TIME & S.CsvConfig.Flags);
	const char** pTimerNames = S.CsvConfig.pTimerNames;
	const char** pGroupNames = S.CsvConfig.pGroupNames;
	const char** pCounterNames = S.CsvConfig.pCounterNames;
	if(UseFrameTime)
		printf("Time");
	else
		printf("FrameNumber");
	for(uint32_t i = 0; i < NumTimers; ++i, ++Offset)
		printf(", %s", pTimerNames[i] ? pTimerNames[i] : S.TimerInfo[TimerIndices[i]].pName);

	for(uint32_t i = 0; i < NumGroups; ++i, ++Offset)
		printf(", %s",  pGroupNames[i] ? pGroupNames[i] : S.GroupInfo[GroupIndices[i]].pName);
	for(uint32_t i = 0; i < NumCounters; ++i, ++Offset)
		printf(", %s",  pCounterNames[i] ? pCounterNames[i] : S.CounterInfo[CounterIndices[i]].pName);
	printf("\n");	

	float* fToMsTimer = (float*)alloca(sizeof(float)*NumTimers);
	float* fToMsGroup = (float*)alloca(sizeof(float)*NumGroups);
	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
	
	for(uint32_t i = 0; i < NumTimers; ++i)
		fToMsTimer[i] = S.TimerInfo[TimerIndices[i]].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;
	for(uint32_t i = 0; i < NumGroups; ++i)
		fToMsGroup[i] = S.GroupInfo[GroupIndices[i]].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;

	uint64_t TickStart = S.Frames[nFirstFrame % MICROPROFILE_MAX_FRAME_HISTORY].nFrameStartCpu;
	
	for(uint32_t i = 0; i < nNumFrames; ++i)
	{
		uint32_t FrameIndex = ((nFirstFrame + i) % MICROPROFILE_MAX_FRAME_HISTORY);
		uint64_t TickFrame = S.Frames[FrameIndex].nFrameStartCpu;
		uint64_t* Data = FrameData + TotalElements * FrameIndex;
		if(UseFrameTime)
			printf("%f", (TickFrame - TickStart) * fToMsCPU);
		else
			printf("%d", i);
		Offset = 0;
		for(uint32_t j = 0; j < NumTimers; ++j)
			printf(", %f", Data[Offset++] * fToMsTimer[j]);
		for(uint32_t j = 0; j < NumGroups; ++j)
			printf(", %f",  Data[Offset++] * fToMsGroup[j]);
		for(uint32_t j = 0; j < NumCounters; ++j)
			printf(", %lld", Data[Offset++]);
		printf("\n");
	}	
}
void MicroProfileDumpCsvTimerFrames(MicroProfileWriteCallback CB, void* Handle, uint32_t nFirstFrame, uint32_t nLastFrame, uint32_t nNumFrames)
{
	MP_ASSERT(S.FrameExtraCounterData);
	uint32_t TotalTimers = S.nTotalTimers;
	float* fToMs = (float*)alloca(sizeof(float)*TotalTimers);
	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
	
	for(uint32_t i = 0; i < TotalTimers; ++i)
		fToMs[i] = S.TimerInfo[i].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;



	for(uint32_t i = 0; i < TotalTimers; ++i)
	{
		printf(i == 0 ? "FrameNumber, \"%s\"" : ",\"%s\"", S.TimerInfo[i].pName);
	}
	printf("\n");


	for(uint32_t i = 0; i < nNumFrames; ++i)
	{
		//printf("%d", i) MicroProfileFrame& F = S.Frames[(i + nFirstFrame) % MICROPROFILE_MAX_FRAME_HISTORY];
		MicroProfileFrameExtraCounterData* Data = S.FrameExtraCounterData;
		uint32_t NumTimers = 0;
		uint32_t j;
		printf("%d", i);
		Data += ((i + nFirstFrame) % MICROPROFILE_MAX_FRAME_HISTORY);
		NumTimers = MicroProfileMin(TotalTimers, (uint32_t)Data->NumTimers);			
		for(j = 0; j < NumTimers; ++j)
		{
			printf(",%f", Data->Timers[j] * fToMs[j]);
		}
		for(; j < TotalTimers; ++j)
			printf(",0");
		printf("\n");
	}	
}

void MicroProfileDumpCsvGroupFrames(MicroProfileWriteCallback CB, void* Handle, uint32_t nFirstFrame, uint32_t nLastFrame, uint32_t nNumFrames)
{
	MP_ASSERT(S.FrameExtraCounterData);
	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());

	uint32_t nGroupCount = S.nGroupCount;

	float* fToMs = (float*)alloca(sizeof(float)*nGroupCount);
	for(uint32_t i = 0; i < nGroupCount; ++i)
		fToMs[i] = S.GroupInfo[i].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;



	for(uint32_t i = 0; i < nGroupCount; ++i)
	{
		printf(i == 0 ? "FrameNumber, \"%s\"" : ",\"%s\"", S.GroupInfo[i].pName);
	}

	printf("\n");
	for(uint32_t i = 0; i < nNumFrames; ++i)
	{
		MicroProfileFrameExtraCounterData* Data = S.FrameExtraCounterData;
		uint32_t NumGroups = 0;
		uint32_t j;
		printf("%d", i);
		Data += ((i + nFirstFrame) % MICROPROFILE_MAX_FRAME_HISTORY);
		NumGroups = MicroProfileMin(nGroupCount, (uint32_t)Data->NumGroups);
		for(j = 0; j < NumGroups; ++j)
		{
			printf(",%f", Data->Groups[j] * fToMs[j]);
		}
		for(; j < nGroupCount; ++j)
			printf(",0");
		printf("\n");
	}
	
}

void MicroProfileDumpCsv(uint32_t nDumpFrameCount)
{
	uint32_t nNumFrames, nFirstFrame, nLastFrame;
	MicroProfileGetFramesToDump((uint64_t)-1, nDumpFrameCount, nFirstFrame, nLastFrame, nNumFrames);

	char Path[MICROPROFILE_MAX_PATH];
	int Length;
	if(S.FrameExtraCounterData)
	{
		Length = snprintf(Path, sizeof(S.CsvDumpPath), "%s_timer_frames.csv", S.CsvDumpPath);
		if(Length > 0 && Length < MICROPROFILE_MAX_PATH)
		{
			FILE* F = fopen(Path, "w");
			if(F)
			{
				MicroProfileDumpCsvTimerFrames(MicroProfileWriteFile, F, nFirstFrame, nLastFrame, nNumFrames);
				fclose(F);
			}	
		}
		Length = snprintf(Path, sizeof(S.CsvDumpPath), "%s_group_frames.csv", S.CsvDumpPath);
		if(Length > 0 && Length < MICROPROFILE_MAX_PATH)
		{
			FILE* F = fopen(Path, "w");
			if(F)
			{
				MicroProfileDumpCsvGroupFrames(MicroProfileWriteFile, F, nFirstFrame, nLastFrame, nNumFrames);
				fclose(F);
			}
			
		}
	}
	if(S.CsvConfig.State == MicroProfileCsvConfig::ACTIVE)
	{
		Length = snprintf(Path, sizeof(S.CsvDumpPath), "%s_custom.csv", S.CsvDumpPath);
		if(Length > 0 && Length < MICROPROFILE_MAX_PATH)
		{
			FILE* F = fopen(Path, "w");
			if(F)
			{
				MicroProfileDumpCsvWithConfig(MicroProfileWriteFile, F, nFirstFrame, nLastFrame, nNumFrames);
				fclose(F);
			}
		}
	}

}

void MicroProfileDumpCsvLegacy(MicroProfileWriteCallback CB, void* Handle)
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
		float fToMs = S.GroupInfo[j].Type == MicroProfileTokenTypeGpu ? fToMsGPU : fToMsCPU;
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
}
#undef printf

void MicroProfileDumpCsvLegacy()
{
	char Path[MICROPROFILE_MAX_PATH];
	int Length = snprintf(Path, sizeof(S.CsvDumpPath), "%s.csv", S.CsvDumpPath);
	if(Length > 0 && Length < MICROPROFILE_MAX_PATH)
	{
		FILE* F = fopen(Path, "w");
		if(F)
		{
			MicroProfileDumpCsvLegacy(MicroProfileWriteFile, F);
			fclose(F);
		}
	}
}

void MicroProfileDumpHtmlLive(MicroProfileWriteCallback CB, void* Handle)
{
	for(size_t i = 0; i < g_MicroProfileHtmlLive_begin_count; ++i)
	{
		CB(Handle, g_MicroProfileHtmlLive_begin_sizes[i] - 1, g_MicroProfileHtmlLive_begin[i]);
	}
	for(size_t i = 0; i < g_MicroProfileHtmlLive_end_count; ++i)
	{
		CB(Handle, g_MicroProfileHtmlLive_end_sizes[i] - 1, g_MicroProfileHtmlLive_end[i]);
	}
}
void MicroProfileGetCoreInformation()
{
#ifdef _WIN32
	unsigned long BufferSize;
	HANDLE Process = GetCurrentProcess();
	GetSystemCpuSetInformation(nullptr, 0, &BufferSize, Process, 0);
	char* Buffer = (char*)alloca(BufferSize);
	if(!GetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)Buffer, BufferSize, &BufferSize, Process, 0))
	{
		return;
	}
	for (ULONG Size = 0; Size < BufferSize; )
	{
		PSYSTEM_CPU_SET_INFORMATION CpuSet = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(Buffer);
		if (CpuSet->Type == CPU_SET_INFORMATION_TYPE::CpuSetInformation)
		{
			if (CpuSet->CpuSet.CoreIndex < MICROPROFILE_MAX_CPU_CORES)
			{
				S.CoreEfficiencyClass[CpuSet->CpuSet.LogicalProcessorIndex] = CpuSet->CpuSet.EfficiencyClass;
			}
		}
		Buffer += CpuSet->Size;
		Size += CpuSet->Size;
	}
#endif
}

void MicroProfileDumpHtml(MicroProfileWriteCallback CB, void* Handle, uint64_t nMaxFrames, const char* pHost, uint64_t nStartFrameId = (uint64_t)-1)
{
	// Stall pushing of timers
	uint64_t nActiveGroup[MICROPROFILE_MAX_GROUP_INTS];
	memcpy(nActiveGroup, S.nActiveGroups, sizeof(S.nActiveGroups));
	memset(S.nActiveGroups, 0, sizeof(S.nActiveGroups));
	bool AnyActive = S.AnyActive;
	S.AnyActive = false;

	S.nPauseTicks = MP_TICK();	

	MicroProfileGetCoreInformation();


	if (S.bContextSwitchRunning) {
		auto StallForContextSwitchThread = []()
		{
			int64_t nPauseTicks = S.nPauseTicks;
			int64_t nContextSwitchStalledTick = S.nContextSwitchStalledTick;
			return (nPauseTicks - nContextSwitchStalledTick) > 0;
		};
		int SleepMs = 1;
		while (S.bContextSwitchRunning && !S.bContextSwitchStop && StallForContextSwitchThread())
		{
			MicroProfileSleep(SleepMs);
			SleepMs = SleepMs * 2 / 3;
			SleepMs = MicroProfileMin(128, SleepMs);
		}
		int64_t TicksAfterStall = MP_TICK();
		uprintf("Stalled %7.2fms for context switch data\n", MicroProfileTickToMsMultiplierCpu() * (TicksAfterStall - S.nPauseTicks));
	}

	MicroProfileHashTable StringsHashTable;
	MicroProfileHashTableInit(&StringsHashTable, 50, 25, MicroProfileHashTableCompareString, MicroProfileHashTableHashString);

	defer
	{
		MicroProfileHashTableDestroy(&StringsHashTable);
	};

	MicroProfileCounterFetchCounters();
	for(size_t i = 0; i < g_MicroProfileHtml_begin_count; ++i)
	{
		CB(Handle, g_MicroProfileHtml_begin_sizes[i] - 1, g_MicroProfileHtml_begin[i]);
	}
	// dump info
	uint64_t nTicks = MP_TICK();

	float fToMsCPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	float fToMsGPU = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
	float fAggregateMs = fToMsCPU * (nTicks - S.nAggregateFlipTick);

	uint32_t nNumFrames = 0;
	uint32_t nFirstFrame = (uint32_t)-1;
	if(nStartFrameId != (uint64_t)-1)
	{
		// search for the frane
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
		nNumFrames = (MICROPROFILE_MAX_FRAME_HISTORY - MICROPROFILE_GPU_FRAME_DELAY - 3); // leave a few to not overwrite
		nNumFrames = MicroProfileMin(nNumFrames, (uint32_t)nMaxFrames);
		nFirstFrame = (S.nFrameCurrent + MICROPROFILE_MAX_FRAME_HISTORY - nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	}

	uint32_t nLastFrame = (nFirstFrame + nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
	MP_ASSERT(nFirstFrame < MICROPROFILE_MAX_FRAME_HISTORY);
	MP_ASSERT(nLastFrame < MICROPROFILE_MAX_FRAME_HISTORY);

	MicroProfilePrintf(CB, Handle, "S.DumpHost = '%s';\n", pHost ? pHost : "");
	time_t CaptureTime;
	time(&CaptureTime);
	MicroProfilePrintf(CB, Handle, "S.DumpUtcCaptureTime = %ld;\n", CaptureTime);
	MicroProfilePrintf(CB, Handle, "S.AggregateInfo = {'Frames':%d, 'Time':%f};\n", S.nAggregateFrames, fAggregateMs);

	// categories
	MicroProfilePrintf(CB, Handle, "S.CategoryInfo = Array(%d);\n", S.nCategoryCount);
	for(uint32_t i = 0; i < S.nCategoryCount; ++i)
	{
		MicroProfilePrintf(CB, Handle, "S.CategoryInfo[%d] = \"%s\";\n", i, S.CategoryInfo[i].pName);
	}

	// groups
	MicroProfilePrintf(CB, Handle, "S.GroupInfo = Array(%d);\n\n", S.nGroupCount + 1);
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	float fRcpAggregateFrames = 1.f / nAggregateFrames;
	(void)fRcpAggregateFrames;
	char ColorString[32];
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		MP_ASSERT(i == S.GroupInfo[i].nGroupIndex);
		float fToMs = S.GroupInfo[i].Type == MicroProfileTokenTypeCpu ? fToMsCPU : fToMsGPU;
		const char* pColorStr = "";
		if(S.GroupInfo[i].nColor != 0x42)
		{
			stbsp_snprintf(ColorString,
						   sizeof(ColorString) - 1,
						   "#%02x%02x%02x",
						   MICROPROFILE_UNPACK_RED(S.GroupInfo[i].nColor) & 0xff,
						   MICROPROFILE_UNPACK_GREEN(S.GroupInfo[i].nColor) & 0xff,
						   MICROPROFILE_UNPACK_BLUE(S.GroupInfo[i].nColor) & 0xff);
			pColorStr = &ColorString[0];
		}
		MicroProfilePrintf(CB,
						   Handle,
						   "S.GroupInfo[%d] = MakeGroup(%d, \"%s\", %d, %d, %d, %f, %f, %f, '%s');\n",
						   S.GroupInfo[i].nGroupIndex,
						   S.GroupInfo[i].nGroupIndex,
						   S.GroupInfo[i].pName,
						   S.GroupInfo[i].nCategory,
						   S.GroupInfo[i].nNumTimers,
						   S.GroupInfo[i].Type == MicroProfileTokenTypeGpu ? 1 : 0,
						   fToMs * S.AggregateGroup[i],
						   fToMs * S.AggregateGroup[i] / nAggregateFrames,
						   fToMs * S.AggregateGroupMax[i],
						   pColorStr);
	}
	uint32_t nUncategorized = S.nGroupCount;

	MicroProfilePrintf(CB,
					   Handle,
					   "S.GroupInfo[%d] = MakeGroup(%d, \"%s\", %d, %d, %d, %f, %f, %f, 'grey');\n",
					   nUncategorized,
					   nUncategorized,
					   "Uncategorized",
					   -1,
					   1,
					   // S.GroupInfo[i].Type == MicroProfileTokenTypeGpu ? 1 :
					   0,
					   0,
					   0,
					   0);

	// timers

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

	MicroProfilePrintf(CB, Handle, "\nS.TimerInfo = Array(%d);\n\n", S.nTotalTimers);
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t nIdx = i * 2;
		MP_ASSERT(i == S.TimerInfo[i].nTimerIndex);
		MicroProfilePrintf(CB, Handle, "S.Meta%d = [];\n", i);
		MicroProfilePrintf(CB, Handle, "S.MetaAvg%d = [];\n", i);
		MicroProfilePrintf(CB, Handle, "S.MetaMax%d = [];\n", i);

		uint32_t nColor = S.TimerInfo[i].nColor;
		uint32_t nColorDark = (nColor >> 1) & ~0x80808080;
		MicroProfilePrintf(CB,
						   Handle,
						   "S.TimerInfo[%d] = MakeTimer(%d, \"%s\", %d, '#%02x%02x%02x','#%02x%02x%02x', %f, %f, %f, %f, %f, %f, %d, %f, S.Meta%d, S.MetaAvg%d, S.MetaMax%d, %d);\n",
						   S.TimerInfo[i].nTimerIndex,
						   S.TimerInfo[i].nTimerIndex,
						   S.TimerInfo[i].pName,
						   S.TimerInfo[i].nGroupIndex,
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
						   i,
						   i,
						   i,
						   S.TimerInfo[i].Flags);
	}

	uint32_t nTotalTimersExt = S.nTotalTimers;
	{
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfileThreadLog* pLog = S.Pool[j];
			uint32_t nLogStart = S.Frames[nFirstFrame].nLogStart[j];
			uint32_t nLogEnd = S.Frames[nLastFrame].nLogStart[j];
			uint64_t nLogType;
			if(nLogStart != nLogEnd)
			{
				for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint64_t v = pLog->Log[k];
					nLogType = MicroProfileLogGetType(v);
					uint32_t tidx = MicroProfileLogGetTimerIndex(v);
					if((nLogType == MP_LOG_ENTER || nLogType == MP_LOG_LEAVE) && tidx == ETOKEN_CSTR_PTR)
					{
						MP_ASSERT(k + 1 != nLogEnd);
						uint64_t v1 = pLog->Log[(k + 1) % MICROPROFILE_BUFFER_SIZE];
						const char* pString = (const char*)MicroProfileLogGetExtendedPayloadNoDataPtr(v1);
						uintptr_t value;
						if(!MicroProfileHashTableGet(&StringsHashTable, (uint64_t)pString, &value))
						{
							uintptr_t nTimerIndex = nTotalTimersExt++;
							MicroProfileHashTableSet(&StringsHashTable, (uint64_t)pString, nTimerIndex);
							MicroProfilePrintf(
								CB, Handle, "S.TimerInfo.push(MakeTimer(%d, \"%s\", %d, '#000000','#000000', 0, 0, 0, 0, 0, 0, 0, 0, null, null, null, 0));\n", nTimerIndex, pString, nUncategorized);
						}
					}
				}
			}
		}
	}

	MicroProfilePrintf(CB, Handle, "\nS.ThreadNames = [");
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
	MicroProfilePrintf(CB, Handle, "\nS.ISGPU = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		MicroProfilePrintf(CB, Handle, "%d,", (S.Pool[i] && S.Pool[i]->nGpu) ? 1 : 0);
	}
	MicroProfilePrintf(CB, Handle, "];\n\n");

	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "S.ThreadGroupTime%d = [", i);
			float fToMs = S.Pool[i]->nGpu ? fToMsGPU : fToMsCPU;
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				MicroProfilePrintf(CB, Handle, "%f,", S.Pool[i]->nAggregateGroupTicks[j] / nAggregateFrames * fToMs);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
		}
	}
	MicroProfilePrintf(CB, Handle, "\nS.ThreadGroupTimeArray = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "S.ThreadGroupTime%d,", i);
		}
	}
	MicroProfilePrintf(CB, Handle, "];\n");

	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "S.ThreadGroupTimeTotal%d = [", i);
			float fToMs = S.Pool[i]->nGpu ? fToMsGPU : fToMsCPU;
			for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
			{
				MicroProfilePrintf(CB, Handle, "%f,", S.Pool[i]->nAggregateGroupTicks[j] * fToMs);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
		}
	}
	MicroProfilePrintf(CB, Handle, "\nS.ThreadGroupTimeTotalArray = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfilePrintf(CB, Handle, "S.ThreadGroupTimeTotal%d,", i);
		}
	}
	MicroProfilePrintf(CB, Handle, "];");

	MicroProfilePrintf(CB, Handle, "\nS.ThreadIds = [");
	for(uint32_t i = 0; i < S.nNumLogs; ++i)
	{
		if(S.Pool[i])
		{
			MicroProfileThreadIdType ThreadId = S.Pool[i]->nThreadId;
			if(!ThreadId)
			{
				ThreadId = (MicroProfileThreadIdType)-1;
			}
			MicroProfilePrintf(CB, Handle, "%" PRIu64 ",", (uint64_t)ThreadId);
		}
		else
		{
			MicroProfilePrintf(CB, Handle, "-1,");
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
			MicroProfilePrintf(CB, Handle, "\nS.CounterHistoryArray%d =[", i);
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
			double fRcp = nCounterHeightBase ? (1.0 / nCounterHeightBase) : 0;

			MicroProfilePrintf(CB, Handle, "\nS.CounterHistoryArrayPrc%d =[", i);
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				uint32_t nHistoryIndex = (nBaseIndex + j) % MICROPROFILE_GRAPH_HISTORY;
				int64_t nValue = MicroProfileClamp(S.nCounterHistory[nHistoryIndex][i], nCounterMin, nCounterMax);
				float fPrc = (nValue + nCounterOffset) * fRcp;
				MicroProfilePrintf(CB, Handle, "%f,", fPrc);
			}
			MicroProfilePrintf(CB, Handle, "];\n");
			MicroProfilePrintf(CB, Handle, "S.CounterHistory%d = MakeCounterHistory(%d, S.CounterHistoryArray%d, S.CounterHistoryArrayPrc%d)\n", i, i, i, i);
		}
		else
		{
			MicroProfilePrintf(CB, Handle, "S.CounterHistory%d;\n", i);
		}
	}

	MicroProfilePrintf(CB, Handle, "\nS.CounterInfo = [");

	for(int i = 0; i < (int)S.nNumCounters; ++i)
	{
		uint64_t nCounter = S.Counters[i].load();
		uint64_t nLimit = S.CounterInfo[i].nLimit;
		float fCounterPrc = 0.f;
		float fBoxPrc = 1.f;
		if(nLimit)
		{
			fCounterPrc = (float)nCounter / nLimit;
			if(fCounterPrc > 1.f)
			{
				fBoxPrc = 1.f / fCounterPrc;
				fCounterPrc = 1.f;
			}
		}

		char Formatted[64];
		char FormattedLimit[64];
		MicroProfileFormatCounter(S.CounterInfo[i].eFormat, nCounter, Formatted, sizeof(Formatted) - 1);
		MicroProfileFormatCounter(S.CounterInfo[i].eFormat, S.CounterInfo[i].nLimit, FormattedLimit, sizeof(FormattedLimit) - 1);
		MicroProfilePrintf(CB,
						   Handle,
						   "MakeCounter(%d, %d, %d, %d, %d, '%s', %lld, %lld, %lld, '%s', %lld, '%s', %f, %f, %d, S.CounterHistory%d),",
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
						   i);
	}
	MicroProfilePrintf(CB, Handle, "];\n\n");

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

	uprintf("dumping %d frames\n", nNumFrames);
	uprintf("dumping frame %d to %d\n", nFirstFrame, nLastFrame);

	uint32_t* nTimerCounter = (uint32_t*)alloca(sizeof(uint32_t) * S.nTotalTimers);
	memset(nTimerCounter, 0, sizeof(uint32_t) * S.nTotalTimers);

	{
		MicroProfilePrintf(CB, Handle, " //Timeline begin\n");
		MicroProfileThreadLog* pLog = &S.TimelineLog;
		uint32_t nFrameIndexFirst = (nFirstFrame) % MICROPROFILE_MAX_FRAME_HISTORY;
		uint32_t nFrameIndexLast = (nFirstFrame + nNumFrames) % MICROPROFILE_MAX_FRAME_HISTORY;
		{
			// find the frame that has an active marker the furtest distance from the selected range
			int nDelta = 0;
			int nOffset = 0;
			for(uint32_t i = nFrameIndexFirst; i != nFrameIndexLast; i = (i + 1) % MICROPROFILE_MAX_FRAME_HISTORY)
			{
				int D = (int)S.Frames[i].nTimelineFrameMax - nOffset;
				nDelta = MicroProfileMax(D, nDelta);
				nOffset++;
			}
			nFrameIndexFirst = (nFirstFrame - nDelta) % MICROPROFILE_MAX_FRAME_HISTORY;
		}

		uint32_t nLogStart = S.Frames[nFrameIndexFirst].nLogStartTimeline;
		uint32_t nLogEnd = S.Frames[nFrameIndexLast].nLogStartTimeline;
		float fToMs = MicroProfileTickToMsMultiplier(nTicksPerSecondCpu);

#define pp(...) MicroProfilePrintf(CB, Handle, __VA_ARGS__)

		if(nLogStart != nLogEnd)
		{
			uint32_t nLogType;
			float fTime;
			int f = 0;

			pp("S.TimelineColorArray=[");
			for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
			{
				uint64_t v = pLog->Log[k];
				uint64_t nIndex = MicroProfileLogGetTimerIndex(v);
				uint64_t nTick = MicroProfileLogGetTick(v);
				(void)nTick;
				nLogType = MicroProfileLogGetType(v);
				switch(nLogType)
				{
				case MP_LOG_ENTER:
					break;
				case MP_LOG_LEAVE:
					pp("%c'%s'", f++ ? ',' : ' ', "#ff8080");
					break;

				case MP_LOG_EXTENDED:
				case MP_LOG_EXTENDED_NO_DATA:
					uint32_t payload = MicroProfileLogGetExtendedPayload(v);
					if(nIndex == ETOKEN_CUSTOM_COLOR)
					{
						uint32_t nColor = payload;
						pp("%c'#%02x%02x%02x'", f++ ? ',' : ' ', MICROPROFILE_UNPACK_RED(nColor) & 0xff, MICROPROFILE_UNPACK_GREEN(nColor) & 0xff, MICROPROFILE_UNPACK_BLUE(nColor) & 0xff);
					}
					k += MicroProfileLogGetDataSize(v);
					break;
				}
			}
			pp("];\n");

			f = 0;
			pp("S.TimelineIdArray=[");
			for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
			{
				uint64_t v = pLog->Log[k];
				uint64_t nIndex = MicroProfileLogGetTimerIndex(v);
				uint64_t nTick = MicroProfileLogGetTick(v);
				(void)nTick;
				nLogType = MicroProfileLogGetType(v);
				switch(nLogType)
				{
				case MP_LOG_ENTER:
				case MP_LOG_LEAVE:
				case MP_LOG_EXTENDED_NO_DATA:

					break;
				case MP_LOG_EXTENDED:
					if(nIndex == ETOKEN_CUSTOM_ID)
					{
						pp("%c%d", f++ ? ',' : ' ', (uint32_t)nTick);
					}
					k += MicroProfileLogGetDataSize(v);
					break;
				}
			}
			pp("];\n");

			f = 0;

			pp("S.TimelineArray=[");
			for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
			{
				uint64_t v = pLog->Log[k];
				nLogType = MicroProfileLogGetType(v);
				switch(nLogType)
				{
				case MP_LOG_ENTER:
				case MP_LOG_LEAVE:
					fTime = MicroProfileLogTickDifference(nTickStart, v) * fToMs;
					pp("%c%f", f++ ? ',' : ' ', fTime);
					break;
				case MP_LOG_EXTENDED:
					k += MicroProfileLogGetDataSize(v);
					break;
				case MP_LOG_EXTENDED_NO_DATA:
					break;
				}
			}
			pp("];\n");
			pp("S.TimelineNames=[");
			f = 0;
			char String[MICROPROFILE_MAX_STRING + 1];
			for(uint32_t k = nLogStart; k != nLogEnd;)
			{
				uint64_t v = pLog->Log[k];
				nLogType = MicroProfileLogGetType(v);
				uint64_t nIndex = MicroProfileLogGetTimerIndex(v);
				uint64_t nTick = MicroProfileLogGetTick(v);
				(void)nTick;
				switch(nLogType)
				{
				case MP_LOG_ENTER:
				case MP_LOG_LEAVE:
					if(nIndex == ETOKEN_CUSTOM_NAME && nLogType == MP_LOG_LEAVE)
					{
						// pp(f++ ? ",''" : "''");
					}
					k = (k + 1) % MICROPROFILE_BUFFER_SIZE;
					break;
				case MP_LOG_EXTENDED_NO_DATA:
					k = (k + 1) % MICROPROFILE_BUFFER_SIZE;
					break;
				case MP_LOG_EXTENDED:
					uint32_t nSize = MicroProfileLogGetDataSize(v);

					if(nIndex == ETOKEN_CUSTOM_ID)
					{
						char* pSource = (char*)&pLog->Log[(k + 1) % MICROPROFILE_BUFFER_SIZE];
						const char* pOut = nullptr;
						if(nSize == 0)
						{
							pOut = "";
						}
						else if(k + nSize <= MICROPROFILE_BUFFER_SIZE)
						{
							pOut = pSource;
						}
						else
						{
							pOut = &String[0];
							char* pDest = &String[0];
							MP_ASSERT(nSize * 8 < sizeof(MICROPROFILE_MAX_STRING) + 1);
							uint32_t Index = (k + 1) % MICROPROFILE_BUFFER_SIZE;
							for(uint32_t l = 0; l < nSize; ++l)
							{
								memcpy(pDest, (char*)pLog->Log[Index], sizeof(uint64_t));
								Index = (Index + 1) % MICROPROFILE_BUFFER_SIZE;
							}
						}
						if(f++)
						{
							pp(",'%s'", pOut);
						}
						else
						{
							pp("'%s'", pOut);
						}
					}
					k = (k + 1 + nSize) % MICROPROFILE_BUFFER_SIZE;
					break;
				}
			}
			pp("];\n");
		}
		MicroProfilePrintf(CB, Handle, " //Timeline end\n");
	}

	MicroProfilePrintf(CB, Handle, "S.Frames = Array(%d);\n", nNumFrames);
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
			uint32_t nLogType;
			float fToMs;
			uint64_t nStartTick;
			float fToMsCpu = MicroProfileTickToMsMultiplier(nTicksPerSecondCpu);
			float fToMsBase = MicroProfileTickToMsMultiplier(pLog->nGpu ? nTicksPerSecondGpu : nTicksPerSecondCpu);
			MicroProfilePrintf(CB, Handle, "S.ts_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				int f = 0;
				for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
				{
					float fTime;
					MicroProfileLogEntry v = pLog->Log[k];
					nLogType = MicroProfileLogGetType(v);
					fToMs = fToMsBase;
					nStartTick = nStartTickBase;
					switch(nLogType)
					{
					case MP_LOG_EXTENDED:
					{
						fTime = 0.f;
						k += MicroProfileLogGetDataSize(v);
						break;
					}
					case MP_LOG_EXTENDED_NO_DATA:
					{
						uint32_t nTimerIndex = (uint32_t)MicroProfileLogGetTimerIndex(v);
						if(nTimerIndex == ETOKEN_GPU_CPU_TIMESTAMP)
						{
							fToMs = fToMsCpu;
							nStartTick = nTickStart;
							fTime = MicroProfileLogTickDifference(nStartTick, v) * fToMs;
						}
						else
						{
							fTime = 0.f;
						}
						break;
					}
					default:
						fTime = MicroProfileLogTickDifference(nStartTick, pLog->Log[k]) * fToMs;
					}
					MicroProfilePrintf(CB, Handle, f++ ? ",%f" : "%f", fTime);
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");

			MicroProfilePrintf(CB, Handle, "S.tt_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				uint32_t k = nLogStart;
				MicroProfilePrintf(CB, Handle, "%d", MicroProfileLogGetType(pLog->Log[k]));
				for(k = (k + 1) % MICROPROFILE_BUFFER_SIZE; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint64_t v = pLog->Log[k];
					uint32_t nLogType2 = MicroProfileLogGetType(v);

					if(nLogType2 > MP_LOG_ENTER)
						nLogType2 |= (MicroProfileLogGetExtendedToken(v))
									<< 2; // pack extended token here.. this way all code can check agains ENTER/LEAVE, and only the ext code needs to care about the top bits.
					MicroProfilePrintf(CB, Handle, ",%d", nLogType2);
					if(nLogType2 == MP_LOG_EXTENDED)
						k += MicroProfileLogGetDataSize(v);
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");

			MicroProfilePrintf(CB, Handle, "S.ti_%d_%d = [", i, j);
			if(nLogStart != nLogEnd)
			{
				for(uint32_t k = nLogStart; k != nLogEnd; k = (k + 1) % MICROPROFILE_BUFFER_SIZE)
				{
					uint64_t v = pLog->Log[k];
					nLogType = MicroProfileLogGetType(v);
					const char* pFormat = k == nLogStart ? "%d" : ",%d";
					if(nLogType == MP_LOG_ENTER || nLogType == MP_LOG_LEAVE)
					{
						uint32_t nTimerIndex = (uint32_t)MicroProfileLogGetTimerIndex(pLog->Log[k]);
						if(ETOKEN_CSTR_PTR == nTimerIndex)
						{
							MP_ASSERT(k + 1 != nLogEnd);
							uint64_t v1 = pLog->Log[(k + 1) % MICROPROFILE_BUFFER_SIZE];

							const char* pString = (const char*)MicroProfileLogGetExtendedPayloadNoDataPtr(v1);
							uintptr_t value;
							if(!MicroProfileHashTableGet(&StringsHashTable, (uint64_t)pString, &value))
							{
								MP_BREAK(); // should be covered earlier.
							}
							MicroProfilePrintf(CB, Handle, pFormat, value);
						}
						else
						{
							if(nTimerIndex < S.nTotalTimers)
							{
								nTimerCounter[nTimerIndex]++;
							}
							MicroProfilePrintf(CB, Handle, pFormat, nTimerIndex);
						}
					}
					else
					{
						uint64_t ExtendedToken = MicroProfileLogGetExtendedToken(v);
						uint64_t PayloadNoData = MicroProfileLogGetExtendedPayloadNoData(v);
						switch(ExtendedToken)
						{
						case ETOKEN_GPU_CPU_SOURCE_THREAD:
							MicroProfilePrintf(CB, Handle, pFormat, PayloadNoData);
							break;
						default:
							MicroProfilePrintf(CB, Handle, pFormat, -1);
						}

						if(nLogType == MP_LOG_EXTENDED)
							k += MicroProfileLogGetDataSize(v);
					}
				}
			}
			MicroProfilePrintf(CB, Handle, "];\n");
		}

		MicroProfilePrintf(CB, Handle, "S.ts%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "S.ts_%d_%d,", i, j);
		}
		MicroProfilePrintf(CB, Handle, "];\n");
		MicroProfilePrintf(CB, Handle, "S.tt%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "S.tt_%d_%d,", i, j);
		}
		MicroProfilePrintf(CB, Handle, "];\n");

		MicroProfilePrintf(CB, Handle, "S.ti%d = [", i);
		for(uint32_t j = 0; j < S.nNumLogs; ++j)
		{
			MicroProfilePrintf(CB, Handle, "S.ti_%d_%d,", i, j);
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
		MicroProfilePrintf(CB, Handle, "S.Frames[%d] = MakeFrame(%d, %f, %f, %f, %f, S.ts%d, S.tt%d, S.ti%d);\n", i, 0, fFrameMs, fFrameEndMs, fFrameGpuMs, fFrameGpuEndMs, i, i, i);
	}

	uint32_t nContextSwitchStart = 0;
	uint32_t nContextSwitchEnd = 0;
	MicroProfileContextSwitchSearch(&nContextSwitchStart, &nContextSwitchEnd, nTickStart, nTickEnd);

	uprintf("CONTEXT SWITCH SEARCH .... %d %d %d .... %lld, %lld\n", nContextSwitchStart, nContextSwitchEnd, nContextSwitchEnd - nContextSwitchStart, nTickStart, nTickEnd);

	uint32_t nWrittenBefore = S.nWebServerDataSent;
	MicroProfilePrintf(CB, Handle, "S.CSwitchThreadInOutCpu = [");
	for(uint32_t j = nContextSwitchStart; j != nContextSwitchEnd; j = (j + 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE)
	{
		MicroProfileContextSwitch CS = S.ContextSwitch[j];
		int nCpu = CS.nCpu;
		MicroProfilePrintf(CB, Handle, "%d,%d,%d,", CS.nThreadIn, CS.nThreadOut, nCpu);
	}
	MicroProfilePrintf(CB, Handle, "];\n");
	MicroProfilePrintf(CB, Handle, "S.CSwitchTime = [");
	float fToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
	for(uint32_t j = nContextSwitchStart; j != nContextSwitchEnd; j = (j + 1) % MICROPROFILE_CONTEXT_SWITCH_BUFFER_SIZE)
	{
		MicroProfileContextSwitch CS = S.ContextSwitch[j];
		float fTime = MicroProfileLogTickDifference(nTickStart, CS.nTicks) * fToMsCpu;
		MicroProfilePrintf(CB, Handle, "%f,", fTime);
	}
	MicroProfilePrintf(CB, Handle, "];\n");

	MicroProfilePrintf(CB, Handle, "S.CSwitchThreads = {");

	MicroProfileThreadInfo* pThreadInfo = nullptr;
	uint32_t nNumThreads = MicroProfileGetThreadInfoArray(&pThreadInfo);
	for(uint32_t i = 0; i < nNumThreads; ++i)
	{
		const char* p1 = pThreadInfo[i].pThreadModule ? pThreadInfo[i].pThreadModule : "?";
		const char* p2 = pThreadInfo[i].pProcessModule ? pThreadInfo[i].pProcessModule : "?";

		MicroProfilePrintf(CB,
						   Handle,
						   "%" PRId64 ":{\'tid\':%" PRId64 ",\'pid\':%" PRId64 ",\'t\':\'%s\',\'p\':\'%s\'},",
						   (uint64_t)pThreadInfo[i].tid,
						   (uint64_t)pThreadInfo[i].tid,
						   (uint64_t)pThreadInfo[i].pid,
						   p1,
						   p2);
	}

	MicroProfilePrintf(CB, Handle, "};\n");
	MicroProfilePrintf(CB, Handle, "S.CoreEfficiencyClass = [");
	for (uint32_t i = 0; i < MICROPROFILE_MAX_CPU_CORES; ++i)
	{
		MicroProfilePrintf(CB, Handle, "%d,", S.CoreEfficiencyClass[i]);
	}
	MicroProfilePrintf(CB, Handle, "];\n");

	{
		MicroProfilePrintf(CB, Handle, "//String Table\n");
		MicroProfilePrintf(CB, Handle, "S.StringTable = {}\n");
		// dump string table
		MicroProfileHashTableIterator beg = MicroProfileGetHashTableIteratorBegin(&StringsHashTable);
		MicroProfileHashTableIterator end = MicroProfileGetHashTableIteratorEnd(&StringsHashTable);
		while(beg != end)
		{
			uint64_t Key = beg->Key;
			uint64_t Value = beg->Value;
			MicroProfilePrintf(CB, Handle, "S.StringTable[%d] = '%s';\n", Value, (const char*)Key);
			beg++;
		}
	}

	uint32_t nWrittenAfter = S.nWebServerDataSent;

	MicroProfilePrintf(CB, Handle, "//CSwitch Size %d\n", nWrittenAfter - nWrittenBefore);

	for(size_t i = 0; i < g_MicroProfileHtml_end_count; ++i)
	{
		CB(Handle, g_MicroProfileHtml_end_sizes[i] - 1, g_MicroProfileHtml_end[i]);
	}

	uint32_t* nGroupCounter = (uint32_t*)alloca(sizeof(uint32_t) * S.nGroupCount);

	memset(nGroupCounter, 0, sizeof(uint32_t) * S.nGroupCount);
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint32_t nGroupIndex = S.TimerInfo[i].nGroupIndex;
		nGroupCounter[nGroupIndex] += nTimerCounter[i];
	}

	uint32_t* nGroupCounterSort = (uint32_t*)alloca(sizeof(uint32_t) * S.nGroupCount);
	uint32_t* nTimerCounterSort = (uint32_t*)alloca(sizeof(uint32_t) * S.nTotalTimers);
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		nGroupCounterSort[i] = i;
	}
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		nTimerCounterSort[i] = i;
	}
	std::sort(nGroupCounterSort, nGroupCounterSort + S.nGroupCount, [nGroupCounter](const uint32_t l, const uint32_t r) { return nGroupCounter[l] > nGroupCounter[r]; });

	std::sort(nTimerCounterSort, nTimerCounterSort + S.nTotalTimers, [nTimerCounter](const uint32_t l, const uint32_t r) { return nTimerCounter[l] > nTimerCounter[r]; });

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

	memcpy(S.nActiveGroups, nActiveGroup, sizeof(S.nActiveGroups));
	S.AnyActive = AnyActive;
#if MICROPROFILE_DEBUG
	int64_t nTicksEnd = MP_TICK();
	float fMs = fToMsCpu * (nTicksEnd - S.nPauseTicks);
	uprintf("html dump took %6.2fms\n", fMs);
#endif

#undef pp

	S.nPauseTicks = 0;
}

void MicroProfileWriteFile(void* Handle, size_t nSize, const char* pData)
{
	fwrite(pData, nSize, 1, (FILE*)Handle);
}

void MicroProfileDumpToFile()
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileMutex());
	if(S.nDumpFileNextFrame & 1)
	{
		char Path[MICROPROFILE_MAX_PATH];
		int Length = snprintf(Path, sizeof(S.HtmlDumpPath), "%s.html", S.HtmlDumpPath);
		if(Length > 0 && Length < MICROPROFILE_MAX_PATH)
		{
			FILE* F = fopen(Path, "w");
			if(F)
			{
				MicroProfileDumpHtml(MicroProfileWriteFile, F, S.DumpFrameCount, S.HtmlDumpPath);
				fclose(F);
			}
		}
	}
	if(S.nDumpFileNextFrame & 2)
	{
#if MICROPROFILE_LEGACY_CSV
		MicroProfileDumpCsvLegacy();
#else
		MicroProfileDumpCsv(S.DumpFrameCount);
#endif
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
		if(S.WebServerPut > MICROPROFILE_WEBSERVER_SOCKET_BUFFER_SIZE / 2)
		{
			MicroProfileFlushSocket(Socket);
		}
	}
}

#if MICROPROFILE_MINIZ
#ifndef MICROPROFILE_COMPRESS_BUFFER_SIZE
#define MICROPROFILE_COMPRESS_BUFFER_SIZE (256 << 10)
#endif

#define MICROPROFILE_COMPRESS_CHUNK (MICROPROFILE_COMPRESS_BUFFER_SIZE / 2)
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

#ifndef MicroProfileSetNonBlocking // fcntl doesnt work on a some unix like platforms..
void MicroProfileSetNonBlocking(MpSocket Socket, int NonBlocking)
{
#ifdef _WIN32
	u_long nonBlocking = NonBlocking ? 1 : 0;
	ioctlsocket(Socket, FIONBIO, &nonBlocking);
#else
	int Options = fcntl(Socket, F_GETFL);
	if(NonBlocking)
	{
		fcntl(Socket, F_SETFL, Options | O_NONBLOCK);
	}
	else
	{
		fcntl(Socket, F_SETFL, Options & (~O_NONBLOCK));
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

	{
		int r = 0;
		int on = 1;
#if defined(_WIN32)
		r = setsockopt(S.ListenerSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
#else
		r = setsockopt(S.ListenerSocket, SOL_SOCKET, SO_REUSEADDR, (void*)&on, sizeof(on));
#endif
		(void)r;
	}

	int nStartPort = S.nWebServerPort;
	struct sockaddr_in Addr;
	Addr.sin_family = AF_INET;
	Addr.sin_addr.s_addr = INADDR_ANY;
	for(int i = 0; i < 20; ++i)
	{
		Addr.sin_port = htons(nStartPort + i);
		if(0 == bind(S.ListenerSocket, (sockaddr*)&Addr, sizeof(Addr)))
		{
			S.nWebServerPort = (uint32_t)(nStartPort + i);
			break;
		}
	}
	listen(S.ListenerSocket, 8);
}

void MicroProfileWebServerJoin()
{
	if(S.WebSocketThreadRunning)
	{
		MicroProfileThreadJoin(&S.WebSocketSendThread);
	}
	S.WebSocketThreadJoined = 1;
}

void MicroProfileWebServerStop()
{
	MP_ASSERT(S.WebSocketThreadJoined);
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
		S.nWSWasConnected = 1; // do not load default when url has one specified.
		return EMICROPROFILE_GET_COMMAND_LIVE;
	}
	if(*pStart == 'r') // range
	{
		// very very manual parsing
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
		pResult->nFrames = MICROPROFILE_WEBSERVER_DEFAULT_FRAMES;
	}
	return EMICROPROFILE_GET_COMMAND_DUMP;
}

void MicroProfileBase64Encode(char* pOut, const uint8_t* pIn, uint32_t nLen)
{
	static const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
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
			if(i + 2 < nLen)
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

// begin: SHA-1 in C
// ftp://ftp.funet.fi/pub/crypt/hash/sha/sha1.c
// SHA-1 in C
// By Steve Reid <steve@edmweb.com>
// 100% Public Domain

typedef struct
{
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
#define blk(i) (block->l[i & 15] = rol(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

#define R0(v, w, x, y, z, i)                                                                                                                                                                           \
	z += ((w & (x ^ y)) ^ y) + blk0(i) + 0x5A827999 + rol(v, 5);                                                                                                                                       \
	w = rol(w, 30);
#define R1(v, w, x, y, z, i)                                                                                                                                                                           \
	z += ((w & (x ^ y)) ^ y) + blk(i) + 0x5A827999 + rol(v, 5);                                                                                                                                        \
	w = rol(w, 30);
#define R2(v, w, x, y, z, i)                                                                                                                                                                           \
	z += (w ^ x ^ y) + blk(i) + 0x6ED9EBA1 + rol(v, 5);                                                                                                                                                \
	w = rol(w, 30);
#define R3(v, w, x, y, z, i)                                                                                                                                                                           \
	z += (((w | x) & y) | (w & x)) + blk(i) + 0x8F1BBCDC + rol(v, 5);                                                                                                                                  \
	w = rol(w, 30);
#define R4(v, w, x, y, z, i)                                                                                                                                                                           \
	z += (w ^ x ^ y) + blk(i) + 0xCA62C1D6 + rol(v, 5);                                                                                                                                                \
	w = rol(w, 30);

// Hash a single 512-bit block. This is the core of the algorithm.

static void MicroProfile_SHA1_Transform(uint32_t state[5], const unsigned char buffer[64])
{
	uint32_t a, b, c, d, e;
	typedef union
	{
		unsigned char c[64];
		uint32_t l[16];
	} CHAR64LONG16;
	CHAR64LONG16* block;

	block = (CHAR64LONG16*)buffer;
	// Copy context->state[] to working vars
	a = state[0];
	b = state[1];
	c = state[2];
	d = state[3];
	e = state[4];
	// 4 rounds of 20 operations each. Loop unrolled.
	R0(a, b, c, d, e, 0);
	R0(e, a, b, c, d, 1);
	R0(d, e, a, b, c, 2);
	R0(c, d, e, a, b, 3);
	R0(b, c, d, e, a, 4);
	R0(a, b, c, d, e, 5);
	R0(e, a, b, c, d, 6);
	R0(d, e, a, b, c, 7);
	R0(c, d, e, a, b, 8);
	R0(b, c, d, e, a, 9);
	R0(a, b, c, d, e, 10);
	R0(e, a, b, c, d, 11);
	R0(d, e, a, b, c, 12);
	R0(c, d, e, a, b, 13);
	R0(b, c, d, e, a, 14);
	R0(a, b, c, d, e, 15);
	R1(e, a, b, c, d, 16);
	R1(d, e, a, b, c, 17);
	R1(c, d, e, a, b, 18);
	R1(b, c, d, e, a, 19);
	R2(a, b, c, d, e, 20);
	R2(e, a, b, c, d, 21);
	R2(d, e, a, b, c, 22);
	R2(c, d, e, a, b, 23);
	R2(b, c, d, e, a, 24);
	R2(a, b, c, d, e, 25);
	R2(e, a, b, c, d, 26);
	R2(d, e, a, b, c, 27);
	R2(c, d, e, a, b, 28);
	R2(b, c, d, e, a, 29);
	R2(a, b, c, d, e, 30);
	R2(e, a, b, c, d, 31);
	R2(d, e, a, b, c, 32);
	R2(c, d, e, a, b, 33);
	R2(b, c, d, e, a, 34);
	R2(a, b, c, d, e, 35);
	R2(e, a, b, c, d, 36);
	R2(d, e, a, b, c, 37);
	R2(c, d, e, a, b, 38);
	R2(b, c, d, e, a, 39);
	R3(a, b, c, d, e, 40);
	R3(e, a, b, c, d, 41);
	R3(d, e, a, b, c, 42);
	R3(c, d, e, a, b, 43);
	R3(b, c, d, e, a, 44);
	R3(a, b, c, d, e, 45);
	R3(e, a, b, c, d, 46);
	R3(d, e, a, b, c, 47);
	R3(c, d, e, a, b, 48);
	R3(b, c, d, e, a, 49);
	R3(a, b, c, d, e, 50);
	R3(e, a, b, c, d, 51);
	R3(d, e, a, b, c, 52);
	R3(c, d, e, a, b, 53);
	R3(b, c, d, e, a, 54);
	R3(a, b, c, d, e, 55);
	R3(e, a, b, c, d, 56);
	R3(d, e, a, b, c, 57);
	R3(c, d, e, a, b, 58);
	R3(b, c, d, e, a, 59);
	R4(a, b, c, d, e, 60);
	R4(e, a, b, c, d, 61);
	R4(d, e, a, b, c, 62);
	R4(c, d, e, a, b, 63);
	R4(b, c, d, e, a, 64);
	R4(a, b, c, d, e, 65);
	R4(e, a, b, c, d, 66);
	R4(d, e, a, b, c, 67);
	R4(c, d, e, a, b, 68);
	R4(b, c, d, e, a, 69);
	R4(a, b, c, d, e, 70);
	R4(e, a, b, c, d, 71);
	R4(d, e, a, b, c, 72);
	R4(c, d, e, a, b, 73);
	R4(b, c, d, e, a, 74);
	R4(a, b, c, d, e, 75);
	R4(e, a, b, c, d, 76);
	R4(d, e, a, b, c, 77);
	R4(c, d, e, a, b, 78);
	R4(b, c, d, e, a, 79);
	// Add the working vars back into context.state[]
	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
	// Wipe variables
	a = b = c = d = e = 0;
}

void MicroProfile_SHA1_Init(MicroProfile_SHA1_CTX* context)
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

void MicroProfile_SHA1_Update(MicroProfile_SHA1_CTX* context, const unsigned char* data, unsigned int len)
{
	unsigned int i, j;

	j = (context->count[0] >> 3) & 63;
	if((context->count[0] += len << 3) < (len << 3))
		context->count[1]++;
	context->count[1] += (len >> 29);
	i = 64 - j;
	while(len >= i)
	{
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

void MicroProfile_SHA1_Final(unsigned char digest[20], MicroProfile_SHA1_CTX* context)
{
	uint32_t i, j;
	unsigned char finalcount[8];

	for(i = 0; i < 8; i++)
	{
		finalcount[i] = (unsigned char)((context->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255); // Endian independent
	}
	MicroProfile_SHA1_Update(context, (unsigned char*)"\200", 1);
	while((context->count[0] & 504) != 448)
	{
		MicroProfile_SHA1_Update(context, (unsigned char*)"\0", 1);
	}
	MicroProfile_SHA1_Update(context, finalcount, 8); // Should cause a SHA1Transform()
	for(i = 0; i < 20; i++)
	{
		digest[i] = (unsigned char)((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
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

// end: SHA-1 in C

void MicroProfileWebSocketSendState(MpSocket C);
void MicroProfileWebSocketSendEnabled(MpSocket C);
void MicroProfileWSPrintStart(MpSocket C);
void MicroProfileWSPrintf(const char* pFmt, ...);
void MicroProfileWSPrintEnd();
void MicroProfileWSFlush();
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
	MSG_TIMER_TREE = 1,
	MSG_ENABLED = 2,
	MSG_FRAME = 3,
	MSG_LOADSETTINGS = 4,
	MSG_PRESETS = 5,
	MSG_CURRENTSETTINGS = 6,
	MSG_COUNTERS = 7,
	MSG_FUNCTION_RESULTS = 8,
	MSG_INACTIVE_FRAME = 9,
	MSG_FUNCTION_NAMES = 10,
	MSG_INSTRUMENT_ERROR = 11
	// MSG_MODULE_NAME = 12,
};

enum
{
	VIEW_GRAPH_SPLIT = 0,
	VIEW_GRAPH_PERCENTILE = 1,
	VIEW_GRAPH_THREAD_GROUP = 2,
	VIEW_BAR = 3,
	VIEW_BAR_ALL = 4,
	VIEW_BAR_SINGLE = 5,
	VIEW_COUNTERS = 6,
	VIEW_SIZE = 7,
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
		LastSocket = MicroProfileMax(LastSocket, S.WebSockets[i] + 1);
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
		uprintf("%" PRId64 " ", (uint64_t)s);

		if(FD_ISSET(s, &Error))
		{
			uprintf("e");
		}
		else
		{
			uprintf("_");
		}
		if(FD_ISSET(s, &Read))
		{
			uprintf("r");
		}
		else
		{
			uprintf(" ");
		}
		if(FD_ISSET(s, &Write))
		{
			uprintf("w");
		}
		else
		{
			uprintf(" ");
		}
	}
	uprintf("\n");
	for(uint32_t i = 1; i < S.nNumWebSockets; i++)
	{
		MpSocket s = S.WebSockets[i];
		int error_code;
		socklen_t error_code_size = sizeof(error_code);
		int r = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size);
		MP_ASSERT(r >= 0);
		if(error_code != 0)
		{
#ifdef _WIN32
			char buffer[1024];
			strerror_s(buffer, sizeof(buffer) - 1, error_code);
			fprintf(stderr, "socket error: %d %s\n", (int)s, buffer);
#else
			fprintf(stderr, "socket error: %d %s\n", (int)s, strerror(error_code));
#endif
			MP_ASSERT(0);
		}
	}
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
			MICROPROFILE_SCOPE(g_MicroProfileSendLoop);
			MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(g_MicroProfileBytesPerFlip, nSendAmount);
			if(!MicroProfileSocketSend2(S.WebSockets[0], &S.WSBuf.SendBuffer[nSendStart], nSendAmount))
			{
				S.nSocketFail = 1;
			}
			else
			{
				S.WSBuf.nSendGet.store((nGet + nSendAmount) % MICROPROFILE_WEBSOCKET_BUFFER_SIZE);
			}
		}
		else
		{
			MicroProfileSleep(20);
		}
	}
	MicroProfileOnThreadExit();
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
			pMessage = (void*)((char*)pMessage + nAmount);
			nLen -= nAmount;
			S.WSBuf.nSendPut.store((nPut + nAmount) % MICROPROFILE_WEBSOCKET_BUFFER_SIZE);
		}
		else
		{
			if(S.nSocketFail)
			{
				return;
			}
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
	// MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSocketSend2", 0);
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
	while(nLen)
	{
		s = send(Connection, (const char*)pMessage, nLen, 0);
		if(s < 0)
		{
			const int error = errno;
			if(error == EAGAIN || error == EWOULDBLOCK)
			{
				MicroProfileSleep(20);
				continue;
			}
			break;
		}

		nLen -= s;
		pMessage = (const char*)pMessage + s;
	}
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
			uint8_t opcode : 4;
			uint8_t RSV3 : 1;
			uint8_t RSV2 : 1;
			uint8_t RSV1 : 1;
			uint8_t FIN : 1;
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
			uint8_t MASK : 1;
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
			nExtraSize[nExtraSizeBytes - i - 1] = nCount & 0xff;
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
	MP_ASSERT(pMessage == S.WSBuf.pBuffer);				   // space for header is preallocated here
	MP_ASSERT(pMessage == S.WSBuf.pBufferAllocation + 20); // space for header is preallocated here
	MP_ASSERT(nExtraSizeBytes < 18);
	char* pTmp = (char*)(pMessage - nExtraSizeBytes - 2);
	memcpy(pTmp + 2, &nExtraSize[0], nExtraSizeBytes);
	pTmp[1] = *(char*)&h1;
	pTmp[0] = *(char*)&h0;
// MicroProfileSocketSend(Connection, pTmp, nExtraSizeBytes + 2 + nLen);
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
		uprintf("unknown type %d\n", nType);
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
#ifndef MICROPROFILE_SETTINGS_FILE
#define MICROPROFILE_SETTINGS_FILE MICROPROFILE_SETTINGS_FILE_PATH "mppresets.cfg"
#endif
#ifndef MICROPROFILE_SETTINGS_FILE_BUILTIN
#define MICROPROFILE_SETTINGS_FILE_BUILTIN MICROPROFILE_SETTINGS_FILE_PATH "mppresets.builtin.cfg"
#endif
#define MICROPROFILE_SETTINGS_FILE_TEMP MICROPROFILE_SETTINGS_FILE ".tmp"

template <typename T>
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
	if(nFileSize > (32 << 10))
	{
		uprintf("trying to load a >32kb settings file on the stack. this should never happen!\n");
		MP_BREAK();
	}
	char* pFile = (char*)alloca(nFileSize + 1);
	fseek(F, 0, SEEK_SET);
	if(1 != fread(pFile, nFileSize, 1, F))
	{
		uprintf("failed to read settings file\n");
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

		auto SkipWhite = [&](char* pPos, const char* pEnd) {
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

		auto ParseName = [&](char* pPos, char* pEnd, const char** ppName, int* pLen) {
			pPos = SkipWhite(pPos, pEnd);
			int nLen = 0;
			*ppName = pPos;

			while(pPos != pEnd && (isalpha(*pPos) || isdigit(*pPos) || *pPos == '_'))
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

		auto ParseJson = [&](char* pPos, char* pEnd, const char** pJson, int* pLen) -> char* {
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

	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE, [&](const char* pName, uint32_t nNameSize, const char* pJson, uint32_t nJsonSize) -> bool {
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
	});
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
	MoveFileExA(MICROPROFILE_SETTINGS_FILE_TEMP, MICROPROFILE_SETTINGS_FILE, MOVEFILE_REPLACE_EXISTING);
#else
	rename(MICROPROFILE_SETTINGS_FILE_TEMP, MICROPROFILE_SETTINGS_FILE);
#endif
	return false;
}

void MicroProfileWriteJsonString(const char* pJson, uint32_t nJsonLen)
{
	char* pCur = (char*)pJson;
	char* pEnd = pCur + nJsonLen;
	MicroProfileWSPrintf("\"", pCur);
	while(pCur != pEnd)
	{
		char* pTag = strchr(pCur, '\"');
		if(pTag)
		{
			*pTag = '\0';
			MicroProfileWSPrintf("%s\\\"", pCur);
			*pTag = '\"';
			pCur = pTag + 1;
		}
		else
		{
			MicroProfileWSPrintf("%s\"", pCur);
			pCur = pEnd;
		}
	}
};

void MicroProfileWebSocketSendPresets(MpSocket Connection)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());
	uprintf("sending presets ... \n");
	MicroProfileWSPrintStart(Connection);
	MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{", MSG_PRESETS);
	MicroProfileWSPrintf("\"p\":{\"Default\":\"{}\"");

	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE, [](const char* pName, uint32_t nNameLen, const char* pJson, uint32_t nJsonLen) {
		MicroProfileWSPrintf(",\"%s\":", pName);
		MicroProfileWriteJsonString(pJson, nJsonLen);

		return true;
	});
	MicroProfileWSPrintf("},\"r\":{");
	bool bFirst = true;
	MicroProfileParseSettings(MICROPROFILE_SETTINGS_FILE_BUILTIN, [&bFirst](const char* pName, uint32_t nNameLen, const char* pJson, uint32_t nJsonLen) {
		MicroProfileWSPrintf("%c\"%s\":", bFirst ? ' ' : ',', pName);
		MicroProfileWriteJsonString(pJson, nJsonLen);

		bFirst = false;
		return true;
	});
	MicroProfileWSPrintf("}}}");
	MicroProfileWSFlush();
	MicroProfileWSPrintEnd();
}

void MicroProfileLoadPresets(const char* pSettingsName, bool bReadOnlyPreset)
{
	std::lock_guard<std::recursive_mutex> Lock(MicroProfileGetMutex());
	const char* pPresetFile = bReadOnlyPreset ? MICROPROFILE_SETTINGS_FILE_BUILTIN : MICROPROFILE_SETTINGS_FILE;
	MicroProfileParseSettings(pPresetFile, [=](const char* pName, uint32_t l0, const char* pJson, uint32_t l1) {
		if(0 == MP_STRCASECMP(pName, pSettingsName))
		{
			uint32_t nLen = (uint32_t)strlen(pJson) + 1;
			if(nLen > S.nJsonSettingsBufferSize)
			{
				S.pJsonSettings = (char*)MP_REALLOC(S.pJsonSettings, nLen);
				S.nJsonSettingsBufferSize = nLen;
			}
			memcpy(S.pJsonSettings, pJson, nLen);
			S.nJsonSettingsPending = 1;
			S.bJsonSettingsReadOnly = bReadOnlyPreset ? 1 : 0;
			return false;
		}
		return true;
	});
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
	static uint64_t BytesAllocated = 0;
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
		if((int)nSizeBytes != r)
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

	MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(g_MicroProfileBytesPerFlip, nSize);
	if(nSize + 1 > BytesAllocated)
	{
		Bytes = (unsigned char*)MP_REALLOC(Bytes, nSize + 1);
		BytesAllocated = nSize + 1;
	}
	recv(Connection, (char*)Bytes, nSize, 0);
	for(uint32_t i = 0; i < nSize; ++i)
		Bytes[i] ^= Mask[i & 3];

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
			MicroProfileSavePresets((const char*)Bytes + 1, (const char*)pJson + 1);
		}
		break;
	}

	case 'l':
	{
		MicroProfileLoadPresets((const char*)Bytes + 1, 0);
		break;
	}
	case 'm':
	{
		MicroProfileLoadPresets((const char*)Bytes + 1, 1);
		break;
	}
	case 'd':
	{
		MicroProfileWebSocketClearTimers();
		memset(&S.nActiveGroupsWanted, 0, sizeof(S.nActiveGroupsWanted));
		S.nWebSocketDirty |= MICROPROFILE_WEBSOCKET_DIRTY_ENABLED;
		break;
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
		S.nWSViewMode = (int)Bytes[1] - '0';
		break;
	case 'r':
		uprintf("got clear message\n");
		S.nAggregateClear = 1;
		break;
	case 'x':
		MicroProfileWebSocketClearTimers();
		break;
#if MICROPROFILE_DYNAMIC_INSTRUMENT
	case 'D': // instrumentation without loading queryable symbols.
	{
		uprintf("got INSTRUMENT Message: %s\n", (const char*)&Bytes[0]);
		char* pGet = (char*)&Bytes[1];
		uint32_t nNumArguments = 0;
#ifdef _WIN32
		int r = sscanf_s(pGet, "%d", &nNumArguments);
#else
		int r = sscanf(pGet, "%d", &nNumArguments);
#endif
		if(r != 1)
		{
			uprintf("failed to parse..\n");
			break;
		}
		while(' ' == *pGet || (*pGet >= '0' && *pGet <= '9'))
		{
			pGet++;
		}
		if(nNumArguments > 200)
			nNumArguments = 200;
		uint32_t nParsedArguments = 0;
		const char* pModule = 0;
		const char* pSymbol = 0;
		const char** pModules = (const char**)(alloca(sizeof(const char*) * nNumArguments));
		const char** pSymbols = (const char**)(alloca(sizeof(const char*) * nNumArguments));
		auto Next = [&pGet]() -> const char* {
			if(!pGet)
				return 0;
			const char* pRet = pGet;
			pGet = (char*)strchr(pRet, '!');
			if(!pGet)
			{
				return 0;
			}
			*pGet++ = '\0';
			return (const char*)pRet;
		};
		do
		{
			pModule = Next();
			pSymbol = Next();
			if(pModule && pSymbol)
			{
				pModules[nParsedArguments] = pModule;
				pSymbols[nParsedArguments] = pSymbol;
				uprintf("found symbol %s   ::: %s \n", pModule, pSymbol);
				nParsedArguments++;
				if(nParsedArguments == nNumArguments)
				{
					break;
				}
			}
		} while(pGet);

		MicroProfileInstrumentWithoutSymbols(pModules, pSymbols, nParsedArguments);

		break;
	}
	case 'I':
	case 'i':
	{
		uprintf("got Message: %s\n", (const char*)&Bytes[0]);
		void* p = 0;
		uint32_t nColor = 0x0;
#ifdef _WIN32
		int r = sscanf_s((const char*)&Bytes[1], "%p %x", &p, &nColor);
#else
		int r = sscanf((const char*)&Bytes[1], "%p %x", &p, &nColor);
#endif
		if(r == 2)
		{
			uprintf("success!\n");
			const char* pModule = (const char*)&Bytes[1];
			int nNumChars = stbsp_snprintf(0, 0, "%p %x", p, nColor);
			pModule += nNumChars;
			while(*pModule != ' ' && *pModule != '\0')
				++pModule;

			if(*pModule == '\0')
				break;

			pModule++;
			const char* pName = pModule;
			while(*pName != '!' && *pName != '\0')
			{
				pName++;
			}
			if(*pName == '!')
			{
				// name and module seperately
				*(char*)pName = '\0';
				pName++;
			}
			else
			{
				// name only
				pName = pModule;
				pModule = "";
			}

			uprintf("scanning for ptr %p %x mod:'%s' name'%s'\n", p, nColor, pModule, pName);
			if(Bytes[0] == 'I')
			{
				MicroProfileInstrumentFunctionsCalled(p, pModule, pName);
			}
			else
			{
				MicroProfileInstrumentFunction(p, pModule, pName, nColor);
			}
		}
	}
	break;
	case 'S':
		uprintf("loading symbols...\n");
		MicroProfileSymbolInitialize(true);
		break;
	case 'q':
		MicroProfileSymbolQueryFunctions(Connection, 1 + (const char*)Bytes);
		break;
	case 'L':
		uprintf("LOAD MODULE: '%s'\n", 1 + (const char*)Bytes);
		MicroProfileSymbolInitialize(true, 1 + (const char*)Bytes);
		break;
#else
	case 'D':
	case 'I':
	case 'i':
	case 'S':
	case 'q':
	case 'L':
		break;
#endif
	default:
		uprintf("got unknown message size %lld: '%s'\n", (long long)nSize, Bytes);
	}
	return true;

fail:
	return false;
}
void MicroProfileWebSocketSendPresets(MpSocket Connection);

void MicroProfileWebSocketHandshake(MpSocket Connection, char* pWebSocketKey)
{
	// reset web socket buffer
	S.WSBuf.nSendPut.store(0);
	S.WSBuf.nSendGet.store(0);
	S.nSocketFail = 0;

	const char* pGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char* pHandShake = "HTTP/1.1 101 Switching Protocols\r\n"
							 "Upgrade: websocket\r\n"
							 "Connection: Upgrade\r\n"
							 "Sec-WebSocket-Accept: ";

	char EncodeBuffer[512];
	int nLen = stbsp_snprintf(EncodeBuffer, sizeof(EncodeBuffer) - 1, "%s%s", pWebSocketKey, pGUID);
	// uprintf("encode buffer is '%s' %d, %d\n", EncodeBuffer, nLen, (int)strlen(EncodeBuffer));

	uint8_t sha[20];
	MicroProfile_SHA1_CTX ctx;
	MicroProfile_SHA1_Init(&ctx);
	MicroProfile_SHA1_Update(&ctx, (unsigned char*)EncodeBuffer, nLen);
	MicroProfile_SHA1_Final((unsigned char*)&sha[0], &ctx);
	char HashOut[(2 + sizeof(sha) / 3) * 4];
	memset(&HashOut[0], 0, sizeof(HashOut));
	MicroProfileBase64Encode(&HashOut[0], &sha[0], sizeof(sha));

	char Reply[11024];
	nLen = stbsp_snprintf(Reply, sizeof(Reply) - 1, "%s%s\r\n\r\n", pHandShake, HashOut);
	;
	MP_ASSERT(nLen >= 0);
	MicroProfileSocketSend(Connection, Reply, nLen);
	S.WebSockets[S.nNumWebSockets++] = Connection;

	S.WSCategoriesSent = 0;
	S.WSGroupsSent = 0;
	S.WSTimersSent = 0;
	S.WSCountersSent = 0;
	S.nJsonSettingsPending = 0;
#if MICROPROFILE_DYNAMIC_INSTRUMENT
	S.WSFunctionsInstrumentedSent = 0;
	S.WSSymbolModulesSent = 0;
	{
		uint64_t t0 = MP_TICK();
		MicroProfileSymbolUpdateModuleList();
		uint64_t t1 = MP_TICK();
		float fTime = float(MicroProfileTickToMsMultiplierCpu()) * (t1 - t0);
		(void)fTime;
		uprintf("update module list time %6.2fms\n", fTime);
	}
#endif

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
			MicroProfileWSPrintStart(Connection);
			MicroProfileWSPrintf("{\"k\":\"%d\",\"ro\":%d,\"v\":%s}", MSG_CURRENTSETTINGS, S.bJsonSettingsReadOnly ? 1 : 0, S.pJsonSettings);
			MicroProfileWSFlush();
			MicroProfileWSPrintEnd();
		}
	}
}

void MicroProfileWebSocketSendCounters()
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileWebSocketSendCounters", MP_GREEN4);
	if(S.nWSViewMode == VIEW_COUNTERS)
	{
		MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":[", MSG_COUNTERS);
		for(uint32_t i = 0; i < S.nNumCounters; ++i)
		{
			uint64_t nCounter = S.Counters[i].load();
			MicroProfileWSPrintf("%c%lld", i == 0 ? ' ' : ',', nCounter);
		}
		MicroProfileWSPrintf("]}");
		MicroProfileWSFlush();
	}
}

#if MICROPROFILE_DYNAMIC_INSTRUMENT
void MicroProfileSymbolSendModuleState()
{
	if(S.WSSymbolModulesSent != S.SymbolNumModules || S.nSymbolsDirty.load()) // todo: tag when modulestate is updated.
	{
		S.nSymbolsDirty.exchange(0);
		MicroProfileWSPrintf(",\"M\":[");
		bool bFirst = true;
		for(int i = 0; i < S.SymbolNumModules; ++i)
		{
			MicroProfileSymbolModule& M = S.SymbolModules[i];
			const char* pModuleName = (const char*)M.pBaseString;
			uint64_t nAddrBegin = M.Regions[0].nBegin;
			// intptr_t nProgress = M.nProgress;
			intptr_t nProgressTarget = M.nProgressTarget;
			nProgressTarget = MicroProfileMax(intptr_t(1), M.nProgressTarget);
			// nProgress = MicroProfileMin(nProgressTarget, M.nProgress);
			float fLoadPrc = M.nProgress / float(nProgressTarget);
			uint64_t nNumSymbols = M.nSymbolsLoaded;
#define FMT "{\"n\":\"%s\",\"a\":\"%llx\",\"s\":\"%lld\", \"p\":%f}"
			MicroProfileWSPrintf(bFirst ? FMT : ("," FMT), pModuleName, nAddrBegin, nNumSymbols, fLoadPrc);
#undef FMT
			bFirst = false;
		}
		MicroProfileWSPrintf("]");
		S.WSSymbolModulesSent = S.SymbolNumModules;
	}
}
#endif

void MicroProfileWebSocketSendFrame(MpSocket Connection)
{
	if(S.nFrameCurrent != S.WebSocketFrameLast[0] || S.nFrozen)
	{
		MicroProfileWebSocketSendState(Connection);
		MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileWebSocketSendFrame", MP_GREEN4);
		MicroProfileWSPrintStart(Connection);
		float fTickToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		float fTickToMsGpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondGpu());
		MicroProfileFrameState* pFrameCurrent = &S.Frames[S.nFrameCurrent];
		MicroProfileFrameState* pFrameNext = &S.Frames[S.nFrameNext];

		uint64_t nFrameTicks = pFrameNext->nFrameStartCpu - pFrameCurrent->nFrameStartCpu;
		uint64_t nFrame = pFrameCurrent->nFrameId;
		double fTime = nFrameTicks * fTickToMsCpu;
		MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"t\":%f,\"f\":%lld,\"a\":%d,\"fr\":%d,\"m\":%d", MSG_FRAME, fTime, nFrame, MicroProfileGetCurrentAggregateFrames(), S.nFrozen, S.nWSViewMode);
#if MICROPROFILE_DYNAMIC_INSTRUMENT
		MicroProfileWSPrintf(",\"s\":{\"n\":%d,\"f\":%d,\"r\":%d,\"l\":%d,\"q\":%d}",
							 S.SymbolNumModules,
							 S.SymbolState.nModuleLoadsFinished.load(),
							 S.SymbolState.nModuleLoadsRequested.load(),
							 S.SymbolState.nSymbolsLoaded.load(),
							 S.pPendingQuery ? 1 : 0);
		MicroProfileSymbolSendModuleState();
#endif

		auto WriteTickArray = [fTickToMsCpu, fTickToMsGpu](MicroProfile::GroupTime* pFrameGroup) {
			MicroProfileWSPrintf("[");
			int f = 0;
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
			{
				uint64_t nTicksExcl = pFrameGroup[i].nTicksExclusive;
				if(nTicksExcl)
				{
					uint64_t nTicks = pFrameGroup[i].nTicks;
					float fCount = (float)pFrameGroup[i].nCount;
					float fToMs = S.GroupInfo[i].Type == MicroProfileTokenTypeCpu ? fTickToMsCpu : fTickToMsGpu;

					MicroProfileWSPrintf("%c[%f,%f,%f]", f ? ',' : ' ', nTicks * fToMs, nTicksExcl * fToMs, fCount);
					f = 1;
				}
			}
			MicroProfileWSPrintf("]");
		};
		auto WriteIndexArray = [](MicroProfile::GroupTime* pFrameGroup) {
			MicroProfileWSPrintf("[");
			int f = 0;
			for(uint32_t i = 0; i < MICROPROFILE_MAX_GROUPS; ++i)
			{
				uint64_t nTicksExcl = pFrameGroup[i].nTicksExclusive;
				if(nTicksExcl)
				{
					uint32_t id = MicroProfileWebSocketIdPack(TYPE_GROUP, i);
					MicroProfileWSPrintf("%c%d", f ? ',' : ' ', id);
					f = 1;
				}
			}
			MicroProfileWSPrintf("]");
		};

		MicroProfileWSPrintf(",\"g\":");
		WriteTickArray(S.FrameGroup);
		MicroProfileWSPrintf(",\"gi\":");
		WriteIndexArray(S.FrameGroup);
		if(S.nWSViewMode == VIEW_GRAPH_THREAD_GROUP)
		{
			MicroProfileWSPrintf(",\"gt\":[");
			int f = 0;
			for(uint32_t i = 0; i < MICROPROFILE_MAX_THREADS; ++i)
			{
				if(0 != (S.FrameGroupThreadValid[i / 32] & (1 << (i % 32))))
				{
					if(!f)
						MicroProfileWSPrintf("{");
					else
						MicroProfileWSPrintf(",{");
					MicroProfileThreadLog* pLog = S.Pool[i];
					MicroProfileWSPrintf("\"i\":%d,\"n\":\"%s\",\"g\":", i, pLog->ThreadName);
					WriteTickArray(&S.FrameGroupThread[i][0]);
					MicroProfileWSPrintf(",\"gi\":");
					WriteIndexArray(&S.FrameGroupThread[i][0]);
					MicroProfileWSPrintf("}");
					f = 1;
				}
			}
			MicroProfileWSPrintf("]");
		}

		if(S.nFrameCurrent != S.WebSocketFrameLast[0])
		{
			MicroProfileWSPrintf(",\"x\":{");
			int nTimer = S.WebSocketTimers;
			// uprintf("T : ");
			while(-1 != nTimer)
			{
				MicroProfileTimerInfo& TI = S.TimerInfo[nTimer];
				float fTickToMs = TI.Type == MicroProfileTokenTypeGpu ? fTickToMsGpu : fTickToMsCpu;
				uint32_t id = MicroProfileWebSocketIdPack(TYPE_TIMER, nTimer);
				fTime = fTickToMs * S.Frame[nTimer].nTicks;
				float fCount = (float)S.Frame[nTimer].nCount;
				float fTimeExcl = fTickToMs * S.FrameExclusive[nTimer];
				// uprintf("%4.2f, ", fTimeExcl);
				if(!MicroProfileGroupActive(TI.nGroupIndex))
				{
					fTime = fCount = fTimeExcl = 0.f;
				}
				nTimer = TI.nWSNext;
				MicroProfileWSPrintf("\"%d\":[%f,%f,%f]%c", id, fTime, fTimeExcl, fCount, nTimer == -1 ? ' ' : ',');
			}
			// uprintf("\n");
			MicroProfileWSPrintf("}");
		}
		MicroProfileWSPrintf("}}");
		MicroProfileWSFlush();
		MicroProfileWebSocketSendCounters();
		MicroProfileWSPrintEnd();
		S.WebSocketFrameLast[0] = S.nFrameCurrent;
	}
	else
	{
		MicroProfileWSPrintStart(Connection);
		MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"fr\":%d,\"m\":%d", MSG_INACTIVE_FRAME, S.nFrozen, S.nWSViewMode);
#if MICROPROFILE_DYNAMIC_INSTRUMENT
		MicroProfileWSPrintf(",\"s\":{\"n\":%d,\"f\":%d,\"r\":%d,\"l\":%d,\"q\":%d}",
							 S.SymbolNumModules,
							 S.SymbolState.nModuleLoadsFinished.load(),
							 S.SymbolState.nModuleLoadsRequested.load(),
							 S.SymbolState.nSymbolsLoaded.load(),
							 S.pPendingQuery ? 1 : 0);
#endif
		MicroProfileWSPrintf("}}");
		MicroProfileWSFlush();
		MicroProfileWebSocketSendCounters();
		MicroProfileWSPrintEnd();
	}
#if MICROPROFILE_DYNAMIC_INSTRUMENT
	MicroProfileSymbolQuerySendResult(Connection);
	MicroProfileSymbolSendFunctionNames(Connection);
	MicroProfileSymbolSendErrors(Connection);
#endif
}

void MicroProfileWebSocketFrame()
{
	if(!S.nNumWebSockets)
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
		LastSocket = MicroProfileMax(LastSocket, S.WebSockets[i] + 1);
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
				MicroProfileWSPrintStart(s);
				MicroProfileWSPrintf("{\"k\":\"%d\",\"ro\":%d,\"v\":%s}", MSG_LOADSETTINGS, S.bJsonSettingsReadOnly ? 1 : 0, S.pJsonSettings);
				MicroProfileWSFlush();
				MicroProfileWSPrintEnd();
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
			uprintf("removing socket %" PRId64 "\n", (uint64_t)s);

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
			uprintf("done removing\n");
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

void MicroProfileWSPrintStart(MpSocket C)
{
	MP_ASSERT(S.WSBuf.Socket == 0);
	MP_ASSERT(S.WSBuf.nPut == 0);
	S.WSBuf.Socket = C;
}

void MicroProfileResizeWSBuf(uint32_t nMinSize = 0)
{
	uint32_t nNewSize = MicroProfileMax(S.WSBuf.nPut + 2 * (nMinSize + 2 + 20), MicroProfileMax(S.WSBuf.nBufferSize * 3 / 2, (uint32_t)MICROPROFILE_WEBSOCKET_BUFFER_SIZE));
	S.WSBuf.pBufferAllocation = (char*)MICROPROFILE_REALLOC(S.WSBuf.pBufferAllocation, nNewSize);
	S.WSBuf.pBuffer = S.WSBuf.pBufferAllocation + 20;
	S.WSBuf.nBufferSize = nNewSize - 20;
}

char* MicroProfileWSPrintfCallback(const char* buf, void* user, int len)
{
	MP_ASSERT(S.WSBuf.nPut == buf - S.WSBuf.pBuffer);
	S.WSBuf.nPut += len;
	if(S.WSBuf.nPut + STB_SPRINTF_MIN + 2 >= S.WSBuf.nBufferSize) //
	{
		MicroProfileResizeWSBuf(S.WSBuf.nPut + STB_SPRINTF_MIN);
	}
	return S.WSBuf.pBuffer + S.WSBuf.nPut;
}

void MicroProfileWSPrintf(const char* pFmt, ...)
{
	if(!S.WSBuf.nBufferSize)
	{
		MicroProfileResizeWSBuf(STB_SPRINTF_MIN * 2);
	}
	va_list args;
	va_start(args, pFmt);
	MP_ASSERT(S.WSBuf.nPut + STB_SPRINTF_MIN < S.WSBuf.nBufferSize);
	stbsp_vsprintfcb(MicroProfileWSPrintfCallback, 0, S.WSBuf.pBuffer + S.WSBuf.nPut, pFmt, args);
	va_end(args);
}

void MicroProfileWSPrintEnd()
{
	MP_ASSERT(S.WSBuf.nPut == 0);
	S.WSBuf.Socket = 0;
}

void MicroProfileWSFlush()
{
	MP_ASSERT(S.WSBuf.Socket != 0);
	MP_ASSERT(S.WSBuf.nPut != 0);
	MicroProfileWebSocketSend(S.WSBuf.Socket, &S.WSBuf.pBuffer[0], S.WSBuf.nPut);
	S.WSBuf.nPut = 0;
}
void MicroProfileWebSocketSendEnabledMessage(uint32_t id, int bEnabled)
{
	MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"e\":%d}}", MSG_ENABLED, id, bEnabled ? 1 : 0);
	MicroProfileWSFlush();
}
void MicroProfileWebSocketSendEnabled(MpSocket C)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Websocket-SendEnabled", MP_GREEN4);
	MicroProfileWSPrintStart(C);
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

	MicroProfileWSPrintEnd();
}
void MicroProfileWebSocketSendEntry(uint32_t id, uint32_t parent, const char* pName, int nEnabled, uint32_t nColor)
{
	MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"pid\":%d,", MSG_TIMER_TREE, id, parent);
	MicroProfileWSPrintf("\"name\":\"%s\",", pName);
	MicroProfileWSPrintf("\"e\":%d,", nEnabled);
	if(nColor == 0x42)
	{
		MicroProfileWSPrintf("\"color\":\"\"");
	}
	else
	{
		MicroProfileWSPrintf("\"color\":\"#%02x%02x%02x\"", MICROPROFILE_UNPACK_RED(nColor) & 0xff, MICROPROFILE_UNPACK_GREEN(nColor) & 0xff, MICROPROFILE_UNPACK_BLUE(nColor) & 0xff);
	}
	MicroProfileWSPrintf("}}");
	MicroProfileWSFlush();
}

void MicroProfileWebSocketSendCounterEntry(uint32_t id, uint32_t parent, const char* pName, int64_t nLimit, int nFormat)
{
	MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"id\":%d,\"pid\":%d,", MSG_TIMER_TREE, id, parent);
	MicroProfileWSPrintf("\"name\":\"%s\",", pName);
	MicroProfileWSPrintf("\"limit\":%d,", nLimit);
	MicroProfileWSPrintf("\"format\":%d", nFormat);
	MicroProfileWSPrintf("}}");
	MicroProfileWSFlush();
}

void MicroProfileWebSocketSendState(MpSocket C)
{
	if(S.WSCategoriesSent != S.nCategoryCount || S.WSGroupsSent != S.nGroupCount || S.WSTimersSent != S.nTotalTimers || S.WSCountersSent != S.nNumCounters)
	{
		MicroProfileWSPrintStart(C);
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
			uint32_t parent = CI.nParent == -1 ? 0u : MicroProfileWebSocketIdPack(TYPE_COUNTER, CI.nParent);
			MicroProfileWebSocketSendCounterEntry(id, parent, CI.pName, CI.nLimit, CI.eFormat);
		}
#if MICROPROFILE_CONTEXT_SWITCH_TRACE
		MicroProfileWebSocketSendEntry(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_CONTEXT_SWITCH_TRACE), 0, "Context Switch Trace", S.bContextSwitchRunning, (uint32_t)-1);
#endif
#if MICROPROFILE_PLATFORM_MARKERS
		MicroProfileWebSocketSendEntry(MicroProfileWebSocketIdPack(TYPE_SETTING, SETTING_PLATFORM_MARKERS), 0, "Platform Markers", S.bContextSwitchRunning, (uint32_t)-1);
#endif
		MicroProfileWSPrintEnd();

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
		int nReceived = recv(Connection, Req, sizeof(Req) - 1, 0);
		if(nReceived > 0)
		{
			Req[nReceived] = '\0';
			//uprintf("req received\n%s", Req);
#if MICROPROFILE_MINIZ
			// Expires: Tue, 01 Jan 2199 16:00:00 GMT\r\n
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Encoding: deflate\r\n\r\n"
#else
#define MICROPROFILE_HTML_HEADER "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"
#endif
			char* pHttp = strstr(Req, "HTTP/");
			char* pGet = strstr(Req, "GET /");
			char* pHost = strstr(Req, "Host: ");
			char* pWebSocketKey = strstr(Req, "Sec-WebSocket-Key: ");
			auto Terminate = [](char* pString) {
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
				pWebSocketKey += sizeof("Sec-WebSocket-Key: ") - 1;
				Terminate(pWebSocketKey);
				MicroProfileWebSocketHandshake(Connection, pWebSocketKey);
				return false;
			}

			if(pHost)
			{
				pHost += sizeof("Host: ") - 1;
				Terminate(pHost);
			}

			if(pHttp && pGet)
			{
				*pHttp = '\0';
				pGet += sizeof("GET /") - 1;
				Terminate(pGet);
				MicroProfileParseGetResult R;
				auto P = MicroProfileParseGet(pGet, &R);
				switch(P)
				{
				case EMICROPROFILE_GET_COMMAND_LIVE:
				{
					MicroProfileSetNonBlocking(Connection, 0);
					uint64_t nTickStart = MP_TICK();
					send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER) - 1, 0);
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
					int nKb = ((nDataEnd - nDataStart) >> 10) + 1;
					int nCompressedKb = ((CompressState.nCompressedSize) >> 10) + 1;
					MicroProfilePrintf(MicroProfileCompressedWriteSocket, &CompressState, "\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
					MicroProfileCompressedSocketFinish(&CompressState);
					MicroProfileFlushSocket(Connection);
#endif

					uprintf("\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
					(void)nCompressedKb;
				}
				break;
				case EMICROPROFILE_GET_COMMAND_DUMP_RANGE:
				case EMICROPROFILE_GET_COMMAND_DUMP:
				{
					{
						MicroProfileSetNonBlocking(Connection, 0);
						uint64_t nTickStart = MP_TICK();
						send(Connection, MICROPROFILE_HTML_HEADER, sizeof(MICROPROFILE_HTML_HEADER) - 1, 0);
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
						int nKb = ((nDataEnd - nDataStart) >> 10) + 1;
						int nCompressedKb = ((CompressState.nCompressedSize) >> 10) + 1;
						MicroProfilePrintf(MicroProfileCompressedWriteSocket, &CompressState, "\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
						MicroProfileCompressedSocketFinish(&CompressState);
						MicroProfileFlushSocket(Connection);
#endif

						uprintf("\n<!-- Sent %dkb(compressed %dkb) in %.2fms-->\n\n", nKb, nCompressedKb, fMs);
						(void)nCompressedKb;
					}
				}
				break;
				case EMICROPROFILE_GET_COMMAND_UNKNOWN:
				{
					uprintf("unknown get command %s\n", pGet);
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
// functions that need to be implemented per platform.
void* MicroProfileTraceThread(void* unused);
int MicroProfileIsLocalThread(uint32_t nThreadId);

void MicroProfileStartContextSwitchTrace()
{
	if(!S.bContextSwitchRunning && !S.nMicroProfileShutdown)
	{
		S.bContextSwitchRunning = true;
		S.bContextSwitchStop = false;
		MicroProfileThreadStart(&S.ContextSwitchThread, MicroProfileTraceThread);
	}
}

void MicroProfileJoinContextSwitchTrace()
{
	if(S.bContextSwitchStop)
	{
		MicroProfileThreadJoin(&S.ContextSwitchThread);
	}
}

void MicroProfileStopContextSwitchTrace()
{
	if(S.bContextSwitchRunning)
	{
		S.bContextSwitchStop = true;
	}
}

#ifdef _WIN32
#define INITGUID
#include <evntcons.h>
#include <evntrace.h>
#include <strsafe.h>

static GUID g_MicroProfileThreadClassGuid = { 0x3d6fa8d1, 0xfe05, 0x11d0, 0x9d, 0xda, 0x00, 0xc0, 0x4f, 0xd7, 0xba, 0x7c };

struct MicroProfileSCSwitch
{
	uint32_t NewThreadId;
	uint32_t OldThreadId;
	int8_t NewThreadPriority;
	int8_t OldThreadPriority;
	uint8_t PreviousCState;
	int8_t SpareByte;
	int8_t OldThreadWaitReason;
	int8_t OldThreadWaitMode;
	int8_t OldThreadState;
	int8_t OldThreadWaitIdealProcessor;
	uint32_t NewThreadWaitTime;
	uint32_t Reserved;
};

VOID WINAPI MicroProfileContextSwitchCallback(PEVENT_TRACE pEvent)
{
	if(pEvent->Header.Guid == g_MicroProfileThreadClassGuid)
	{
		if(pEvent->Header.Class.Type == 36)
		{
			MicroProfileSCSwitch* pCSwitch = (MicroProfileSCSwitch*)pEvent->MofData;
			if((pCSwitch->NewThreadId != 0) || (pCSwitch->OldThreadId != 0))
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

ULONG WINAPI MicroProfileBufferCallback(PEVENT_TRACE_LOGFILEA Buffer)
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
	sessionProperties.Wnode.ClientContext = 1; // QPC clock resolution
	sessionProperties.Wnode.Guid = SystemTraceControlGuid;
	sessionProperties.BufferSize = 1;
	sessionProperties.NumberOfBuffers = 128;
	sessionProperties.EnableFlags = EVENT_TRACE_FLAG_CSWITCH;
	sessionProperties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	sessionProperties.MaximumFileSize = 0;
	sessionProperties.LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	sessionProperties.LogFileNameOffset = 0;

	EVENT_TRACE_LOGFILEA log;
	ZeroMemory(&log, sizeof(log));
	log.LoggerName = (LPSTR)KERNEL_LOGGER_NAMEA;
	log.ProcessTraceMode = 0;
	TRACEHANDLE hLog = OpenTraceA(&log);
	if(hLog)
	{
		ControlTrace(SessionHandle, KERNEL_LOGGER_NAME, &sessionProperties, EVENT_TRACE_CONTROL_STOP);
	}
	CloseTrace(hLog);
}

typedef VOID (WINAPI *EventCallback)(PEVENT_TRACE);
typedef ULONG (WINAPI *BufferCallback)(PEVENT_TRACE_LOGFILEA);
bool MicroProfileStartWin32Trace(EventCallback EvtCb, BufferCallback BufferCB)
{
	MicroProfileContextSwitchShutdownTrace();
	ULONG status = ERROR_SUCCESS;
	TRACEHANDLE SessionHandle = 0;
	MicroProfileKernelTraceProperties sessionProperties;

	ZeroMemory(&sessionProperties, sizeof(sessionProperties));
	sessionProperties.Wnode.BufferSize = sizeof(sessionProperties);
	sessionProperties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	sessionProperties.Wnode.ClientContext = 1; // QPC clock resolution
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

	if(ERROR_SUCCESS != status)
	{
		return false;
	}

	EVENT_TRACE_LOGFILEA log;
	ZeroMemory(&log, sizeof(log));

	log.LoggerName = (LPSTR)KERNEL_LOGGER_NAME;
	log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_RAW_TIMESTAMP;
	log.EventCallback = EvtCb;
	log.BufferCallback = BufferCB;

	TRACEHANDLE hLog = OpenTraceA(&log);
	ProcessTrace(&hLog, 1, 0, 0);
	CloseTrace(hLog);
	MicroProfileContextSwitchShutdownTrace();
	return true;
}

#include <psapi.h>
#include <tlhelp32.h>
#include <winternl.h>
#define ThreadQuerySetWin32StartAddress 9
typedef LONG NTSTATUS;
typedef NTSTATUS(WINAPI* pNtQIT)(HANDLE, LONG, PVOID, ULONG, PULONG);
#define STATUS_SUCCESS ((NTSTATUS)0x000 00000L)
#define ThreadQuerySetWin32StartAddress 9
#undef Process32First
#undef Process32Next
#undef PROCESSENTRY32
#undef Module32First
#undef Module32Next
#undef MODULEENTRY32

struct MicroProfileWin32ContextSwitchShared
{
	std::atomic<int64_t> nPut;
	std::atomic<int64_t> nGet;
	std::atomic<int64_t> nQuit;
	std::atomic<int64_t> nTickTrace;
	std::atomic<int64_t> nTickProgram;
	enum
	{
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
	enum
	{
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
	for(uint32_t i = 0; i < MAX_SEARCH; ++i)
	{
		uint32_t idx = (i + nHash) % MicroProfileWin32ThreadInfo::MAX_STRINGS;
		if(0 == g_ThreadInfo.pStrings[idx])
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
	if(Module32First(hModuleSnapshot, &me))
	{
		do
		{
			if(g_ThreadInfo.nNumModules < MicroProfileWin32ThreadInfo::MAX_MODULES)
			{
				auto& M = g_ThreadInfo.M[g_ThreadInfo.nNumModules++];
				P.nNumModules++;
				intptr_t nBase = (intptr_t)me.modBaseAddr;
				intptr_t nEnd = nBase + me.modBaseSize;
				M.nBase = nBase;
				M.nEnd = nEnd;
				M.pName = MicroProfileWin32ThreadInfoAddString(&me.szModule[0]);
			}
		} while(Module32Next(hModuleSnapshot, &me));
	}
	if(hModuleSnapshot)
		CloseHandle(hModuleSnapshot);
}
void MicroProfileWin32InitThreadInfo2()
{
	memset(&g_ThreadInfo, 0, sizeof(g_ThreadInfo));
#if MICROPROFILE_DEBUG
	float fToMsCpu = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
#endif

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	PROCESSENTRY32 pe32;
	THREADENTRY32 te32;
	te32.dwSize = sizeof(THREADENTRY32);
	pe32.dwSize = sizeof(PROCESSENTRY32);
	{
#if MICROPROFILE_DEBUG
		int64_t nTickStart = MP_TICK();
#endif
		if(Process32First(hSnap, &pe32))
		{
			do
			{

				MicroProfileWin32ThreadInfo::Process P;
				P.pid = pe32.th32ProcessID;
				P.pProcessModule = MicroProfileWin32ThreadInfoAddString(pe32.szExeFile);
				g_ThreadInfo.P[g_ThreadInfo.nNumProcesses++] = P;
			} while(Process32Next(hSnap, &pe32) && g_ThreadInfo.nNumProcesses < MicroProfileWin32ThreadInfo::MAX_PROCESSES);
		}
#if MICROPROFILE_DEBUG
		int64_t nTicksEnd = MP_TICK();
		float fMs = fToMsCpu * (nTicksEnd - nTickStart);
		uprintf("Process iteration %6.2fms processes %d\n", fMs, g_ThreadInfo.nNumProcesses);
#endif
	}
	{
#if MICROPROFILE_DEBUG
		int64_t nTickStart = MP_TICK();
#endif
		for(uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
		{
			g_ThreadInfo.P[i].nModuleStart = g_ThreadInfo.nNumModules;
			g_ThreadInfo.P[i].nNumModules = 0;
			MicroProfileWin32ExtractModules(g_ThreadInfo.P[i]);
		}
#if MICROPROFILE_DEBUG
		int64_t nTicksEnd = MP_TICK();
		float fMs = fToMsCpu * (nTicksEnd - nTickStart);
		uprintf("Module iteration %6.2fms NumModules %d\n", fMs, g_ThreadInfo.nNumModules);
#endif
	}

	pNtQIT NtQueryInformationThread = (pNtQIT)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread");
	intptr_t dwStartAddress;
	ULONG olen;
	uint32_t nThreadsTested = 0;
	uint32_t nThreadsSucceeded = 0;

	if(Thread32First(hSnap, &te32))
	{
#if MICROPROFILE_DEBUG
		int64_t nTickStart = MP_TICK();
#endif
		do
		{
			nThreadsTested++;
			const char* pModule = "?";
			HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
			if(hThread)
			{

				NTSTATUS ntStatus = NtQueryInformationThread(hThread, (THREADINFOCLASS)ThreadQuerySetWin32StartAddress, &dwStartAddress, sizeof(dwStartAddress), &olen);
				if(0 == ntStatus)
				{
					uint32_t nProcessIndex = (uint32_t)-1;
					for(uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
					{
						if(g_ThreadInfo.P[i].pid == te32.th32OwnerProcessID)
						{
							nProcessIndex = i;
							break;
						}
					}
					if(nProcessIndex != (uint32_t)-1)
					{
						uint32_t nModuleStart = g_ThreadInfo.P[nProcessIndex].nModuleStart;
						uint32_t nNumModules = g_ThreadInfo.P[nProcessIndex].nNumModules;
						for(uint32_t i = 0; i < nNumModules; ++i)
						{
							auto& M = g_ThreadInfo.M[nModuleStart + i];
							if(M.nBase <= dwStartAddress && M.nEnd >= dwStartAddress)
							{
								pModule = M.pName;
							}
						}
					}
				}
			}
			if(hThread)
				CloseHandle(hThread);
			{
				MicroProfileThreadInfo T;
				T.pid = te32.th32OwnerProcessID;
				T.tid = te32.th32ThreadID;
				const char* pProcess = "unknown";
				for(uint32_t i = 0; i < g_ThreadInfo.nNumProcesses; ++i)
				{
					if(g_ThreadInfo.P[i].pid == T.pid)
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

		} while(Thread32Next(hSnap, &te32) && g_ThreadInfo.nNumThreads < MicroProfileWin32ThreadInfo::MAX_THREADS);

#if MICROPROFILE_DEBUG
		int64_t nTickEnd = MP_TICK();
		float fMs = fToMsCpu * (nTickEnd - nTickStart);
		uprintf("Thread iteration %6.2fms Threads %d\n", fMs, g_ThreadInfo.nNumThreads);
#endif
	}
}

void MicroProfileWin32UpdateThreadInfo()
{
	static int nWasRunning = 1;
	static int nOnce = 0;
	int nRunning = MicroProfileAnyGroupActive() ? 1 : 0;

	if((0 == nRunning && 1 == nWasRunning) || nOnce == 0)
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
	for(uint32_t i = 0; i < g_ThreadInfo.nNumThreads; ++i)
	{
		if(g_ThreadInfo.T[i].tid == nThreadId)
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
#define MICROPROFILE_WIN32_CSWITCH_TIMEOUT 15 // seconds to wait before collector exits
static MicroProfileWin32ContextSwitchShared* g_pShared = 0;
VOID WINAPI MicroProfileContextSwitchCallbackCollector(PEVENT_TRACE pEvent)
{
	static int64_t nPackets = 0;
	static int64_t nSkips = 0;
	if(pEvent->Header.Guid == g_MicroProfileThreadClassGuid)
	{
		if(pEvent->Header.Class.Type == 36)
		{
			MicroProfileSCSwitch* pCSwitch = (MicroProfileSCSwitch*)pEvent->MofData;
			if((pCSwitch->NewThreadId != 0) || (pCSwitch->OldThreadId != 0))
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
					g_pShared->Buffer[nPut % MicroProfileWin32ContextSwitchShared::BUFFER_SIZE] = Switch;
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
	if(0 == (nPackets % (4 << 10)))
	{
		int64_t nTickTrace = MP_TICK();
		g_pShared->nTickTrace.store(nTickTrace);
		int64_t nTickProgram = g_pShared->nTickProgram.load();
		float fTickToMs = MicroProfileTickToMsMultiplier(MicroProfileTicksPerSecondCpu());
		float fTime = fabs(fTickToMs * (nTickTrace - nTickProgram));
		printf("\rRead %" PRId64 " CSwitch Packets, Skips %" PRId64 " Time difference %6.3fms         ", nPackets, nSkips, fTime);
		fflush(stdout);
		if(fTime > MICROPROFILE_WIN32_CSWITCH_TIMEOUT * 1000)
		{
			g_pShared->nQuit.store(1);
		}
	}
}

ULONG WINAPI MicroProfileBufferCallbackCollector(PEVENT_TRACE_LOGFILEA Buffer)
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
	if(hMemory == NULL)
	{
		return 1;
	}
	g_pShared = (MicroProfileWin32ContextSwitchShared*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MicroProfileWin32ContextSwitchShared));

	if(g_pShared != NULL)
	{
		MicroProfileStartWin32Trace(MicroProfileContextSwitchCallbackCollector, MicroProfileBufferCallbackCollector);
		UnmapViewOfFile(g_pShared);
	}

	CloseHandle(hMemory);
	return 0;
}
#endif
#include <shellapi.h>
void* MicroProfileTraceThread(void* unused)
{
	MicroProfileOnThreadCreate("ContextSwitchThread");
	MicroProfileContextSwitchShutdownTrace();
	if(!MicroProfileStartWin32Trace(MicroProfileContextSwitchCallback, MicroProfileBufferCallback))
	{
		MicroProfileContextSwitchShutdownTrace();
		// not running as admin. try and start other process.
		MicroProfileWin32ContextSwitchShared* pShared = 0;
		char Filename[512];
		time_t t = time(NULL);
		_snprintf_s(Filename, sizeof(Filename), "%s_%d", MICROPROFILE_FILEMAPPING, (int)t);

		HANDLE hMemory = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(MicroProfileWin32ContextSwitchShared), Filename);
		if(hMemory != NULL)
		{
			pShared = (MicroProfileWin32ContextSwitchShared*)MapViewOfFile(hMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MicroProfileWin32ContextSwitchShared));
			if(pShared != NULL)
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
						if(nPut == nGet)
						{
							Sleep(20);
						}
						else
						{
							for(int64_t i = nGet; i != nPut; i++)
							{
								MicroProfileContextSwitchPut(&pShared->Buffer[i % MicroProfileWin32ContextSwitchShared::BUFFER_SIZE]);
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
	MicroProfileOnThreadExit();
	return 0;
}

MicroProfileThreadInfo MicroProfileGetThreadInfo(MicroProfileThreadIdType nThreadId)
{
	MicroProfileWin32UpdateThreadInfo();

	for(uint32_t i = 0; i < g_ThreadInfo.nNumThreads; ++i)
	{
		if(g_ThreadInfo.T[i].tid == nThreadId)
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
		uprintf("CONTEXT SWITCH FAILED TO OPEN FILE: make sure to run dtrace script\n");
		S.bContextSwitchRunning = false;
		return 0;
	}
	uprintf("STARTING TRACE THREAD\n");
	char* pLine = 0;
	size_t cap = 0;
	size_t len = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	uint64_t nsSinceEpoch = ((uint64_t)(tv.tv_sec) * 1000000 + (uint64_t)(tv.tv_usec)) * 1000;
	uint64_t nTickEpoch = MP_TICK();
	uint32_t nLastThread[MICROPROFILE_MAX_CONTEXT_SWITCH_THREADS] = { 0 };
	mach_timebase_info_data_t sTimebaseInfo;
	mach_timebase_info(&sTimebaseInfo);
	S.bContextSwitchRunning = true;

	uint64_t nProcessed = 0;
	uint64_t nProcessedLast = 0;
	while((len = getline(&pLine, &cap, pFile)) > 0 && !S.bContextSwitchStop)
	{
		nProcessed += len;
		if(nProcessed - nProcessedLast > 10 << 10)
		{
			nProcessedLast = nProcessed;
			uprintf("processed %llukb %llukb\n", (nProcessed - nProcessedLast) >> 10, nProcessed >> 10);
		}

		char* pX = strchr(pLine, 'X');
		if(pX)
		{
			int cpu = atoi(pX + 1);
			char* pX2 = strchr(pX + 1, 'X');
			char* pX3 = strchr(pX2 + 1, 'X');
			int thread = atoi(pX2 + 1);
			char* lala;
			int64_t timestamp = strtoll(pX3 + 1, &lala, 10);
			MicroProfileContextSwitch Switch;

			// convert to ticks.
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
	uprintf("EXITING TRACE THREAD\n");
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
void MicroProfileStopContextSwitchTrace()
{
}
void MicroProfileJoinContextSwitchTrace()
{
}
void MicroProfileStartContextSwitchTrace()
{
}

#endif

#if MICROPROFILE_GPU_TIMERS_D3D11

uint32_t MicroProfileGpuInsertTimeStamp(void* pContext_)
{
	if(S.pGPU)
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
			} while(!Frame.m_nQueryCount.compare_exchange_weak(nIndex, nIndex + 1));
			nIndex += nStart;
			uint32_t nQueryIndex = nIndex % MICROPROFILE_D3D_MAX_QUERIES;

			ID3D11Query* pQuery = (ID3D11Query*)S.pGPU->m_pQueries[nQueryIndex];
			ID3D11DeviceContext* pContext = (ID3D11DeviceContext*)pContext_;
			pContext->End(pQuery);
			return nQueryIndex;
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
	} while(hr == S_FALSE);
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
	if(S.pGPU)
	{
		return S.pGPU->m_nQueryFrequency;
	}
	return 1;
}

uint32_t MicroProfileGpuFlip(void* pDeviceContext_)
{
	if(!pDeviceContext_)
	{
		return (uint32_t)-1;
	}
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
	S.pGPU = MP_ALLOC_OBJECT(MicroProfileGpuTimerState);
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

	MP_FREE(S.pGPU);
	S.pGPU = 0;
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	if(S.pGPU)
	{
		MicroProfileD3D11Frame& Frame = S.pGPU->m_QueryFrames[S.pGPU->m_nQueryFrame];
		if(Frame.m_nRateQueryStarted)
		{
			ID3D11Query* pSyncQuery = (ID3D11Query*)S.pGPU->pSyncQuery;
			ID3D11DeviceContext* pImmediateContext = (ID3D11DeviceContext*)S.pGPU->m_pImmediateContext;
			pImmediateContext->End(pSyncQuery);

			HRESULT hr;
			do
			{
				hr = pImmediateContext->GetData(pSyncQuery, pOutGpu, sizeof(*pOutGpu), 0);
			} while(hr == S_FALSE);
			*pOutCPU = MP_TICK();
			switch(hr)
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
	}
	return 0;
}

#elif MICROPROFILE_GPU_TIMERS_D3D12
#include <d3d12.h>
uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	uint32_t nNode = S.pGPU->nCurrentNode;
	uint32_t nFrame = S.pGPU->nFrame;
	uint32_t nQueryIndex = (S.pGPU->nFrameCount.fetch_add(1) + S.pGPU->nFrameStart) % MICROPROFILE_D3D_MAX_QUERIES;

	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)pContext;
	pCommandList->EndQuery(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nQueryIndex);
	MP_ASSERT(nQueryIndex <= 0xffff);
	// uprintf("insert timestamp %d :: %d ... ctx %p\n", nQueryIndex, nFrame, pContext);
	return ((nFrame << 16) & 0xffff0000) | (nQueryIndex);
}

void MicroProfileGpuFetchRange(uint32_t nBegin, int32_t nCount, uint64_t nFrame, int64_t nTimestampOffset)
{
	if(nCount <= 0)
		return;
	void* pData = 0;
	// uprintf("fetch [%d-%d]\n", nBegin, nBegin + nCount);
	D3D12_RANGE Range = { sizeof(uint64_t) * nBegin, sizeof(uint64_t) * (nBegin + nCount) };
	S.pGPU->pBuffer->Map(0, &Range, &pData);
	memcpy(&S.pGPU->nResults[nBegin], nBegin + (uint64_t*)pData, nCount * sizeof(uint64_t));
	for(int i = 0; i < nCount; ++i)
	{
		S.pGPU->nQueryFrames[i + nBegin] = nFrame;
		S.pGPU->nResults[i + nBegin] -= nTimestampOffset;
	}
	S.pGPU->pBuffer->Unmap(0, 0);
}
void MicroProfileGpuWaitFence(uint32_t nNode, uint64_t nFence)
{
	uint64_t nCompletedFrame = S.pGPU->NodeState[nNode].pFence->GetCompletedValue();
	// while(nCompletedFrame < nPending)
	// while(0 < nPending - nCompletedFrame)
	while(0 < (int64_t)(nFence - nCompletedFrame))
	{
		MICROPROFILE_SCOPEI("Microprofile", "gpu-wait", MP_GREEN4);
		Sleep(20); // todo: use event.
		nCompletedFrame = S.pGPU->NodeState[nNode].pFence->GetCompletedValue();
	}
}

void MicroProfileGpuFetchResults(uint64_t nFrame)
{
	uint64_t nPending = S.pGPU->nPendingFrame;
	// while(nPending <= nFrame)
	// while(0 <= nFrame - nPending)
	while(0 <= (int64_t)(nFrame - nPending))
	{
		uint32_t nInternal = nPending % MICROPROFILE_D3D_INTERNAL_DELAY;
		uint32_t nNode = S.pGPU->Frames[nInternal].nNode;
		MicroProfileGpuWaitFence(nNode, nPending);
		int64_t nTimestampOffset = 0;
		if(nNode != 0)
		{
			// Adjust timestamp queries from GPU x to be in GPU 0's frame of reference
			HRESULT hr;
			int64_t nCPU0, nGPU0;
			hr = S.pGPU->NodeState[0].pCommandQueue->GetClockCalibration((uint64_t*)&nGPU0, (uint64_t*)&nCPU0);
			MP_ASSERT(hr == S_OK);
			int64_t nCPUx, nGPUx;
			hr = S.pGPU->NodeState[nNode].pCommandQueue->GetClockCalibration((uint64_t*)&nGPUx, (uint64_t*)&nCPUx);
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
	// uprintf("read TS [%d <- %lld]\n", nQueryIndex, S.pGPU->nResults[nQueryIndex]);
	MP_ASSERT((0xffff & lala) == nFrame);
	uint64_t r = S.pGPU->nResults[nQueryIndex];
	if(r == 0x7fffffffffffffff)
	{
		MP_BREAK();
	}
	return r;
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
	if(S.pGPU->nFrameCount)
	{

		nCount = S.pGPU->nFrameCount.exchange(0);
		nStart = S.pGPU->nFrameStart;
		S.pGPU->nFrameStart = (S.pGPU->nFrameStart + nCount) % MICROPROFILE_D3D_MAX_QUERIES;
		uint32_t nEnd = MicroProfileMin(nStart + nCount, (uint32_t)MICROPROFILE_D3D_MAX_QUERIES);
		MP_ASSERT(nStart != nEnd);
		uint32_t nSize = nEnd - nStart;
		pCommandList->ResolveQueryData(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, nStart, nEnd - nStart, S.pGPU->pBuffer, nStart * sizeof(int64_t));
		if(nStart + nCount > MICROPROFILE_D3D_MAX_QUERIES)
		{
			pCommandList->ResolveQueryData(S.pGPU->NodeState[nNode].pHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, nEnd + nStart - MICROPROFILE_D3D_MAX_QUERIES, S.pGPU->pBuffer, 0);
		}
	}
	pCommandList->Close();
	ID3D12CommandList* pList = pCommandList;
	S.pGPU->NodeState[nNode].pCommandQueue->ExecuteCommandLists(1, &pList);
	// uprintf("EXECUTE %p\n", pCommandList);
	S.pGPU->NodeState[nNode].pCommandQueue->Signal(S.pGPU->NodeState[nNode].pFence, S.pGPU->nFrame);
	S.pGPU->Frames[nFrameIndex].nBegin = nStart;
	S.pGPU->Frames[nFrameIndex].nCount = nCount;
	S.pGPU->Frames[nFrameIndex].nNode = nNode;

	S.pGPU->nFrame++;
	// fetch from earlier frames

	MicroProfileGpuFetchResults(S.pGPU->nFrame - MICROPROFILE_GPU_FRAME_DELAY);
	return nFrameTimeStamp;
}
void MicroProfileGpuInitD3D12(void* pDevice_, uint32_t nNodeCount, void** pCommandQueues_)
{
	ID3D12Device* pDevice = (ID3D12Device*)pDevice_;
	S.pGPU = MP_ALLOC_OBJECT(MicroProfileGpuTimerState);
	memset(S.pGPU, 0, sizeof(*S.pGPU));
	S.pGPU->pDevice = pDevice;
	S.pGPU->nNodeCount = nNodeCount;
	MP_ASSERT(S.pGPU->nNodeCount <= MICROPROFILE_D3D_MAX_NODE_COUNT);

	for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
	{
		S.pGPU->NodeState[nNode].pCommandQueue = (ID3D12CommandQueue*)pCommandQueues_[nNode];
		if(nNode == 0)
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
		HRESULT hr = pDevice->CreateQueryHeap(&QHDesc, IID_PPV_ARGS(&S.pGPU->NodeState[nNode].pHeap));
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

	hr = pDevice->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&S.pGPU->pBuffer));
	MP_ASSERT(hr == S_OK);

	S.pGPU->nFrame = 0;
	S.pGPU->nPendingFrame = 0;

	for(MicroProfileD3D12Frame& Frame : S.pGPU->Frames)
	{
		hr = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Frame.pCommandAllocator));
		MP_ASSERT(hr == S_OK);
		for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
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
	for(uint32_t nNode = 0; nNode < S.pGPU->nNodeCount; ++nNode)
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

	MP_FREE(S.pGPU);
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
#elif MICROPROFILE_GPU_TIMERS_VULKAN

#ifndef MICROPROFILE_VULKAN_MAX_QUERIES
#define MICROPROFILE_VULKAN_MAX_QUERIES (32 << 10)
#endif

#define MICROPROFILE_VULKAN_MAX_NODE_COUNT 4
#define MICROPROFILE_VULKAN_INTERNAL_DELAY 8

#include <vulkan/vulkan.h>
struct MicroProfileVulkanFrame
{
	uint32_t nBegin;
	uint32_t nCount;
	uint32_t nNode;
	VkCommandBuffer CommandBuffer[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
	VkFence Fences[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
};
struct MicroProfileGpuTimerState
{
	VkDevice Devices[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
	VkPhysicalDevice PhysicalDevices[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
	VkQueue Queues[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
	VkQueryPool QueryPool[MICROPROFILE_VULKAN_MAX_NODE_COUNT];
	VkCommandPool CommandPool[MICROPROFILE_VULKAN_MAX_NODE_COUNT];

	uint32_t nNodeCount;
	uint32_t nCurrentNode;
	uint64_t nFrame;
	uint64_t nPendingFrame;
	uint32_t nFrameStart;
	std::atomic<uint32_t> nFrameCount;
	int64_t nFrequency;

	uint16_t nQueryFrames[MICROPROFILE_VULKAN_MAX_QUERIES];
	int64_t nResults[MICROPROFILE_VULKAN_MAX_QUERIES];

	MicroProfileVulkanFrame Frames[MICROPROFILE_VULKAN_INTERNAL_DELAY];
};

uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	VkCommandBuffer CB = (VkCommandBuffer)pContext;
	uint32_t nNode = S.pGPU->nCurrentNode;
	uint32_t nFrame = S.pGPU->nFrame;
	uint32_t nQueryIndex = (S.pGPU->nFrameCount.fetch_add(1) + S.pGPU->nFrameStart) % MICROPROFILE_VULKAN_MAX_QUERIES;
	vkCmdWriteTimestamp(CB, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, S.pGPU->QueryPool[nNode], nQueryIndex);
	MP_ASSERT(nQueryIndex <= 0xffff);
	// uprintf("insert timestamp %d :: %d ... ctx %p\n", nQueryIndex, nFrame, pContext);
	return ((nFrame << 16) & 0xffff0000) | (nQueryIndex);
}

void MicroProfileGpuFetchRange(VkCommandBuffer CommandBuffer, uint32_t nNode, uint32_t nBegin, int32_t nCount, uint64_t nFrame, int64_t nTimestampOffset)
{
	if(nCount <= 0)
		return;

	vkGetQueryPoolResults(S.pGPU->Devices[nNode], S.pGPU->QueryPool[nNode], nBegin, nCount, 8 * nCount, &S.pGPU->nResults[nBegin], 8, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT);
	vkCmdResetQueryPool(CommandBuffer, S.pGPU->QueryPool[nNode], nBegin, nCount);
	for(int i = 0; i < nCount; ++i)
	{
		S.pGPU->nQueryFrames[i + nBegin] = nFrame;
	}
}
void MicroProfileGpuWaitFence(uint32_t nNode, uint64_t nFrame)
{
	int r;
	int c = 0;
	do
	{
		MICROPROFILE_SCOPEI("Microprofile", "gpu-wait", MP_GREEN4);
		r = vkWaitForFences(S.pGPU->Devices[nNode], 1, &S.pGPU->Frames[nFrame].Fences[nNode], 1, 1000 * 30);
#if 0
		if(c++ > 1000 && (c%100) == 0)
		{
			uprintf("waiting really long time for fence\n");
			OutputDebugString("waiting really long time for fence\n");
		}
#endif
	} while(r != VK_SUCCESS);
}

void MicroProfileGpuFetchResults(VkCommandBuffer Buffer, uint64_t nFrame)
{
	uint64_t nPending = S.pGPU->nPendingFrame;
	// while(nPending <= nFrame)
	// while(0 <= nFrame - nPending)
	while(0 <= (int64_t)(nFrame - nPending))
	{
		uint32_t nInternal = nPending % MICROPROFILE_VULKAN_INTERNAL_DELAY;
		uint32_t nNode = S.pGPU->Frames[nInternal].nNode;
		MicroProfileGpuWaitFence(nNode, nInternal);
		int64_t nTimestampOffset = 0;

		if(nNode != 0)
		{
			MP_ASSERT(0 && "NOT IMPLEMENTED");
			// note: timestamp adjustment not implemented.
		}

		uint32_t nBegin = S.pGPU->Frames[nInternal].nBegin;
		uint32_t nCount = S.pGPU->Frames[nInternal].nCount;
		MicroProfileGpuFetchRange(Buffer, nNode, nBegin, (nBegin + nCount) > MICROPROFILE_VULKAN_MAX_QUERIES ? MICROPROFILE_VULKAN_MAX_QUERIES - nBegin : nCount, nPending, nTimestampOffset);
		MicroProfileGpuFetchRange(Buffer, nNode, 0, (nBegin + nCount) - MICROPROFILE_VULKAN_MAX_QUERIES, nPending, nTimestampOffset);

		nPending = ++S.pGPU->nPendingFrame;
		MP_ASSERT(S.pGPU->nFrame > nPending);
	}
}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nIndex)
{
	if(nIndex == (uint32_t)-1)
	{
		return 0;
	}
	uint32_t nFrame = nIndex >> 16;
	uint32_t nQueryIndex = nIndex & 0xffff;
	uint32_t lala = S.pGPU->nQueryFrames[nQueryIndex];
	MP_ASSERT((0xffff & lala) == nFrame);
	// uprintf("read TS [%d <- %lld]\n", nQueryIndex, S.pGPU->nResults[nQueryIndex]);
	return S.pGPU->nResults[nQueryIndex];
	return 0;
}

uint64_t MicroProfileTicksPerSecondGpu()
{

	return S.pGPU->nFrequency;
	return 1;
}

uint32_t MicroProfileGpuFlip(void* pContext)
{
	uint32_t nNode = S.pGPU->nCurrentNode;
	uint32_t nFrameIndex = S.pGPU->nFrame % MICROPROFILE_VULKAN_INTERNAL_DELAY;
	uint32_t nCount = 0, nStart = 0;

	VkCommandBuffer CommandBuffer = S.pGPU->Frames[nFrameIndex].CommandBuffer[nNode];
	auto& F = S.pGPU->Frames[nFrameIndex];
	VkFence Fence = F.Fences[nNode];
	VkDevice Device = S.pGPU->Devices[nNode];
	VkQueue Queue = S.pGPU->Queues[nNode];

	vkWaitForFences(Device, 1, &Fence, 1, (uint64_t)-1);
	uint32_t nFrameTimeStamp = MicroProfileGpuInsertTimeStamp(pContext);
	vkResetCommandBuffer(F.CommandBuffer[nNode], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	VkCommandBufferBeginInfo CBI;
	CBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CBI.pNext = 0;
	CBI.pInheritanceInfo = 0;
	CBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(F.CommandBuffer[nNode], &CBI);
	vkResetFences(Device, 1, &Fence);

	nCount = S.pGPU->nFrameCount.exchange(0);
	nStart = S.pGPU->nFrameStart;
	S.pGPU->nFrameStart = (S.pGPU->nFrameStart + nCount) % MICROPROFILE_VULKAN_MAX_QUERIES;
	uint32_t nEnd = MicroProfileMin(nStart + nCount, (uint32_t)MICROPROFILE_VULKAN_MAX_QUERIES);
	MP_ASSERT(nStart != nEnd);
	uint32_t nSize = nEnd - nStart;

	S.pGPU->Frames[nFrameIndex].nBegin = nStart;
	S.pGPU->Frames[nFrameIndex].nCount = nCount;
	S.pGPU->Frames[nFrameIndex].nNode = nNode;
	S.pGPU->nFrame++;
	////fetch from earlier frames
	MicroProfileGpuFetchResults(CommandBuffer, S.pGPU->nFrame - MICROPROFILE_GPU_FRAME_DELAY);

	vkEndCommandBuffer(F.CommandBuffer[nNode]);
	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.pNext = nullptr;
	SubmitInfo.waitSemaphoreCount = 0;
	SubmitInfo.pWaitSemaphores = nullptr;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CommandBuffer;
	SubmitInfo.signalSemaphoreCount = 0;
	SubmitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(Queue, 1, &SubmitInfo, Fence);
	return nFrameTimeStamp;
}

void MicroProfileGpuInitVulkan(VkDevice* pDevices, VkPhysicalDevice* pPhysicalDevices, VkQueue* pQueues, uint32_t* QueueFamily, uint32_t nNodeCount)
{
	S.pGPU = MP_ALLOC_OBJECT(MicroProfileGpuTimerState);
	memset(S.pGPU, 0, sizeof(*S.pGPU));
	S.pGPU->nNodeCount = nNodeCount;
	VkQueryPoolCreateInfo Q;
	Q.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	Q.pNext = 0;
	Q.flags = 0;
	Q.queryType = VK_QUERY_TYPE_TIMESTAMP;
	Q.queryCount = MICROPROFILE_VULKAN_MAX_QUERIES + 1;

	VkCommandPoolCreateInfo CreateInfo;
	CreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	CreateInfo.pNext = 0;
	CreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult r;
	for(uint32_t i = 0; i < nNodeCount; ++i)
	{
		S.pGPU->Devices[i] = pDevices[i];
		S.pGPU->PhysicalDevices[i] = pPhysicalDevices[i];
		S.pGPU->Queues[i] = pQueues[i];
		r = vkCreateQueryPool(S.pGPU->Devices[i], &Q, 0, &S.pGPU->QueryPool[i]);
		MP_ASSERT(r == VK_SUCCESS);

		CreateInfo.queueFamilyIndex = QueueFamily[i];
		r = vkCreateCommandPool(S.pGPU->Devices[i], &CreateInfo, 0, &S.pGPU->CommandPool[i]);
		MP_ASSERT(r == VK_SUCCESS);

		for(uint32_t j = 0; j < MICROPROFILE_VULKAN_INTERNAL_DELAY; ++j)
		{
			auto& F = S.pGPU->Frames[j];
			VkCommandBufferAllocateInfo AllocInfo;
			AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			AllocInfo.pNext = 0;
			AllocInfo.commandBufferCount = 1;
			AllocInfo.commandPool = S.pGPU->CommandPool[i];
			AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			r = vkAllocateCommandBuffers(S.pGPU->Devices[i], &AllocInfo, &F.CommandBuffer[i]);
			MP_ASSERT(r == VK_SUCCESS);

			VkFenceCreateInfo FCI;
			FCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			FCI.pNext = 0;
			FCI.flags = j == 0 ? 0 : VK_FENCE_CREATE_SIGNALED_BIT;
			r = vkCreateFence(S.pGPU->Devices[i], &FCI, 0, &F.Fences[i]);
			MP_ASSERT(r == VK_SUCCESS);
			if(j == 0)
			{
				VkCommandBufferBeginInfo CBI;
				CBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				CBI.pNext = 0;
				CBI.pInheritanceInfo = 0;
				CBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				vkBeginCommandBuffer(F.CommandBuffer[i], &CBI);
				vkCmdResetQueryPool(F.CommandBuffer[i], S.pGPU->QueryPool[i], 0, MICROPROFILE_VULKAN_MAX_QUERIES + 1);

				vkEndCommandBuffer(F.CommandBuffer[i]);
				VkSubmitInfo SubmitInfo = {};
				SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				SubmitInfo.pNext = nullptr;
				SubmitInfo.waitSemaphoreCount = 0;
				SubmitInfo.pWaitSemaphores = nullptr;
				SubmitInfo.commandBufferCount = 1;
				SubmitInfo.pCommandBuffers = &F.CommandBuffer[i];
				SubmitInfo.signalSemaphoreCount = 0;
				SubmitInfo.pSignalSemaphores = nullptr;
				vkQueueSubmit(pQueues[i], 1, &SubmitInfo, F.Fences[i]);
				vkWaitForFences(S.pGPU->Devices[i], 1, &F.Fences[i], 1, (uint64_t)-1);
				vkResetCommandBuffer(F.CommandBuffer[i], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			}
		}
	}

	VkPhysicalDeviceProperties Properties;
	vkGetPhysicalDeviceProperties(pPhysicalDevices[0], &Properties);
	S.pGPU->nFrequency = 1000000000ll / Properties.limits.timestampPeriod;
}

void MicroProfileGpuShutdown()
{
	MP_FREE(S.pGPU);
	S.pGPU = 0;
}
void MicroProfileSetCurrentNodeVulkan(uint32_t nNode)
{
	S.pGPU->nCurrentNode = nNode;
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
	auto& F = S.pGPU->Frames[S.pGPU->nFrame % MICROPROFILE_VULKAN_INTERNAL_DELAY];
	uint32_t nGpu = S.pGPU->nCurrentNode;

	VkCommandBufferBeginInfo CBI;
	CBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	CBI.pNext = 0;
	CBI.pInheritanceInfo = 0;
	CBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkCommandBuffer CB = F.CommandBuffer[nGpu];
	VkDevice Device = S.pGPU->Devices[nGpu];
	VkFence Fence = F.Fences[nGpu];

	vkWaitForFences(Device, 1, &Fence, 1, (uint64_t)-1);
	vkResetFences(Device, 1, &Fence);
	vkResetCommandBuffer(CB, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
	vkBeginCommandBuffer(CB, &CBI);
	vkCmdResetQueryPool(CB, S.pGPU->QueryPool[nGpu], MICROPROFILE_VULKAN_MAX_QUERIES, 1);
	vkCmdWriteTimestamp(CB, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, S.pGPU->QueryPool[nGpu], MICROPROFILE_VULKAN_MAX_QUERIES);
	vkEndCommandBuffer(CB);
	VkSubmitInfo SubmitInfo = {};
	SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	SubmitInfo.pNext = nullptr;
	SubmitInfo.waitSemaphoreCount = 0;
	SubmitInfo.pWaitSemaphores = nullptr;
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &CB;
	SubmitInfo.signalSemaphoreCount = 0;
	SubmitInfo.pSignalSemaphores = nullptr;
	vkQueueSubmit(S.pGPU->Queues[nGpu], 1, &SubmitInfo, Fence);
	vkWaitForFences(Device, 1, &Fence, 1, (uint64_t)-1);
	*pOutGpu = 0;
	vkGetQueryPoolResults(Device, S.pGPU->QueryPool[nGpu], MICROPROFILE_VULKAN_MAX_QUERIES, 1, 8, pOutGpu, 8, VK_QUERY_RESULT_64_BIT);
	*pOutCPU = MP_TICK();
	return 1;
}
#elif MICROPROFILE_GPU_TIMERS_GL
void MicroProfileGpuInitGL()
{
	S.pGPU = MP_ALLOC_OBJECT(MicroProfileGpuTimerState);
	S.pGPU->GLTimerPos = 0;
	glGenQueries(MICROPROFILE_GL_MAX_QUERIES, &S.pGPU->GLTimers[0]);
}

uint32_t MicroProfileGpuInsertTimeStamp(void* pContext)
{
	uint32_t nIndex = (S.pGPU->GLTimerPos + 1) % MICROPROFILE_GL_MAX_QUERIES;
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
#if 0 // debug test if timestamp diverges
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
uint32_t MicroProfileGpuFlip(void* pContext)
{
	return MicroProfileGpuInsertTimeStamp(pContext);
}

void MicroProfileGpuShutdown()
{
	glDeleteQueries(MICROPROFILE_GL_MAX_QUERIES, &S.pGPU->GLTimers[0]);
	MP_FREE(S.pGPU);
}

#endif

uint32_t MicroProfileStringHash(const char* pString) // note matching: code in javascript: microprofilelive.html: function StringHash(s)
{
	uint32_t h = 0xfeedba3e;
	char c;
	while(0 != (c = *pString++))
	{
		h = c + ((h << 5) - h);
	}
	return h;
}

const char* MicroProfileStrDup(const char* pStr)
{
	size_t len = strlen(pStr) + 1;
	char* pOut = (char*)MP_ALLOC(len, 8);
	memcpy(pOut, pStr, len);
	return pOut;
}

#if MICROPROFILE_DYNAMIC_INSTRUMENT
// '##::::'##::'#######:::'#######::'##:::'##:::::'######::'##::::'##::::'###::::'########::'########:'########::
//  ##:::: ##:'##.... ##:'##.... ##: ##::'##:::::'##... ##: ##:::: ##:::'## ##::: ##.... ##: ##.....:: ##.... ##:
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:'##:::::: ##:::..:: ##:::: ##::'##:. ##:: ##:::: ##: ##::::::: ##:::: ##:
//  #########: ##:::: ##: ##:::: ##: #####:::::::. ######:: #########:'##:::. ##: ########:: ######::: ##:::: ##:
//  ##.... ##: ##:::: ##: ##:::: ##: ##. ##:::::::..... ##: ##.... ##: #########: ##.. ##::: ##...:::: ##:::: ##:
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:. ##:::::'##::: ##: ##:::: ##: ##.... ##: ##::. ##:: ##::::::: ##:::: ##:
//  ##:::: ##:. #######::. #######:: ##::. ##::::. ######:: ##:::: ##: ##:::: ##: ##:::. ##: ########: ########::
// ..:::::..:::.......::::.......:::..::::..::::::......:::..:::::..::..:::::..::..:::::..::........::........:::

#include <distorm.h>
#include <mnemonics.h>

#if MICROPROFILE_BREAK_ON_PATCH_FAIL
#define BREAK_ON_PATCH_FAIL() MP_BREAK()
#else
#define BREAK_ON_PATCH_FAIL()                                                                                                                                                                          \
	do                                                                                                                                                                                                 \
	{                                                                                                                                                                                                  \
	} while(0)
#endif

void* MicroProfileX64FollowJump(void* pSrc);
bool MicroProfileCopyInstructionBytes(char* pDest,
									  void* pSrc,
									  const int nLimit,
									  const int nMaxSize,
									  char* pTrunk,
									  intptr_t nTrunkSize,
									  uint32_t nUsableJumpRegs,
									  int* nBytesDest,
									  int* nBytesSrc,
									  uint32_t* pRegsWritten,
									  uint32_t* nRetSafe);
bool MicroProfilePatchFunction(void* f, int Argument, MicroProfileHookFunc enter, MicroProfileHookFunc leave, MicroProfilePatchError* pError);
template <typename Callback>
void MicroProfileIterateSymbols(Callback CB, uint32_t* nModules, uint32_t nNumModules);
const char* MicroProfileGetUnmangledSymbolName(void* pFunction);

#if 1
#define STRING_MATCH_SIZE 64
typedef uint64_t uint_string_match;
#else
#define STRING_MATCH_SIZE 32
typedef uint32_t uint_string_match;
#endif

struct MicroProfileStringMatchMask
{
	uint_string_match nMask;
	uint_string_match M[64];
};

struct MicroProfileSymbolDesc
{
	const char* pName;
	const char* pShortName;
	intptr_t nAddress;
	intptr_t nAddressEnd;
	uint_string_match nMask;
	int nIgnoreSymbol;
	uint32_t nModule;
};

struct MicroProfileSymbolBlock
{
	MicroProfileSymbolBlock* pNext;
	uint32_t nNumSymbols;
	uint32_t nNumChars;
	uint_string_match nMask;
	MicroProfileStringMatchMask MatchMask;
	enum
	{
		ESIZE = 4 << 10,
	};
	union
	{
		MicroProfileSymbolDesc Symbols[ESIZE / sizeof(MicroProfileSymbolDesc)];
		char Chars[ESIZE];
	};
};

typedef void (*MicroProfileOnSymbolCallback)(const char* pSymbolName, intptr_t nAddress);

MP_THREAD_LOCAL uintptr_t g_MicroProfile_TLS[17] = { 16 };

extern "C" MP_NOINLINE uintptr_t MicroProfile_Patch_TLS_PUSH(uintptr_t t)
{
	uintptr_t* pTLS = &g_MicroProfile_TLS[0];

	uintptr_t Limit = (uint32_t)pTLS[0];
	uintptr_t Pos = (uint32_t)(pTLS[0] >> 32);
	if(Pos == Limit)
	{
		return 0;
	}
	else
	{
		pTLS[0] = (Limit) | ((Pos + 1) << 32);
	}
	pTLS[Pos + 1] = t;
	return 1;
}
extern "C" MP_NOINLINE uintptr_t MicroProfile_Patch_TLS_POP()
{
	uintptr_t* pTLS = &g_MicroProfile_TLS[0];
	uintptr_t Limit = (uint32_t)pTLS[0];
	uintptr_t Pos = (uint32_t)(pTLS[0] >> 32);
	if(Pos == 0)
	{
		MP_BREAK(); // this should never happen
		return 0;
	}
	else
	{
		pTLS[0] = (Limit) | ((Pos - 1) << 32);
	}
	uintptr_t t = pTLS[Pos];
	return t;
}

char* MicroProfileInsertRegisterJump(char* pCode, intptr_t pDest, int reg)
{
	MP_ASSERT(reg >= R_RAX && reg <= R_R15);
	int large = reg >= R_R8 ? 1 : 0;
	int offset = large ? (reg - R_R8) : (reg - R_RAX);
	unsigned char* uc = (unsigned char*)pCode;
	*uc++ = large ? 0x49 : 0x48;
	*uc++ = 0xb8 + offset;
	memcpy(uc, &pDest, 8);
	uc += 8;
	if(large)
		*uc++ = 0x41;
	*uc++ = 0xff;
	*uc++ = 0xe0 + offset;
	return (char*)uc;
	// 164:	48 b8 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rax
	// 16e:	48 b9 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rcx
	// 178:	48 ba 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rdx
	// 182:	48 bb 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rbx
	// 18c:	48 bc 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rsp
	// 196:	48 bd 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rbp
	// 1a0:	48 be 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rsi
	// 1aa:	48 bf 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %rdi
	// 1b4:	49 b8 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r8
	// 1be:	49 b9 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r9
	// 1c8:	49 ba 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r10
	// 1d2:	49 bb 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r11
	// 1dc:	49 bc 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r12
	// 1e6:	49 bd 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r13
	// 1f0:	49 be 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r14
	// 1fa:	49 bf 08 07 06 05 04 03 02 01 	movabsq	$72623859790382856, %r15
	// 204:	ff e0 	jmpq	*%rax
	// 206:	ff e1 	jmpq	*%rcx
	// 208:	ff e2 	jmpq	*%rdx
	// 20a:	ff e3 	jmpq	*%rbx
	// 20c:	ff e4 	jmpq	*%rsp
	// 20e:	ff e5 	jmpq	*%rbp
	// 210:	ff e6 	jmpq	*%rsi
	// 212:	ff e7 	jmpq	*%rdi
	// 214:	41 ff e0 	jmpq	*%r8
	// 217:	41 ff e1 	jmpq	*%r9
	// 21a:	41 ff e2 	jmpq	*%r10
	// 21d:	41 ff e3 	jmpq	*%r11
	// 220:	41 ff e4 	jmpq	*%r12
	// 223:	41 ff e5 	jmpq	*%r13
	// 226:	41 ff e6 	jmpq	*%r14
	// 229:	41 ff e7 	jmpq	*%r15
}

char* MicroProfileInsertRelativeJump(char* pCode, intptr_t pDest)
{
	intptr_t src = intptr_t(pCode) + 5;
	intptr_t off = pDest - src;
	MP_ASSERT(off > intptr_t(0xffffffff80000000) && off <= 0x7fffffff);
	int32_t i32off = (int32_t)off;
	unsigned char* uc = (unsigned char*)pCode;
	unsigned char* c = (unsigned char*)&i32off;
	*uc++ = 0xe9;
	memcpy(uc, c, 4);
	uc += 4;
	return (char*)uc;
}

char* MicroProfileInsertRetJump(char* pCode, intptr_t pDest)
{
	uint32_t lower = (uint32_t)pDest;
	uint32_t upper = (uint32_t)(pDest >> 32);
	unsigned char* uc = (unsigned char*)pCode;
	*uc++ = 0x68;
	memcpy(uc, &lower, 4);
	uc += 4;
	*uc++ = 0xc7;
	*uc++ = 0x44;
	*uc++ = 0x24;
	*uc++ = 0x04;
	memcpy(uc, &upper, 4);
	uc += 4;
	*uc++ = 0xc3;
	return (char*)uc;
}

uint8_t* MicroProfileInsertMov(uint8_t* p, uint8_t* pend, int r, intptr_t value)
{
	int Large = r >= R_R8 ? 1 : 0;
	int RegIndex = Large ? (r - R_R8) : (r - R_RAX);
	*p++ = Large ? 0x49 : 0x48;
	*p++ = 0xb8 + RegIndex; // + (reg - (large?(R_R8-R_RAX):0));
	intptr_t* pAddress = (intptr_t*)p;
	pAddress[0] = value;
	p = (uint8_t*)(pAddress + 1);
	MP_ASSERT(p < pend);
	return p;
}

uint8_t* MicroProfileInsertCall(uint8_t* p, uint8_t* pend, int r)
{
	int Large = r >= R_R8 ? 1 : 0;
	int RegIndex = Large ? (r - R_R8) : (r - R_RAX);
	if(Large)
	{
		*p++ = 0x41;
	}
	*p++ = 0xff;
	*p++ = 0xd0 + RegIndex;
	MP_ASSERT(p < pend);
	return p;
}

bool MicroProfileStringMatch(const char* pSymbol, uint32_t nStartOffset, const char** pPatterns, uint32_t* nPatternLength, uint32_t nNumPatterns)
{
	MP_ASSERT(nStartOffset <= nNumPatterns);
	const char* p = pSymbol;
	for(uint32_t i = nStartOffset; i < nNumPatterns; ++i)
	{
		p = MP_STRCASESTR(p, pPatterns[i]);
		if(p)
		{
			p += nPatternLength[i];
		}
		else
		{
			return false;
		}
	}
	return true;
}

int MicroProfileStringMatchOffset(const char* pSymbol, const char** pPatterns, uint32_t* nPatternLength, uint32_t nNumPatterns)
{
	int nOffset = 0;
	const char* p = pSymbol;
	for(uint32_t i = 0; i < nNumPatterns; ++i)
	{
		p = MP_STRCASESTR(p, pPatterns[i]);
		if(p)
		{
			p += nPatternLength[i];
			nOffset++;
		}
		else
		{
			break;
		}
	}
	return nOffset;
}

void* MicroProfileX64FollowJump(void* pSrc)
{
	for(uint32_t i = 0; i < S.DynamicTokenIndex; ++i)
		if(S.FunctionsInstrumented[i] == pSrc)
			return pSrc; // if already instrumented, do not follow the jump inserted by itself.

	// uprintf("deref possible trampoline for %p\n", pSrc);
	_DecodeType dt = Decode64Bits;
	_DInst Instructions[1];
	unsigned int nCount = 0;

	_CodeInfo ci;
	ci.code = (uint8_t*)pSrc;
	ci.codeLen = 15;
	ci.codeOffset = 0;
	ci.dt = dt;
	ci.features = DF_NONE;
	int r = distorm_decompose(&ci, Instructions, 1, &nCount);
	if(!r || nCount != 1)
	{
		return pSrc; // fail, just return
	}

	auto& I = Instructions[0];
	if(I.opcode == I_JMP)
	{
		if(I.ops[0].type == O_PC)
		{
			if(I.ops[0].size == 0x20)
			{
				intptr_t p = (intptr_t)pSrc;
				p += I.size;
				p += I.imm.sdword;
				return (void*)p;
			}
		}
		else if(I.ops[0].type == O_SMEM)
		{
			if(I.ops[0].index == R_RIP)
			{
				intptr_t p = (intptr_t)pSrc;
				p += I.size;
				p += I.disp;
				void* pHest = *(void**)p;
				return pHest;
			}
		}
		uprintf("failed to interpret I_JMP %p    %d    %d\n", pSrc, I.ops[0].size, I.ops[0].type);
		return pSrc;
		MP_BREAK();
	}
	return pSrc;
}

bool MicroProfileCopyInstructionBytes(char* pDest,
									  void* pSrc,
									  const int nLimit,
									  const int nMaxSize,
									  char* pTrunk,
									  intptr_t nTrunkSize,
									  const uint32_t nUsableJumpRegs,
									  int* pBytesDest,
									  int* pBytesSrc,
									  uint32_t* pRegsWritten,
									  uint32_t* pRetSafe)
{

	_DecodeType dt = Decode64Bits;
	_DInst Instructions[128];
	int rip[128] = { 0 };
	uint32_t nRegsWrittenInstr[128] = { 0 };
	int offsets[129] = { 0 };
	unsigned int nCount = 0;

	_CodeInfo ci;
	ci.code = (uint8_t*)pSrc;
	ci.codeLen = nLimit + 15;
	ci.codeOffset = 0;
	ci.dt = dt;
	ci.features = DF_NONE;
	int r = distorm_decompose(&ci, Instructions, 128, &nCount);
	if(r != DECRES_SUCCESS)
	{
		BREAK_ON_PATCH_FAIL();
		return false;
	}
	int offset = 0;
	unsigned int i = 0;
	unsigned nInstructions = 0;
	int64_t nTrunkUsage = 0;
	offsets[0] = 0;
	uint32_t nRegsWritten = 0;

	auto Align16 = [](intptr_t p) { return (p + 15) & (~15); };

	{

		intptr_t iTrunk = (intptr_t)pTrunk;
		intptr_t iTrunkEnd = iTrunk + nTrunkSize;
		intptr_t iTrunkAligned = (iTrunk + 15) & ~15;
		nTrunkSize = iTrunkEnd - iTrunkAligned;
		pTrunk = (char*)iTrunkAligned;
	}
	const uint8_t* pTrunkEnd = (uint8_t*)(pTrunk + nTrunkSize);

	auto RegToBit = [](int r) -> uint32_t {
		if(r >= R_RAX && r <= R_R15)
		{
			return (1u << (r - R_RAX));
		}
		else if(r >= R_EAX && r <= R_R15D)
		{
			return (1u << (r - R_EAX));
		}
		else if(r >= R_AX && r <= R_R15W)
		{
			return (1u << (r - R_AX));
		}
		else if(r >= R_AL && r <= R_R15B)
		{
			return (1u << (r - R_AL));
		}
		return 0; // might hit on registers like RIP
		MP_BREAK();
	};
#ifdef _WIN32
	const uint32_t nUsableRegisters = RegToBit(R_RAX) | RegToBit(R_R10) | RegToBit(R_R11);
#else
	const uint32_t nUsableRegisters = RegToBit(R_RAX) | RegToBit(R_R10) | RegToBit(R_R11);
#endif

	int nBytesToMove = 0;
	for(i = 0; i < nCount; ++i)
	{
		nBytesToMove += Instructions[i].size;
		if(nBytesToMove >= nLimit)
			break;
	}
	*pBytesSrc = nBytesToMove;

	uint32_t nRspMask = RegToBit(R_RSP);
	*pRetSafe = 1;

	for(i = 0; i < nCount; ++i)
	{
		rip[i] = 0;
		auto& I = Instructions[i];
		// bool bHasRipReference = false;
		if(I.opcode == I_LEA)
		{
		}
		if(I.opcode == I_CALL)
		{
			auto& O = I.ops[0];
			if(O.type != O_PC || O.size != 0x20)
			{
				uprintf("unknown call encountered. cannot move\n");
				BREAK_ON_PATCH_FAIL();
				return false;
			}
			if((nRegsWritten & nUsableRegisters) == nUsableRegisters)
			{
				uprintf("call encountered, but register all regs was written to. TODO: push regs?\n");
				BREAK_ON_PATCH_FAIL();
				return false;
			}
			// return value might be used past return so preserve registers.
#ifdef _WIN32
			nRegsWritten |= RegToBit(R_RAX);
#else
			nRegsWritten |= RegToBit(R_RAX) | RegToBit(R_RDX);
#endif
		}

		switch(I.ops[0].type)
		{
		case O_REG:
		{
			uint32_t reg = I.ops[0].index;
			nRegsWritten |= RegToBit(reg);
			auto& O2 = I.ops[1];
			switch(O2.type)
			{
			case O_REG:
			case O_MEM:
			case O_SMEM:
			{
				// if register is RSP 'contaminated', it prevents us from using that to do retjmps
				uint32_t nMask = RegToBit(O2.index);
				if(nRspMask & nMask)
				{
					nRspMask |= RegToBit(reg);
				}
			}
			default:
				break;
			}
			break;
		}
		case O_MEM:
		case O_SMEM:
		{
			uint32_t reg = I.ops[0].index;
			if(nRspMask & RegToBit(reg))
			{
				uprintf("found contaminated reg at +%lld\n", (long long)I.addr);
				*pRetSafe = 0;
			}
			break;
		}
		}
		nRegsWrittenInstr[i] = nRegsWritten;
		for(int j = 0; j < 4; ++j)
		{
			auto& O = I.ops[j];

			switch(O.type)
			{
			case O_REG:
			case O_SMEM:
			case O_MEM:
			{
				if(O.index == R_RIP)
				{
					if(j != 1)
					{
						uprintf("found non base reference of rip. fail\n");
						BREAK_ON_PATCH_FAIL();
						return false;
					}
					if(I.dispSize != 0x20 && I.dispSize != 0x10)
					{
						uprintf("found offset size != 32 && != 16 bit. not implemented\n");
						BREAK_ON_PATCH_FAIL();
						return false;
					}
					rip[i] = 1;
					nTrunkUsage += Align16(O.size / 8);
					if(nTrunkUsage > nTrunkSize)
					{
						uprintf("overuse of trunk %lld\n", (long long)nTrunkUsage);
						BREAK_ON_PATCH_FAIL();
						return false;
					}
				}
				break;
			}
			}
		}
		if(rip[i])
		{
			if(I.ops[0].type != O_REG)
			{
				uprintf("arg 0 should be O_REG, fail\n");
				BREAK_ON_PATCH_FAIL();
				return false;
			}
			if(I.ops[1].type != O_SMEM)
			{
				uprintf("arg 1 should be O_SMEM, fail was %d\n", O_SMEM);
				BREAK_ON_PATCH_FAIL();
				return false;
			}
		}
		int fc = META_GET_FC(Instructions[i].meta);
		switch(fc)
		{
		case FC_CALL:
		{
			break;
		}
		case FC_RET:
		case FC_SYS:
		case FC_UNC_BRANCH:
		case FC_CND_BRANCH:
			uprintf("found branch inst %d :: %d\n", fc, offset);
			BREAK_ON_PATCH_FAIL();
			return false;
		}
		offset += Instructions[i].size;
		offsets[i + 1] = offset;
		if(offset >= nLimit)
		{
			nInstructions = i + 1;
			break;
		}
	}
	if(nTrunkUsage > nTrunkSize)
	{
		uprintf("function using too much trunk space\n");
		BREAK_ON_PATCH_FAIL();
		return false;
	}
	if(offset < nLimit)
	{
		uprintf("function only had %d bytes of %d\n", offset, nLimit);
		BREAK_ON_PATCH_FAIL();
		return false;
	}

	if(0 == *pRetSafe && 0 == (nUsableJumpRegs & ~nRegsWritten))
	{
		// if ret jump is unsafe all of the usable jump regs are taken, fail.
		uprintf("cannot patch function without breaking code]\n");
		BREAK_ON_PATCH_FAIL();
		MP_BREAK();
		return false;
	}

	// MP_BREAK();
	*pRegsWritten = nRegsWritten;
	uint8_t* d = (uint8_t*)pDest;
	uint8_t* dend = d + nMaxSize;
	const uint8_t* s = (const uint8_t*)pSrc;

	nTrunkUsage = 0;

	for(i = 0; i < nInstructions; ++i)
	{
		auto& I = Instructions[i];
		unsigned size = Instructions[i].size;
		if(I.opcode == I_CALL)
		{
			// find reg
			uint32_t nRegsWritten = nRegsWrittenInstr[i];
			uint32_t nUsable = nUsableRegisters & ~nRegsWritten;
			MP_ASSERT(nUsable);
			int r = R_RAX;
			while(0 == (1 & nUsable))
			{
				nUsable >>= 1;
				r++;
			}

			intptr_t p = offsets[i + 1];
			p += (intptr_t)pSrc;
			p += I.imm.sdword;
			d = MicroProfileInsertMov(d, dend, r, p);
			d = MicroProfileInsertCall(d, dend, r);
			s += size;
		}
		else if(rip[i])
		{
			if(I.opcode == I_LEA)
			{
				if(I.ops[0].type != O_REG)
				{
					MP_BREAK();
				}
				if(I.ops[1].index != R_RIP)
				{
					MP_BREAK();
				}
				int reg = I.ops[0].index - R_RAX;
				int large = I.ops[0].index >= R_R8 ? 1 : 0;
				*d++ = large ? 0x49 : 0x48;
				*d++ = 0xb8 + (reg - (large ? (R_R8 - R_RAX) : 0));
				// calculate the offset
				int64_t offset = offsets[i + 1] + I.disp;
				intptr_t base = (intptr_t)pSrc;

				intptr_t sum = base + offset;
				intptr_t* pAddress = (intptr_t*)d;
				pAddress[0] = sum;
				s += size;
				d += 10;
				d = (uint8_t*)(pAddress + 1);
			}
			else
			{
				if(15 & (intptr_t)pTrunk)
				{
					MP_BREAK();
				}
				intptr_t t = (intptr_t)pTrunk;
				t = (t + 15) & ~15;
				pTrunk = (char*)t;
				auto& O = I.ops[1];
				uint32_t Op1Size = O.size / 8;

				memcpy(d, s, size);
				int32_t DispOriginal = (int32_t)I.disp;
				const uint8_t* pOriginal = (s + size) + DispOriginal;

				intptr_t DispNew = ((uint8_t*)pTrunk - (d + size));
				if(!((intptr_t)pTrunk + Op1Size <= (intptr_t)pTrunkEnd))
				{
					MP_BREAK();
				}
				memcpy(pTrunk, pOriginal, Op1Size);
				pTrunk += Align16(Op1Size);
				if(I.dispSize == 32)
				{
					int32_t off = (int32_t)DispNew;
					if(DispNew > 0x7fffffff || DispNew < 0)
					{
						MP_BREAK();
					}
					memcpy(d + size - 4, &off, 4);
				}
				else if(I.dispSize == 16)
				{
					int16_t off = (int16_t)DispNew;
					if(DispNew > 0x7fff || DispNew < 0)
					{
						MP_BREAK();
					}
					memcpy(d + size - 2, &off, 2);
				}

				d += size;
				s += size;
			}
		}
		else
		{
			memcpy(d, s, size);
			d += size;
			s += size;
		}
	}

	*pBytesDest = (int)(d - (uint8_t*)pDest);

	return true;
}

extern "C" void MicroProfileInterceptEnter(int a)
{
	MicroProfileToken T = S.DynamicTokens[a];
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope < MICROPROFILE_STACK_MAX); // if youre hitting this assert your instrumenting a deeply nested function
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[pLog->nStackScope++];
	pScopeState->Token = T;
	if(T)
	{
		pScopeState->nTick = MicroProfileEnterInternal(T);
	}
	else
	{
		pScopeState->nTick = MICROPROFILE_INVALID_TICK;
	}
}
extern "C" void MicroProfileInterceptLeave(int a)
{
	MicroProfileThreadLog* pLog = MicroProfileGetThreadLog2();
	MP_ASSERT(pLog->nStackScope > 0); // if youre hitting this assert you probably have mismatched _ENTER/_LEAVE markers
	MicroProfileScopeStateC* pScopeState = &pLog->ScopeState[--pLog->nStackScope];
	MicroProfileLeaveInternal(pScopeState->Token, pScopeState->nTick);
}

uint32_t MicroProfileColorFromString(const char* pString) // note matching code/constants in javascript: microprofilelive.html: function StringToColor(s)
{
	// 	var h = StringHash(s);
	// var cidx = h % 360;
	// return "hsl(" + cidx + ",50%, 70%)"; //note: matching code constants in microprofile.cpp: MicroProfileColorFromString

	float h = MicroProfileStringHash(pString) % 360;
	float s = 0.5f;
	float l = 0.7f;
	// from https://www.rapidtables.com/convert/color/hsl-to-rgb.html
	float c = (1 - fabsf(2 * l - 1)) * s;
	float x = c * (1 - fabsf(fmodf(h / 60, 2.f) - 1));
	float m = l - c / 2.f;
	float r = 0.f, g = 0.f, b = 0.f;
	if(h < 60)
	{
		r = c;
		g = x;
	}
	else if(h < 120.f)
	{
		r = x;
		g = c;
	}
	else if(h < 180.f)
	{
		g = c;
		b = x;
	}
	else if(h < 240.f)
	{
		g = x;
		b = c;
	}
	else if(h < 300.f)
	{
		r = x;
		b = c;
	}
	else
	{
		r = c;
		b = x;
	}
	r += m;
	g += m;
	b += m;

	r *= 255.f;
	g *= 255.f;
	b *= 255.f;

	uint32_t R = MicroProfileMin(0xffu, (uint32_t)r);
	uint32_t G = MicroProfileMin(0xffu, (uint32_t)g);
	uint32_t B = MicroProfileMin(0xffu, (uint32_t)b);

	return (R << 16) | (G << 8) | B;
}

void MicroProfileInstrumentFromAddressOnly(void* pFunction)
{
	MicroProfileSymbolDesc* pDesc = MicroProfileSymbolFindFuction(pFunction);
	if(pDesc)
	{
		uprintf("Found function %p :: %s %s\n", (void*)pDesc->nAddress, pDesc->pName, pDesc->pShortName);
		uint32_t nColor = MicroProfileColorFromString(pDesc->pName);

		MicroProfileInstrumentFunction(pFunction, MicroProfileSymbolModuleGetString(pDesc->nModule), pDesc->pName, nColor);
	}
	else
	{
		uprintf("No Function Found %p\n", pFunction);
	}
}

void MicroProfileInstrumentFunctionsCalled(void* pFunction, const char* pModuleName, const char* pFunctionName)
{
	pFunction = MicroProfileX64FollowJump(pFunction);

	MicroProfileSymbolDesc* pDesc = MicroProfileSymbolFindFuction(pFunction);
	if(pDesc)
	{
		uprintf("instrumenting child functions %p %p :: %s :: %s\n", (void*)pDesc->nAddress, (void*)pDesc->nAddressEnd, pDesc->pName, pDesc->pShortName);
		int a = 0;
		(void)a;
	}
	else
	{
		uprintf("could not find symbol info\n");
		return;
	}

	const intptr_t nCodeLen = (intptr_t)pDesc->nAddressEnd - (intptr_t)pDesc->nAddress;
	const uint32_t nMaxInstructions = 15;
	intptr_t nOffset = 0;
	_DecodeType dt = Decode64Bits;
	_DInst Instructions[15];
	int cc = 0;

	_CodeInfo ci;
	do
	{
		if(cc++ > 100)
		{
			int a = 0;
			(void)a;
		}
		ci.code = nOffset + (uint8_t*)pFunction;
		ci.codeLen = nCodeLen - nOffset;
		ci.codeOffset = 0;
		ci.dt = dt;
		ci.features = DF_RETURN_FC_ONLY;
		uint32_t nCount = 0;
		uint32_t nOffsetNext = 0;

		int r = distorm_decompose(&ci, Instructions, nMaxInstructions, &nCount);
		// uprintf("decomposed %d\n", nCount);
		if(r != DECRES_SUCCESS && r != DECRES_MEMORYERR)
		{
			BREAK_ON_PATCH_FAIL();
			return;
		}
		if(nCount == 0)
		{
			// no instructions left
			break;
		}
		// uprintf("instructions decoded %d      %p ::\n", nCount, pFunction);
		for(int i = 0; i < (int)nCount; ++i)
		{
			// rip[i] = 0;
			auto& I = Instructions[i];
			// bool bHasRipReference = false;
			if(I.addr < nOffsetNext)
				MP_BREAK();
			nOffsetNext = I.addr + I.size;
			if(I.opcode == I_CALL)
			{
				auto& O = I.ops[0];
				if(O.type != O_PC || O.size != 0x20)
				{
					uprintf("non immediate call encountered. cannot follow\n");
					BREAK_ON_PATCH_FAIL();
					continue;
				}
				intptr_t pDst = nOffset + (intptr_t)pFunction;
				pDst += I.addr;
				pDst += I.size;
				pDst += I.imm.sdword;

				void* fFun1 = MicroProfileX64FollowJump((void*)pDst);
				MicroProfileInstrumentFromAddressOnly(fFun1);
			}
			else if(I.opcode == I_JMP)
			{

				// uprintf("%d JMP          %p\n", i, (void*)I.addr);
				// I_JMP
			}
			else if(I.opcode == I_JNZ)
			{

				// uprintf("%d JNZ          %p\n", i, (void*)I.addr);
				// I_JMP
			}
			else if(I.opcode == I_JZ)
			{

				// uprintf("%d JZ          %p\n", i, (void*)I.addr);
				// I_JMP
			}
			else
			{
				// uprintf("%d ?? %d           %p\n", i, I.opcode, (void*)I.addr);
			}
		}
		nOffset += nOffsetNext;
	} while(nOffset < nCodeLen);
	// uprintf("total bytes processed %ld\n", nOffset);
}

void MicroProfileInstrumentFunction(void* pFunction, const char* pModuleName, const char* pFunctionName, uint32_t nColor)
{
	MicroProfileScopeLock L(MicroProfileMutex());
	MicroProfilePatchError Err;
	if(S.DynamicTokenIndex == MICROPROFILE_MAX_DYNAMIC_TOKENS)
	{
		uprintf("instrument failing, out of dynamic tokens %d\n", S.DynamicTokenIndex);
		return;
	}
	for(uint32_t i = 0; i < S.DynamicTokenIndex; ++i)
	{
		if(S.FunctionsInstrumented[i] == pFunction)
		{
			uprintf("function %p already instrumented\n", pFunction);
			return;
		}
	}
	if(MicroProfilePatchFunction(pFunction, S.DynamicTokenIndex, MicroProfileInterceptEnter, MicroProfileInterceptLeave, &Err))
	{
		MicroProfileToken Tok = S.DynamicTokens[S.DynamicTokenIndex] = MicroProfileGetToken("PATCHED", pFunctionName, nColor, MicroProfileTokenTypeCpu, 0);
		S.FunctionsInstrumented[S.DynamicTokenIndex] = pFunction;
		S.FunctionsInstrumentedName[S.DynamicTokenIndex] = MicroProfileStringIntern(pFunctionName);
		S.FunctionsInstrumentedModuleNames[S.DynamicTokenIndex] = MicroProfileStringIntern(pModuleName);
		S.FunctionsInstrumentedUnmangled[S.DynamicTokenIndex] = MicroProfileGetUnmangledSymbolName(pFunction);
		S.DynamicTokenIndex++;

		uint16_t nGroup = MicroProfileGetGroupIndex(Tok);
		if(!MicroProfileGroupActive(nGroup))
		{
			MicroProfileToggleGroup(nGroup);
		}
#if MICROPROFILE_WEBSERVER
		MicroProfileToggleWebSocketToggleTimer(MicroProfileGetTimerIndex(Tok));
#endif
	}
	else
	{
		bool bFound = false;
		for(int i = 0; i < S.nNumPatchErrors; ++i)
		{
			if(Err.nCodeSize == S.PatchErrors[i].nCodeSize && 0 == memcmp(Err.Code, S.PatchErrors[i].Code, Err.nCodeSize))
			{
				bFound = true;
				break;
			}
		}
		if(!bFound && S.nNumPatchErrors < MICROPROFILE_MAX_PATCH_ERRORS)
		{
			memcpy(&S.PatchErrors[S.nNumPatchErrors++], &Err, sizeof(Err));
		}
		bFound = false;
		for(int i = 0; i < S.nNumPatchErrorFunctions; ++i)
		{
			if(0 == strcmp(pFunctionName, S.PatchErrorFunctionNames[i]))
			{
				bFound = true;
			}
		}
		if(!bFound && S.nNumPatchErrorFunctions < MICROPROFILE_MAX_PATCH_ERRORS)
		{
			S.PatchErrorFunctionNames[S.nNumPatchErrorFunctions++] = pFunctionName;
		}
		uprintf("interception fail!!\n");
	}
}

void MicroProfileSymbolInitializeInternal();
void MicroProfileSymbolFreeDataInternal();
void MicroProfileSymbolKickThread();
void MicroProfileQueryJoinThread();

bool MicroProfileSymbolInitialize(bool bStartLoad, const char* pModuleName)
{
	if(!bStartLoad)
		return S.SymbolState.nModuleLoadsFinished.load() != 0;
	// int nRequests = 0;
	{
		MicroProfileScopeLock L(MicroProfileMutex());
		for(int i = 0; i < S.SymbolNumModules; ++i)
		{
			if(0 == pModuleName || 0 == strcmp(pModuleName, (const char*)S.SymbolModules[i].pBaseString))
			{
				if(0 == S.SymbolModules[i].nModuleLoadRequested.exchange(1))
				{
					S.SymbolState.nModuleLoadsRequested.fetch_add(1);
				}
			}
		}
	}

	// todo: unload modules
	MicroProfileSymbolKickThread();
	return S.SymbolState.nModuleLoadsRequested.load() == S.SymbolState.nModuleLoadsFinished.load();

	// if(S.SymbolState.nState == MICROPROFILE_SYMBOLSTATE_DEFAULT)
	// {
	// 	if(!bStartLoad)
	// 		return false;
	// 	{
	// 		MicroProfileScopeLock L(MicroProfileMutex());
	// 		S.SymbolState.nState.store(MICROPROFILE_SYMBOLSTATE_LOADING);
	// 		S.SymbolState.nSymbolsLoaded.store(0);
	// 	}
	// 	MicroProfileSymbolKickThread();
	// 	return false;
	// }
	// if(nRequests)
	// {
	// }
	// if(S.SymbolState.nState.load() == MICROPROFILE_SYMBOLSTATE_DONE)
	// {
	// 	MicroProfileQueryJoinThread();
	// }
	// if(S.SymbolState.nState == MICROPROFILE_SYMBOLSTATE_DONE && bStartLoad)
	// {
	// 	MicroProfileSymbolFreeDataInternal();
	// 	{
	// 		MicroProfileScopeLock L(MicroProfileMutex());
	// 		S.SymbolState.nState.store(MICROPROFILE_SYMBOLSTATE_LOADING);
	// 		S.SymbolState.nSymbolsLoaded.store(0);
	// 	}
	// 	MicroProfileSymbolKickThread();
	// 	return false;

	// }
	// else
	// {
	// 	return S.SymbolState.nState == MICROPROFILE_SYMBOLSTATE_DONE;
	// }
}

void MicroProfileSymbolFreeDataInternal()
{
	{
		uprintf("todod;....\n");
		MP_BREAK();
		// MP_ASSERT(S.SymbolState.nState == MICROPROFILE_SYMBOLSTATE_DONE);

		S.nNumPatchErrorFunctions = 0;
		memset(S.PatchErrorFunctionNames, 0, sizeof(S.PatchErrorFunctionNames));

		for(int i = 0; i < S.SymbolNumModules; ++i)
		{

			while(S.SymbolModules[i].pSymbolBlock)
			{
				MicroProfileSymbolBlock* pBlock = S.SymbolModules[i].pSymbolBlock;
				S.SymbolModules[i].pSymbolBlock = pBlock->pNext;
				MP_FREE(pBlock);
				MICROPROFILE_COUNTER_SUB("/MicroProfile/Symbols/Allocs", 1);
				MICROPROFILE_COUNTER_SUB("/MicroProfile/Symbols/Memory", sizeof(MicroProfileSymbolBlock));
			}
		}
		memset(&S.SymbolModules[0], 0, sizeof(S.SymbolModules));
		memset(&S.SymbolModuleNameBuffer[0], 0, sizeof(S.SymbolModuleNameBuffer));
		S.SymbolModuleNameOffset = 0;
		S.SymbolNumModules = 0;
	}
}
#if STRING_MATCH_SIZE == 64
int MicroProfileCharacterMaskCharIndex(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = 'a' + (c - 'A');
	// abcdefghijklmnopqrstuvwxyz
	if(c >= 'a' && c <= 'z')
	{
		int b = c - 'a';
		return b;
	}
	if(c >= '0' && c <= '9')
	{
		int b = c - '0';
		return b + 26;
	}
	switch(c)
	{
	case ':':
		return 37;
	case ';':
		return 38;
	case '\\':
		return 39;
	case '\'':
		return 40;
	case '\"':
		return 41;
	case '/':
		return 42;
	case '{':
		return 43;
	case '}':
		return 44;
	case '(':
		return 45;
	case ')':
		return 46;
	case '[':
		return 47;
	case ']':
		return 48;
	case '<':
		return 49;
	case '>':
		return 50;
	case '.':
		return 51;
	case ',':
		return 52; // special characters
	case ' ':
		return -1; // special characters
	}
	return 63;
}

uint64_t MicroProfileCharacterMaskChar(char c)
{
	uint64_t nMask = 1;
	int nIndex = MicroProfileCharacterMaskCharIndex(c);
	if(nIndex == -1)
		return 0;
	return nMask << nIndex;
}

#else
uint32_t MicroProfileCharacterMaskChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = 'a' + (c - 'A');
	// abcdefghijklmnopqrstuvwxyz
	if(c >= 'a' && c <= 'z')
	{
		int b = c - 'a';
		b = MicroProfileMin(20, b); // squish the last together
		// static int once = 0;
		// if(0 == once)
		//{
		//	for(int i = 20; i < 28; ++i)
		//	{
		//		uprintf("char %d is %c\n", i, (char)('a' + i));
		//	}
		//	once = 1;
		//}
		uint32_t v = 1;
		return v << b;
	}
	if(c >= '0' && c <= '9')
	{
		int b = c - '0';
		b += 21;
		if(b < 21 || b > 30)
			MP_BREAK();
		return 1 << b;
	}
	switch(c)
	{
	case ':':
	case ';':
	case '\\':
	case '\'':
	case '\"':
	case '/':
	case '{':
	case '}':
	case '(':
	case ')':
	case '[':
	case ']':
		return 1u << 31; // special characters
	case ' ':
		return 0;
	}
	return 0;
}
int MicroProfileCharacterMaskCharIndex(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = 'a' + (c - 'A');
	// abcdefghijklmnopqrstuvwxyz
	if(c >= 'a' && c <= 'z')
	{
		int b = c - 'a';
		b = MicroProfileMin(20, b); // squish the last together
		static int once = 0;
		if(0 == once)
		{
			for(int i = 20; i < 28; ++i)
			{
				uprintf("char %d is %c\n", i, (char)('a' + i));
			}
			once = 1;
		}
		return b;
	}
	if(c >= '0' && c <= '9')
	{
		int b = c - '0';
		b += 21;
		if(b < 21 || b > 30)
			MP_BREAK();
		return b;
	}
	switch(c)
	{
	case ':':
	case ';':
	case '\\':
	case '\'':
	case '\"':
	case '/':
	case '{':
	case '}':
	case '(':
	case ')':
	case '[':
	case ']':
		return 31; // special characters
	case ' ':
		return -1;
	}
	return 1;
}
#endif

uint_string_match MicroProfileCharacterMaskString(const char* pStr)
{
	uint_string_match nMask = 0;
	char c = 0;
	while(0 != (c = *pStr++))
	{
		nMask |= MicroProfileCharacterMaskChar(c);
	}
	return nMask;
}

void MicroProfileCharacterMaskString2(const char* pStr, MicroProfileStringMatchMask& M)
{
	uint_string_match nMask = 0;
	char c = 0;
	int nLast = -1;
	while(0 != (c = *pStr++))
	{
		nMask |= MicroProfileCharacterMaskChar(c);
		int nIndex = MicroProfileCharacterMaskCharIndex(c);
		if(nIndex >= 0 && nLast >= 0)
		{
			MP_ASSERT(nIndex < STRING_MATCH_SIZE);
			M.M[nLast] |= 1llu << nIndex;
		}
		nLast = nIndex;
	}
	M.nMask |= nMask;
}

bool MicroProfileCharacterMatch(const MicroProfileStringMatchMask& Block, const MicroProfileStringMatchMask& String)
{
	if(String.nMask != (Block.nMask & String.nMask))
		return false;
	for(uint32_t i = 0; i < STRING_MATCH_SIZE; ++i)
	{
		if(String.M[i] != (Block.M[i] & String.M[i]))
			return false;
	}
	return true;
}

uint32_t MicroProfileSymbolGetModule(const char* pString, intptr_t nBaseAddr)
{

	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		for(int j = 0; j < M.nNumExecutableRegions; ++j)
		{
			if(M.Regions[j].nBegin <= nBaseAddr && nBaseAddr < M.Regions[j].nEnd)
				return i;
		}
	}
	MP_BREAK(); // should never happen.
	return 0;
}

void MicroProfileSymbolMergeExecutableRegions()
{
	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		if(M.nNumExecutableRegions > 1)
		{
			std::sort(&M.Regions[0], &M.Regions[M.nNumExecutableRegions], [](const MicroProfileSymbolModuleRegion& l, const MicroProfileSymbolModuleRegion& r) { return l.nBegin < r.nBegin; });

			int p = 0;
			int g = 1;
			while(g < M.nNumExecutableRegions)
			{
				if(M.Regions[p].nEnd == M.Regions[g].nBegin)
				{
					M.Regions[p].nEnd = M.Regions[g].nEnd;
					g++;
				}
				else
				{
					++p;
					if(p != g)
						M.Regions[p] = M.Regions[g];
					g++;
				}
			}
			M.nNumExecutableRegions = p + 1;
		}
	}
	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		uprintf("region %s %s\n", M.pTrimmedString, M.pBaseString);
		for(int j = 0; j < M.nNumExecutableRegions; ++j)
			uprintf("\t[%p-%p]\n", (void*)M.Regions[j].nBegin, (void*)M.Regions[j].nEnd);
	}
}

uint32_t MicroProfileSymbolInitModule(const char* pString_, intptr_t nAddrBegin, intptr_t nAddrEnd)
{
	const char* pString = MicroProfileStringIntern(pString_);
	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		for(int j = 0; j < M.nNumExecutableRegions; ++j)
		{
			if(M.Regions[j].nBegin <= nAddrBegin && nAddrEnd < M.Regions[j].nEnd)
			{
				MP_ASSERT(pString == M.pBaseString);
				return i;
			}
		}
	}

	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		if(M.pBaseString == pString)
		{
			MP_ASSERT((intptr_t)pString != -2);
			for(int j = 0; j < M.nNumExecutableRegions; ++j)
				if(nAddrBegin == M.Regions[j].nBegin)
					return i;

			if(M.nNumExecutableRegions == MICROPROFILE_MAX_MODULE_EXEC_REGIONS)
			{
				return (uint32_t)-1;
			}
			M.Regions[M.nNumExecutableRegions].nBegin = nAddrBegin;
			M.Regions[M.nNumExecutableRegions].nEnd = nAddrEnd;
			// uprintf("added module region %d %p %p %s \n", M.nNumExecutableRegions, (void*)nAddrBegin, (void*)nAddrEnd, pString);
			M.nNumExecutableRegions++;
			return i;
		}
	}

	MP_ASSERT((intptr_t)pString != -2);
	// trim untill last path char
	const char* pTrimmedString = pString;

	const char* pWork = pTrimmedString;
	bool bLastSeperator = false;
	while(*pWork != '\0')
	{
		if(bLastSeperator)
			pTrimmedString = pWork;
		bLastSeperator = *pWork == '\\' || *pWork == '/';

		pWork++;
	}
	int nLen = (int)strlen(pTrimmedString) + 1;
	// uprintf("STRING '%s' :: trimmedstring %s   . len %d\n", pString, pTrimmedString, nLen);

	const char* pTrimmedIntern = MicroProfileStringIntern(pTrimmedString);
	if(S.SymbolModuleNameOffset + nLen > MICROPROFILE_INSTRUMENT_MAX_MODULE_CHARS)
		return 0;
	memcpy(S.SymbolModuleNameOffset + &S.SymbolModuleNameBuffer[0], pTrimmedString, nLen);

	MP_ASSERT(S.SymbolNumModules < MICROPROFILE_INSTRUMENT_MAX_MODULES);
	S.SymbolModules[S.SymbolNumModules].nMatchOffset = 0;
	S.SymbolModules[S.SymbolNumModules].nStringOffset = S.SymbolModuleNameOffset;
	S.SymbolModules[S.SymbolNumModules].pBaseString = (const char*)pString;
	S.SymbolModules[S.SymbolNumModules].pTrimmedString = pTrimmedIntern;
	S.SymbolModules[S.SymbolNumModules].Regions[0].nBegin = nAddrBegin;
	S.SymbolModules[S.SymbolNumModules].Regions[0].nEnd = nAddrEnd;
	S.SymbolModules[S.SymbolNumModules].nNumExecutableRegions = 1;
	S.SymbolModules[S.SymbolNumModules].nProgress = 0;
	S.SymbolModules[S.SymbolNumModules].nProgressTarget = 0;

	S.SymbolModuleNameOffset += nLen;
	return S.SymbolNumModules++;
}

const char* MicroProfileSymbolModuleGetString(uint32_t nIndex)
{
	MP_ASSERT(S.SymbolNumModules > (int)nIndex);
	return S.SymbolModules[nIndex].nStringOffset + &S.SymbolModuleNameBuffer[0];
}

void MicroProfileSymbolInitializeInternal()
{
	uprintf("Starting load...\n");
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSymbolInitialize", MP_CYAN);

	auto AllocBlock = []() -> MicroProfileSymbolBlock* {
		MicroProfileSymbolBlock* pBlock = MP_ALLOC_OBJECT(MicroProfileSymbolBlock);
		MICROPROFILE_COUNTER_ADD("/MicroProfile/Symbols/Allocs", 1);
		MICROPROFILE_COUNTER_ADD("/MicroProfile/Symbols/Memory", sizeof(MicroProfileSymbolBlock));
		MICROPROFILE_COUNTER_CONFIG_ONCE("/MicroProfile/Symbols/Memory", MICROPROFILE_COUNTER_FORMAT_BYTES, 0, 0);
		memset(pBlock, 0, sizeof(MicroProfileSymbolBlock));
		return pBlock;
	};

	auto SymbolCallback = [&](const char* pName, const char* pShortName, intptr_t nAddress, intptr_t nAddressEnd, uint32_t nModuleId) {
		uint32_t nModule = nModuleId;
		intptr_t delta = nAddressEnd - nAddress;
		S.SymbolModules[nModule].nProgress += delta;

		int nIgnoreSymbol = 0;
		if(strstr(pName, "MicroProfile"))
		{
#if MICROPROFILE_INSTRUMENT_MICROPROFILE == 0
			nIgnoreSymbol = 1;
#else
			if(strstr(pName, "Log") || strstr(pName, "Scope") || strstr(pName, "Tick") || strstr(pName, "Enter") || strstr(pName, "Leave") || strstr(pName, "Thread") || strstr(pName, "Thread") ||
			   strstr(pName, "Mutex")) // just for debugging: skip these so we can play around with the sample projects
			{
				nIgnoreSymbol = 1;
			}
#endif
		}
#ifdef _WIN32
		if(strstr(pName, "__security_check_cookie") || strstr(pName, "_RTC_CheckStackVars") || strstr(pName, "__chkstk") || strstr(pName, "std::_Atomic"))
		{
			nIgnoreSymbol = 1;
		}
#endif
		MicroProfileSymbolBlock* pActiveBlock = S.SymbolModules[nModule].pSymbolBlock;
		if(!pActiveBlock)
		{
			pActiveBlock = AllocBlock();
			pActiveBlock->pNext = S.SymbolModules[nModule].pSymbolBlock;
			S.SymbolModules[nModule].pSymbolBlock = pActiveBlock;
		}

		if(pName == pShortName)
		{
			pShortName = 0;
		}
		uint32_t nLen = (uint32_t)strlen(pName) + 1;

		if(nLen > MICROPROFILE_INSTRUMENT_SYMBOLNAME_MAXLEN)
			nLen = MICROPROFILE_INSTRUMENT_SYMBOLNAME_MAXLEN;
		uint32_t nLenShort = (uint32_t)(pShortName ? 1 + strlen(pShortName) : 0);
		if(nLenShort && nLenShort > MICROPROFILE_INSTRUMENT_SYMBOLNAME_MAXLEN)
			nLenShort = MICROPROFILE_INSTRUMENT_SYMBOLNAME_MAXLEN;
		uint32_t S0 = sizeof(MicroProfileSymbolDesc) * pActiveBlock->nNumSymbols;
		uint32_t S1 = pActiveBlock->nNumChars;
		uint32_t S3 = nLenShort + nLen + sizeof(MicroProfileSymbolDesc) + 64;
		if(S0 + S1 + S3 >= MicroProfileSymbolBlock::ESIZE)
		{
			MicroProfileSymbolBlock* pNewBlock = AllocBlock();
			MP_ASSERT(pActiveBlock == S.SymbolModules[nModule].pSymbolBlock);
			pNewBlock->pNext = pActiveBlock;
			S.SymbolModules[nModule].pSymbolBlock = pNewBlock;
			pActiveBlock = pNewBlock;
		}
		S0 = sizeof(MicroProfileSymbolDesc) * pActiveBlock->nNumSymbols;
		S1 = pActiveBlock->nNumChars;
		S3 = nLenShort + nLen + sizeof(MicroProfileSymbolDesc);
		MP_ASSERT(S0 + S1 + S3 < MicroProfileSymbolBlock::ESIZE);
		pActiveBlock->nNumChars += nLen;
		char* pStr = &pActiveBlock->Chars[MicroProfileSymbolBlock::ESIZE - pActiveBlock->nNumChars - 1];
		memcpy(pStr, pName, nLen);
		pStr[nLen - 1] = '\0';
		MicroProfileSymbolDesc& E = pActiveBlock->Symbols[pActiveBlock->nNumSymbols++];
		E.pName = pStr;
		E.nAddress = nAddress;
		E.nAddressEnd = nAddressEnd;
		E.nIgnoreSymbol = nIgnoreSymbol;
		E.nModule = nModule;
		if(pShortName && strlen(pShortName))
		{
			pActiveBlock->nNumChars += nLenShort;
			char* pStrShort = &pActiveBlock->Chars[MicroProfileSymbolBlock::ESIZE - pActiveBlock->nNumChars - 1];
			memcpy(pStrShort, pShortName, nLenShort);
			pStrShort[nLenShort - 1] = '\0';
			E.pShortName = pStrShort;
		}
		else
		{
			E.pShortName = E.pName;
		}
#define SYMDBG 0
#if SYMDBG
		uprintf("Got symbol %lld %lld %f .. %llx    %llx    %llx %s\n",
				S.SymbolModules[nModule].nProgress,
				S.SymbolModules[nModule].nProgressTarget,
				S.SymbolModules[nModule].nProgressTarget ? float(S.SymbolModules[nModule].nProgress) / float(S.SymbolModules[nModule].nProgressTarget) : 0.f,
				(int64_t)E.nAddress,
				(int64_t)S.SymbolModules[nModule].nAddrBegin,
				(int64_t)S.SymbolModules[nModule].nAddrEnd,
				E.pName);
		if(E.nAddress < (int64_t)S.SymbolModules[nModule].nAddrBegin || E.nAddress > (int64_t)S.SymbolModules[nModule].nAddrEnd)
		{
			MP_BREAK();
		}
#endif
		E.nMask = MicroProfileCharacterMaskString(E.pShortName);
		MicroProfileCharacterMaskString2(E.pShortName, pActiveBlock->MatchMask);

		pActiveBlock->nMask |= E.nMask;
		MICROPROFILE_COUNTER_ADD("/MicroProfile/Symbols/Count", 1);
		if(nIgnoreSymbol)
		{
			MICROPROFILE_COUNTER_ADD("/MicroProfile/Symbols/Ignored", 1);
		}
		MicroProfileSleep(1);
#if SYMDBG
		MicroProfileSleep(10);
#endif
#undef SYMDBG

		S.SymbolModules[nModule].nSymbolsLoaded.fetch_add(1);
		S.nSymbolsDirty.exchange(1);
		S.SymbolState.nSymbolsLoaded.fetch_add(1);
		MP_ASSERT((intptr_t)E.pShortName >= (intptr_t)&E); // assert pointer arithmetic is correct.
	};
	do
	{
		uint32_t nModuleLoad[MICROPROFILE_INSTRUMENT_MAX_MODULES];
		uint32_t nNumModulesRequested = 0;
		for(int i = 0; i < S.SymbolNumModules; ++i)
		{
			if(S.SymbolModules[i].nModuleLoadRequested.load() != 0 && S.SymbolModules[i].nModuleLoadFinished.load() == 0)
			{
				nModuleLoad[nNumModulesRequested] = i;
				S.SymbolModules[i].nProgress = 0;
				nNumModulesRequested++;
			}
		}
		if(0 == nNumModulesRequested)
		{
			break;
		}
		MicroProfileIterateSymbols(SymbolCallback, nModuleLoad, nNumModulesRequested);
		S.SymbolState.nModuleLoadsFinished.fetch_add(nNumModulesRequested);
		for(uint32_t i = 0; i < nNumModulesRequested; ++i)
		{
			if(S.SymbolModules[nModuleLoad[i]].nModuleLoadRequested.load() == S.SymbolModules[nModuleLoad[i]].nModuleLoadFinished.load())
			{
				S.SymbolModules[nModuleLoad[i]].nProgress = S.SymbolModules[nModuleLoad[i]].nProgressTarget;
				S.nSymbolsDirty.exchange(1);
			}
		}
	} while(1);
}

MicroProfileSymbolDesc* MicroProfileSymbolFindFuction(void* pAddress)
{
	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		MicroProfileSymbolBlock* pSymbols = S.SymbolModules[i].pSymbolBlock;
		while(pSymbols)
		{
			for(uint32_t i = 0; i < pSymbols->nNumSymbols; ++i)
			{
				MicroProfileSymbolDesc& E = pSymbols->Symbols[i];
				if(E.nAddress == (intptr_t)pAddress && 0 == E.nIgnoreSymbol)
				{
					return &E;
				}
			}
			pSymbols = pSymbols->pNext;
		}
	}
	return nullptr;
}

#define MICROPROFILE_MAX_FILTER 32
#define MICROPROFILE_MAX_QUERY_RESULTS 32
#define MICROPROFILE_MAX_FILTER_STRING 1024

struct MicroProfileFunctionQuery
{
	MicroProfileFunctionQuery* pNext;
	uint32_t nState;
	const char* pFilterStrings[MICROPROFILE_MAX_FILTER];
	uint32_t nPatternLength[MICROPROFILE_MAX_FILTER];
	int nMaxFilter;

	uint32_t nModuleFilterMatch[MICROPROFILE_INSTRUMENT_MAX_MODULES]; // prematch the modules, so it can be skipped during search
	uint32_t nMask[MICROPROFILE_MAX_FILTER];						  // masks for subpatterns skipped
	MicroProfileStringMatchMask MatchMask[MICROPROFILE_MAX_FILTER];	  // masks for subpatterns skipped

	// results
	MicroProfileSymbolDesc* Results[MICROPROFILE_MAX_QUERY_RESULTS];
	uint32_t nNumResults;
	char FilterString[MICROPROFILE_MAX_FILTER_STRING];

	uint32_t QueryId;
};

MicroProfileFunctionQuery* MicroProfileAllocFunctionQuery()
{
	MicroProfileScopeLock L(MicroProfileMutex());
	MicroProfileFunctionQuery* pQ = nullptr;
	S.nNumQueryAllocated++;
	if(S.pQueryFreeList != 0)
	{
		pQ = S.pQueryFreeList;
		S.pQueryFreeList = pQ->pNext;
		S.nNumQueryFree--;
	}
	else
	{
		pQ = MP_ALLOC_OBJECT(MicroProfileFunctionQuery);
		MICROPROFILE_COUNTER_ADD("MicroProfile/Symbols/FunctionQuery", 1);
		MICROPROFILE_COUNTER_ADD("MicroProfile/Symbols/FunctionQueryMem", sizeof(MicroProfileFunctionQuery));
		S.nNumQueryAllocated++;
	}
	memset(pQ, 0, sizeof(MicroProfileFunctionQuery));
	return pQ;
}
void MicroProfileFreeFunctionQuery(MicroProfileFunctionQuery* pQ)
{
	pQ->pNext = S.pQueryFreeList;
	S.pQueryFreeList = pQ;
}

void MicroProfileProcessQuery(MicroProfileFunctionQuery* pQuery)
{
	MicroProfileFunctionQuery& Q = *pQuery;

	int nCnt = 0;
	(void)nCnt;

	int nBlocksTested = 0, nSymbolsTested = 0, nStringsTested = 0, nStringsTested0 = 0;
	int nBlocks = 0;

	int64_t t = MP_TICK();
	int64_t tt = 0;

	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		int nModule = i;
		uint32_t nModuleMatchOffset = Q.nModuleFilterMatch[nModule];
		MicroProfileSymbolBlock* pSymbols = S.SymbolModules[nModule].pSymbolBlock;

		uint32_t nMaskQ = Q.nMask[nModuleMatchOffset];
		MicroProfileStringMatchMask& MatchMaskQ = Q.MatchMask[nModuleMatchOffset];
		{
			while(pSymbols && 0 == S.pPendingQuery && Q.nNumResults < MICROPROFILE_MAX_QUERY_RESULTS)
			{

				MICROPROFILE_SCOPEI("MicroProfile", "SymbolQueryLoop", MP_YELLOW);
				nBlocks++;
				if(MicroProfileCharacterMatch(pSymbols->MatchMask, MatchMaskQ))
				{
					nBlocksTested++;
					for(uint32_t i = 0; i < pSymbols->nNumSymbols && 0 == S.pPendingQuery && Q.nNumResults < MICROPROFILE_MAX_QUERY_RESULTS; ++i)
					{
						MicroProfileSymbolDesc& E = pSymbols->Symbols[i];
						if(0 == E.nIgnoreSymbol)
						{
							nSymbolsTested++;
							if(nMaskQ == (nMaskQ & E.nMask))
							{
								nStringsTested++;
								MP_ASSERT((int)E.nModule < S.SymbolNumModules);
								if(MicroProfileStringMatch(E.pShortName, nModuleMatchOffset, &Q.pFilterStrings[0], Q.nPatternLength, Q.nMaxFilter))
								{
									if(Q.nNumResults < MICROPROFILE_MAX_QUERY_RESULTS)
									{

										Q.Results[Q.nNumResults++] = &E;
										if(Q.nNumResults == MICROPROFILE_MAX_QUERY_RESULTS)
											tt = MP_TICK();
									}
								}
								if(Q.nNumResults < MICROPROFILE_MAX_QUERY_RESULTS)
									nStringsTested0++;
							}
						}
					}
				}
				pSymbols = pSymbols->pNext;
			}
		}
	}
	int64_t tend = MP_TICK();
	float ToMS = MicroProfileTickToMsMultiplierCpu();
	float TIME = (tend - t) * ToMS;
	float TIME0 = (tt - t) * ToMS;
	(void)TIME;
	(void)TIME0;
	uprintf(" %6.3fms [%6.3f]: %5d/%5d blocks tested. %5d symbols %5d/%5d string compares\n", TIME, TIME0, nBlocksTested, nBlocks, nSymbolsTested, nStringsTested, nStringsTested0);
}

void* MicroProfileQueryThread(void* p)
{
	MicroProfileOnThreadCreate("MicroProfileSymbolThread");
	{
		while(1)
		{
			MicroProfileSleep(100); // todo:: use an event instead
			MicroProfileScopeLock L(MicroProfileMutex());
			if(S.pPendingQuery != nullptr)
			{
				MICROPROFILE_SCOPEI("MicroProfile", "SymbolQuery", MP_WHEAT);
				MicroProfileFunctionQuery* pQuery = S.pPendingQuery;

				MP_ASSERT(pQuery->QueryId > S.nQueryProcessed);
				S.pPendingQuery = 0;
				L.Unlock();

				// uprintf("processing query %d\n", pQuery->QueryId);
				MicroProfileProcessQuery(pQuery);

				L.Lock();
				S.nQueryProcessed = MicroProfileMax(pQuery->QueryId, S.nQueryProcessed);

				pQuery->pNext = S.pFinishedQuery;
				S.pFinishedQuery = pQuery;
			}
			if(S.SymbolState.nModuleLoadsRequested.load() != S.SymbolState.nModuleLoadsFinished.load())
			{
				L.Unlock();
				MicroProfileSymbolInitializeInternal();
				L.Lock();
			}
		}

		S.SymbolThreadFinished = 1;
	}
	MicroProfileOnThreadExit();
	return 0;
}

void MicroProfileQueryJoinThread()
{
	if(S.SymbolThreadFinished)
	{
		MicroProfileThreadJoin(&S.SymbolThread);
		S.SymbolThreadFinished = 0;
		S.SymbolThreadRunning = 0;
	}
}
void MicroProfileSymbolKickThread()
{
	// MicroProfileQueryJoinThread();
	if(S.SymbolThreadRunning == 0)
	{
		S.SymbolThreadRunning = 1;
		MicroProfileThreadStart(&S.SymbolThread, MicroProfileQueryThread);
	}
}
#if MICROPROFILE_WEBSERVER
void MicroProfileSymbolSendFunctionNames(MpSocket Connection)
{
	if(S.WSFunctionsInstrumentedSent < S.DynamicTokenIndex)
	{
		MicroProfileWSPrintStart(Connection);
		MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":[", MSG_FUNCTION_NAMES);
		bool bFirst = true;
		for(uint32_t i = S.WSFunctionsInstrumentedSent; i < S.DynamicTokenIndex; ++i)
		{
			const char* pString = S.FunctionsInstrumentedName[i];
			const char* pModuleString = S.FunctionsInstrumentedModuleNames[i];
			const char* pUnmangled = S.FunctionsInstrumentedUnmangled[i];
			MicroProfileWSPrintf(bFirst ? "[\"%s\",\"%s\",\"%s\"]" : ",[\"%s\",\"%s\",\"%s\"]", pString, pModuleString, pUnmangled);
			bFirst = false;
		}
		MicroProfileWSPrintf("]}");
		MicroProfileWSFlush();
		MicroProfileWSPrintEnd();

		S.WSFunctionsInstrumentedSent = S.DynamicTokenIndex;
	}
}

void MicroProfileSymbolSendErrors(MpSocket Connection)
{
	if(S.nNumPatchErrors)
	{
		MicroProfileWSPrintStart(Connection);
		MicroProfileWSPrintf("{\"k\":\"%d\",\"v\":{\"version\":\"%d.%d\",\"data\":[", MSG_INSTRUMENT_ERROR, MICROPROFILE_MAJOR_VERSION, MICROPROFILE_MINOR_VERSION);
		bool bFirst = true;
		for(int i = 0; i < S.nNumPatchErrors; ++i)
		{
			MicroProfilePatchError& E = S.PatchErrors[i];
			(void)E;
			if(!bFirst)
				MicroProfileWSPrintf(",");
			MicroProfileWSPrintf("{\"code\":\"");
			for(int i = 0; i < E.nCodeSize; ++i)
				MicroProfileWSPrintf("%02x", E.Code[i] & 0xff);
			MicroProfileWSPrintf("\",\"message\":\"%s\",\"already\":%d}", &E.Message[0], E.AlreadyInstrumented);
			bFirst = false;
		}

		MicroProfileWSPrintf("],\"functions\":[");
		bFirst = true;
		for(int i = 0; i < S.nNumPatchErrorFunctions; ++i)
		{
			if(!bFirst)
				MicroProfileWSPrintf(",");

			MicroProfileWSPrintf("\"%s\"", S.PatchErrorFunctionNames[i]);

			bFirst = false;
		}

		MicroProfileWSPrintf("]}}");

		MicroProfileWSFlush();
		MicroProfileWSPrintEnd();

		S.nNumPatchErrors = 0;
		S.nNumPatchErrorFunctions = 0;
	}
}

void MicroProfileSymbolQuerySendResult(MpSocket Connection)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSymbolQuerySendResult", MP_PINK2);
	MicroProfileFunctionQuery* pQuery = 0;
	{
		MicroProfileScopeLock L(MicroProfileMutex());

		uint32_t nBest = 0;

		while(S.pFinishedQuery != nullptr)
		{
			if(!pQuery)
			{
				pQuery = S.pFinishedQuery;
				nBest = pQuery->QueryId;
				S.pFinishedQuery = pQuery->pNext;
			}
			else
			{
				MicroProfileFunctionQuery* pQ = S.pFinishedQuery;
				S.pFinishedQuery = pQ->pNext;
				if(pQ->QueryId > nBest)
				{
					MicroProfileFreeFunctionQuery(pQuery);
					nBest = pQ->QueryId;
					pQuery = pQ;
				}
				else
				{
					MicroProfileFreeFunctionQuery(pQ);
				}
			}
		}
	}

	if(pQuery)
	{
		uprintf("Sending result for query %d\n", pQuery->QueryId);
		MicroProfileWSPrintStart(Connection);
		MicroProfileWSPrintf("{\"k\":\"%d\",\"q\":%d,\"v\":[", MSG_FUNCTION_RESULTS, pQuery->QueryId);
		bool bFirst = true;
		for(uint32_t i = 0; i < pQuery->nNumResults; ++i)
		{
			MicroProfileSymbolDesc& E = *pQuery->Results[i];
			if(bFirst)
			{
				MicroProfileWSPrintf("{\"a\":\"%p\",\"n\":\"%s\",\"sn\":\"%s\",\"m\":\"%s\"}", E.nAddress, E.pName, E.pShortName, MicroProfileSymbolModuleGetString(E.nModule));
				bFirst = false;
			}
			else
			{
				MicroProfileWSPrintf(",{\"a\":\"%p\",\"n\":\"%s\",\"sn\":\"%s\",\"m\":\"%s\"}", E.nAddress, E.pName, E.pShortName, MicroProfileSymbolModuleGetString(E.nModule));
			}
		}
		MicroProfileWSPrintf("]}");
		MicroProfileWSFlush();
		MicroProfileWSPrintEnd();

		MicroProfileScopeLock L(MicroProfileMutex());
		MicroProfileFreeFunctionQuery(pQuery);
	}
}
#endif

void MicroProfileSymbolQueryFunctions(MpSocket Connection, const char* pFilter)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSymbolQueryFunctions", MP_WHEAT);

	if(!MicroProfileSymbolInitialize(false))
	{
		return;
	}
	{
		int QueryId = atoi(pFilter);
		pFilter = strchr(pFilter, 'x');
		pFilter++;
		MicroProfileScopeLock L(MicroProfileMutex());
		if(0 == S.pPendingQuery || S.pPendingQuery->QueryId < (uint32_t)QueryId)
		{
			MicroProfileFunctionQuery* pQuery = S.pPendingQuery;
			if(!pQuery)
			{
				S.pPendingQuery = pQuery = MicroProfileAllocFunctionQuery();
			}
			MP_ASSERT(pQuery->pNext == 0);
			memset(pQuery, 0, sizeof(*pQuery));

			MicroProfileFunctionQuery& Q = *pQuery;
			Q.QueryId = QueryId;

			uint32_t nLen = (uint32_t)strlen(pFilter) + 1;
			if(nLen >= MICROPROFILE_MAX_FILTER_STRING)
				nLen = MICROPROFILE_MAX_FILTER_STRING - 1;

			memcpy(Q.FilterString, pFilter, nLen);
			Q.FilterString[nLen] = '\0';

			char* pBuffer = Q.FilterString;
			bool bStartString = true;
			for(uint32_t i = 0; i < nLen; ++i)
			{
				char c = pBuffer[i];
				if(c == '\0')
				{
					break;
				}
				if(isspace(c) || c == '*')
				{
					pBuffer[i] = '\0';
					bStartString = true;
				}
				else
				{
					if(bStartString)
					{
						if(Q.nMaxFilter < MICROPROFILE_MAX_FILTER)
						{
							const char* pstr = &pBuffer[i];
							Q.nMask[Q.nMaxFilter] = MicroProfileCharacterMaskString(pstr);
							MicroProfileCharacterMaskString2(pstr, Q.MatchMask[Q.nMaxFilter]);
							Q.pFilterStrings[Q.nMaxFilter++] = &pBuffer[i];
						}
					}
					bStartString = false;
				}
			}
			memset(Q.nModuleFilterMatch, 0xff, sizeof(Q.nModuleFilterMatch));
			for(int i = 0; i < S.SymbolNumModules; ++i)
			{
				Q.nModuleFilterMatch[i] = MicroProfileStringMatchOffset(MicroProfileSymbolModuleGetString(i), Q.pFilterStrings, Q.nPatternLength, Q.nMaxFilter);
			}

#if 0
			uprintf("query %d::",QueryId);
			for(int i = 0; i < Q.nMaxFilter; ++i)
			{
				Q.nPatternLength[i] = (uint32_t)strlen(Q.pFilterStrings[i]);
				uprintf("'%s' ", Q.pFilterStrings[i]);
			}
			uprintf("\n");
#endif
		}
	}
	MicroProfileSymbolKickThread();
}

#if defined(_WIN32)
// '##::::'##::'#######:::'#######::'##:::'##::::'##:::::'##:'####:'##::: ##::'#######:::'#######::
//  ##:::: ##:'##.... ##:'##.... ##: ##::'##::::: ##:'##: ##:. ##:: ###:: ##:'##.... ##:'##.... ##:
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:'##:::::: ##: ##: ##:: ##:: ####: ##:..::::: ##:..::::: ##:
//  #########: ##:::: ##: ##:::: ##: #####::::::: ##: ##: ##:: ##:: ## ## ##::'#######:::'#######::
//  ##.... ##: ##:::: ##: ##:::: ##: ##. ##:::::: ##: ##: ##:: ##:: ##. ####::...... ##:'##::::::::
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:. ##::::: ##: ##: ##:: ##:: ##:. ###:'##:::: ##: ##::::::::
//  ##:::: ##:. #######::. #######:: ##::. ##::::. ###. ###::'####: ##::. ##:. #######:: #########:
// ..:::::..:::.......::::.......:::..::::..::::::...::...:::....::..::::..:::.......:::.........::

#ifdef _WIN32
static void* MicroProfileAllocExecutableMemory(void* pBase, size_t s);
static void* MicroProfileAllocExecutableMemoryFar(size_t s);
static void MicroProfileMakeMemoryExecutable(void* p, size_t s);
static void MicroProfileMakeWriteable(void* p_, size_t size, DWORD* oldFlags);
static void MicroProfileRestore(void* p_, size_t size, DWORD* oldFlags);

extern "C" void microprofile_tramp_enter_patch();
extern "C" void microprofile_tramp_enter();
extern "C" void microprofile_tramp_code_begin();
extern "C" void microprofile_tramp_code_end();
extern "C" void microprofile_tramp_intercept0();
extern "C" void microprofile_tramp_end();
extern "C" void microprofile_tramp_exit();
extern "C" void microprofile_tramp_leave();
extern "C" void microprofile_tramp_trunk();
extern "C" void microprofile_tramp_call_patch_pop();
extern "C" void microprofile_tramp_call_patch_push();

bool MicroProfilePatchFunction(void* f, int Argument, MicroProfileHookFunc enter, MicroProfileHookFunc leave, MicroProfilePatchError* pError)
{
	char* pOriginal = (char*)f;

	f = MicroProfileX64FollowJump(f);

	intptr_t t_enter = (intptr_t)microprofile_tramp_enter;
	intptr_t t_enter_patch_offset = (intptr_t)microprofile_tramp_enter_patch - t_enter;
	intptr_t t_code_begin_offset = (intptr_t)microprofile_tramp_code_begin - t_enter;
	intptr_t t_code_end_offset = (intptr_t)microprofile_tramp_code_end - t_enter;
	intptr_t t_code_intercept0_offset = (intptr_t)microprofile_tramp_intercept0 - t_enter;
	intptr_t t_code_exit_offset = (intptr_t)microprofile_tramp_exit - t_enter;
	intptr_t t_code_leave_offset = (intptr_t)microprofile_tramp_leave - t_enter;

	intptr_t t_code_call_patch_push_offset = (intptr_t)microprofile_tramp_call_patch_push - t_enter;
	intptr_t t_code_call_patch_pop_offset = (intptr_t)microprofile_tramp_call_patch_pop - t_enter;
	intptr_t codemaxsize = t_code_end_offset - t_code_begin_offset;
	intptr_t t_end_offset = (intptr_t)microprofile_tramp_end - t_enter;
	intptr_t t_trunk_offset = (intptr_t)microprofile_tramp_trunk - t_enter;
	int t_trunk_size = (int)((intptr_t)microprofile_tramp_end - (intptr_t)microprofile_tramp_trunk);

	char* ptramp = (char*)MicroProfileAllocExecutableMemory(f, t_end_offset);
	if(!ptramp)
		ptramp = (char*)MicroProfileAllocExecutableMemoryFar(t_end_offset);

	intptr_t offset = ((intptr_t)f + 6 - (intptr_t)ptramp);

	uint32_t nBytesToCopy = 14;
	if(offset < 0x80000000 && offset > -0x7fffffff)
	{
		/// offset is small enough to insert a relative jump
		nBytesToCopy = 5;
	}

	memcpy(ptramp, (void*)t_enter, t_end_offset);

	int nInstructionBytesDest = 0;
	char* pInstructionMoveDest = ptramp + t_code_begin_offset;
	char* pTrunk = ptramp + t_trunk_offset;

	int nInstructionBytesSrc = 0;
	uint32_t nRegsWritten = 0;
	uint32_t nRetSafe = 1;
	uint32_t nUsableJumpRegs = (1 << R_RAX) | (1 << R_R10) | (1 << R_R11);
	static_assert(R_RAX == 0, "R_RAX must be 0");
	if(!MicroProfileCopyInstructionBytes(
		   pInstructionMoveDest, f, nBytesToCopy, (int)codemaxsize, pTrunk, t_trunk_size, nUsableJumpRegs, &nInstructionBytesDest, &nInstructionBytesSrc, &nRegsWritten, &nRetSafe))
	{
		if(pError)
		{
			const char* pCode = (const char*)f;
			memset(pError->Code, 0, sizeof(pError->Code));
			memcpy(pError->Code, pCode, nInstructionBytesSrc);
			int off = stbsp_snprintf(pError->Message, sizeof(pError->Message), "Failed to move %d code bytes ", nInstructionBytesSrc);
			pError->nCodeSize = nInstructionBytesSrc;
			for(int i = 0; i < nInstructionBytesSrc; ++i)
			{
				off += stbsp_snprintf(off + pError->Message, sizeof(pError->Message) - off, "%02x ", 0xff & pCode[i]);
			}
			uprintf("%s\n", pError->Message);
		}
		return false;
	}

	intptr_t phome = nInstructionBytesSrc + (intptr_t)f;
	uint32_t reg = nUsableJumpRegs & ~nRegsWritten;
	if(0 == reg)
	{
		if(0 == nRetSafe)
			MP_BREAK(); // should be caught earlier
		MicroProfileInsertRetJump(pInstructionMoveDest + nInstructionBytesDest, phome);
	}
	else
	{
		int r = R_RAX;
		while((reg & 1) == 0)
		{
			reg >>= 1;
			r++;
		}
		MicroProfileInsertRegisterJump(pInstructionMoveDest + nInstructionBytesDest, phome, r);
	}

	// PATCH 1 TRAMP EXIT
	intptr_t microprofile_tramp_exit = (intptr_t)ptramp + t_code_exit_offset;
	memcpy(ptramp + t_enter_patch_offset + 2, (void*)&microprofile_tramp_exit, 8);

	char* pintercept = t_code_intercept0_offset + ptramp;

	// PATCH 1.5 Argument
	memcpy(pintercept - 4, (void*)&Argument, 4);

	// PATCH 2 INTERCEPT0
	intptr_t addr = (intptr_t)enter; //&intercept0;
	memcpy(pintercept + 2, (void*)&addr, 8);

	// PATHC 2.5 argument
	memcpy(ptramp + t_code_exit_offset + 3, (void*)&Argument, 4);

	intptr_t microprofile_tramp_leave = (intptr_t)ptramp + t_code_leave_offset;
	// PATCH 3 INTERCEPT1
	intptr_t addr1 = (intptr_t)leave; //&intercept1;
	memcpy((char*)microprofile_tramp_leave + 2, (void*)&addr1, 8);

	intptr_t patch_push_addr = (intptr_t)(&MicroProfile_Patch_TLS_PUSH);
	intptr_t patch_pop_addr = (intptr_t)(&MicroProfile_Patch_TLS_POP);
	memcpy((char*)ptramp + t_code_call_patch_push_offset + 2, &patch_push_addr, 8);
	memcpy((char*)ptramp + t_code_call_patch_pop_offset + 2, &patch_pop_addr, 8);
	MicroProfileMakeMemoryExecutable(ptramp, t_end_offset);

	{
		// PATCH 4 DEST FUNC

		DWORD OldFlags[2] = { 0 };
		MicroProfileMakeWriteable(f, nInstructionBytesSrc, OldFlags);
		char* pp = (char*)f;
		char* ppend = pp + nInstructionBytesSrc;

		if(nInstructionBytesSrc < 14)
		{
			pp = MicroProfileInsertRelativeJump((char*)pp, (intptr_t)ptramp);
		}
		else
		{
			pp = MicroProfileInsertRegisterJump((char*)pp, (intptr_t)ptramp, R_RAX);
		}

		while(pp != ppend)
		{
			char c = (unsigned char)0x90;
			MP_ASSERT((unsigned char)c == (unsigned char)0x90);
			*pp++ = (unsigned char)0x90;
		}
		MicroProfileRestore(f, nInstructionBytesSrc, OldFlags);
	}
	return true;
}

static void MicroProfileMakeWriteable(void* p_, size_t s, DWORD* oldFlags)
{
	static uint64_t nPageSize = 4 << 10;

	intptr_t aligned = (intptr_t)p_;
	aligned = (aligned & (~(nPageSize - 1)));
	intptr_t aligned_end = (intptr_t)p_;
	aligned_end += s;
	aligned_end = (aligned_end + nPageSize - 1) & (~(nPageSize - 1));
	uint32_t nNumPages = (uint32_t)((aligned_end - aligned) / nPageSize);
	MP_ASSERT(nNumPages >= 1 && nNumPages <= 2);
	for(uint32_t i = 0; i < nNumPages; ++i)
	{
		if(!VirtualProtect((void*)(aligned + nPageSize * i), nPageSize, PAGE_EXECUTE_READWRITE, oldFlags + i))
		{
			MP_BREAK();
		}
	}
	//*(unsigned char*)p_ = 0x90;
}

static void MicroProfileRestore(void* p_, size_t s, DWORD* oldFlags)
{
	static uint64_t nPageSize = 4 << 10;

	intptr_t aligned = (intptr_t)p_;
	aligned = (aligned & (~(nPageSize - 1)));
	intptr_t aligned_end = (intptr_t)p_;
	aligned_end += s;
	aligned_end = (aligned_end + nPageSize - 1) & (~(nPageSize - 1));
	uint32_t nNumPages = (uint32_t)((aligned_end - aligned) / nPageSize);
	DWORD Dummy;
	for(uint32_t i = 0; i < nNumPages; ++i)
	{
		if(!VirtualProtect((void*)(aligned + nPageSize * i), nPageSize, oldFlags[i], &Dummy))
		{
			MP_BREAK();
		}
	}
}

void* MicroProfileAllocExecutableMemoryUp(intptr_t nBase, size_t s)
{
	intptr_t nEnd = nBase + 0x80000000;
	MEMORY_BASIC_INFORMATION mbi;
	while(nBase < nEnd)
	{
		if(!VirtualQuery((void*)nBase, &mbi, sizeof(mbi)))
		{
			return 0;
		}
		if(mbi.State == MEM_FREE && mbi.RegionSize >= s)
		{
			void* pMemory = VirtualAlloc((void*)nBase, s, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if(pMemory)
			{
				return pMemory;
			}
		}
		nBase = (intptr_t)mbi.BaseAddress + mbi.RegionSize;
		nBase = (nBase + s - 1) & ~(s - 1);
	}
	return 0;
}

void* MicroProfileAllocExecutableMemoryDown(intptr_t nBase, size_t s)
{
	intptr_t nEnd = nBase - 0x80000000;
	MEMORY_BASIC_INFORMATION mbi;
	while(nBase > nEnd)
	{
		nBase &= ~(s - 1);
		if(!VirtualQuery((void*)nBase, &mbi, sizeof(mbi)))
		{
			return 0;
		}
		if(mbi.State == MEM_FREE && mbi.RegionSize >= s)
		{
			void* pMemory = VirtualAlloc((void*)nBase, s, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if(pMemory)
			{
				return pMemory;
			}
		}
		nBase = (intptr_t)mbi.BaseAddress - s;
	}
	return 0;
}

static void* MicroProfileAllocExecutableMemory(void* pBase, size_t s)
{
	s = (s + 4095) & ~(4095);
	intptr_t nBase = (intptr_t)pBase;
	void* pResult = 0;
	if(0 == pResult && nBase > 0x40000000)
	{
		pResult = MicroProfileAllocExecutableMemoryDown(nBase - 0x40000000, s);
		if(0 == pResult)
		{
			pResult = MicroProfileAllocExecutableMemoryUp(nBase - 0x40000000, s);
		}
	}
	if(0 == pResult && nBase < 0xffffffff40000000)
	{
		pResult = MicroProfileAllocExecutableMemoryUp(nBase + 0x40000000, s);
		if(0 == pResult)
		{
			pResult = MicroProfileAllocExecutableMemoryUp(nBase + 0x40000000, s);
		}
	}
	return pResult;
}
static void* MicroProfileAllocExecutableMemoryFar(size_t s)
{
	static uint64_t nPageSize = 4 << 10;
	s = (s + (nPageSize - 1)) & (~(nPageSize - 1));

	void* pMem = VirtualAlloc(0, s, MEM_COMMIT, PAGE_READWRITE);
	MP_ASSERT(pMem);

	// uprintf("Allocating %zu  %p\n", s, pMem);
	return pMem;
}
static void MicroProfileMakeMemoryExecutable(void* p, size_t s)
{
	static uint64_t nPageSize = 4 << 10;
	s = (s + (nPageSize - 1)) & (~(nPageSize - 1));
	DWORD Unused;
	if(!VirtualProtect(p, s, PAGE_EXECUTE_READ, &Unused))
	{
		MP_BREAK();
	}
}
#endif

int MicroProfileTrimFunctionName(const char* pStr, char* pOutBegin, char* pOutEnd)
{
	const char* pStart = pOutBegin;
	int l = (int)strlen(pStr) - 1;
	int sz = 0;
	pOutEnd--;
	if(l < 1024 && pOutBegin != pOutEnd)
	{
		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd && pOutBegin != pOutEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*pOutBegin++ = c;
				sz++;
			}
		}

		*pOutBegin++ = '\0';
	}
	return sz;
}

int MicroProfileFindFunctionName(const char* pStr, const char** ppStart)
{
	int l = (int)strlen(pStr) - 1;
	if(l < 1024)
	{
		char b[1024] = { 0 };
		char* put = &b[0];

		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*put++ = c;
			}
		}

		*put++ = '\0';
		uprintf("trimmed %s\n", b);
	}

	// int nFirstParen = l;
	int nNumParen = 0;
	int c = 0;

	while(l >= 0 && pStr[l] != ')' && c++ < sizeof(" const") - 1)
	{
		l--;
	}
	if(pStr[l] == ')')
	{
		do
		{
			if(pStr[l] == ')')
			{
				nNumParen++;
			}
			else if(pStr[l] == '(')
			{
				nNumParen--;
			}
			l--;
		} while(nNumParen > 0 && l >= 0);
	}
	else
	{
		*ppStart = pStr;
		return 0;
	}
	while(l >= 0 && isspace(pStr[l]))
	{
		--l;
	}
	int nLast = l;
	while(l >= 0 && !isspace(pStr[l]))
	{
		l--;
	}
	int nFirst = l;
	if(nFirst == nLast)
		return 0;
	int nCount = nLast - nFirst + 1;
	*ppStart = pStr + nFirst;
	return nCount;
}

#include <dbghelp.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winternl.h>
struct MicroProfileQueryContext
{

	const char* pFilterStrings[MICROPROFILE_MAX_FILTER];
	uint32_t nPatternLength[MICROPROFILE_MAX_FILTER];
	int nMaxFilter = 0;
	char TempBuffer[128];
	uint32_t size = 0;
	bool bFirst = false;
};

BOOL CALLBACK MicroProfileEnumModules(_In_ PCTSTR ModuleName, _In_ DWORD64 BaseOfDll, _In_opt_ PVOID UserContext)
{

	MODULEINFO MI;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)BaseOfDll, &MI, sizeof(MI));
	MEMORY_BASIC_INFORMATION B;
	int r = VirtualQuery((LPCVOID)BaseOfDll, (MEMORY_BASIC_INFORMATION*)&B, sizeof(B));
	char buffer[1024];
	int r1 = GetLastError();
	if(r == 0)
	{
		stbsp_snprintf(buffer, sizeof(buffer) - 1, "Error %d\n", r1);
		OutputDebugString(buffer);
		MP_BREAK();
	}
	MicroProfileSymbolInitModule(ModuleName, BaseOfDll, BaseOfDll + MI.SizeOfImage);
	return true;
}

namespace
{
struct QueryCallbackBase // fucking c++, this is a pain in the ass
{
	virtual void CB(const char* pName, const char* pShortName, intptr_t addr, intptr_t addrend, uint32_t nModuleId) = 0;
};
template <typename T>
struct QueryCallbackImpl : public QueryCallbackBase
{
	T t;
	QueryCallbackImpl(T t)
		: t(t)
	{
	}
	virtual void CB(const char* pName, const char* pShortName, intptr_t addr, intptr_t addrend, uint32_t nModuleId)
	{
		t(pName, pShortName, addr, addrend, nModuleId);
	}
};
} // namespace

static uint32_t nLastModuleIdWin32 = (uint32_t)-1;
static intptr_t nLastModuleBaseWin32 = (intptr_t)-1;

BOOL MicroProfileQueryContextEnumSymbols(_In_ PSYMBOL_INFO pSymInfo, _In_ ULONG SymbolSize, _In_opt_ PVOID UserContext)
{
	uint32_t nModuleId = nLastModuleIdWin32;
	if(nLastModuleBaseWin32 != (intptr_t)pSymInfo->ModBase)
	{
		nLastModuleIdWin32 = nModuleId = MicroProfileSymbolGetModule((const char*)(intptr_t)-2, pSymInfo->ModBase);
		nLastModuleBaseWin32 = (intptr_t)pSymInfo->ModBase;
	}

	if(pSymInfo->Tag == 5 || pSymInfo->Tag == 10)
	{

		char FunctionName[1024];
		int ret = 0;
		int l = MicroProfileTrimFunctionName(pSymInfo->Name, &FunctionName[0], &FunctionName[1024]);
		QueryCallbackBase* pCB = (QueryCallbackBase*)UserContext;

		pCB->CB(pSymInfo->Name, l ? &FunctionName[0] : 0, (intptr_t)pSymInfo->Address, pSymInfo->Size + (intptr_t)pSymInfo->Address, nModuleId);
	}
	return TRUE;
};

template <typename Callback>
void MicroProfileIterateSymbols(Callback CB, uint32_t* nModules, uint32_t nNumModules)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileIterateSymbols", MP_PINK3);
	QueryCallbackImpl<Callback> Context(CB);
	if(MicroProfileSymInit())
	{
		// uprintf("symbols loaded!\n");
		// API_VERSION* pv = ImagehlpApiVersion();
		// uprintf("VERSION %d.%d.%d\n", pv->MajorVersion, pv->MinorVersion, pv->Revision);

		nLastModuleBaseWin32 = -1;
		if(SymEnumerateModules64(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK64)MicroProfileEnumModules, NULL))
		{
		}
		QueryCallbackBase* pBase = &Context;
		if(nNumModules)
		{
			HANDLE hProcess = GetCurrentProcess();
			char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
			PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
			uint64_t t0 = MP_TICK();
			for(uint32_t i = 0; i < nNumModules; ++i)
			{
				uint32_t nModule = nModules[i];
				int64_t nBytes = 0;
				MEMORY_BASIC_INFORMATION B;

				for(int j = 0; j < S.SymbolModules[nModule].nNumExecutableRegions; ++j)
				{
					intptr_t b = S.SymbolModules[nModule].Regions[j].nBegin;
					intptr_t e = S.SymbolModules[nModule].Regions[j].nEnd;
					while(b < e)
					{
						int r = VirtualQuery((LPCVOID)b, &B, sizeof(B));
						if(!r)
							break;
						switch(B.Protect)
						{
						case PAGE_EXECUTE:
						case PAGE_EXECUTE_READ:
						case PAGE_EXECUTE_READWRITE:
						case PAGE_EXECUTE_WRITECOPY:
							nBytes += B.RegionSize;
							// uprintf("RANGE %p, %p .. %5.2fkb %08x, %08x\n", B.BaseAddress, (void*)(intptr_t(B.BaseAddress) + B.RegionSize), B.RegionSize / 1024.f, B.State, B.Protect);
						}
						b = intptr_t(B.BaseAddress) + B.RegionSize;
					}
				}
				S.SymbolModules[nModule].nProgressTarget = nBytes;
				//				uprintf("ITERATE MODULES %p :: %s\n", S.SymbolModules[nModule].nAddrBegin, (const char*)S.SymbolModules[nModule].pBaseString);
				for(int j = 0; j < S.SymbolModules[nModule].nNumExecutableRegions; ++j)
				{
					if(SymEnumSymbols(GetCurrentProcess(), S.SymbolModules[nModule].Regions[j].nBegin, "*", MicroProfileQueryContextEnumSymbols, pBase))
					{
						uprintf("SymEnumSymbols Succeeds!\n");
					}
				}
				S.SymbolModules[nModule].nProgress = S.SymbolModules[nModule].nProgressTarget;
				S.SymbolModules[nModule].nModuleLoadFinished.exchange(1);
			}

			uint64_t t1 = MP_TICK();
			float fTime = float(MicroProfileTickToMsMultiplierCpu()) * (t1 - t0);
			uprintf("load symbol time %6.2fms\n", fTime);
		}
		else
		{
			uprintf("Enumerating all modules\n");
			if(SymEnumSymbols(GetCurrentProcess(), 0, "*!*", MicroProfileQueryContextEnumSymbols, pBase))
			{
				uprintf("SymEnumSymbols Succeeds!\n");
			}
		}
		MicroProfileSymCleanup();
	}
}

static int MicroProfileWin32SymInitCount = 0;
static int MicroProfileWin32SymInitSuccess = 0;

bool MicroProfileSymInit()
{
	if(0 == MicroProfileWin32SymInitCount++)
	{
		auto h = GetCurrentProcess();
		SymCleanup(h);
		// SymSetOptions( SYMOPT_DEBUG | SYMOPT_DEFERRED_LOADS);
		SymSetOptions(SYMOPT_UNDNAME);
		if(SymInitialize(h, 0, TRUE))
		{
			MicroProfileWin32SymInitSuccess = 1;
		}
		else
		{
			MicroProfileWin32SymInitSuccess = 0;
		}
	}
	return MicroProfileWin32SymInitSuccess != 0;
}
void MicroProfileSymCleanup()
{
	if(0 == --MicroProfileWin32SymInitCount)
	{
		MicroProfileWin32SymInitSuccess = 0;
		SymCleanup(GetCurrentProcess());
	}
}
const char* MicroProfileGetUnmangledSymbolName(void* pFunction)
{
	HANDLE h = GetCurrentProcess();
	const char* pStr = 0;
	if(MicroProfileSymInit())
	{

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;
		DWORD64 off = 0;
		if(SymFromAddr(h, (DWORD64)pFunction, &off, pSymbol))
		{
			pStr = MicroProfileStrDup(pSymbol->Name);
			OutputDebugStringA(pStr);
			OutputDebugStringA(" <<< \n");
		}
		else
		{
			DWORD error = GetLastError();
			uprintf("SymFromAddr returned error : %d\n", error);
		}
		MicroProfileSymCleanup();
	}
	if(!pStr)
	{
		OutputDebugStringA("MISSING!!!!\n");
		return MicroProfileStrDup("??");
	}
	else
	{
		return pStr;
	}
}

static void* g_pFunctionFoundHack = 0;
static const char* g_pFunctionpNameFound = 0;
static char g_Demangled[512];

BOOL MicroProfileQueryContextEnumSymbols1(_In_ PSYMBOL_INFO pSymInfo, _In_ ULONG SymbolSize, _In_opt_ PVOID UserContext)
{
	if(pSymInfo->Tag == 5 || pSymInfo->Tag == 10)
	{
		char str[200];
		stbsp_snprintf(str, sizeof(str) - 1, "%s : %p\n", pSymInfo->Name, (void*)pSymInfo->Address);
		OutputDebugStringA(str);
		g_pFunctionpNameFound = pSymInfo->Name;
		g_pFunctionFoundHack = (void*)pSymInfo->Address;
		return FALSE;
	}
	return TRUE;
};

const char* MicroProfileDemangleSymbol(const char* pSymbol)
{
	return pSymbol; // todo: for some reasons all symbols im seaing right now are already undecorated?
}

void MicroProfileInstrumentWithoutSymbols(const char** pModules, const char** pSymbols, uint32_t nNumSymbols)
{
	char SymString[512];
	const char* pStr = 0;
	if(MicroProfileSymInit())
	{
		HANDLE h = GetCurrentProcess();
		for(uint32_t i = 0; i < nNumSymbols; ++i)
		{
			int nCount = stbsp_snprintf(SymString, sizeof(SymString) - 1, "%s!%s", pModules[i], pSymbols[i]);
			if(nCount <= sizeof(SymString) - 1)
			{
				g_pFunctionFoundHack = 0;
				if(SymEnumSymbols(h, 0, SymString, MicroProfileQueryContextEnumSymbols1, 0))
				{
					if(g_pFunctionFoundHack)
					{
						uint32_t nColor = MicroProfileColorFromString(pSymbols[i]);
						const char* pDemangled = pSymbols[i]; // MicroProfileDemangleSymbol(pSymbols[i]);
						MicroProfileInstrumentFunction(g_pFunctionFoundHack, pModules[i], pDemangled, nColor);
					}
				}
			}
		}
		MicroProfileSymCleanup();
	}
}

void MicroProfileSymbolUpdateModuleList()
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileSymbolUpdateModuleList", MP_PINK3);
	//	QueryCallbackImpl<Callback> Context(CB);
	if(MicroProfileSymInit())
	{
		uprintf("symbols loaded!\n");
		API_VERSION* pv = ImagehlpApiVersion();
		uprintf("VERSION %d.%d.%d\n", pv->MajorVersion, pv->MinorVersion, pv->Revision);

		nLastModuleBaseWin32 = -1;
		if(SymEnumerateModules64(GetCurrentProcess(), (PSYM_ENUMMODULES_CALLBACK64)MicroProfileEnumModules, NULL))
		{
			uprintf("SymEnumerateModules64 Succeeds!\n");
		}
		MicroProfileSymCleanup();
	}
}

#endif

#if defined(__APPLE__) && defined(__MACH__)
// '##::::'##::'#######:::'#######::'##:::'##:::::'#######:::'######::'##::::'##:
//  ##:::: ##:'##.... ##:'##.... ##: ##::'##:::::'##.... ##:'##... ##:. ##::'##::
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:'##:::::: ##:::: ##: ##:::..:::. ##'##:::
//  #########: ##:::: ##: ##:::: ##: #####::::::: ##:::: ##:. ######::::. ###::::
//  ##.... ##: ##:::: ##: ##:::: ##: ##. ##:::::: ##:::: ##::..... ##::: ## ##:::
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:. ##::::: ##:::: ##:'##::: ##:: ##:. ##::
//  ##:::: ##:. #######::. #######:: ##::. ##::::. #######::. ######:: ##:::. ##:
// ..:::::..:::.......::::.......:::..::::..::::::.......::::......:::..:::::..::

#include <cxxabi.h>
#include <distorm.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mnemonics.h>
#include <sys/mman.h>
#include <unistd.h>

static void* MicroProfileAllocExecutableMemory(void* f, size_t s);
static void MicroProfileMakeWriteable(void* p_);

extern "C" void microprofile_tramp_enter_patch();
extern "C" void microprofile_tramp_enter();
extern "C" void microprofile_tramp_code_begin();
extern "C" void microprofile_tramp_code_end();
extern "C" void microprofile_tramp_intercept0();
extern "C" void microprofile_tramp_end();
extern "C" void microprofile_tramp_exit();
extern "C" void microprofile_tramp_leave();
extern "C" void microprofile_tramp_trunk();
extern "C" void microprofile_tramp_call_patch_pop();
extern "C" void microprofile_tramp_call_patch_push();

bool MicroProfilePatchFunction(void* f, int Argument, MicroProfileHookFunc enter, MicroProfileHookFunc leave, MicroProfilePatchError* pError) __attribute__((optnone))
{
	if(pError)
	{
		memcpy(&pError->Code[0], f, 12);
	}

	intptr_t t_enter = (intptr_t)microprofile_tramp_enter;
	intptr_t t_enter_patch_offset = (intptr_t)microprofile_tramp_enter_patch - t_enter;
	intptr_t t_code_begin_offset = (intptr_t)microprofile_tramp_code_begin - t_enter;
	intptr_t t_code_end_offset = (intptr_t)microprofile_tramp_code_end - t_enter;
	intptr_t t_code_intercept0_offset = (intptr_t)microprofile_tramp_intercept0 - t_enter;
	intptr_t t_code_exit_offset = (intptr_t)microprofile_tramp_exit - t_enter;
	intptr_t t_code_leave_offset = (intptr_t)microprofile_tramp_leave - t_enter;

	intptr_t t_code_call_patch_push_offset = (intptr_t)microprofile_tramp_call_patch_push - t_enter;
	intptr_t t_code_call_patch_pop_offset = (intptr_t)microprofile_tramp_call_patch_pop - t_enter;
	intptr_t codemaxsize = t_code_end_offset - t_code_begin_offset;
	intptr_t t_end_offset = (intptr_t)microprofile_tramp_end - t_enter;
	intptr_t t_trunk_offset = (intptr_t)microprofile_tramp_trunk - t_enter;
	intptr_t t_trunk_size = (intptr_t)microprofile_tramp_end - (intptr_t)microprofile_tramp_trunk;

	char* ptramp = (char*)MicroProfileAllocExecutableMemory(f, t_end_offset);

	intptr_t offset = ((intptr_t)f + 6 - (intptr_t)ptramp);

	uint32_t nBytesToCopy = 14;
	if(offset < 0x80000000 && offset > -0x7fffffff)
	{
		/// offset is small enough to insert a relative jump
		nBytesToCopy = 5;
	}

	memcpy(ptramp, (void*)t_enter, t_end_offset);

	int nInstructionBytesDest = 0;
	char* pInstructionMoveDest = ptramp + t_code_begin_offset;
	char* pTrunk = ptramp + t_trunk_offset;

	int nInstructionBytesSrc = 0;

	uint32_t nRegsWritten = 0;
	uint32_t nRetSafe = 0;
	uint32_t nUsableJumpRegs = (1 << R_RAX) | (1 << R_R10) | (1 << R_R11); // scratch && !parameter register
	if(!MicroProfileCopyInstructionBytes(
		   pInstructionMoveDest, f, nBytesToCopy, codemaxsize, pTrunk, t_trunk_size, nUsableJumpRegs, &nInstructionBytesDest, &nInstructionBytesSrc, &nRegsWritten, &nRetSafe))
	{
		if(pError)
		{
			const char* pCode = (const char*)f;
			memset(pError->Code, 0, sizeof(pError->Code));
			memcpy(pError->Code, pCode, nInstructionBytesSrc);
			int off = stbsp_snprintf(pError->Message, sizeof(pError->Message), "Failed to move %d code bytes ", nInstructionBytesSrc);
			pError->nCodeSize = nInstructionBytesSrc;
			for(int i = 0; i < nInstructionBytesSrc; ++i)
			{
				off += stbsp_snprintf(off + pError->Message, sizeof(pError->Message) - off, "%02x ", 0xff & pCode[i]);
			}
			uprintf("%s\n", pError->Message);
		}
		return false;
	}
	intptr_t phome = nInstructionBytesSrc + (intptr_t)f;
	uint32_t reg = nUsableJumpRegs & ~nRegsWritten;
	static_assert(R_RAX == 0, "R_RAX must be 0");
	if(0 == reg)
	{
		if(nRetSafe == 0)
		{
			MP_BREAK(); // shout fail earlier
		}
		MicroProfileInsertRetJump(pInstructionMoveDest + nInstructionBytesDest, phome);
	}
	else
	{
		int r = R_RAX;
		while((reg & 1) == 0)
		{
			reg >>= 1;
			r++;
		}
		MicroProfileInsertRegisterJump(pInstructionMoveDest + nInstructionBytesDest, phome, r);
	}

	// PATCH 1 TRAMP EXIT
	intptr_t microprofile_tramp_exit = (intptr_t)ptramp + t_code_exit_offset;
	memcpy(ptramp + t_enter_patch_offset + 2, (void*)&microprofile_tramp_exit, 8);

	char* pintercept = t_code_intercept0_offset + ptramp;

	// PATCH 1.5 Argument
	memcpy(pintercept - 4, (void*)&Argument, 4);

	// PATCH 2 INTERCEPT0
	intptr_t addr = (intptr_t)enter; //&intercept0;
	memcpy(pintercept + 2, (void*)&addr, 8);

	// PATHC 2.5 argument
	memcpy(ptramp + t_code_exit_offset + 3, (void*)&Argument, 4);

	intptr_t microprofile_tramp_leave = (intptr_t)ptramp + t_code_leave_offset;
	// PATCH 3 INTERCEPT1
	intptr_t addr1 = (intptr_t)leave; //&intercept1;
	memcpy((char*)microprofile_tramp_leave + 2, (void*)&addr1, 8);

	intptr_t patch_push_addr = (intptr_t)(&MicroProfile_Patch_TLS_PUSH);
	intptr_t patch_pop_addr = (intptr_t)(&MicroProfile_Patch_TLS_POP);
	memcpy((char*)ptramp + t_code_call_patch_push_offset + 2, &patch_push_addr, 8);
	memcpy((char*)ptramp + t_code_call_patch_pop_offset + 2, &patch_pop_addr, 8);

	{
		// PATCH 4 DEST FUNC

		MicroProfileMakeWriteable(f);
		char* pp = (char*)f;
		char* ppend = pp + nInstructionBytesSrc;
		if(nInstructionBytesSrc < 14)
		{
			uprintf("inserting 5b jump\n");
			pp = MicroProfileInsertRelativeJump((char*)pp, (intptr_t)ptramp);
		}
		else
		{
			uprintf("inserting 14b jump\n");
			pp = MicroProfileInsertRegisterJump(pp, (intptr_t)ptramp, R_RAX);
		}
		while(pp != ppend)
		{
			*pp++ = 0x90;
		}
	}
	return true;
}

static void MicroProfileMakeWriteable(void* p_)
{
#ifdef _PATCH_TEST
	// for testing..
	static const uint32_t WritableSize = 16;
	static uint32_t WritableCount = 0;
	static intptr_t WritableStart[WritableSize] = { 0 };
	static intptr_t WritableEnd[WritableSize] = { 0 };
	for(uint32_t i = 0; i < WritableCount; ++i)
	{
		intptr_t x = (intptr_t)p_;
		if(x >= WritableStart[i] && x < WritableEnd[i])
		{
			return;
		}
	}

#endif

	intptr_t p = (intptr_t)p_;
	// uprintf("MicroProfilemakewriteable %lx\n", p);
	mach_port_name_t task = mach_task_self();
	vm_map_offset_t vmoffset = 0;
	mach_vm_size_t vmsize = 0;
	uint32_t nd;
	kern_return_t kr;
	vm_region_submap_info_64 vbr;
	mach_msg_type_number_t vbrcount = sizeof(vbr) / 4;

	while(KERN_SUCCESS == (kr = mach_vm_region_recurse(task, &vmoffset, &vmsize, &nd, (vm_region_recurse_info_t)&vbr, &vbrcount)))
	{
		if(p >= (intptr_t)vmoffset && p <= intptr_t(vmoffset + vmsize))
		{
			if(0 == (vbr.protection & VM_PROT_WRITE))
			{
				// uprintf("region match .. enabling write\n");
				int x = mprotect((void*)vmoffset, vmsize, PROT_WRITE | PROT_READ | PROT_EXEC);
				if(x)
				{
					// uprintf("mprotect failed ... err %d:: %d   %s\n", errno, x, strerror(errno));
				}
				else
				{
					uprintf("region is [%llx,%llx] .. %08llx  %d", vmoffset, vmoffset + vmsize, vmsize, vbr.is_submap);
					uprintf("prot: %c%c%c  %c%c%c\n",
							vbr.protection & VM_PROT_READ ? 'r' : '-',
							vbr.protection & VM_PROT_WRITE ? 'w' : '-',
							vbr.protection & VM_PROT_EXECUTE ? 'x' : '-',

							vbr.max_protection & VM_PROT_READ ? 'r' : '-',
							vbr.max_protection & VM_PROT_WRITE ? 'w' : '-',
							vbr.max_protection & VM_PROT_EXECUTE ? 'x' : '-');
					continue;
				}
			}
			else
			{
#ifdef _PATCH_TEST
				if(WritableCount < WritableSize)
				{
					WritableStart[WritableCount] = vmoffset;
					WritableEnd[WritableCount] = vmoffset + vmsize;
					WritableCount++;
				}

#endif
			}
		}

		vmoffset += vmsize;
		vbrcount = sizeof(vbr) / 4;
	}
}

int MicroProfileTrimFunctionName(const char* pStr, char* pOutBegin, char* pOutEnd)
{
	int l = strlen(pStr) - 1;
	int sz = 0;
	pOutEnd--;
	if(l < pOutEnd - pOutBegin && pOutBegin != pOutEnd)
	{
		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd && pOutBegin != pOutEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*pOutBegin++ = c;
				sz++;
			}
		}

		*pOutBegin++ = '\0';
	}
	return sz;
}

int MicroProfileFindFunctionName(const char* pStr, const char** ppStart)
{
	int l = strlen(pStr) - 1;
	if(l < 1024)
	{
		char b[1024] = { 0 };
		char* put = &b[0];

		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*put++ = c;
			}
		}

		*put++ = '\0';
		uprintf("trimmed %s\n", b);
	}

	// int nFirstParen = l;
	int nNumParen = 0;
	int c = 0;

	while(l >= 0 && pStr[l] != ')' && c++ < (int)(sizeof(" const") - 1))
	{
		l--;
	}
	if(pStr[l] == ')')
	{
		do
		{
			if(pStr[l] == ')')
			{
				nNumParen++;
			}
			else if(pStr[l] == '(')
			{
				nNumParen--;
			}
			l--;
		} while(nNumParen > 0 && l >= 0);
	}
	else
	{
		*ppStart = pStr;
		return 0;
	}
	while(l >= 0 && isspace(pStr[l]))
	{
		--l;
	}
	int nLast = l;
	while(l >= 0 && !isspace(pStr[l]))
	{
		l--;
	}
	int nFirst = l;
	if(nFirst == nLast)
		return 0;
	int nCount = nLast - nFirst + 1;
	*ppStart = pStr + nFirst;
	return nCount;
}

const char* MicroProfileDemangleSymbol(const char* pSymbol)
{
	static unsigned long size = 128;
	static char* pTempBuffer = (char*)malloc(size); // needs to be malloc because demangle function might realloc it.
	unsigned long len = size;
	int ret = 0;
	char* pBuffer = pTempBuffer;
	pBuffer = abi::__cxa_demangle(pSymbol, pTempBuffer, &len, &ret);
	if(ret == 0)
	{
		if(pBuffer != pTempBuffer)
		{
			pTempBuffer = pBuffer;
			if(len < size)
				__builtin_trap();
			size = len;
		}
		return pTempBuffer;
	}
	else
	{
		return pSymbol;
	}
}

template <typename Callback>
void MicroProfileIterateSymbols(Callback CB, uint32_t* nModules, uint32_t nNumModules)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileIterateSymbols", MP_PINK3);
	char FunctionName[1024];
	(void)FunctionName;
	mach_port_name_t task = mach_task_self();
	vm_map_offset_t vmoffset = 0;
	mach_vm_size_t vmsize = 0;
	uint32_t nd;
	kern_return_t kr;
	vm_region_submap_info_64 vbr;
	mach_msg_type_number_t vbrcount = sizeof(vbr) / 4;

	intptr_t nCurrentModule = -1;
	uint32_t nCurrentModuleId = -1;

	auto OnFunction = [&](void* addr, void* addrend, const char* pSymbol, const char* pModuleName, void* pModuleAddr) -> bool {
		const char* pStr = MicroProfileDemangleSymbol(pSymbol);
		;
		int l = MicroProfileTrimFunctionName(pStr, &FunctionName[0], &FunctionName[1024]);
		if(nCurrentModule != (intptr_t)pModuleAddr)
		{
			nCurrentModule = (intptr_t)pModuleAddr;
			nCurrentModuleId = MicroProfileSymbolGetModule(pModuleName, nCurrentModule);
		}

		CB(l ? &FunctionName[0] : pStr, l ? &FunctionName[0] : 0, (intptr_t)addr, (intptr_t)addrend, nCurrentModuleId);
		return true;
	};
	vm_offset_t addr_prev = 0;

	while(KERN_SUCCESS == (kr = mach_vm_region_recurse(task, &vmoffset, &vmsize, &nd, (vm_region_recurse_info_t)&vbr, &vbrcount)))
	{
		{
			addr_prev = vmoffset + vmsize;
			if(0 != (vbr.protection & VM_PROT_EXECUTE))
			{
				bool bProcessModule = true;
				int nModule = -1;
				if(nNumModules)
				{
					bProcessModule = false;
					for(uint32_t i = 0; i < nNumModules; ++i)
					{
						intptr_t nBase = S.SymbolModules[nModules[i]].Regions[0].nBegin;
						if((intptr_t)vmoffset == nBase)
						{
							bProcessModule = true;
							nModule = nModules[i];
							break;
						}
					}
				}
				if(bProcessModule)
				{
					S.SymbolModules[nModule].nProgressTarget = S.SymbolModules[nModule].Regions[0].nEnd - S.SymbolModules[nModule].Regions[0].nBegin;
					dl_info di;
					int r = 0;
					r = dladdr((void*)vmoffset, &di);
					if(r)
					{
						OnFunction(di.dli_saddr, (void*)addr_prev, di.dli_sname, di.dli_fname, di.dli_fbase);
					}
					intptr_t addr = vmoffset + vmsize - 1;
					while(1)
					{
						r = dladdr((void*)(addr), &di);
						if(r)
						{
							if(!di.dli_sname)
							{
								break;
							}
							OnFunction(di.dli_saddr, (void*)addr_prev, di.dli_sname, di.dli_fname, di.dli_fbase);
						}
						else
						{
							break;
						}
						addr_prev = (vm_offset_t)di.dli_saddr;
						addr = (intptr_t)di.dli_saddr - 1;
						if(di.dli_saddr < (void*)vmoffset)
						{
							break;
						}
					}
					for(int i = 0; i < S.SymbolNumModules; ++i)
					{
						if(S.SymbolModules[i].Regions[0].nBegin == (intptr_t)vmoffset)
						{
							S.SymbolModules[i].nModuleLoadFinished.store(1);
						}
					}
				}
			}
		}
		vmoffset += vmsize;
		vbrcount = sizeof(vbr) / 4;
	}
}

void MicroProfileSymbolUpdateModuleList()
{
	char FunctionName[1024];
	(void)FunctionName;
	mach_port_name_t task = mach_task_self();
	vm_map_offset_t vmoffset = 0;
	mach_vm_size_t vmsize = 0;
	uint32_t nd;
	kern_return_t kr;
	vm_region_submap_info_64 vbr;
	mach_msg_type_number_t vbrcount = sizeof(vbr) / 4;

	while(KERN_SUCCESS == (kr = mach_vm_region_recurse(task, &vmoffset, &vmsize, &nd, (vm_region_recurse_info_t)&vbr, &vbrcount)))
	{
		{
			if(0 != (vbr.protection & VM_PROT_EXECUTE))
			{
				dl_info di;
				int r = 0;
				r = dladdr((void*)vmoffset, &di);
				if(r)
				{
					uprintf("[0x%p-0x%p] (0x%p) %s %s\n", (void*)vmoffset, (void*)addr_prev, di.dli_fbase, di.dli_fname, di.dli_sname);
					MicroProfileSymbolInitModule(di.dli_fname, (intptr_t)vmoffset, (intptr_t)vmoffset + vmsize);
				}
			}
		}
		vmoffset += vmsize;
		vbrcount = sizeof(vbr) / 4;
	}
}

static void* MicroProfileAllocExecutableMemory(void* f, size_t s)
{
	static uint64_t nPageSize = 0;
	if(!nPageSize)
	{
		nPageSize = getpagesize();
	}
	s = (s + (nPageSize - 1)) & (~(nPageSize - 1));

	void* pMem = mmap((void*)f, s, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);

	// uprintf("Allocating %zu  %p\n", s, pMem);
	return pMem;
}

const char* MicroProfileGetUnmangledSymbolName(void* pFunction)
{
	dl_info di;
	int r = 0;
	r = dladdr(pFunction, &di);
	if(r)
	{
		return MicroProfileStrDup(di.dli_sname);
	}
	else
	{
		return 0;
	}
}

void MicroProfileInstrumentWithoutSymbols(const char** pModules, const char** pSymbols, uint32_t nNumSymbols)
{
	void* M = dlopen(0, 0);
	for(uint32_t i = 0; i < nNumSymbols; ++i)
	{
		// uprintf("trying to find symbol %s\n", pSym);
		void* s = dlsym(M, pSymbols[i]);
		uprintf("sym returned %p\n", s);
		if(s)
		{
			uint32_t nColor = MicroProfileColorFromString(pSymbols[i]);
			const char* pDemangled = MicroProfileDemangleSymbol(pSymbols[i]);
			MicroProfileInstrumentFunction(s, pModules[i], pDemangled, nColor);
		}
	}
	dlclose(M);
}
#endif

#if defined(__unix__) && defined(__x86_64__)
// '##::::'##::'#######:::'#######::'##:::'##::::'##:::::::'####:'##::: ##:'##::::'##:'##::::'##:
//  ##:::: ##:'##.... ##:'##.... ##: ##::'##::::: ##:::::::. ##:: ###:: ##: ##:::: ##:. ##::'##::
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:'##:::::: ##:::::::: ##:: ####: ##: ##:::: ##::. ##'##:::
//  #########: ##:::: ##: ##:::: ##: #####::::::: ##:::::::: ##:: ## ## ##: ##:::: ##:::. ###::::
//  ##.... ##: ##:::: ##: ##:::: ##: ##. ##:::::: ##:::::::: ##:: ##. ####: ##:::: ##::: ## ##:::
//  ##:::: ##: ##:::: ##: ##:::: ##: ##:. ##::::: ##:::::::: ##:: ##:. ###: ##:::: ##:: ##:. ##::
//  ##:::: ##:. #######::. #######:: ##::. ##:::: ########:'####: ##::. ##:. #######:: ##:::. ##:
// ..:::::..:::.......::::.......:::..::::..:::::........::....::..::::..:::.......:::..:::::..::

#include <cxxabi.h>
#include <distorm.h>
#include <dlfcn.h>
#include <mnemonics.h>
#include <sys/mman.h>
#include <unistd.h>

static void* MicroProfileAllocExecutableMemory(void* f, size_t s);
static void MicroProfileMakeWriteable(void* p_);

extern "C" void microprofile_tramp_enter_patch() asm("_microprofile_tramp_enter_patch");
extern "C" void microprofile_tramp_enter() asm("_microprofile_tramp_enter");
extern "C" void microprofile_tramp_code_begin() asm("_microprofile_tramp_code_begin");
extern "C" void microprofile_tramp_code_end() asm("_microprofile_tramp_code_end");
extern "C" void microprofile_tramp_intercept0() asm("_microprofile_tramp_intercept0");
extern "C" void microprofile_tramp_end() asm("_microprofile_tramp_end");
extern "C" void microprofile_tramp_exit() asm("_microprofile_tramp_exit");
extern "C" void microprofile_tramp_leave() asm("_microprofile_tramp_leave");
extern "C" void microprofile_tramp_trunk() asm("_microprofile_tramp_trunk");
extern "C" void microprofile_tramp_call_patch_pop() asm("_microprofile_tramp_call_patch_pop");
extern "C" void microprofile_tramp_call_patch_push() asm("_microprofile_tramp_call_patch_push");

bool MicroProfilePatchFunction(void* f, int Argument, MicroProfileHookFunc enter, MicroProfileHookFunc leave, MicroProfilePatchError* pError)
{
	if(pError)
	{
		memcpy(&pError->Code[0], f, 12);
	}

	intptr_t t_enter = (intptr_t)microprofile_tramp_enter;
	intptr_t t_enter_patch_offset = (intptr_t)microprofile_tramp_enter_patch - t_enter;
	intptr_t t_code_begin_offset = (intptr_t)microprofile_tramp_code_begin - t_enter;
	intptr_t t_code_end_offset = (intptr_t)microprofile_tramp_code_end - t_enter;
	intptr_t t_code_intercept0_offset = (intptr_t)microprofile_tramp_intercept0 - t_enter;
	intptr_t t_code_exit_offset = (intptr_t)microprofile_tramp_exit - t_enter;
	intptr_t t_code_leave_offset = (intptr_t)microprofile_tramp_leave - t_enter;

	intptr_t t_code_call_patch_push_offset = (intptr_t)microprofile_tramp_call_patch_push - t_enter;
	intptr_t t_code_call_patch_pop_offset = (intptr_t)microprofile_tramp_call_patch_pop - t_enter;
	intptr_t codemaxsize = t_code_end_offset - t_code_begin_offset;
	intptr_t t_end_offset = (intptr_t)microprofile_tramp_end - t_enter;
	intptr_t t_trunk_offset = (intptr_t)microprofile_tramp_trunk - t_enter;
	intptr_t t_trunk_size = (intptr_t)microprofile_tramp_end - (intptr_t)microprofile_tramp_trunk;

	char* ptramp = (char*)MicroProfileAllocExecutableMemory(f, t_end_offset);

	intptr_t offset = ((intptr_t)f + 6 - (intptr_t)ptramp);

	uint32_t nBytesToCopy = 14;
	if(offset < 0x80000000 && offset > -0x7fffffff)
	{
		/// offset is small enough to insert a relative jump
		nBytesToCopy = 5;
	}

	memcpy(ptramp, (void*)t_enter, t_end_offset);

	int nInstructionBytesDest = 0;
	char* pInstructionMoveDest = ptramp + t_code_begin_offset;
	char* pTrunk = ptramp + t_trunk_offset;

	int nInstructionBytesSrc = 0;

	uint32_t nRegsWritten = 0;
	uint32_t nRetSafe = 0;
	uint32_t nUsableJumpRegs = (1 << R_RAX) | (1 << R_R10) | (1 << R_R11); // scratch && !parameter register
	if(!MicroProfileCopyInstructionBytes(
		   pInstructionMoveDest, f, nBytesToCopy, codemaxsize, pTrunk, t_trunk_size, nUsableJumpRegs, &nInstructionBytesDest, &nInstructionBytesSrc, &nRegsWritten, &nRetSafe))
	{
		if(pError)
		{
			const char* pCode = (const char*)f;
			memset(pError->Code, 0, sizeof(pError->Code));
			memcpy(pError->Code, pCode, nInstructionBytesSrc);
			int off = stbsp_snprintf(pError->Message, sizeof(pError->Message), "Failed to move %d code bytes ", nInstructionBytesSrc);
			pError->nCodeSize = nInstructionBytesSrc;
			for(int i = 0; i < nInstructionBytesSrc; ++i)
			{
				off += stbsp_snprintf(off + pError->Message, sizeof(pError->Message) - off, "%02x ", 0xff & pCode[i]);
			}
			uprintf("%s\n", pError->Message);
		}
		return false;
	}
	intptr_t phome = nInstructionBytesSrc + (intptr_t)f;
	uint32_t reg = nUsableJumpRegs & ~nRegsWritten;
	static_assert(R_RAX == 0, "R_RAX must be 0");
	if(0 == reg)
	{
		if(nRetSafe == 0)
		{
			MP_BREAK(); // shout fail earlier
		}
		MicroProfileInsertRetJump(pInstructionMoveDest + nInstructionBytesDest, phome);
	}
	else
	{
		int r = R_RAX;
		while((reg & 1) == 0)
		{
			reg >>= 1;
			r++;
		}
		MicroProfileInsertRegisterJump(pInstructionMoveDest + nInstructionBytesDest, phome, r);
	}

	// PATCH 1 TRAMP EXIT
	intptr_t microprofile_tramp_exit = (intptr_t)ptramp + t_code_exit_offset;
	memcpy(ptramp + t_enter_patch_offset + 2, (void*)&microprofile_tramp_exit, 8);

	char* pintercept = t_code_intercept0_offset + ptramp;

	// PATCH 1.5 Argument
	memcpy(pintercept - 4, (void*)&Argument, 4);

	// PATCH 2 INTERCEPT0
	intptr_t addr = (intptr_t)enter; //&intercept0;
	memcpy(pintercept + 2, (void*)&addr, 8);

	// PATHC 2.5 argument
	memcpy(ptramp + t_code_exit_offset + 3, (void*)&Argument, 4);

	intptr_t microprofile_tramp_leave = (intptr_t)ptramp + t_code_leave_offset;
	// PATCH 3 INTERCEPT1
	intptr_t addr1 = (intptr_t)leave; //&intercept1;
	memcpy((char*)microprofile_tramp_leave + 2, (void*)&addr1, 8);

	intptr_t patch_push_addr = (intptr_t)(&MicroProfile_Patch_TLS_PUSH);
	intptr_t patch_pop_addr = (intptr_t)(&MicroProfile_Patch_TLS_POP);
	memcpy((char*)ptramp + t_code_call_patch_push_offset + 2, &patch_push_addr, 8);
	memcpy((char*)ptramp + t_code_call_patch_pop_offset + 2, &patch_pop_addr, 8);

	{
		// PATCH 4 DEST FUNC

		MicroProfileMakeWriteable(f);
		char* pp = (char*)f;
		char* ppend = pp + nInstructionBytesSrc;

		if(nInstructionBytesSrc < 14)
		{
			uprintf("inserting 5b jump\n");
			pp = MicroProfileInsertRelativeJump((char*)pp, (intptr_t)ptramp);
		}
		else
		{
			uprintf("inserting 14b jump\n");
			pp = MicroProfileInsertRegisterJump(pp, (intptr_t)ptramp, R_RAX);
		}
		while(pp != ppend)
		{
			*pp++ = 0x90;
		}
	}
	return true;
}

static void MicroProfileMakeWriteable(void* p_)
{
	intptr_t nPageSize = (intptr_t)getpagesize();
	intptr_t p = ((intptr_t)p_) & ~(nPageSize - 1);
	intptr_t e = nPageSize + ((14 + (intptr_t)p_) & ~(nPageSize - 1));
	size_t s = e - p;
	mprotect((void*)p, s, PROT_READ | PROT_WRITE | PROT_EXEC);
}

int MicroProfileTrimFunctionName(const char* pStr, char* pOutBegin, char* pOutEnd)
{
	int l = strlen(pStr) - 1;
	int sz = 0;
	pOutEnd--;
	if(l < pOutEnd - pOutBegin && pOutBegin != pOutEnd)
	{
		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd && pOutBegin != pOutEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*pOutBegin++ = c;
				sz++;
			}
		}

		*pOutBegin++ = '\0';
	}
	return sz;
}

int MicroProfileFindFunctionName(const char* pStr, const char** ppStart)
{
	int l = strlen(pStr) - 1;
	if(l < 1024)
	{
		char b[1024] = { 0 };
		char* put = &b[0];

		const char* p = pStr;
		const char* pEnd = pStr + l + 1;
		int in = 0;
		while(p != pEnd)
		{
			char c = *p++;
			if(c == '(' || c == '<')
			{
				in++;
			}
			else if(c == ')' || c == '>')
			{
				in--;
				continue;
			}

			if(in == 0)
			{
				*put++ = c;
			}
		}

		*put++ = '\0';
		uprintf("trimmed %s\n", b);
	}

	// int nFirstParen = l;
	int nNumParen = 0;
	int c = 0;

	while(l >= 0 && pStr[l] != ')' && c++ < (int)(sizeof(" const") - 1))
	{
		l--;
	}
	if(pStr[l] == ')')
	{
		do
		{
			if(pStr[l] == ')')
			{
				nNumParen++;
			}
			else if(pStr[l] == '(')
			{
				nNumParen--;
			}
			l--;
		} while(nNumParen > 0 && l >= 0);
	}
	else
	{
		*ppStart = pStr;
		return 0;
	}
	while(l >= 0 && isspace(pStr[l]))
	{
		--l;
	}
	int nLast = l;
	while(l >= 0 && !isspace(pStr[l]))
	{
		l--;
	}
	int nFirst = l;
	if(nFirst == nLast)
		return 0;
	int nCount = nLast - nFirst + 1;
	*ppStart = pStr + nFirst;
	return nCount;
}

const char* MicroProfileDemangleSymbol(const char* pSymbol)
{
	static unsigned long size = 128;
	static char* pTempBuffer = (char*)malloc(size); // needs to be malloc because demangle function might realloc it.
	unsigned long len = size;
	int ret = 0;
	char* pBuffer = pTempBuffer;
	pBuffer = abi::__cxa_demangle(pSymbol, pTempBuffer, &len, &ret);
	if(ret == 0)
	{
		if(pBuffer != pTempBuffer)
		{
			pTempBuffer = pBuffer;
			if(len < size)
				__builtin_trap();
			size = len;
		}
		return pTempBuffer;
	}
	else
	{
		return pSymbol;
	}
}

template <typename Callback>
void MicroProfileIterateSymbols(Callback CB, uint32_t* nModules, uint32_t nNumModules)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileIterateSymbols", MP_PINK3);
	char FunctionName[1024];

	intptr_t nCurrentModule = -1;
	uint32_t nCurrentModuleId = -1;

	auto OnFunction = [&](void* addr, void* addrend, const char* pSymbol, const char* pModuleName, void* pModuleAddr) -> bool {
		const char* pStr = MicroProfileDemangleSymbol(pSymbol);
		;
		int l = MicroProfileTrimFunctionName(pStr, &FunctionName[0], &FunctionName[1024]);
		MP_ASSERT(nCurrentModule == (intptr_t)pModuleAddr);
		CB(l ? &FunctionName[0] : pStr, l ? &FunctionName[0] : 0, (intptr_t)addr, (intptr_t)addrend, nCurrentModuleId);
		return true;
	};

	for(int i = 0; i < S.SymbolNumModules; ++i)
	{
		auto& M = S.SymbolModules[i];
		if(0 != nNumModules)
		{
			bool bProcess = false;
			for(uint32_t j = 0; j < nNumModules; ++j)
			{
				if(nModules[j] == (uint32_t)i)
				{
					bProcess = true;
					break;
				}
			}
			if(!bProcess)
				continue;
		}
		nCurrentModuleId = i;
		Dl_info di;
		int r = 0;
		r = dladdr((void*)(M.Regions[0].nBegin), &di);
		if(r)
		{
			nCurrentModule = (intptr_t)di.dli_fbase;
			M.nProgressTarget = 0;
			for(int j = 0; j < M.nNumExecutableRegions; ++j)
			{
				M.nProgressTarget += M.Regions[j].nEnd - M.Regions[j].nBegin;
			}
			for(int j = 0; j < M.nNumExecutableRegions; ++j)
			{
				const intptr_t nBegin = M.Regions[j].nBegin;
				const intptr_t nEnd = M.Regions[j].nEnd;
				int r = 0;
				intptr_t nAddr = (nEnd - 8) & ~7;
				intptr_t nAddrPrev = nEnd;
				while(1)
				{
					r = dladdr((void*)(nAddr), &di);
					if(r && di.dli_sname)
					{
						OnFunction(di.dli_saddr, (void*)nAddrPrev, di.dli_sname, di.dli_fname, di.dli_fbase);
						nAddrPrev = (intptr_t)di.dli_saddr;
						nAddr = (intptr_t)di.dli_saddr - 1;
					}
					else
					{
						nAddr = (nAddr - 7) & ~7; // pretty ineffecient, but it seems linux just returns 0 when there is no symbols, making this the only option I can come up with?
					}
					if(nAddr < nBegin)
					{
						break;
					}
				}
			}
			M.nProgress = M.nProgressTarget;
			M.nModuleLoadFinished.store(1);
		}
	}
}

void MicroProfileSymbolUpdateModuleList()
{
	// So, this was the only way I could find to do this..
	// Is this seriously how they want this to be done?
	FILE* F = fopen("/proc/self/maps", "r");
	char* line = 0;
	size_t len;
	ssize_t read;
	Dl_info di;
	while((read = getline(&line, &len, F)) != -1)
	{
		void* pBase = 0;
		void* pEnd = 0;
		char c, r, w, x, p;

		if(8 == sscanf(line, "%p%c%p%c%c%c%c%c", &pBase, &c, &pEnd, &c, &r, &w, &x, &p))
		{
			if('x' == x)
			{
				int r = 0;
				r = dladdr(pBase, &di);
				if(r)
				{
					if('[' != di.dli_fname[0])
					{
						MicroProfileSymbolInitModule(di.dli_fname, (intptr_t)pBase, (intptr_t)pEnd);
					}
				}
			}
		}
	}
	fclose(F);
	MicroProfileSymbolMergeExecutableRegions();
}

static void* MicroProfileAllocExecutableMemory(void* f, size_t s)
{
	static uint64_t nPageSize = 0;
	if(!nPageSize)
	{
		nPageSize = getpagesize();
	}
	s = (s + (nPageSize - 1)) & (~(nPageSize - 1));

	void* pMem = mmap(f, s, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
	return pMem;
}

const char* MicroProfileGetUnmangledSymbolName(void* pFunction)
{
	Dl_info di;
	int r = 0;
	r = dladdr(pFunction, &di);
	if(r)
	{
		return MicroProfileStrDup(di.dli_sname);
	}
	else
	{
		return 0;
	}
}

// not yet tested.
void MicroProfileInstrumentWithoutSymbols(const char** pModules, const char** pSymbols, uint32_t nNumSymbols)
{
	void* M = dlopen(0, 0);
	for(uint32_t i = 0; i < nNumSymbols; ++i)
	{
		// uprintf("trying to find symbol %s\n", pSym);
		void* s = dlsym(M, pSymbols[i]);
		uprintf("sym returned %p\n", s);
		if(s)
		{
			uint32_t nColor = MicroProfileColorFromString(pSymbols[i]);
			const char* pDemangled = MicroProfileDemangleSymbol(pSymbols[i]);
			MicroProfileInstrumentFunction(s, pModules[i], pDemangled, nColor);
		}
	}
	dlclose(M);
}

#endif

#endif

void MicroProfileHashTableInit(MicroProfileHashTable* pTable, uint32_t nInitialSize, uint32_t nSearchLimit, MicroProfileHashCompareFunction CompareFunc, MicroProfileHashFunction HashFunc)
{
	pTable->nAllocated = nInitialSize;
	pTable->nUsed = 0;
	uint32_t nSize = nInitialSize * sizeof(MicroProfileHashTableEntry);
	pTable->pEntries = (MicroProfileHashTableEntry*)MICROPROFILE_ALLOC(nSize, 8);
	pTable->CompareFunc = CompareFunc;
	pTable->HashFunc = HashFunc;
	pTable->nSearchLimit = nSearchLimit;
	pTable->nLim = pTable->nAllocated / 5;
	if(pTable->nLim > pTable->nSearchLimit)
		pTable->nLim = pTable->nSearchLimit;
	memset(pTable->pEntries, 0, nSize);
}
void MicroProfileHashTableDestroy(MicroProfileHashTable* pTable)
{
	MICROPROFILE_FREE(pTable->pEntries);
}

uint64_t MicroProfileHashTableHash(MicroProfileHashTable* pTable, uint64_t K)
{
	uint64_t H = pTable->HashFunc ? (*pTable->HashFunc)(K) : K;
	return H == 0 ? 1 : H;
}

void MicroProfileHashTableGrow(MicroProfileHashTable* pTable)
{
	uint32_t nAllocated = pTable->nAllocated;
	uint32_t nNewSize = nAllocated * 2;
	uprintf("GROW %d -> %d\n", nAllocated, nNewSize);

	MicroProfileHashTable New;
	MicroProfileHashTableInit(&New, nNewSize, pTable->nSearchLimit, pTable->CompareFunc, pTable->HashFunc);
	for(uint32_t i = 0; i < nAllocated; ++i)
	{
		MicroProfileHashTableEntry& E = pTable->pEntries[i];
		if(E.Hash != 0)
		{
			MicroProfileHashTableSet(&New, E.Key, E.Value, E.Hash, false);
		}
	}
	MicroProfileHashTableDestroy(pTable);
	*pTable = New;
}

bool MicroProfileHashTableSet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t Value)
{
	uint64_t H = MicroProfileHashTableHash(pTable, Key);
	return MicroProfileHashTableSet(pTable, Key, Value, H, true);
}

MicroProfileHashTableIterator MicroProfileGetHashTableIteratorBegin(MicroProfileHashTable* HashTable)
{
	return MicroProfileHashTableIterator(0, HashTable);
}

MicroProfileHashTableIterator MicroProfileGetHashTableIteratorEnd(MicroProfileHashTable* HashTable)
{
	return MicroProfileHashTableIterator(HashTable->nAllocated, HashTable);
}

bool MicroProfileHashTableSet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t Value, uint64_t H, bool bAllowGrow)
{
	if(H == 0)
		MP_BREAK(); // not supported.
	MicroProfileHashCompareFunction Cmp = pTable->CompareFunc;
	while(1)
	{
		const uint32_t nLim = pTable->nLim;
		uint32_t B = H % pTable->nAllocated;
		MicroProfileHashTableEntry* pEntries = pTable->pEntries;

		for(uint32_t i = 0; i < pTable->nAllocated; ++i)
		{
			uint32_t Idx = (B + i) % pTable->nAllocated;
			if(pEntries[Idx].Hash == 0)
			{
				pEntries[Idx].Hash = H;
				pEntries[Idx].Key = Key;
				pEntries[Idx].Value = Value;
				return true;
			}
			else if(pEntries[Idx].Hash == H && (Cmp ? (Cmp)(Key, pEntries[Idx].Key) : Key == pEntries[Idx].Key))
			{
				pEntries[Idx].Value = Value;
				return true;
			}
			else if(i > nLim)
			{
				break;
			}
		}
		if(bAllowGrow)
		{
			MicroProfileHashTableGrow(pTable);
		}
		else
		{
			MP_BREAK();
		}
	}
	MP_BREAK();
}

bool MicroProfileHashTableGet(MicroProfileHashTable* pTable, uint64_t Key, uintptr_t* pValue)
{
	uint64_t H = MicroProfileHashTableHash(pTable, Key);
	uint32_t B = H % pTable->nAllocated;
	MicroProfileHashTableEntry* pEntries = pTable->pEntries;
	MicroProfileHashCompareFunction Cmp = pTable->CompareFunc;
	for(uint32_t i = 0; i < pTable->nAllocated; ++i)
	{
		uint32_t Idx = (B + i) % pTable->nAllocated;
		if(pEntries[Idx].Hash == 0)
		{
			return false;
		}
		else if(pEntries[Idx].Hash == H && (Cmp ? (Cmp)(Key, pEntries[Idx].Key) : Key == pEntries[Idx].Key))
		{
			*pValue = pEntries[Idx].Value;
			return true;
		}
	}
	return false;
}

bool MicroProfileHashTableRemove(MicroProfileHashTable* pTable, uint64_t Key)
{

	uint64_t H = MicroProfileHashTableHash(pTable, Key);
	uint32_t B = H % pTable->nAllocated;
	MicroProfileHashTableEntry* pEntries = pTable->pEntries;
	MicroProfileHashCompareFunction Cmp = pTable->CompareFunc;
	uint32_t nBase = (uint32_t)-1;
	uint32_t nAllocated = pTable->nAllocated;
	for(uint32_t i = 0; i < nAllocated; ++i)
	{
		uint32_t Idx = (B + i) % nAllocated;
		if(pEntries[Idx].Hash == 0)
		{
			return false;
		}
		else if(pEntries[Idx].Hash == H && (Cmp ? (Cmp)(Key, pEntries[Idx].Key) : Key == pEntries[Idx].Key))
		{
			nBase = Idx;
			break;
		}
	}
	pEntries[nBase].Hash = 0;
	pEntries[nBase].Key = 0;
	pEntries[nBase].Value = 0;
	nBase++;
	for(uint32_t i = 0; i < nAllocated; ++i)
	{
		uint32_t Idx = (nBase + i) % nAllocated;
		if(pEntries[Idx].Hash == 0)
		{
			break;
		}
		else
		{
			MicroProfileHashTableEntry E = pEntries[Idx];
			pEntries[Idx] = {};
			MicroProfileHashTableSet(pTable, E.Key, E.Value, E.Hash, false);
		}
	}
	return true;
}
uint64_t MicroProfileHashTableHashString(uint64_t pString)
{
	return MicroProfileStringHash((const char*)pString);
}

bool MicroProfileHashTableCompareString(uint64_t L, uint64_t R)
{
	return 0 == strcmp((const char*)L, (const char*)R);
}

bool MicroProfileHashTableSetString(MicroProfileHashTable* pTable, const char* pKey, const char* pValue)
{
	return MicroProfileHashTableSet(pTable, (uint64_t)pKey, (uintptr_t)pValue);
}

bool MicroProfileHashTableGetString(MicroProfileHashTable* pTable, const char* pKey, const char** pValue)
{
	return MicroProfileHashTableGet(pTable, (uint64_t)pKey, (uintptr_t*)pValue);
}

bool MicroProfileHashTableRemoveString(MicroProfileHashTable* pTable, const char* pKey)
{
	return MicroProfileHashTableRemove(pTable, (uint64_t)pKey);
}

void MicroProfileStringBlockFree(MicroProfileStringBlock* pBlock)
{
	MicroProfileCounterAdd(S.CounterToken_StringBlock_Count, -1);
	MicroProfileCounterAdd(S.CounterToken_StringBlock_Memory, -(int64_t)(pBlock->nSize + sizeof(MicroProfileStringBlock)));

	MP_FREE(pBlock);
}
MicroProfileStringBlock* MicroProfileStringBlockAlloc(uint32_t nSize)
{
	nSize = MicroProfileMax(nSize, (uint32_t)(MicroProfileStringBlock::DEFAULT_SIZE - sizeof(MicroProfileStringBlock)));
	nSize += sizeof(MicroProfileStringBlock);
	MicroProfileCounterAdd(S.CounterToken_StringBlock_Count, 1);
	MicroProfileCounterAdd(S.CounterToken_StringBlock_Memory, nSize);
	// uprintf("alloc string block %d sizeof strings is %d\n", nSize, (int)sizeof(MicroProfileStringBlock));
	MicroProfileStringBlock* pBlock = (MicroProfileStringBlock*)MP_ALLOC(nSize, 8);
	pBlock->pNext = 0;
	pBlock->nSize = nSize - sizeof(MicroProfileStringBlock);
	pBlock->nUsed = 0;
	return pBlock;
}

void MicroProfileStringsInit(MicroProfileStrings* pStrings)
{
	MicroProfileHashTableInit(&pStrings->HashTable, 1, 25, MicroProfileHashTableCompareString, MicroProfileHashTableHashString);
	pStrings->pFirst = 0;
	pStrings->pLast = 0;
}
void MicroProfileStringsDestroy(MicroProfileStrings* pStrings)
{
	MicroProfileStringBlock* pBlock = pStrings->pFirst;
	while(pBlock)
	{
		MicroProfileStringBlock* pNext = pBlock->pNext;
		MicroProfileStringBlockFree(pBlock);
		pBlock = pNext;
	}
	MicroProfileCounterSet(S.CounterToken_StringBlock_Waste, 0);
	MicroProfileCounterSet(S.CounterToken_StringBlock_Strings, 0);

	memset(pStrings, 0, sizeof(*pStrings));
}

const char* MicroProfileStringIntern(const char* pStr)
{
	return MicroProfileStringIntern(pStr, (uint32_t)strlen(pStr), 0);
}

const char* MicroProfileStringInternLower(const char* pStr)
{
	return MicroProfileStringIntern(pStr, (uint32_t)strlen(pStr), ESTRINGINTERN_LOWERCASE);
}

const char* MicroProfileStringIntern(const char* pStr_, uint32_t nLen, uint32_t nFlags)
{
	MicroProfileStrings* pStrings = &S.Strings;
	const char* pStr = pStr_;
	char* pLowerCaseStr = (char*)alloca(nLen + 1);
	if(0 != (nFlags & ESTRINGINTERN_LOWERCASE))
	{
		for(uint32_t i = 0; i < nLen; ++i)
		{
			pLowerCaseStr[i] = tolower(pStr[i]);
		}
		pLowerCaseStr[nLen] = '\0';
		pStr = pLowerCaseStr;
	}
	const char* pRet;
	if(MicroProfileHashTableGetString(&pStrings->HashTable, pStr, &pRet))
	{
		if(0 != strcmp(pStr, pRet))
		{
			MP_BREAK();
		}
		return pRet;
	}
	else
	{
		if(pStr[nLen] != '\0')
			MP_BREAK(); // string should be 0 terminated.
		nLen += 1;
		MicroProfileStringBlock* pBlock = pStrings->pLast;
		if(0 == pBlock || pBlock->nUsed + nLen > pBlock->nSize)
		{
			MicroProfileStringBlock* pNewBlock = MicroProfileStringBlockAlloc(nLen);
			if(pBlock)
			{
				pBlock->pNext = pNewBlock;
				pStrings->pLast = pNewBlock;
				MicroProfileCounterAdd(S.CounterToken_StringBlock_Waste, pBlock->nSize - pBlock->nUsed);
			}
			else
			{
				pStrings->pLast = pStrings->pFirst = pNewBlock;
			}
			pBlock = pNewBlock;
		}
		MicroProfileCounterAdd(S.CounterToken_StringBlock_Strings, 1);
		char* pDest = &pBlock->Memory[pBlock->nUsed];
		pBlock->nUsed += nLen;
		MP_ASSERT(pBlock->nUsed <= pBlock->nSize);
		memcpy(pDest, pStr, nLen);
		MicroProfileHashTableSetString(&pStrings->HashTable, pDest, pDest);

#if 0
		void DumpTableStr(MicroProfileHashTable* pTable);
		DumpTableStr(&pStrings->HashTable);
#endif

		return pDest;
	}
}

void DumpTable(MicroProfileHashTable* pTable)
{
	for(uint32_t i = 0; i < pTable->nAllocated; ++i)
	{
		if(pTable->pEntries[i].Hash != 0)
		{
			uprintf("[%05d,%05" PRIu64 "]  ::::%" PRIx64 ", %p .. hash %" PRIx64 "\n",
					i,
					pTable->pEntries[i].Hash % pTable->nAllocated,
					pTable->pEntries[i].Key,
					(void*)pTable->pEntries[i].Value,
					pTable->pEntries[i].Hash);
		}
	}
};
void DumpTableStr(MicroProfileHashTable* pTable)
{
	int c = 0;
	(void)c;
	for(uint32_t i = 0; i < pTable->nAllocated; ++i)
	{
		if(pTable->pEntries[i].Hash != 0)
		{
			uprintf("%03d [%05d,%05" PRIu64 "]  ::::%s, %s .. hash %" PRIx64 "\n",
					c++,
					i,
					pTable->pEntries[i].Hash % pTable->nAllocated,
					(const char*)pTable->pEntries[i].Key,
					(const char*)pTable->pEntries[i].Value,
					pTable->pEntries[i].Hash);
		}
	}
	uprintf("FillPrc %f\n", 100.f * c / (float)pTable->nAllocated);
};

static const char* txt[] = { "gaudy",	 "chilly",	  "obtain",		"suspend",	  "jelly",		 "peel",	"nauseating", "complain", "cave",		"practise",		 "sail",	   "close",
							 "drawer",	 "mature",	  "impossible", "exist",	  "sister",		 "poke",	"ancient",	  "paddle",	  "ask",		"shallow",		 "outrageous", "healthy",
							 "reading",	 "obey",	  "water",		"elbow",	  "abnormal",	 "trap",	"wholesale",  "lovely",	  "stupid",		"comparison",	 "swim",	   "brash",
							 "towering", "accept",	  "invention",	"plantation", "spooky",		 "tiger",	"knot",		  "literate", "awake",		"itch",			 "medical",	   "ticket",
							 "tawdry",	 "correct",	  "mine",		"accidental", "dinner",		 "produce", "protective", "red",	  "dreary",		"toe",			 "drain",	   "zesty",
							 "inform",	 "boundless", "ghost",		"attend",	  "rely",		 "fill",	"liquid",	  "pump",	  "continue",	"spark",		 "church",	   "fortunate",
							 "truthful", "conscious", "possible",	"motion",	  "evanescent",	 "branch",	"skirt",	  "number",	  "meek",		"hour",			 "form",	   "work",
							 "car",		 "post",	  "talk",		"fear",		  "tightfisted", "dress",	"perform",	  "fry",	  "courageous", "dysfunctional", "page",	   "one",
							 "annoy",	 "abrasive",  "dependent",	"payment" };

void MicroProfileStringInternTest()
{
	MicroProfileStringsInit(&S.Strings);
	uint32_t nCount = sizeof(txt) / sizeof(txt[0]);
	const char* pStrings[100];
	const char* pStrings2[100];

	DumpTableStr(&S.Strings.HashTable);
	for(uint32_t i = 0; i < nCount; ++i)
	{
		pStrings[i] = MicroProfileStringIntern(txt[i]);
		pStrings2[i] = MicroProfileStrDup(txt[i]);
	}

	for(uint32_t i = 0; i < nCount; ++i)
	{
		const char* pStr = MicroProfileStringIntern(pStrings2[i]);
		if(pStr != pStrings[i])
		{
			MP_BREAK();
		}
	}
	DumpTableStr(&S.Strings.HashTable);

	MicroProfileStringsDestroy(&S.Strings);
}

void MicroProfileHashTableTest()
{
	MicroProfileStringInternTest();

	MicroProfileHashTable T;
	MicroProfileHashTable* pTable = &T;
	MicroProfileHashTableInit(pTable, 1, 100, 0, 0);

#define NUM_ITEMS 100

	uint64_t Keys[NUM_ITEMS];
	uint64_t Values[NUM_ITEMS];
	memset(Keys, 0xff, sizeof(Keys));
	memset(Values, 0xff, sizeof(Values));

	static int l = 0;
	auto RR = [&]() -> uint64_t {
		if(l++ % 4 < 2)
		{
			return l;
		}
		uint64_t l2 = rand();
		uint64_t u = rand();
		return l2 | (u << 32);
	};
	auto RRUnique = [&]() {
		bool bFound = false;
		uint64_t V = 0;
		do
		{
			V = RR();
			for(uint32_t i = 0; i != NUM_ITEMS; ++i)
			{
				if(V == Keys[i])
				{
					bFound = true;
				}
			}
			if(!bFound)
			{
				return V;
			}
		} while(bFound);
		MP_BREAK();
		return (uint64_t)0;
	};

	Keys[0] = 0;
	Values[0] = 42;
	for(uint32_t i = 1; i < NUM_ITEMS; ++i)
	{
		Keys[i] = RRUnique();
		Values[i] = RR();
	}

	for(uint32_t i = 0; i < NUM_ITEMS; ++i)
	{
		MicroProfileHashTableSet(pTable, Keys[i], Values[i]);
	}

	for(uint32_t i = 0; i < NUM_ITEMS; ++i)
	{
		uintptr_t V;
		if(MicroProfileHashTableGet(pTable, Keys[i], &V))
		{
			if(V != Values[i])
			{
				MP_BREAK();
			}
		}
		else
		{
			MP_BREAK();
		}
		uint64_t nonkey = RRUnique();
		if(MicroProfileHashTableGet(pTable, nonkey, &V))
		{
			MP_BREAK();
		}
	}

	DumpTable(pTable);
	if(!MicroProfileHashTableRemove(pTable, 0))
	{
		MP_BREAK();
	}
	uprintf("removed\n");
	DumpTable(pTable);
	uintptr_t v;
	if(MicroProfileHashTableGet(pTable, 0, &v))
	{
		MP_BREAK();
	}
	if(MicroProfileHashTableGet(pTable, 1, &v))
	{
		if(v != 2)
			MP_BREAK();
	}

	MicroProfileHashTableDestroy(pTable);

	MicroProfileHashTable Strings;
	MicroProfileHashTableInit(&Strings, 1, 25, MicroProfileHashTableCompareString, MicroProfileHashTableHashString);
	uint32_t nCount = sizeof(txt) / sizeof(txt[0]);
	for(uint32_t i = 0; i < nCount; i += 2)
	{
		MicroProfileHashTableSetString(&Strings, txt[i], txt[i + 1]);
	}
	DumpTableStr(&Strings);

	for(uint32_t i = 0; i < nCount; i += 2)
	{
		const char* pKey = txt[i];
		const char* pValue = txt[i + 1];
		const char* pRes = 0;
		if(MicroProfileHashTableGetString(&Strings, pKey, &pRes))
		{
			if(pRes != pValue)
			{
				MP_BREAK();
			}
		}
		else
		{
			MP_BREAK();
		}
	}
	uint32_t nRem = nCount / 2;
	for(uint32_t i = 0; i < nRem; i += 2)
	{
		const char* pKey = txt[i];
		const char* pValue = txt[i + 1];

		if(!MicroProfileHashTableRemoveString(&Strings, pKey))
		{
			MP_BREAK();
		}
		if(MicroProfileHashTableRemoveString(&Strings, pValue))
		{
			MP_BREAK();
		}
	}
	for(uint32_t i = 0; i < nRem; i += 2)
	{
		const char* pKey = txt[i];
		if(MicroProfileHashTableRemoveString(&Strings, pKey))
		{
			MP_BREAK();
		}
	}

	for(uint32_t i = 0; i < nCount; i += 2)
	{
		const char* pKey = txt[i];
		const char* pValue = txt[i + 1];
		const char* V;
		if(MicroProfileHashTableGetString(&Strings, pKey, &V))
		{
			if(i < nRem)
			{
				MP_BREAK();
			}
			else
			{
				if(V != pValue)
					MP_BREAK();
			}
		}
		else
		{
			if(i >= nRem)
				MP_BREAK();
		}
	}

	DumpTableStr(&Strings);
	MicroProfileHashTableDestroy(&Strings);
}

#undef uprintf

#undef S
#ifdef _WIN32
#pragma warning(pop)
#undef microprofile_fopen_helper
#endif

#ifdef MICROPROFILE_PS4
#define MICROPROFILE_PS4_IMPL
#include "microprofile_ps4.h"
#endif
#ifdef MICROPROFILE_XBOXONE
#define MICROPROFILE_XBOXONE_IMPL
#include "microprofile_xboxone.h"
#endif

#endif //#if MICROPROFILE_ENABLED

#include "microprofile_html.h"
