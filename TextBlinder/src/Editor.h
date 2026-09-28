// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <vector>

#include "core/Range.h"

// 한 Scintilla 보기에 보내는 메시지들. indicator 의 범위는 문서에, 모양은 보기에 있다.
namespace tb::editor
{
// 가림 모양을 정한다. visible 이 false 면 불투명도를 0 으로 둔다.
void SetupIndicator(HWND sci, int indicator, int colorBgr, bool visible);

void Fill(HWND sci, int indicator, const Range& range);
void Clear(HWND sci, int indicator, const Range& range);
void ClearAll(HWND sci, int indicator);

// indicator 가 칠해진 범위들. 위치 순서다.
std::vector<Range> IndicatorRanges(HWND sci, int indicator);

// 선택 영역들. 선택한 글이 없으면 start == end 인 범위 하나다.
std::vector<Range> Selections(HWND sci);

// 정규식 (UTF-8) 에 맞는 범위를 문서 전체에서 찾는다. Notepad++ 의 Boost 정규식을 쓴다.
// 대소문자를 가린다. 무시하려면 정규식 맨 앞에 (?i) 를 쓴다. 정규식이 틀리면 false.
bool FindAllRegex(HWND sci, const std::string& pattern, std::vector<Range>& found);

// 문서의 [start, start + length) 바이트. 문서 밖이면 nullopt.
std::optional<std::string> TextRange(HWND sci, Pos start, Pos length);

Pos Length(HWND sci);
bool IsModified(HWND sci);

// 보기가 보여 주는 문서 (SCI_GETDOCPOINTER). 두 보기가 같은 문서를 보이면 값이 같다.
LRESULT DocPointer(HWND sci);
}
