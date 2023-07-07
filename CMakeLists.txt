# basic cmake file

cmake_minimum_required(VERSION 3.25)

project(UEInspector)

set(UE_ROOT_DIR "G:/UE/UnrealEngine-4.21" CACHE FILEPATH "Unreal Engine Root Directory")
set(DXSDK_DIR "D:/DirectXSDK" CACHE FILEPATH "DirectX SDK Directory")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.cpp")
set(HEADERS_DIRECTORIES src src/dxhook src/gui src/mem src/res)

include(FetchContent)
FetchContent_Declare(
    imgui_project
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG        v1.89.7
)
FetchContent_MakeAvailable(imgui_project)
file(GLOB IMGUI_SOURCES
    ${imgui_project_SOURCE_DIR}/*.cpp
    ${imgui_project_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_project_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)
set(IMGUI_HEADER_DIRECTORIES ${imgui_project_SOURCE_DIR} ${imgui_project_SOURCE_DIR}/backends)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PRIVATE ${imgui_project_SOURCE_DIR})


FetchContent_Declare(
    minhook_project
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook
)
FetchContent_Populate(minhook_project)
file(GLOB MINHOOK_SOURCES ${minhook_project_SOURCE_DIR}/src/*.c)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(MINHOOK_SOURCES ${MINHOOK_SOURCES} "${minhook_project_SOURCE_DIR}/src/hde/hde64.c")
else()
  set(MINHOOK_SOURCES ${MINHOOK_SOURCES} "${minhook_project_SOURCE_DIR}/src/hde/hde32.c")
endif()
set(MINHOOK_HEADER_DIRECTORIES ${minhook_project_SOURCE_DIR}/src ${minhook_project_SOURCE_DIR}/src/hde)
add_library(minhook STATIC ${MINHOOK_SOURCES})
target_include_directories(minhook PRIVATE ${MINHOOK_HEADER_DIRECTORIES})

# UE Insepctor
add_library(UEInspector SHARED ${SOURCES})

target_compile_definitions(UEInspector PRIVATE
    UE_BUILD_MINIMAL=1
    COREUOBJECT_VTABLE=
    UBT_COMPILED_PLATFORM=Windows
    UE_BUILD_SHIPPING=1
    WITH_EDITOR=0
    WITH_EDITORONLY_DATA=0
    WITH_ENGINE=1
    WITH_UNREAL_DEVELOPER_TOOLS=0
    WITH_PLUGIN_SUPPORT=0
    WITH_SERVER_CODE=0
    IS_MONOLITHIC=1
    IS_PROGRAM=1
    PLATFORM_WINDOWS
    PLATFORM_64BITS
    CORE_API=
    COREUOBJECT_API=
    NOMINMAX
    WIN32
    NDEBUG
    UEINSPECTOR_EXPORTS
    _WINDOWS
    _USRDLL
    UNICODE
    _UNICODE
)
set(UE_INCLUDE_DIR
    ${UE_ROOT_DIR}/Engine/Source/Runtime/Core/Public
    ${UE_ROOT_DIR}/Engine/Source/Runtime/Core/Public/UObject
    ${UE_ROOT_DIR}/Engine/Source/Runtime/CoreUObject/Public
    ${UE_ROOT_DIR}/Engine/Source/Runtime/TraceLog/Public
)

target_include_directories(UEInspector PRIVATE ${HEADERS_DIRECTORIES} ${IMGUI_HEADER_DIRECTORIES} ${minhook_project_SOURCE_DIR}/include ${UE_INCLUDE_DIR})
target_link_directories(UEInspector PRIVATE ${DXSDK_DIR}/Lib)
target_link_libraries(UEInspector PRIVATE minhook imgui
    d3d11 d3dcompiler dxgi
)
