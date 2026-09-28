// SPDX-License-Identifier: GPL-3.0-only
#include "Toggle.h"

namespace tb
{
namespace
{
// masks 가 r 전체를 빈틈없이 덮으면 true.
bool Covered(const Range& r, const std::vector<Range>& masks)
{
    Pos pos = r.start;
    for (const Range& m : masks)
    {
        if (m.end <= pos)
            continue;
        if (m.start > pos)
            break;
        pos = m.end;
        if (pos >= r.end)
            return true;
    }
    return pos >= r.end;
}
}

std::vector<ToggleAction> DecideToggle(const std::vector<Range>& selections, const std::vector<Range>& masks)
{
    std::vector<ToggleAction> actions;

    // 규칙 3: 선택한 글이 없으면 커서가 든 가림 범위를 푼다. 범위 끝 바로 뒤도 안으로 친다.
    if (selections.size() == 1 && selections[0].Empty())
    {
        const Pos caret = selections[0].start;
        for (const Range& m : masks)
        {
            if (m.start <= caret && caret <= m.end)
            {
                actions.push_back({ToggleKind::Unmask, m});
                break;
            }
        }
        return actions;
    }

    // 규칙 1, 2, 4: 선택 영역마다 따로 정한다. 다중 선택의 빈 선택은 건너뛴다.
    for (const Range& s : selections)
    {
        if (s.Empty())
            continue;
        actions.push_back({Covered(s, masks) ? ToggleKind::Unmask : ToggleKind::Mask, s});
    }
    return actions;
}
}
