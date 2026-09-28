// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <vector>

#include "Range.h"

namespace tb
{
enum class ToggleKind
{
    Mask,
    Unmask
};

struct ToggleAction
{
    ToggleKind kind;
    Range range;
};

// 가리기 / 풀기 명령이 할 일을 정한다 (계획 5.2 절 규칙 1 ~ 4).
// selections: 선택 영역들. 선택한 글이 없으면 start == end 다.
// masks: 지금 가린 범위들. 위치 순서로 정렬되어 있어야 한다.
std::vector<ToggleAction> DecideToggle(const std::vector<Range>& selections, const std::vector<Range>& masks);
}
