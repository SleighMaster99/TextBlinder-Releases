// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <string_view>

namespace tb
{
// 기본 가림 색 #808080. 회색이라 RGB 와 BGR 값이 같다.
constexpr int kDefaultMaskColor = 0x808080;

// "#RRGGBB" 를 Scintilla 가 받는 BGR 정수로 바꾼다. 형식이 틀리면 기본값을 준다.
int ParseMaskColor(std::string_view text);
}
