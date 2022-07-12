#include <cstdarg>

#include <Windows.h>

#include "String.hpp"

using namespace std;

namespace kanan {
string narrow(wstring_view str) {
    auto length = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0, nullptr, nullptr);
    string narrowStr{};

    narrowStr.resize(length);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.length(), (LPSTR)narrowStr.c_str(), length, nullptr, nullptr);

    return narrowStr;
}

wstring widen(string_view str) {
    auto length = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), nullptr, 0);
    wstring wideStr{};

    wideStr.resize(length);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.length(), (LPWSTR)wideStr.c_str(), length);

    return wideStr;
}

string formatString(const char* format, va_list args) {
    va_list argsCopy{};

    va_copy(argsCopy, args);

    auto len = vsnprintf(nullptr, 0, format, argsCopy);

    va_end(argsCopy);

    if (len <= 0) {
        return {};
    }

    string buffer{};

    buffer.resize(len + 1, 0);
    vsnprintf(buffer.data(), buffer.size(), format, args);
    buffer.resize(buffer.size() - 1); // Removes the extra 0 vsnprintf adds.

    return buffer;
}

std::vector<std::string> split(std::string str, const std::string& delim) {
    std::vector<std::string> pieces{};
    std::size_t last{};
    std::size_t next{};

    while ((next = str.find(delim, last)) != std::string::npos) {
        pieces.emplace_back(str.substr(last, next - last));
        last = next + delim.length();
    }

    return pieces;
}
} // namespace kanan
