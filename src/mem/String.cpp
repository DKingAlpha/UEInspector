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


bool strEndsWith(std::string_view s, std::string_view sub)
{
    if (sub.size() > s.size()) return false;
    return std::equal(sub.rbegin(), sub.rend(), s.rbegin());
}

} // namespace kanan
