#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace kanan {
//
// String utilities.
//

// Conversion functions for UTF8<->UTF16.
std::string narrow(std::wstring_view str);
std::wstring widen(std::string_view str);

template<typename ... Args>
std::string stringFormat(const std::string& format, Args ... args) {
    size_t size = 1 + snprintf(nullptr, 0, format.c_str(), args ...);  // Extra space for \0
    // unique_ptr<char[]> buf(new char[size]);
    char* bytes = (char*)calloc(size, 1);
    snprintf(bytes, size, format.c_str(), args ...);
    std::string result(bytes);
    free(bytes);
    return result;
}

bool strEndsWith(std::string_view s, std::string_view sub);


std::vector<std::string> split(std::string str, const std::string& delim);
} // namespace kanan
