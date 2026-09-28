// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <string>
#include <string_view>

namespace tb
{
// 앞뒤 공백 (스페이스, 탭, CR, LF) 을 뺀다.
std::string_view Trim(std::string_view s);

// ASCII 대소문자를 가리지 않고 비교한다. 설정 키와 값에 쓴다.
bool EqualsNoCase(std::string_view a, std::string_view b);
bool StartsWithNoCase(std::string_view s, std::string_view prefix);

std::wstring Utf8ToWide(std::string_view s);
std::string WideToUtf8(std::wstring_view s);
}
