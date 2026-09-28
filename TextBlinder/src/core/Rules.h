// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <string>
#include <vector>

#include "Ini.h"

namespace tb
{
// 규칙 가림 한 개 (계획 5.6 절).
struct Rule
{
    std::string name;     // 키 이름. 예: Rule1
    std::string pattern;  // 정규식 (UTF-8). 맞은 글 전체를 가린다.
};

// [Rules] 섹션에서 이름이 Rule 로 시작하는 키를 파일 순서대로 읽는다. 값이 빈 키는 건너뛴다.
std::vector<Rule> ReadRules(const Ini& ini);
}
