#include <Windows.h>
#include <tchar.h>
#include "dxhook/DX11Hook.h"
#include "dxhook/DirectX11.h"

static void PrepareForDllUnload()
{
	StopImguiHookThread();
	DestroyHookAndMem();
}

static DWORD WINAPI UnloadDllCurrentThread(void* args)
{
	FreeLibraryAndExitThread(ProcessInfo::Module, 0);
}

void SelfUnload() {
	CloseHandle(CreateThread(NULL, 0, UnloadDllCurrentThread, NULL, 0, 0));
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		ProcessInfo::Module = hModule;
		CloseHandle(CreateThread(0, 0, ImguiHookThread, NULL, 0, 0));
		break;
	case DLL_PROCESS_DETACH:
		PrepareForDllUnload();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}