#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace kanan {
std::optional<uintptr_t> scan(const std::string& module, const std::string& pattern);
std::optional<uintptr_t> scan(const std::string& module, uintptr_t start, const std::string& pattern);
std::optional<uintptr_t> scan(uintptr_t start, size_t length, const std::string& pattern);
std::optional<uintptr_t> scan(const std::string& pattern);
} // namespace kanan
