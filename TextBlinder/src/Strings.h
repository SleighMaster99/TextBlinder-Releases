// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "core/Language.h"

namespace tb
{
enum class TextId
{
    // 메뉴 글. 순서는 PluginDefinition.h 의 CommandIndex 와 같다.
    MenuToggleMask,
    MenuToggleVisible,
    MenuUnmaskAll,
    MenuOpenSettings,
    MenuShortcutMapper,

    // 상태 표시줄 글. %d 는 개수, %s 는 규칙 이름 목록이다.
    StatusSkipped,
    StatusBadRules,
};

const wchar_t* Text(Lang lang, TextId id);
}
