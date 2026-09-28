// SPDX-License-Identifier: GPL-3.0-only
#include "Strings.h"

namespace tb
{
const wchar_t* Text(Lang lang, TextId id)
{
    const bool ko = (lang == Lang::Ko);
    switch (id)
    {
    case TextId::MenuToggleMask:
        return ko ? L"선택 영역 가리기 / 풀기" : L"Mask / Unmask Selection";
    case TextId::MenuToggleVisible:
        return ko ? L"가림 보이기 / 숨기기" : L"Show / Hide Masks";
    case TextId::MenuUnmaskAll:
        return ko ? L"모든 가림 풀기" : L"Unmask All";
    case TextId::MenuOpenSettings:
        return ko ? L"설정 파일 열기" : L"Open Settings File";
    case TextId::MenuShortcutMapper:
        return ko ? L"단축키 설정..." : L"Shortcut Mapper...";
    // 상태 표시줄 첫 칸은 창 너비에서 다른 칸 너비를 뺀 만큼이라 좁다 (1024 픽셀 창에서 약 255 픽셀).
    // 그 너비에 들어가게 짧게 쓴다.
    case TextId::StatusSkipped:
        return ko ? L"TextBlinder: 가림 %d 개 복원 안 됨 (파일 바뀜)"
                  : L"TextBlinder: %d mask(s) not restored (changed)";
    case TextId::StatusBadRules:
        return ko ? L"TextBlinder: 틀린 규칙 건너뜀: %s"
                  : L"TextBlinder: invalid rule skipped: %s";
    }
    return L"";
}
}
