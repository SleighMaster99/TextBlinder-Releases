// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <cstddef>

namespace tb
{
// Scintilla 위치 값 (바이트). Sci_Position 과 같은 크기다.
using Pos = std::ptrdiff_t;

// 문서의 한 범위. [start, end) 이다.
struct Range
{
    Pos start = 0;
    Pos end = 0;

    bool Empty() const { return start >= end; }
};

inline bool operator==(const Range& a, const Range& b)
{
    return a.start == b.start && a.end == b.end;
}
}
