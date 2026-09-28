// SPDX-License-Identifier: GPL-3.0-only
#include "Text.h"

#include <windows.h>

namespace tb
{
namespace
{
bool IsSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

char Lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}
}

std::string_view Trim(std::string_view s)
{
    while (!s.empty() && IsSpace(s.front()))
        s.remove_prefix(1);
    while (!s.empty() && IsSpace(s.back()))
        s.remove_suffix(1);
    return s;
}

bool EqualsNoCase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (Lower(a[i]) != Lower(b[i]))
            return false;
    }
    return true;
}

bool StartsWithNoCase(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && EqualsNoCase(s.substr(0, prefix.size()), prefix);
}

std::wstring Utf8ToWide(std::string_view s)
{
    if (s.empty())
        return {};
    const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}

std::string WideToUtf8(std::wstring_view s)
{
    if (s.empty())
        return {};
    const int n = ::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(n), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), out.data(), n, nullptr, nullptr);
    return out;
}
}
