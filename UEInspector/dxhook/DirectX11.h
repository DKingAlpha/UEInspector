#pragma once

#include <dxgi.h>
#include <d3d11.h>
#include <stdint.h>

extern WNDCLASSEX WindowClass;
extern HWND WindowHwnd;

bool InitWindow(const TCHAR* className, const TCHAR* windowName);

bool DeleteWindow();

bool CreateHook(uint16_t Index, void** Original, void* Function);

void DisableHook(uint16_t Index);
void DestroyHookAndMem();

namespace DirectX11 {
	bool Init(const TCHAR* className, const TCHAR* windowName);
}