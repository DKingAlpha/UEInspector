#include "DX11Hook.h"
#include "DirectX11.h"

#include "../gui/MainMenu.h"
#include "../res/droidsans.cpp"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <psapi.h>
#include <tchar.h>

bool ShowMenu = true;
bool ImGui_Initialised = false;

HANDLE g_StopMutex = NULL;

void StopImguiHookThread()
{
	g_StopMutex = CreateMutex(NULL, FALSE, NULL);
	WaitForSingleObject(g_StopMutex, 5000);	// allow up to 5s for graceful exit.
	CloseHandle(g_StopMutex);
}

namespace ProcessInfo {
	DWORD ID;
	HANDLE Handle;
	HWND Hwnd;
	HMODULE Module;
	WNDPROC WndProc;
	int WindowWidth;
	int WindowHeight;
	LPTSTR Title;
	LPTSTR ClassName;
	LPTSTR Path;
}

namespace DirectX11Interface {
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	ID3D11RenderTargetView* RenderTargetView;
}


typedef HRESULT(APIENTRY* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
IDXGISwapChainPresent oIDXGISwapChainPresent;

typedef HRESULT(__stdcall* IDXGIResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
IDXGIResizeBuffers oIDXGIResizeBuffers;

typedef void(APIENTRY* ID3D11DrawIndexed)(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation);
ID3D11DrawIndexed oID3D11DrawIndexed;


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ShowMenu) {
		ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
		return true;
	}
	return CallWindowProc(ProcessInfo::WndProc, hwnd, uMsg, wParam, lParam);
}

HRESULT APIENTRY MJPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	static ImFont* font = nullptr;
	if (!ImGui_Initialised) {
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&DirectX11Interface::Device))){
			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			// ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

			DirectX11Interface::Device->GetImmediateContext(&DirectX11Interface::DeviceContext);

			DXGI_SWAP_CHAIN_DESC Desc;
			pSwapChain->GetDesc(&Desc);
			WindowHwnd = Desc.OutputWindow;

			ID3D11Texture2D* BackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
			DirectX11Interface::Device->CreateRenderTargetView(BackBuffer, NULL, &DirectX11Interface::RenderTargetView);
			BackBuffer->Release();

			ImGui_ImplWin32_Init(WindowHwnd);
			ImGui_ImplDX11_Init(DirectX11Interface::Device, DirectX11Interface::DeviceContext);
			ImGui_ImplDX11_CreateDeviceObjects();
			io.ImeWindowHandle = ProcessInfo::Hwnd;
			ProcessInfo::WndProc = (WNDPROC)SetWindowLongPtr(ProcessInfo::Hwnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
			ImGui_Initialised = true;
		}
	}
	if (GetAsyncKeyState(OPEN_MENU_KEY) & 1)
		ShowMenu = !ShowMenu;
	
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().MouseDrawCursor = ShowMenu;
	if (ShowMenu == true) {
		ShowMenu = DrawMainMenu();
	}
	ImGui::EndFrame();
	ImGui::Render();
	DirectX11Interface::DeviceContext->OMSetRenderTargets(1, &DirectX11Interface::RenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	return oIDXGISwapChainPresent(pSwapChain, SyncInterval, Flags);
}

void APIENTRY MJDrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) {
	return oID3D11DrawIndexed(pContext, IndexCount, StartIndexLocation, BaseVertexLocation);
}

HRESULT APIENTRY MJResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	if (DirectX11Interface::RenderTargetView) {
		DirectX11Interface::DeviceContext->OMSetRenderTargets(0, 0, 0);
		DirectX11Interface::RenderTargetView->Release();
	}

	HRESULT hr = oIDXGIResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
	// Perform error handling here!

	DirectX11Interface::Device->CreateRenderTargetView(pBuffer, NULL, &DirectX11Interface::RenderTargetView);
	// Perform error handling here!
	pBuffer->Release();

	DirectX11Interface::DeviceContext->OMSetRenderTargets(1, &DirectX11Interface::RenderTargetView, NULL);

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = Width;
	vp.Height = Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DirectX11Interface::DeviceContext->RSSetViewports(1, &vp);
	return hr;
}


DWORD WINAPI ImguiHookThread(LPVOID lpParameter) {
	bool WindowFocus = false;
	while (!g_StopMutex && WindowFocus == false) {
		DWORD ForegroundWindowProcessID;
		GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
		if (GetCurrentProcessId() == ForegroundWindowProcessID) {

			ProcessInfo::ID = GetCurrentProcessId();
			ProcessInfo::Handle = GetCurrentProcess();
			ProcessInfo::Hwnd = GetForegroundWindow();

			RECT TempRect;
			GetWindowRect(ProcessInfo::Hwnd, &TempRect);
			ProcessInfo::WindowWidth = TempRect.right - TempRect.left;
			ProcessInfo::WindowHeight = TempRect.bottom - TempRect.top;

			TCHAR TempTitle[MAX_PATH];
			GetWindowText(ProcessInfo::Hwnd, TempTitle, sizeof(TempTitle) / sizeof(TCHAR));
			ProcessInfo::Title = TempTitle;

			TCHAR TempClassName[MAX_PATH];
			GetClassName(ProcessInfo::Hwnd, TempClassName, sizeof(TempClassName) / sizeof(TCHAR));
			ProcessInfo::ClassName = TempClassName;

			TCHAR TempPath[MAX_PATH];
			GetModuleFileNameEx(ProcessInfo::Handle, NULL, TempPath, sizeof(TempPath) / sizeof(TCHAR));
			ProcessInfo::Path = TempPath;

			WindowFocus = true;
		}
	}
	const TCHAR* guiClassName = _T("UEInspector");
	const TCHAR* guiWindowName = _T("UE Inspector");

	bool InitHook = false;
	while (!g_StopMutex && InitHook == false) {
		if (DirectX11::Init(guiClassName, guiWindowName) == true) {
		    CreateHook(8, (void**)&oIDXGISwapChainPresent, MJPresent);
			CreateHook(13, (void**)&oIDXGIResizeBuffers, MJResizeBuffers);
			CreateHook(12, (void**)&oID3D11DrawIndexed, MJDrawIndexed);
			InitHook = true;
		}
	}

	if (g_StopMutex) {
		ReleaseMutex(g_StopMutex);
	}
	return 0;
}
