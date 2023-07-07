#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kanan {
//
// Module utilities.
//
void* getModuleHandle(const wchar_t* module_wstr);
std::optional<size_t> getModuleSize(const std::string& module);
std::optional<size_t> getModuleSize(void* module);

// Note: This function doesn't validate the dll's headers so make sure you've
// done so before calling it.
std::optional<uintptr_t> ptrFromRVA(uint8_t* dll, uintptr_t rva);
} // namespace kanan
