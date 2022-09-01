# UEInterceptor

**DEVELOPMENT SUSPENDED**

## Features

View / Search / Edit / Invoke UE world objects / Methods / Static Functions / Blueprint / Properties / etc.

*currently only support simple invocation*

## Building
1. git clone **--recursive** <THIS REPO>
2. Download source code of Unreal Engine
3. Install DirectX SDK, update variables in `UEInterceptor/EnvSetup.props`
4. Open sln, build.
5. Fix UE source error popping up. May be 5 places, more or less.

## Usage
Inject dll to game.


### Credit
ImGUI: [ocornut/imgui](https://github.com/ocornut/imgui)
DXHook for ImGUI: [xMajdev/ImGuiHook](https://www.unknowncheats.me/forum/d3d-tutorials-and-source/457178-imgui-hook-directx12-directx11-directx9-x64-x86.html)
MinHook: [TsudaKageyu/minhook](https://github.com/TsudaKageyu/minhook)
Kanan's Pattern Scan
