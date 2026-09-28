// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tb
{
// TextBlinder.ini 를 UTF-8 로 읽는다.
// GetPrivateProfileString 은 값 양 끝의 따옴표를 지우고 UTF-8 을 따로 다루지 않는다.
// 그래서 정규식을 그대로 읽으려고 직접 읽는다.
//
// 규칙:
// - [섹션], 키=값. 섹션과 키 이름은 대소문자를 가리지 않는다.
// - 키와 값의 앞뒤 공백은 뺀다. 값의 따옴표는 그대로 둔다.
// - ; 또는 # 로 시작하는 줄은 주석이다.
struct IniEntry
{
    std::string section;
    std::string key;
    std::string value;
};

class Ini
{
public:
    static Ini Parse(std::string_view text);

    std::optional<std::string> Get(std::string_view section, std::string_view key) const;

    // 섹션 안의 항목을 파일 순서대로 준다.
    std::vector<IniEntry> Section(std::string_view section) const;

private:
    std::vector<IniEntry> entries_;
};

// text 의 다른 줄은 그대로 두고 section 의 key 값만 바꾼다.
// 키가 없으면 섹션 끝에 넣고, 섹션이 없으면 파일 끝에 만든다.
std::string IniSetValue(std::string_view text, std::string_view section, std::string_view key, std::string_view value);
}
