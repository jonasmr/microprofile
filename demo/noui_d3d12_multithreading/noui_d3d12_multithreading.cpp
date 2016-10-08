
// Multithreaded rendering sample, from DirectX-Graphics-Samples repository.
// All files merged into one
// microprofile code embedded.

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

#include <d3d11_2.h>
#include <pix.h>

#include <string>
#include <wrl.h>
#include <process.h>
#include "SquidRoom.h"
#define SINGLETHREADED FALSE

static const UINT FrameCount = 3;

static const UINT NumContexts = 3;
static const UINT NumLights = 3;		// Keep this in sync with "shaders.hlsl".

static const UINT TitleThrottle = 200;	// Only update the titlebar every X number of frames.

										// Command list submissions from main thread.
static const int CommandListCount = 3;
static const int CommandListPre = 0;
static const int CommandListMid = 1;
static const int CommandListPost = 2;


#define MICROPROFILE_GPU_TIMERS_D3D12
#define MICROPROFILE_IMPL
#define MICROPROFILE_GPU_FRAME_DELAY 5
#include "microprofile.h"

ID3D12Device* g_pDevice = 0;
ID3D12CommandQueue* g_pCommandQueue = 0;
ID3D12CommandQueue* g_pComputeQueue = 0;
int g_QueueGraphics = -1;
int g_QueueCompute = -1;
ID3D12RootSignature *g_pComputeRootSignature = 0;
ID3D12PipelineState* g_ComputePSO = 0;
uint64_t g_TokenGpuFrameIndex[2];
uint64_t g_TokenGpuComputeFrameIndex[2];

uint64_t g_FenceGraphics = 0;
uint64_t g_FenceCompute = 0;
ID3D12Fence* g_pFenceGraphics = 0;
ID3D12Fence* g_pFenceCompute = 0;
UINT g_nSrc = 0;
UINT g_nDst = 1;

#if MICROPROFILE_ENABLED
MicroProfileThreadLogGpu* g_gpuThreadLog[NumContexts];
#endif

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

inline void GetAssetsPath(_Out_writes_(pathSize) CHAR* path, UINT pathSize)
{
	if (path == nullptr)
	{
		throw;
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);

	if (size == 0 || size == pathSize)
	{
		// Method failed or path was truncated.
		throw;
	}

	CHAR* lastSlash = strrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = NULL;
	}
}

inline HRESULT ReadDataFromFile(LPCSTR filename, byte** data, UINT* size)
{
	using namespace Microsoft::WRL;

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = { 0 };
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	Wrappers::FileHandle file(
		CreateFile(
			filename,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL)
		);

	if (file.Get() == INVALID_HANDLE_VALUE)
	{
		throw new std::exception();
	}

	FILE_STANDARD_INFO fileInfo = { 0 };
	if (!GetFileInformationByHandleEx(
		file.Get(),
		FileStandardInfo,
		&fileInfo,
		sizeof(fileInfo)
		))
	{
		throw new std::exception();
	}

	if (fileInfo.EndOfFile.HighPart != 0)
	{
		throw new std::exception();
	}

	*data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
	*size = fileInfo.EndOfFile.LowPart;

	if (!ReadFile(
		file.Get(),
		*data,
		fileInfo.EndOfFile.LowPart,
		nullptr,
		nullptr
		))
	{
		throw new std::exception();
	}

	return S_OK;
}



using namespace DirectX;

class Camera
{
public:
	Camera();
	~Camera();

	void Get3DViewProjMatrices(XMFLOAT4X4 *view, XMFLOAT4X4 *proj, float fovInDegrees, float screenWidth, float screenHeight);
	void Reset();
	void Set(XMVECTOR eye, XMVECTOR at, XMVECTOR up);
	static Camera *get();
	void RotateYaw(float deg);
	void RotatePitch(float deg);
	void GetOrthoProjMatrices(XMFLOAT4X4 *view, XMFLOAT4X4 *proj, float width, float height);
	XMVECTOR mEye; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
	XMVECTOR mAt; // What the camera is looking at (world origin)
	XMVECTOR mUp; // Which way is up
private:
	static Camera* mCamera;
};


Camera* Camera::mCamera = nullptr;

Camera::Camera()
{
	Reset();
	mCamera = this;
}

Camera::~Camera()
{
	mCamera = nullptr;
}

Camera* Camera::get()
{
	return mCamera;
}

void Camera::Get3DViewProjMatrices(XMFLOAT4X4 *view, XMFLOAT4X4 *proj, float fovInDegrees, float screenWidth, float screenHeight)
{

	float aspectRatio = (float)screenWidth / (float)screenHeight;
	float fovAngleY = fovInDegrees * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
	{
		fovAngleY /= aspectRatio;
	}

	XMStoreFloat4x4(view, XMMatrixTranspose(XMMatrixLookAtRH(mEye, mAt, mUp)));
	XMStoreFloat4x4(proj, XMMatrixTranspose(XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.01f, 125.0f)));
}

void Camera::GetOrthoProjMatrices(XMFLOAT4X4 *view, XMFLOAT4X4 *proj, float width, float height)
{
	XMStoreFloat4x4(view, XMMatrixTranspose(XMMatrixLookAtRH(mEye, mAt, mUp)));
	XMStoreFloat4x4(proj, XMMatrixTranspose(XMMatrixOrthographicRH(width, height, 0.01f, 125.0f)));
}
void Camera::RotateYaw(float deg)
{
	XMMATRIX rotation = XMMatrixRotationAxis(mUp, deg);

	mEye = XMVector3TransformCoord(mEye, rotation);
}

void Camera::RotatePitch(float deg)
{
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(mEye, mUp));
	XMMATRIX rotation = XMMatrixRotationAxis(right, deg);

	mEye = XMVector3TransformCoord(mEye, rotation);
}

void Camera::Reset()
{
	mEye = XMVectorSet(0.0f, 15.0f, -30.0f, 0.0f);
	mAt = XMVectorSet(0.0f, 8.0f, 0.0f, 0.0f);
	mUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}

void Camera::Set(XMVECTOR eye, XMVECTOR at, XMVECTOR up)
{
	mEye = eye;
	mAt = at;
	mUp = up;
}

class StepTimer
{
public:
	StepTimer() :
		m_elapsedTicks(0),
		m_totalTicks(0),
		m_leftOverTicks(0),
		m_frameCount(0),
		m_framesPerSecond(0),
		m_framesThisSecond(0),
		m_qpcSecondCounter(0),
		m_isFixedTimeStep(false),
		m_targetElapsedTicks(TicksPerSecond / 60)
	{
		QueryPerformanceFrequency(&m_qpcFrequency);
		QueryPerformanceCounter(&m_qpcLastTime);

		// Initialize max delta to 1/10 of a second.
		m_qpcMaxDelta = m_qpcFrequency.QuadPart / 10;
	}

	// Get elapsed time since the previous Update call.
	UINT64 GetElapsedTicks() const { return m_elapsedTicks; }
	double GetElapsedSeconds() const { return TicksToSeconds(m_elapsedTicks); }

	// Get total time since the start of the program.
	UINT64 GetTotalTicks() const { return m_totalTicks; }
	double GetTotalSeconds() const { return TicksToSeconds(m_totalTicks); }

	// Get total number of updates since start of the program.
	UINT32 GetFrameCount() const { return m_frameCount; }

	// Get the current framerate.
	UINT32 GetFramesPerSecond() const { return m_framesPerSecond; }

	// Set whether to use fixed or variable timestep mode.
	void SetFixedTimeStep(bool isFixedTimestep) { m_isFixedTimeStep = isFixedTimestep; }

	// Set how often to call Update when in fixed timestep mode.
	void SetTargetElapsedTicks(UINT64 targetElapsed) { m_targetElapsedTicks = targetElapsed; }
	void SetTargetElapsedSeconds(double targetElapsed) { m_targetElapsedTicks = SecondsToTicks(targetElapsed); }

	// Integer format represents time using 10,000,000 ticks per second.
	static const UINT64 TicksPerSecond = 10000000;

	static double TicksToSeconds(UINT64 ticks) { return static_cast<double>(ticks) / TicksPerSecond; }
	static UINT64 SecondsToTicks(double seconds) { return static_cast<UINT64>(seconds * TicksPerSecond); }

	// After an intentional timing discontinuity (for instance a blocking IO operation)
	// call this to avoid having the fixed timestep logic attempt a set of catch-up 
	// Update calls.

	void ResetElapsedTime()
	{
		QueryPerformanceCounter(&m_qpcLastTime);

		m_leftOverTicks = 0;
		m_framesPerSecond = 0;
		m_framesThisSecond = 0;
		m_qpcSecondCounter = 0;
	}

	typedef void(*LPUPDATEFUNC) (void);

	// Update timer state, calling the specified Update function the appropriate number of times.
	void Tick(LPUPDATEFUNC update)
	{
		// Query the current time.
		LARGE_INTEGER currentTime;

		QueryPerformanceCounter(&currentTime);

		UINT64 timeDelta = currentTime.QuadPart - m_qpcLastTime.QuadPart;

		m_qpcLastTime = currentTime;
		m_qpcSecondCounter += timeDelta;

		// Clamp excessively large time deltas (e.g. after paused in the debugger).
		if (timeDelta > m_qpcMaxDelta)
		{
			timeDelta = m_qpcMaxDelta;
		}

		// Convert QPC units into a canonical tick format. This cannot overflow due to the previous clamp.
		timeDelta *= TicksPerSecond;
		timeDelta /= m_qpcFrequency.QuadPart;

		UINT32 lastFrameCount = m_frameCount;

		if (m_isFixedTimeStep)
		{
			// Fixed timestep update logic

			// If the app is running very close to the target elapsed time (within 1/4 of a millisecond) just clamp
			// the clock to exactly match the target value. This prevents tiny and irrelevant errors
			// from accumulating over time. Without this clamping, a game that requested a 60 fps
			// fixed update, running with vsync enabled on a 59.94 NTSC display, would eventually
			// accumulate enough tiny errors that it would drop a frame. It is better to just round 
			// small deviations down to zero to leave things running smoothly.

			if (abs(static_cast<int>(timeDelta - m_targetElapsedTicks)) < TicksPerSecond / 4000)
			{
				timeDelta = m_targetElapsedTicks;
			}

			m_leftOverTicks += timeDelta;

			while (m_leftOverTicks >= m_targetElapsedTicks)
			{
				m_elapsedTicks = m_targetElapsedTicks;
				m_totalTicks += m_targetElapsedTicks;
				m_leftOverTicks -= m_targetElapsedTicks;
				m_frameCount++;

				if (update)
				{
					update();
				}
			}
		}
		else
		{
			// Variable timestep update logic.
			m_elapsedTicks = timeDelta;
			m_totalTicks += timeDelta;
			m_leftOverTicks = 0;
			m_frameCount++;

			if (update)
			{
				update();
			}
		}

		// Track the current framerate.
		if (m_frameCount != lastFrameCount)
		{
			m_framesThisSecond++;
		}

		if (m_qpcSecondCounter >= static_cast<UINT64>(m_qpcFrequency.QuadPart))
		{
			m_framesPerSecond = m_framesThisSecond;
			m_framesThisSecond = 0;
			m_qpcSecondCounter %= m_qpcFrequency.QuadPart;
		}
	}

private:
	// Source timing data uses QPC units.
	LARGE_INTEGER m_qpcFrequency;
	LARGE_INTEGER m_qpcLastTime;
	UINT64 m_qpcMaxDelta;

	// Derived timing data uses a canonical tick format.
	UINT64 m_elapsedTicks;
	UINT64 m_totalTicks;
	UINT64 m_leftOverTicks;

	// Members for tracking the framerate.
	UINT32 m_frameCount;
	UINT32 m_framesPerSecond;
	UINT32 m_framesThisSecond;
	UINT64 m_qpcSecondCounter;

	// Members for configuring fixed timestep mode.
	bool m_isFixedTimeStep;
	UINT64 m_targetElapsedTicks;
};


class DXSample
{
public:
	DXSample(UINT width, UINT height, std::string name);
	virtual ~DXSample();

	int Run(HINSTANCE hInstance, int nCmdShow);
	void SetCustomWindowText(LPCSTR text);

protected:
	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;
	virtual bool OnEvent(MSG msg) = 0;

	std::string GetAssetFullPath(LPCSTR assetName);
	void GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter);

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Viewport dimensions.
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;

	// Window handle.
	HWND m_hwnd;

	// Adapter info.
	bool m_useWarpDevice;

private:
	void ParseCommandLineArgs();

	// Root assets path.
	std::string m_assetsPath;

	// Window title.
	std::string m_title;
};


DXSample::DXSample(UINT width, UINT height, std::string name) :
	m_width(width),
	m_height(height),
	m_useWarpDevice(false)
{
	ParseCommandLineArgs();

	m_title = name + (m_useWarpDevice ? " (WARP)" : "");

	CHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	m_assetsPath = assetsPath;

	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXSample::~DXSample()
{
}

int DXSample::Run(HINSTANCE hInstance, int nCmdShow)
{
	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = "WindowClass1";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	m_hwnd = CreateWindowEx(NULL,
		"WindowClass1",
		m_title.c_str(),
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,		// We have no parent window, NULL.
		NULL,		// We aren't using menus, NULL.
		hInstance,
		NULL);		// We aren't using multiple windows, NULL.

	ShowWindow(m_hwnd, nCmdShow);

	MICROPROFILE_CONDITIONAL( g_QueueGraphics = MICROPROFILE_GPU_INIT_QUEUE("GPU-Graphics-Queue"));
	MICROPROFILE_CONDITIONAL( g_QueueCompute = MICROPROFILE_GPU_INIT_QUEUE("GPU-Compute-Queue"));

	MicroProfileOnThreadCreate("Main");
//	MicroProfileSetForceEnable(true);
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

	// Initialize the sample. OnInit is defined in each child-implementation of DXSample.
	OnInit();

#if MICROPROFILE_ENABLED
	for (uint32_t i = 0; i < _countof(g_TokenGpuFrameIndex); ++i)
	{
		char frame[255];
		snprintf(frame, sizeof(frame) - 1, "Graphics-Read-%d", i);
		g_TokenGpuFrameIndex[i] = MicroProfileGetToken("GPU", frame, (uint32_t)-1, MicroProfileTokenTypeGpu);
	}

	for (uint32_t i = 0; i < _countof(g_TokenGpuComputeFrameIndex); ++i)
	{
		char frame[255];
		snprintf(frame, sizeof(frame) - 1, "Compute-Write-%d", i);
		g_TokenGpuComputeFrameIndex[i] = MicroProfileGetToken("GPU", frame, (uint32_t)-1, MicroProfileTokenTypeGpu);
	}
	MicroProfileGpuInitD3D12(g_pDevice, 1, (void**)&g_pCommandQueue);
	MicroProfileSetCurrentNodeD3D12(0);


	for (int i = 0; i < NumContexts; ++i)
	{
		g_gpuThreadLog[i] = MicroProfileThreadLogGpuAlloc();
	}
#endif

	// Main sample loop.
	MSG msg = { 0 };
	while (true)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				break;

			// Pass events into our sample.
			OnEvent(msg);
		}

		OnUpdate();
		OnRender();
	}

	OnDestroy();
	MicroProfileGpuShutdown();

	// Return this part of the WM_QUIT message to Windows.
	return static_cast<char>(msg.wParam);
}

// Helper function for resolving the full path of assets.
std::string DXSample::GetAssetFullPath(LPCSTR assetName)
{
	return m_assetsPath + assetName;
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
void DXSample::GetHardwareAdapter(_In_ IDXGIFactory4* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter)
{
	IDXGIAdapter1* pAdapter = nullptr;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = pAdapter;
}

// Helper function for setting the window's title text.
void DXSample::SetCustomWindowText(LPCSTR text)
{
	std::string windowText = m_title + ": " + text;
	SetWindowText(m_hwnd, windowText.c_str());
}

// Helper function for parsing any supplied command line args.
void DXSample::ParseCommandLineArgs()
{
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	for (int i = 1; i < argc; ++i)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			m_useWarpDevice = true;
		}
	}
	LocalFree(argv);
}

// Main message handler for the sample.
LRESULT CALLBACK DXSample::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// Handle destroy/shutdown messages.
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}




using namespace Microsoft::WRL;
using namespace DirectX;

class FrameResource;

struct LightState
{
	XMFLOAT4 position;
	XMFLOAT4 direction;
	XMFLOAT4 color;
	XMFLOAT4 falloff;

	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
};

struct ConstantBuffer
{
	XMFLOAT4X4 model;
	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
	XMFLOAT4 ambientColor;
	BOOL sampleShadowMap;
	BOOL padding[3];		// Must be aligned to be made up of N float4s.
	LightState lights[NumLights];
};


using namespace DirectX;
using namespace Microsoft::WRL;

class FrameResource
{
public:
	ID3D12CommandList* m_batchSubmit[NumContexts * 2 + CommandListCount];

	ComPtr<ID3D12CommandAllocator> m_commandAllocators[CommandListCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandLists[CommandListCount];

	ComPtr<ID3D12CommandAllocator> m_commandAllocatorCompute;
	ComPtr<ID3D12GraphicsCommandList> m_commandListCompute;
	uint64_t m_computeFenceValue;
	ComPtr<ID3D12CommandAllocator> m_commandAllocatorTransition;
	ComPtr<ID3D12GraphicsCommandList> m_commandListTransition;

	ComPtr<ID3D12CommandAllocator> m_shadowCommandAllocators[NumContexts];
	ComPtr<ID3D12GraphicsCommandList> m_shadowCommandLists[NumContexts];
	uint64_t						m_shadowMicroProfile[NumContexts];

	ComPtr<ID3D12CommandAllocator> m_sceneCommandAllocators[NumContexts];
	ComPtr<ID3D12GraphicsCommandList> m_sceneCommandLists[NumContexts];
	uint64_t								m_sceneMicroProfile[NumContexts];

	UINT64 m_fenceValue;

private:
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_pipelineStateShadowMap;
	ComPtr<ID3D12PipelineState> m_pipelineStateCompute;
	ComPtr<ID3D12Resource> m_shadowTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE m_shadowDepthView;
	ComPtr<ID3D12Resource> m_shadowConstantBuffer;
	ComPtr<ID3D12Resource> m_sceneConstantBuffer;
	ConstantBuffer* mp_shadowConstantBufferWO;		// WRITE-ONLY pointer to the shadow pass constant buffer.
	ConstantBuffer* mp_sceneConstantBufferWO;		// WRITE-ONLY pointer to the scene pass constant buffer.
	D3D12_GPU_DESCRIPTOR_HANDLE m_nullSrvHandle;	// Null SRV for out of bounds behavior.
	D3D12_GPU_DESCRIPTOR_HANDLE m_shadowDepthHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_shadowCbvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_sceneCbvHandle;

public:
	FrameResource(ID3D12Device* pDevice, ID3D12PipelineState* pPso, ID3D12PipelineState* pShadowMapPso, ID3D12PipelineState* pComputePso, ID3D12DescriptorHeap* pDsvHeap, ID3D12DescriptorHeap* pCbvSrvHeap, D3D12_VIEWPORT* pViewport, UINT frameResourceIndex);
	~FrameResource();

	void Bind(ID3D12GraphicsCommandList* pCommandList, BOOL scenePass, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle);
	void Init();
	void SwapBarriers();
	void Finish();
	void WriteConstantBuffers(D3D12_VIEWPORT* pViewport, Camera* pSceneCamera, Camera lightCams[NumLights], LightState lights[NumLights]);
};


class D3D12Multithreading : public DXSample
{
public:
	D3D12Multithreading(UINT width, UINT height, std::string name);
	virtual ~D3D12Multithreading();

	static D3D12Multithreading* Get() { return s_app; }

protected:
	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	virtual bool OnEvent(MSG msg);

private:
	struct InputState
	{
		bool rightArrowPressed;
		bool leftArrowPressed;
		bool upArrowPressed;
		bool downArrowPressed;
		bool animate;
	};

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12Resource> m_depthStencil;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
	ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
	ComPtr<ID3D12DescriptorHeap> m_computeSrvUavHeap;

	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12PipelineState> m_pipelineStateShadowMap;

	// App resources.
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViewCompute[2];
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	ComPtr<ID3D12Resource> m_textures[_countof(SampleAssets::Textures)];
	ComPtr<ID3D12Resource> m_textureUploads[_countof(SampleAssets::Textures)];
	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_indexBufferUpload;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_vertexBufferCompute[2];
	D3D12_RESOURCE_STATES m_vbState[2];
	ComPtr<ID3D12Resource> m_vertexBufferUpload;
	UINT m_rtvDescriptorSize;
	InputState m_keyboardInput;
	LightState m_lights[NumLights];
	Camera m_lightCameras[NumLights];
	Camera m_camera;
	StepTimer m_timer;
	StepTimer m_cpuTimer;
	int m_titleCount;
	double m_cpuTime;

	// Synchronization objects.
	HANDLE m_workerBeginRenderFrame[NumContexts];
	HANDLE m_workerFinishShadowPass[NumContexts];
	HANDLE m_workerFinishedRenderFrame[NumContexts];
	HANDLE m_threadHandles[NumContexts];
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;


	// Singleton object so that worker threads can share members.
	static D3D12Multithreading* s_app;

	// Frame resources.
	FrameResource* m_frameResources[FrameCount];
	FrameResource* m_pCurrentFrameResource;
	int m_currentFrameResourceIndex;

	struct ThreadParameter
	{
		int threadIndex;
	};
	ThreadParameter m_threadParameters[NumContexts];

	void WorkerThread(int threadIndex);
	void SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList);

	void LoadPipeline();
	void LoadAssets();
	void LoadContexts();
	void OnKeyUp(WPARAM wParam);
	void OnKeyDown(WPARAM wParam);
	void BeginFrame();
	void MidFrame();
	void EndFrame();
};






D3D12Multithreading* D3D12Multithreading::s_app = nullptr;

D3D12Multithreading::D3D12Multithreading(UINT width, UINT height, std::string name) :
	DXSample(width, height, name),
	m_frameIndex(0),
	m_viewport(),
	m_scissorRect(),
	m_keyboardInput(),
	m_titleCount(0),
	m_cpuTime(0),
	m_fenceValue(0),
	m_rtvDescriptorSize(0),
	m_currentFrameResourceIndex(0),
	m_pCurrentFrameResource(nullptr)
{
	s_app = this;

	m_viewport.Width = static_cast<float>(width);
	m_viewport.Height = static_cast<float>(height);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = static_cast<LONG>(width);
	m_scissorRect.bottom = static_cast<LONG>(height);

	m_keyboardInput.animate = true;
}

D3D12Multithreading::~D3D12Multithreading()
{
	s_app = nullptr;
}

void D3D12Multithreading::OnInit()
{
	LoadPipeline();
	LoadAssets();
	LoadContexts();
}

// Load the rendering pipeline dependencies.
void D3D12Multithreading::LoadPipeline()
{
#ifdef _DEBUG
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)
			));
	}

	g_pDevice = m_device.Get();

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));
	g_pCommandQueue = m_commandQueue.Get();
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

	ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_pComputeQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.BufferDesc.Width = m_width;
	swapChainDesc.BufferDesc.Height = m_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = m_hwnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain> swapChain;
	ThrowIfFailed(factory->CreateSwapChain(
		m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
		&swapChainDesc,
		&swapChain
		));

	ThrowIfFailed(swapChain.As(&m_swapChain));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		// Describe and create a depth stencil view (DSV) descriptor heap.
		// Each frame has its own depth stencils (to write shadows onto) 
		// and then there is one for the scene itself.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1 + FrameCount * 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

		// Describe and create a shader resource view (SRV) and constant 
		// buffer view (CBV) descriptor heap.  Heap layout: null views, 
		// object diffuse + normal textures views, frame 1's shadow buffer, 
		// frame 1's 2x constant buffer, frame 2's shadow buffer, frame 2's 
		// 2x constant buffers, etc...
		const UINT nullSrvCount = 2;		// Null descriptors are needed for out of bounds behavior reads.
		const UINT cbvCount = FrameCount * 2;
		const UINT srvCount = _countof(SampleAssets::Textures) + (FrameCount * 1);
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
		cbvSrvHeapDesc.NumDescriptors = nullSrvCount + cbvCount + srvCount;
		cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

		D3D12_DESCRIPTOR_HEAP_DESC computeHeapDesc = {};
		computeHeapDesc.NumDescriptors = 4;
		computeHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		computeHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&computeHeapDesc, IID_PPV_ARGS(&m_computeSrvUavHeap)));


		// Describe and create a sampler descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
		samplerHeapDesc.NumDescriptors = 2;		// One clamp and one wrap sampler.
		samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

// Load the sample assets.
void D3D12Multithreading::LoadAssets()
{
	// Create the root signature.
	{
		CD3DX12_DESCRIPTOR_RANGE ranges[4]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);		// 2 frequently changed diffuse + normal textures - using registers t1 and t2.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);		// 1 frequently changed constant buffer.
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// 1 infrequently changed shadow texture - starting in register t0.
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);	// 2 static samplers.

		CD3DX12_ROOT_PARAMETER rootParameters[4];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
	}

	{
		CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);		// 2 frequently changed diffuse + normal textures - using registers t1 and t2.
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);		// 1 frequently changed constant buffer.

		CD3DX12_ROOT_PARAMETER rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
		rootParameters[2].InitAsConstants(1, 0);
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_pComputeRootSignature)));
	}


	// Create the pipeline state, which includes loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		ComPtr<ID3DBlob> computeShader;

#ifdef _DEBUG
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "CSMain", "cs_5_0", compileFlags, 0, &computeShader, nullptr));

		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
		inputLayoutDesc.pInputElementDescs = SampleAssets::StandardVertexDescription;
		inputLayoutDesc.NumElements = _countof(SampleAssets::StandardVertexDescription);

		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		depthStencilDesc.StencilEnable = FALSE;

		// Describe and create the PSO for rendering the scene.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = inputLayoutDesc;
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
		psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = depthStencilDesc;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

		// Alter the description and create the PSO for rendering
		// the shadow map.  The shadow map does not use a pixel
		// shader or render targets.
		psoDesc.PS.pShaderBytecode = 0;
		psoDesc.PS.BytecodeLength = 0;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		psoDesc.NumRenderTargets = 0;

		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateShadowMap)));

		D3D12_COMPUTE_PIPELINE_STATE_DESC ComputeDesc;
		ZeroMemory(&ComputeDesc, sizeof(ComputeDesc));
		ComputeDesc.CS = { reinterpret_cast<UINT8*>(computeShader->GetBufferPointer()), computeShader->GetBufferSize() };
		ComputeDesc.pRootSignature = g_pComputeRootSignature;
		ComputeDesc.CachedPSO = { 0,0 };
		ThrowIfFailed(m_device->CreateComputePipelineState(&ComputeDesc, IID_PPV_ARGS(&g_ComputePSO)));
	}

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFenceCompute)));
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFenceGraphics)));
	// Create temporary command list for initial GPU setup.
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&commandList)));

	// Create render target views (RTVs).
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < FrameCount; i++)
	{
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// Create the depth stencil.
	{
		CD3DX12_RESOURCE_DESC shadowTextureDesc(
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0,
			static_cast<UINT>(m_viewport.Width), 
			static_cast<UINT>(m_viewport.Height), 
			1,
			1,
			DXGI_FORMAT_D32_FLOAT,
			1, 
			0,
			D3D12_TEXTURE_LAYOUT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

		D3D12_CLEAR_VALUE clearValue;	// Performance tip: Tell the runtime at resource creation the desired clear value.
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&shadowTextureDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue,
			IID_PPV_ARGS(&m_depthStencil)));

		// Create the depth stencil view.
		m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Load scene assets.
	UINT fileSize = 0;
	UINT8* pAssetData;
	ThrowIfFailed(ReadDataFromFile(SampleAssets::DataFileName, &pAssetData, &fileSize));

	// Create the vertex buffer.
	{
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));
		for (int i = 0; i < 2; ++i)
		{
			m_vbState[i] = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
				m_vbState[i],
				nullptr,
				IID_PPV_ARGS(&m_vertexBufferCompute[i])));
			m_vertexBufferCompute[i]->SetName(i == 0 ? L"VB_COMP0" : L"VB_COMP1");

		}
		{
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::VertexDataSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_vertexBufferUpload)));

			// Copy data to the upload heap and then schedule a copy 
			// from the upload heap to the vertex buffer.
			D3D12_SUBRESOURCE_DATA vertexData = {};
			vertexData.pData = pAssetData + SampleAssets::VertexDataOffset;
			vertexData.RowPitch = SampleAssets::VertexDataSize;
			vertexData.SlicePitch = vertexData.RowPitch;

			PIXBeginEvent(commandList.Get(), 0, L"Copy vertex buffer data to default resource...");

			UpdateSubresources<1>(commandList.Get(), m_vertexBuffer.Get(), m_vertexBufferUpload.Get(), 0, 0, 1, &vertexData);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

			PIXEndEvent(commandList.Get());
		}

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.SizeInBytes = SampleAssets::VertexDataSize;
		m_vertexBufferView.StrideInBytes = SampleAssets::StandardVertexStride;
		m_vertexBufferViewCompute[0].BufferLocation = m_vertexBufferCompute[0]->GetGPUVirtualAddress();
		m_vertexBufferViewCompute[0].SizeInBytes = SampleAssets::VertexDataSize;
		m_vertexBufferViewCompute[0].StrideInBytes = SampleAssets::StandardVertexStride;
		m_vertexBufferViewCompute[1].BufferLocation = m_vertexBufferCompute[1]->GetGPUVirtualAddress();
		m_vertexBufferViewCompute[1].SizeInBytes = SampleAssets::VertexDataSize;
		m_vertexBufferViewCompute[1].StrideInBytes = SampleAssets::StandardVertexStride;



		//create srv/uav from dynamic buffers.
		// Get the CBV SRV descriptor size for the current device.
		const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Get a handle to the start of the descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE computeHandle(m_computeSrvUavHeap->GetCPUDescriptorHandleForHeapStart());
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		SrvDesc.Buffer.FirstElement = 0;
		SrvDesc.Buffer.NumElements = SampleAssets::VertexDataSize / SampleAssets::StandardVertexStride;
		SrvDesc.Buffer.StructureByteStride = SampleAssets::StandardVertexStride;
		SrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		m_device->CreateShaderResourceView(m_vertexBuffer.Get(), &SrvDesc, computeHandle);
		computeHandle.Offset(cbvSrvDescriptorSize);

		m_device->CreateShaderResourceView(m_vertexBuffer.Get(), &SrvDesc, computeHandle);
		computeHandle.Offset(cbvSrvDescriptorSize);

		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
		UavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		UavDesc.Buffer.CounterOffsetInBytes = 0;
		UavDesc.Buffer.FirstElement = 0;
		UavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		UavDesc.Buffer.NumElements = SampleAssets::VertexDataSize / SampleAssets::StandardVertexStride;
		UavDesc.Buffer.StructureByteStride = SampleAssets::StandardVertexStride;
		


		m_device->CreateUnorderedAccessView(m_vertexBufferCompute[0].Get(), nullptr, &UavDesc, computeHandle);
		computeHandle.Offset(cbvSrvDescriptorSize);
		m_device->CreateUnorderedAccessView(m_vertexBufferCompute[1].Get(), nullptr, &UavDesc, computeHandle);
		computeHandle.Offset(cbvSrvDescriptorSize);





	}

	// Create the index buffer.
	{
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		{
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(SampleAssets::IndexDataSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_indexBufferUpload)));

			// Copy data to the upload heap and then schedule a copy 
			// from the upload heap to the index buffer.
			D3D12_SUBRESOURCE_DATA indexData = {};
			indexData.pData = pAssetData + SampleAssets::IndexDataOffset;
			indexData.RowPitch = SampleAssets::IndexDataSize;
			indexData.SlicePitch = indexData.RowPitch;

			PIXBeginEvent(commandList.Get(), 0, L"Copy index buffer data to default resource...");

			UpdateSubresources<1>(commandList.Get(), m_indexBuffer.Get(), m_indexBufferUpload.Get(), 0, 0, 1, &indexData);
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

			PIXEndEvent(commandList.Get());
		}

		// Initialize the index buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.SizeInBytes = SampleAssets::IndexDataSize;
		m_indexBufferView.Format = SampleAssets::StandardIndexFormat;
	}

	// Create shader resources.
	{
		// Get the CBV SRV descriptor size for the current device.
		const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Get a handle to the start of the descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());

		{
			// Describe and create 2 null SRVs. Null descriptors are needed in order 
			// to achieve the effect of an "unbound" resource.
			D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
			nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			nullSrvDesc.Texture2D.MipLevels = 1;
			nullSrvDesc.Texture2D.MostDetailedMip = 0;
			nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, cbvSrvHandle);
			cbvSrvHandle.Offset(cbvSrvDescriptorSize);

			m_device->CreateShaderResourceView(nullptr, &nullSrvDesc, cbvSrvHandle);
			cbvSrvHandle.Offset(cbvSrvDescriptorSize);
		}

		// Create each texture and SRV descriptor.
		const UINT srvCount = _countof(SampleAssets::Textures);
		PIXBeginEvent(commandList.Get(), 0, L"Copy diffuse and normal texture data to default resources...");
		for (int i = 0; i < srvCount; i++)
		{
			// Describe and create a Texture2D.
			const SampleAssets::TextureResource &tex = SampleAssets::Textures[i];
			CD3DX12_RESOURCE_DESC texDesc(
				D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				0,
				tex.Width, 
				tex.Height, 
				1,
				static_cast<UINT16>(tex.MipLevels),
				tex.Format,
				1, 
				0,
				D3D12_TEXTURE_LAYOUT_UNKNOWN,
				D3D12_RESOURCE_FLAG_NONE);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m_textures[i])));

			{
				const UINT subresourceCount = texDesc.DepthOrArraySize * texDesc.MipLevels;
				UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textures[i].Get(), 0, subresourceCount);
				ThrowIfFailed(m_device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_textureUploads[i])));

				// Copy data to the intermediate upload heap and then schedule a copy 
				// from the upload heap to the Texture2D.
				D3D12_SUBRESOURCE_DATA textureData = {};
				textureData.pData = pAssetData + tex.Data->Offset;
				textureData.RowPitch = tex.Data->Pitch;
				textureData.SlicePitch = tex.Data->Size;

				UpdateSubresources(commandList.Get(), m_textures[i].Get(), m_textureUploads[i].Get(), 0, 0, subresourceCount, &textureData);
				commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textures[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
			}

			// Describe and create an SRV.
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = tex.Format;
			srvDesc.Texture2D.MipLevels = tex.MipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			m_device->CreateShaderResourceView(m_textures[i].Get(), &srvDesc, cbvSrvHandle);

			// Move to the next descriptor slot.
			cbvSrvHandle.Offset(cbvSrvDescriptorSize);
		}
		PIXEndEvent(commandList.Get());
	}

	free(pAssetData);

	// Create the samplers.
	{
		// Get the sampler descriptor size for the current device.
		const UINT samplerDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

		// Get a handle to the start of the descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(m_samplerHeap->GetCPUDescriptorHandleForHeapStart());

		// Describe and create the wrapping sampler, which is used for 
		// sampling diffuse/normal maps.
		D3D12_SAMPLER_DESC wrapSamplerDesc = {};
		wrapSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		wrapSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		wrapSamplerDesc.MinLOD = 0;
		wrapSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		wrapSamplerDesc.MipLODBias = 0.0f;
		wrapSamplerDesc.MaxAnisotropy = 1;
		wrapSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		wrapSamplerDesc.BorderColor[0] = wrapSamplerDesc.BorderColor[1] = wrapSamplerDesc.BorderColor[2] = wrapSamplerDesc.BorderColor[3] = 0;
		m_device->CreateSampler(&wrapSamplerDesc, samplerHandle);

		// Move the handle to the next slot in the descriptor heap.
		samplerHandle.Offset(samplerDescriptorSize);

		// Describe and create the point clamping sampler, which is 
		// used for the shadow map.
		D3D12_SAMPLER_DESC clampSamplerDesc = {};
		clampSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		clampSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		clampSamplerDesc.MipLODBias = 0.0f;
		clampSamplerDesc.MaxAnisotropy = 1;
		clampSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		clampSamplerDesc.BorderColor[0] = clampSamplerDesc.BorderColor[1] = clampSamplerDesc.BorderColor[2] = clampSamplerDesc.BorderColor[3] = 0;
		clampSamplerDesc.MinLOD = 0;
		clampSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		m_device->CreateSampler(&clampSamplerDesc, samplerHandle);
	}

	// Create lights.
	for (int i = 0; i < NumLights; i++)
	{
		// Set up each of the light positions and directions (they all start 
		// in the same place).
		m_lights[i].position = { 0.0f, 15.0f, -30.0f, 1.0f };
		m_lights[i].direction = { 0.0, 0.0f, 1.0f, 0.0f };
		m_lights[i].falloff = { 800.0f, 1.0f, 0.0f, 1.0f };
		m_lights[i].color = { 0.7f, 0.7f, 0.7f, 1.0f };

		XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
		XMVECTOR at = XMVectorAdd(eye, XMLoadFloat4(&m_lights[i].direction));
		XMVECTOR up = { 0, 1, 0 };

		m_lightCameras[i].Set(eye, at, up);
	}

	// Close the command list and use it to execute the initial GPU setup.
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create frame resources.
	for (int i = 0; i < FrameCount; i++)
	{
		m_frameResources[i] = new FrameResource(m_device.Get(), m_pipelineState.Get(), m_pipelineStateShadowMap.Get(), g_ComputePSO, m_dsvHeap.Get(), m_cbvSrvHeap.Get(), &m_viewport, i);
		m_frameResources[i]->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
	}
	m_currentFrameResourceIndex = 0;
	m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(m_device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.

		// Signal and increment the fence value.
		const UINT64 fenceToWaitFor = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
		m_fenceValue++;

		// Wait until the fence is completed.
		ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

// Initialize threads and events.
void D3D12Multithreading::LoadContexts()
{
#if !SINGLETHREADED
	struct threadwrapper
	{
		static unsigned int WINAPI thunk(LPVOID lpParameter)
		{
			ThreadParameter* parameter = reinterpret_cast<ThreadParameter*>(lpParameter);
			D3D12Multithreading::Get()->WorkerThread(parameter->threadIndex);
			return 0;
		}
	};

	for (int i = 0; i < NumContexts; i++)
	{
		m_workerBeginRenderFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishedRenderFrame[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_workerFinishShadowPass[i] = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			NULL);

		m_threadParameters[i].threadIndex = i;

		m_threadHandles[i] = reinterpret_cast<HANDLE>(_beginthreadex(
			nullptr,
			0,
			threadwrapper::thunk,
			reinterpret_cast<LPVOID>(&m_threadParameters[i]),
			0,
			nullptr));

		assert(m_workerBeginRenderFrame[i] != NULL);
		assert(m_workerFinishedRenderFrame[i] != NULL);
		assert(m_threadHandles[i] != NULL);

	}
#endif
}

// Update frame-based values.
void D3D12Multithreading::OnUpdate()
{
	m_timer.Tick(NULL);

	PIXSetMarker(m_commandQueue.Get(), 0, L"Getting last completed fence.");

	// Get current GPU progress against submitted workload. Resources still scheduled 
	// for GPU execution cannot be modified or else undefined behavior will result.
	const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

	// Move to the next frame resource.
	m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % FrameCount;
	m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];

	// Make sure that this frame resource isn't still in use by the GPU.
	// If it is, wait for it to complete.
	if (m_pCurrentFrameResource->m_fenceValue > lastCompletedFence)
	{
		HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (eventHandle == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_pCurrentFrameResource->m_fenceValue, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
	}

	m_cpuTimer.Tick(NULL);
	float frameTime = static_cast<float>(m_timer.GetElapsedSeconds());
	float frameChange = 2.0f * frameTime;

	if (m_keyboardInput.leftArrowPressed)
		m_camera.RotateYaw(-frameChange);
	if (m_keyboardInput.rightArrowPressed)
		m_camera.RotateYaw(frameChange);
	if (m_keyboardInput.upArrowPressed)
		m_camera.RotatePitch(frameChange);
	if (m_keyboardInput.downArrowPressed)
		m_camera.RotatePitch(-frameChange);

	if (m_keyboardInput.animate)
	{
		for (int i = 0; i < NumLights; i++)
		{
			float direction = frameChange * pow(-1.0f, i);
			XMStoreFloat4(&m_lights[i].position, XMVector4Transform(XMLoadFloat4(&m_lights[i].position), XMMatrixRotationY(direction)));

			XMVECTOR eye = XMLoadFloat4(&m_lights[i].position);
			XMVECTOR at = { 0.0f, 8.0f, 0.0f };
			XMStoreFloat4(&m_lights[i].direction, XMVector3Normalize(XMVectorSubtract(at, eye)));
			XMVECTOR up = { 0.0f, 1.0f, 0.0f };
			m_lightCameras[i].Set(eye, at, up);

			m_lightCameras[i].Get3DViewProjMatrices(&m_lights[i].view, &m_lights[i].projection, 90.0f, static_cast<float>(m_width), static_cast<float>(m_height));
		}
	}

	m_pCurrentFrameResource->WriteConstantBuffers(&m_viewport, &m_camera, m_lightCameras, m_lights);
}

// Render the scene.
void D3D12Multithreading::OnRender()
{
	MICROPROFILE_SCOPEI("Main", "OnRender", 0);

	BeginFrame();

	for (int i = 0; i < NumContexts; i++)
	{
		SetEvent(m_workerBeginRenderFrame[i]); // Tell each worker to start drawing.
	}

	MidFrame();
	EndFrame();

	WaitForMultipleObjects(NumContexts, m_workerFinishShadowPass, TRUE, INFINITE);
	for (uint32_t i = 0; i < NumContexts; ++i)
	{
		MICROPROFILE_GPU_SUBMIT(g_QueueGraphics, m_pCurrentFrameResource->m_shadowMicroProfile[i]);
	}

	// You can execute command lists on any thread. Depending on the work 
	// load, apps can choose between using ExecuteCommandLists on one thread 
	// vs ExecuteCommandList from multiple threads.
	m_commandQueue->ExecuteCommandLists(NumContexts + 2, m_pCurrentFrameResource->m_batchSubmit); // Submit PRE, MID and shadows.

	WaitForMultipleObjects(NumContexts, m_workerFinishedRenderFrame, TRUE, INFINITE);
	for (uint32_t i = 0; i < NumContexts; ++i)
	{
		MICROPROFILE_GPU_SUBMIT(g_QueueGraphics, m_pCurrentFrameResource->m_sceneMicroProfile[i]);
	}

	for (uint32_t i = 0; i < NumContexts; ++i)
	{
		MICROPROFILE_THREADLOGGPURESET(g_gpuThreadLog[i]);
	}



	ID3D12CommandList** ppCommandLists = m_pCurrentFrameResource->m_batchSubmit + NumContexts + 2;
	uint32_t nCount = _countof(m_pCurrentFrameResource->m_batchSubmit) - NumContexts - 2;
	m_commandQueue->ExecuteCommandLists(_countof(m_pCurrentFrameResource->m_batchSubmit) - NumContexts - 2, m_pCurrentFrameResource->m_batchSubmit + NumContexts + 2);
	
	g_FenceGraphics++;
	m_commandQueue->Signal(g_pFenceGraphics, g_FenceGraphics);


	MicroProfileFlip(0);

	m_cpuTimer.Tick(NULL);
	if (m_titleCount == TitleThrottle)
	{
		CHAR cpu[64];
		sprintf_s(cpu, "%.4f CPU", m_cpuTime / m_titleCount);
		SetCustomWindowText(cpu);

		m_titleCount = 0;
		m_cpuTime = 0;
	}
	else
	{
		m_titleCount++;
		m_cpuTime += m_cpuTimer.GetElapsedSeconds() * 1000;
		m_cpuTimer.ResetElapsedTime();
	}

	// Present and update the frame index for the next frame.
	PIXBeginEvent(m_commandQueue.Get(), 0, L"Presenting to screen");
	ThrowIfFailed(m_swapChain->Present(0, 0));
	PIXEndEvent(m_commandQueue.Get());
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	
	MICROPROFILE_SCOPEI("Main", "WaitPrev", 0);


	// Signal and increment the fence value.
	m_pCurrentFrameResource->m_fenceValue = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
	m_fenceValue++;
	g_nSrc = 1 - g_nSrc;
	g_nDst = 1 - g_nDst;

}

void D3D12Multithreading::OnDestroy()
{
	// Wait for the GPU to be done with all resources.
	{
		const UINT64 fence = m_fenceValue;
		const UINT64 lastCompletedFence = m_fence->GetCompletedValue();

		// Signal and increment the fence value.
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (lastCompletedFence < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
		CloseHandle(m_fenceEvent);
	}

	// Close thread events and thread handles.
	for (int i = 0; i < NumContexts; i++)
	{
		CloseHandle(m_workerBeginRenderFrame[i]);
		CloseHandle(m_workerFinishShadowPass[i]);
		CloseHandle(m_workerFinishedRenderFrame[i]);
		CloseHandle(m_threadHandles[i]);
	}

	for (int i = 0; i < _countof(m_frameResources); i++)
	{
		delete m_frameResources[i];
	}
}

bool D3D12Multithreading::OnEvent(MSG msg)
{
	switch (msg.message)
	{
	case WM_KEYDOWN:
		OnKeyDown(msg.wParam);
		break;
	case WM_KEYUP:
		OnKeyUp(msg.wParam);
		break;
	}

	return false;
}

void D3D12Multithreading::OnKeyDown(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_LEFT:
		m_keyboardInput.leftArrowPressed = true;
		break;
	case VK_RIGHT:
		m_keyboardInput.rightArrowPressed = true;
		break;
	case VK_UP:
		m_keyboardInput.upArrowPressed = true;
		break;
	case VK_DOWN:
		m_keyboardInput.downArrowPressed = true;
		break;
	case VK_SPACE:
		m_keyboardInput.animate = !m_keyboardInput.animate;
		break;
	}
}

void D3D12Multithreading::OnKeyUp(WPARAM wParam)
{
	switch (wParam)
	{
	case VK_LEFT:
		m_keyboardInput.leftArrowPressed = false;
		break;
	case VK_RIGHT:
		m_keyboardInput.rightArrowPressed = false;
		break;
	case VK_UP:
		m_keyboardInput.upArrowPressed = false;
		break;
	case VK_DOWN:
		m_keyboardInput.downArrowPressed = false;
		break;

	}
}

// Assemble the CommandListPre command list.
void D3D12Multithreading::BeginFrame()
{
	m_pCurrentFrameResource->Init();
	const UINT offset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	static UINT counter = 0;
	counter++;
	{
		ID3D12GraphicsCommandList* pCommandList = m_pCurrentFrameResource->m_commandListCompute.Get();
		MICROPROFILE_CONDITIONAL(MicroProfileThreadLogGpu* pGpuLog = MicroProfileThreadLogGpuAlloc());
		MICROPROFILE_GPU_BEGIN(pCommandList, pGpuLog);
		{
			MICROPROFILE_SCOPEGPU_TOKEN_L(pGpuLog, g_TokenGpuComputeFrameIndex[g_nDst]);
			MICROPROFILE_SCOPEGPUI_L(pGpuLog, "Compute-Demo", 0xffffffff);
			ID3D12DescriptorHeap* ppHeaps[] = { m_computeSrvUavHeap.Get() };
			pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
			pCommandList->SetComputeRootSignature(g_pComputeRootSignature);
			CD3DX12_GPU_DESCRIPTOR_HANDLE Handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_computeSrvUavHeap->GetGPUDescriptorHandleForHeapStart());
			CD3DX12_GPU_DESCRIPTOR_HANDLE Srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(Handle, 0 + g_nSrc, offset);
			CD3DX12_GPU_DESCRIPTOR_HANDLE Uav = CD3DX12_GPU_DESCRIPTOR_HANDLE(Handle, 2 + g_nDst, offset);
			pCommandList->SetComputeRootDescriptorTable(0, Srv);
			pCommandList->SetComputeRootDescriptorTable(1, Uav);
			pCommandList->SetComputeRoot32BitConstant(2, counter, 0);
			pCommandList->Dispatch(SampleAssets::VertexDataSize / (SampleAssets::StandardVertexStride * 128) + 1, 1, 1);
			counter++;
		}
		m_pCurrentFrameResource->m_commandListCompute->Close();
		ID3D12CommandList* pLists[] = { m_pCurrentFrameResource->m_commandListCompute.Get() };

		m_commandQueue->Wait(g_pFenceCompute, g_FenceCompute);

		ID3D12GraphicsCommandList* pTransition = m_pCurrentFrameResource->m_commandListTransition.Get();
		if (m_vbState[g_nSrc] != D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
		{
			pTransition->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBufferCompute[g_nSrc].Get(), m_vbState[g_nSrc], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
			m_vbState[g_nSrc] = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}

		if (m_vbState[g_nDst] != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			pTransition->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBufferCompute[g_nDst].Get(), m_vbState[g_nDst], D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
			m_vbState[g_nDst] = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}
		pTransition->Close();
		ID3D12CommandList* pListsTransition[] = { pTransition };
		m_commandQueue->ExecuteCommandLists(1, pListsTransition);

		g_pComputeQueue->Wait(g_pFenceGraphics, g_FenceGraphics);

		g_pComputeQueue->ExecuteCommandLists(1, pLists);
		g_FenceCompute++;
		g_pComputeQueue->Signal(g_pFenceCompute, g_FenceCompute);
		m_pCurrentFrameResource->m_computeFenceValue = g_FenceCompute;
		uint64_t nGpuBlock = MICROPROFILE_GPU_END(pGpuLog);
		MICROPROFILE_GPU_SUBMIT(g_QueueCompute, nGpuBlock);
		MICROPROFILE_CONDITIONAL( MicroProfileThreadLogGpuFree(pGpuLog) );
	}

	// Indicate that the back buffer will be used as a render target.
	m_pCurrentFrameResource->m_commandLists[CommandListPre]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//m_pCurrentFrameResource->m_commandLists[CommandListPre]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBufferSrc, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Clear the render target and depth stencil.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_pCurrentFrameResource->m_commandLists[CommandListPre]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_pCurrentFrameResource->m_commandLists[CommandListPre]->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListPre]->Close());
}

// Assemble the CommandListMid command list.
void D3D12Multithreading::MidFrame()
{
	// Transition our shadow map from the shadow pass to readable in the scene pass.
	m_pCurrentFrameResource->SwapBarriers();

	ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListMid]->Close());
}

// Assemble the CommandListPost command list.
void D3D12Multithreading::EndFrame()
{
	m_pCurrentFrameResource->Finish();

	// Indicate that the back buffer will now be used to present.
	m_pCurrentFrameResource->m_commandLists[CommandListPost]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_pCurrentFrameResource->m_commandLists[CommandListPost]->Close());
}

// Worker thread body. workerIndex is an integer from 0 to NumContexts 
// describing the worker's thread index.
void D3D12Multithreading::WorkerThread(int threadIndex)
{
	assert(threadIndex >= 0);
	assert(threadIndex < NumContexts);

	char threadname[16];
	snprintf(threadname, sizeof(threadname) - 1, "worker-%d", threadIndex);
	MicroProfileOnThreadCreate(threadname);
#if !SINGLETHREADED

	while (threadIndex >= 0 && threadIndex < NumContexts)
	{
		// Wait for main thread to tell us to draw.

		WaitForSingleObject(m_workerBeginRenderFrame[threadIndex], INFINITE);

#endif
		ID3D12GraphicsCommandList* pShadowCommandList = m_pCurrentFrameResource->m_shadowCommandLists[threadIndex].Get();
		ID3D12GraphicsCommandList* pSceneCommandList = m_pCurrentFrameResource->m_sceneCommandLists[threadIndex].Get();

		//
		// Shadow pass
		//
		

		MICROPROFILE_CONDITIONAL( MicroProfileThreadLogGpu* pMicroProfileLog = g_gpuThreadLog[threadIndex] );

		// Populate the command list.
		SetCommonPipelineState(pShadowCommandList);
		m_pCurrentFrameResource->Bind(pShadowCommandList, FALSE, nullptr, nullptr);	// No need to pass RTV or DSV descriptor heap.

		// Set null SRVs for the diffuse/normal textures.
		pShadowCommandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());

		// Distribute objects over threads by drawing only 1/NumContexts 
		// objects per worker (i.e. every object such that objectnum % 
		// NumContexts == threadIndex).
		PIXBeginEvent(pShadowCommandList, 0, L"Worker drawing shadow pass...");
		{
			MICROPROFILE_SCOPEI("CPU", "Shadows", 0xff00ff00);
			MICROPROFILE_GPU_BEGIN(pShadowCommandList, pMicroProfileLog);
			{
				MICROPROFILE_SCOPEGPU_TOKEN_L(pMicroProfileLog, g_TokenGpuFrameIndex[g_nSrc]);
 				MICROPROFILE_SCOPEGPUI_L(pMicroProfileLog, "Shadows", 0xff00);

				for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
				{
					SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];

					pShadowCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
				}
			}

			m_pCurrentFrameResource->m_shadowMicroProfile[threadIndex] = MICROPROFILE_GPU_END(pMicroProfileLog);

		}

		PIXEndEvent(pShadowCommandList);

		ThrowIfFailed(pShadowCommandList->Close());

#if !SINGLETHREADED
		// Submit shadow pass.
		SetEvent(m_workerFinishShadowPass[threadIndex]);
#endif

		//
		// Scene pass
		// 

		// Populate the command list.  These can only be sent after the shadow 
		// passes for this frame have been submitted.
		MICROPROFILE_GPU_BEGIN(pSceneCommandList, pMicroProfileLog);
		{
			MICROPROFILE_SCOPEI("CPU", "Scene", 0xff00ffff);
			MICROPROFILE_SCOPEGPU_TOKEN_L(pMicroProfileLog, g_TokenGpuFrameIndex[g_nSrc]);
			MICROPROFILE_SCOPEGPUI_L(pMicroProfileLog, "scene", 0xff0000);
			SetCommonPipelineState(pSceneCommandList);
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
			CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
			m_pCurrentFrameResource->Bind(pSceneCommandList, TRUE, &rtvHandle, &dsvHandle);

			PIXBeginEvent(pSceneCommandList, 0, L"Worker drawing scene pass...");

			D3D12_GPU_DESCRIPTOR_HANDLE cbvSrvHeapStart = m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart();
			const UINT cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			const UINT nullSrvCount = 2;
			for (int j = threadIndex; j < _countof(SampleAssets::Draws); j += NumContexts)
			{
				SampleAssets::DrawParameters drawArgs = SampleAssets::Draws[j];

				// Set the diffuse and normal textures for the current object.
				CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(cbvSrvHeapStart, nullSrvCount + drawArgs.DiffuseTextureIndex, cbvSrvDescriptorSize);
				pSceneCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);

				pSceneCommandList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.IndexStart, drawArgs.VertexBase, 0);
			}

			PIXEndEvent(pSceneCommandList);
		}
		m_pCurrentFrameResource->m_sceneMicroProfile[threadIndex] = MICROPROFILE_GPU_END(pMicroProfileLog);
		ThrowIfFailed(pSceneCommandList->Close());

#if !SINGLETHREADED
		// Tell main thread that we are done.
		SetEvent(m_workerFinishedRenderFrame[threadIndex]); 
	}
#endif
}

void D3D12Multithreading::SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList)
{
	pCommandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
	pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	pCommandList->RSSetViewports(1, &m_viewport);
	pCommandList->RSSetScissorRects(1, &m_scissorRect);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewCompute[g_nSrc]);
	pCommandList->IASetIndexBuffer(&m_indexBufferView);
	pCommandList->SetGraphicsRootDescriptorTable(3, m_samplerHeap->GetGPUDescriptorHandleForHeapStart());
	pCommandList->OMSetStencilRef(0);

	// Render targets and depth stencil are set elsewhere because the 
	// depth stencil depends on the frame resource being used.

	// Constant buffers are set elsewhere because they depend on the 
	// frame resource being used.

	// SRVs are set elsewhere because they change based on the object 
	// being drawn.
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	D3D12Multithreading sample(1280, 720, "D3D12 Multithreading Sample");
	return sample.Run(hInstance, nCmdShow);
}


FrameResource::FrameResource(ID3D12Device* pDevice, ID3D12PipelineState* pPso, ID3D12PipelineState* pShadowMapPso, ID3D12PipelineState* pComputePso,ID3D12DescriptorHeap* pDsvHeap, ID3D12DescriptorHeap* pCbvSrvHeap, D3D12_VIEWPORT* pViewport, UINT frameResourceIndex) :
	m_fenceValue(0),
	m_pipelineState(pPso),
	m_pipelineStateCompute(pComputePso),
	m_pipelineStateShadowMap(pShadowMapPso)
{
	m_computeFenceValue = 0;
	ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_commandAllocatorCompute)));
	ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocatorCompute.Get(), m_pipelineStateCompute.Get(), IID_PPV_ARGS(&m_commandListCompute)));
	ThrowIfFailed(m_commandListCompute->Close());
	ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorTransition)));
	ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorTransition.Get(), nullptr, IID_PPV_ARGS(&m_commandListTransition)));
	ThrowIfFailed(m_commandListTransition->Close());

	for (int i = 0; i < CommandListCount; i++)
	{
		ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
		ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandLists[i])));

		// Close these command lists; don't record into them for now.
		ThrowIfFailed(m_commandLists[i]->Close());
	}

	for (int i = 0; i < NumContexts; i++)
	{
		// Create command list allocators for worker threads. One alloc is 
		// for the shadow pass command list, and one is for the scene pass.
		ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_shadowCommandAllocators[i])));
		ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[i])));

		ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_shadowCommandAllocators[i].Get(), m_pipelineStateShadowMap.Get(), IID_PPV_ARGS(&m_shadowCommandLists[i])));
		ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandLists[i])));

		// Close these command lists; don't record into them for now. We will 
		// reset them to a recording state when we start the render loop.
		ThrowIfFailed(m_shadowCommandLists[i]->Close());
		ThrowIfFailed(m_sceneCommandLists[i]->Close());
	}

	// Describe and create the shadow map texture.
	CD3DX12_RESOURCE_DESC shadowTexDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		static_cast<UINT>(pViewport->Width),
		static_cast<UINT>(pViewport->Height),
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	D3D12_CLEAR_VALUE clearValue;		// Performance tip: Tell the runtime at resource creation the desired clear value.
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&shadowTexDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_shadowTexture)));

	// Get a handle to the start of the descriptor heap then offset 
	// it based on the frame resource index.
	const UINT dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle(pDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1 + frameResourceIndex, dsvDescriptorSize); // + 1 for the shadow map.

																																		  // Describe and create the shadow depth view and cache the CPU 
																																		  // descriptor handle.
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
	depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;
	pDevice->CreateDepthStencilView(m_shadowTexture.Get(), &depthStencilViewDesc, depthHandle);
	m_shadowDepthView = depthHandle;

	// Get a handle to the start of the descriptor heap then offset it 
	// based on the existing textures and the frame resource index. Each 
	// frame has 1 SRV (shadow tex) and 2 CBVs.
	const UINT nullSrvCount = 2;								// Null descriptors at the start of the heap.
	const UINT textureCount = _countof(SampleAssets::Textures);	// Diffuse + normal textures near the start of the heap.  Ideally, track descriptor heap contents/offsets at a higher level.
	const UINT cbvSrvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(pCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(pCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
	m_nullSrvHandle = cbvSrvGpuHandle;
	cbvSrvCpuHandle.Offset(nullSrvCount + textureCount + (frameResourceIndex * FrameCount), cbvSrvDescriptorSize);
	cbvSrvGpuHandle.Offset(nullSrvCount + textureCount + (frameResourceIndex * FrameCount), cbvSrvDescriptorSize);

	// Describe and create a shader resource view (SRV) for the shadow depth 
	// texture and cache the GPU descriptor handle. This SRV is for sampling 
	// the shadow map from our shader. It uses the same texture that we use 
	// as a depth-stencil during the shadow pass.
	D3D12_SHADER_RESOURCE_VIEW_DESC shadowSrvDesc = {};
	shadowSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	shadowSrvDesc.Texture2D.MipLevels = 1;
	shadowSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	pDevice->CreateShaderResourceView(m_shadowTexture.Get(), &shadowSrvDesc, cbvSrvCpuHandle);
	m_shadowDepthHandle = cbvSrvGpuHandle;

	// Increment the descriptor handles.
	cbvSrvCpuHandle.Offset(cbvSrvDescriptorSize);
	cbvSrvGpuHandle.Offset(cbvSrvDescriptorSize);

	// Create the constant buffers.
	const UINT constantBufferSize = (sizeof(ConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_shadowConstantBuffer)));
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_sceneConstantBuffer)));

	// Map the constant buffers and cache their heap pointers.
	ThrowIfFailed(m_shadowConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mp_shadowConstantBufferWO)));
	ThrowIfFailed(m_sceneConstantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mp_sceneConstantBufferWO)));

	// Create the constant buffer views: one for the shadow pass and
	// another for the scene pass.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = constantBufferSize;

	// Describe and create the shadow constant buffer view (CBV) and 
	// cache the GPU descriptor handle.
	cbvDesc.BufferLocation = m_shadowConstantBuffer->GetGPUVirtualAddress();
	pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
	m_shadowCbvHandle = cbvSrvGpuHandle;

	// Increment the descriptor handles.
	cbvSrvCpuHandle.Offset(cbvSrvDescriptorSize);
	cbvSrvGpuHandle.Offset(cbvSrvDescriptorSize);

	// Describe and create the scene constant buffer view (CBV) and 
	// cache the GPU descriptor handle.
	cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();
	pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
	m_sceneCbvHandle = cbvSrvGpuHandle;

	// Batch up command lists for execution later.
	const UINT batchSize = _countof(m_sceneCommandLists) + _countof(m_shadowCommandLists) + 3;
	m_batchSubmit[0] = m_commandLists[CommandListPre].Get();
	memcpy(m_batchSubmit + 1, m_shadowCommandLists, _countof(m_shadowCommandLists) * sizeof(ID3D12CommandList*));
	m_batchSubmit[_countof(m_shadowCommandLists) + 1] = m_commandLists[CommandListMid].Get();
	memcpy(m_batchSubmit + _countof(m_shadowCommandLists) + 2, m_sceneCommandLists, _countof(m_sceneCommandLists) * sizeof(ID3D12CommandList*));
	m_batchSubmit[batchSize - 1] = m_commandLists[CommandListPost].Get();
}

FrameResource::~FrameResource()
{
	for (int i = 0; i < CommandListCount; i++)
	{
		m_commandAllocators[i] = nullptr;
		m_commandLists[i] = nullptr;
	}

	m_shadowConstantBuffer = nullptr;
	m_sceneConstantBuffer = nullptr;

	for (int i = 0; i < NumContexts; i++)
	{
		m_shadowCommandLists[i] = nullptr;
		m_shadowCommandAllocators[i] = nullptr;

		m_sceneCommandLists[i] = nullptr;
		m_sceneCommandAllocators[i] = nullptr;
	}

	m_shadowTexture = nullptr;
}

// Builds and writes constant buffers from scratch to the proper slots for 
// this frame resource.
void FrameResource::WriteConstantBuffers(D3D12_VIEWPORT* pViewport, Camera* pSceneCamera, Camera lightCams[NumLights], LightState lights[NumLights])
{
	ConstantBuffer sceneConsts = {};
	ConstantBuffer shadowConsts = {};

	// Scale down the world a bit.
	::XMStoreFloat4x4(&sceneConsts.model, XMMatrixScaling(0.1f, 0.1f, 0.1f));
	::XMStoreFloat4x4(&shadowConsts.model, XMMatrixScaling(0.1f, 0.1f, 0.1f));

	// The scene pass is drawn from the camera.
	pSceneCamera->Get3DViewProjMatrices(&sceneConsts.view, &sceneConsts.projection, 90.0f, pViewport->Width, pViewport->Height);

	// The light pass is drawn from the first light.
	lightCams[0].Get3DViewProjMatrices(&shadowConsts.view, &shadowConsts.projection, 90.0f, pViewport->Width, pViewport->Height);

	for (int i = 0; i < NumLights; i++)
	{
		memcpy(&sceneConsts.lights[i], &lights[i], sizeof(LightState));
		memcpy(&shadowConsts.lights[i], &lights[i], sizeof(LightState));
	}

	// The shadow pass won't sample the shadow map, but rather write to it.
	shadowConsts.sampleShadowMap = FALSE;

	// The scene pass samples the shadow map.
	sceneConsts.sampleShadowMap = TRUE;

	shadowConsts.ambientColor = sceneConsts.ambientColor = { 0.1f, 0.2f, 0.3f, 1.0f };

	memcpy(mp_sceneConstantBufferWO, &sceneConsts, sizeof(ConstantBuffer));
	memcpy(mp_shadowConstantBufferWO, &shadowConsts, sizeof(ConstantBuffer));
}

void FrameResource::Init()
{
	while (m_computeFenceValue > g_pFenceCompute->GetCompletedValue()) //need to block explicit as graphics pipe only syncs one frame later
	{
		MICROPROFILE_SCOPEI("fence", "block-on-compute", 0);
		Sleep(1);
	}
	ThrowIfFailed(m_commandAllocatorCompute->Reset());
	ThrowIfFailed(m_commandListCompute->Reset(m_commandAllocatorCompute.Get(), m_pipelineStateCompute.Get()));
	ThrowIfFailed(m_commandAllocatorTransition->Reset());
	ThrowIfFailed(m_commandListTransition->Reset(m_commandAllocatorTransition.Get(), nullptr));

	// Reset the command allocators and lists for the main thread.
	for (int i = 0; i < CommandListCount; i++)
	{
		ThrowIfFailed(m_commandAllocators[i]->Reset());
		ThrowIfFailed(m_commandLists[i]->Reset(m_commandAllocators[i].Get(), m_pipelineState.Get()));
	}

	// Clear the depth stencil buffer in preparation for rendering the shadow map.
	m_commandLists[CommandListPre]->ClearDepthStencilView(m_shadowDepthView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Reset the worker command allocators and lists.
	for (int i = 0; i < NumContexts; i++)
	{
		ThrowIfFailed(m_shadowCommandAllocators[i]->Reset());
		ThrowIfFailed(m_shadowCommandLists[i]->Reset(m_shadowCommandAllocators[i].Get(), m_pipelineStateShadowMap.Get()));

		ThrowIfFailed(m_sceneCommandAllocators[i]->Reset());
		ThrowIfFailed(m_sceneCommandLists[i]->Reset(m_sceneCommandAllocators[i].Get(), m_pipelineState.Get()));
	}
}

void FrameResource::SwapBarriers()
{
	// Transition the shadow map from writeable to readable.
	m_commandLists[CommandListMid]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowTexture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void FrameResource::Finish()
{
	m_commandLists[CommandListPost]->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

// Sets up the descriptor tables for the worker command list to use 
// resources provided by frame resource.
void FrameResource::Bind(ID3D12GraphicsCommandList* pCommandList, BOOL scenePass, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE* pDsvHandle)
{
	if (scenePass)
	{
		// Scene pass. We use constant buf #2 and depth stencil #2
		// with rendering to the render target enabled.
		pCommandList->SetGraphicsRootDescriptorTable(2, m_shadowDepthHandle);		// Set the shadow texture as an SRV.
		pCommandList->SetGraphicsRootDescriptorTable(1, m_sceneCbvHandle);

		assert(pRtvHandle != nullptr);
		assert(pDsvHandle != nullptr);

		pCommandList->OMSetRenderTargets(1, pRtvHandle, FALSE, pDsvHandle);
	}
	else
	{
		// Shadow pass. We use constant buf #1 and depth stencil #1
		// with rendering to the render target disabled.
		pCommandList->SetGraphicsRootDescriptorTable(2, m_nullSrvHandle);			// Set a null SRV for the shadow texture.
		pCommandList->SetGraphicsRootDescriptorTable(1, m_shadowCbvHandle);

		pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &m_shadowDepthView);	// No render target needed for the shadow pass.
	}
}

