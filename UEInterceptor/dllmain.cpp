#include <Windows.h>
#include <tchar.h>
#include "dxhook/DX11Hook.h"
#include "dxhook/DirectX11.h"


BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		ProcessInfo::Module = hModule;
		CreateThread(0, 0, ImguiHookThread, 0, 0, 0);
		break;
	case DLL_PROCESS_DETACH:
		DestroyHookAndMem();
		StopImguiHookThread();
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