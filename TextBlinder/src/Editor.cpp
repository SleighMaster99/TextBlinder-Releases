// SPDX-License-Identifier: GPL-3.0-only
#include "Editor.h"

#include "Scintilla.h"
#include "core/Text.h"

namespace tb::editor
{
namespace
{
LRESULT Call(HWND sci, UINT msg, WPARAM w = 0, LPARAM l = 0)
{
    return ::SendMessage(sci, msg, w, l);
}

// SCI_SETINDICATORCURRENT / VALUE 는 보기에 하나뿐인 상태라 다른 코드도 쓴다. 쓰고 나서 되돌린다.
class IndicatorScope
{
public:
    IndicatorScope(HWND sci, int indicator)
        : sci_(sci),
          oldCurrent_(Call(sci, SCI_GETINDICATORCURRENT)),
          oldValue_(Call(sci, SCI_GETINDICATORVALUE))
    {
        Call(sci_, SCI_SETINDICATORCURRENT, static_cast<WPARAM>(indicator));
        Call(sci_, SCI_SETINDICATORVALUE, 1);
    }

    ~IndicatorScope()
    {
        Call(sci_, SCI_SETINDICATORVALUE, static_cast<WPARAM>(oldValue_));
        Call(sci_, SCI_SETINDICATORCURRENT, static_cast<WPARAM>(oldCurrent_));
    }

    IndicatorScope(const IndicatorScope&) = delete;
    IndicatorScope& operator=(const IndicatorScope&) = delete;

private:
    HWND sci_;
    LRESULT oldCurrent_;
    LRESULT oldValue_;
};
}

void SetupIndicator(HWND sci, int indicator, int colorBgr, bool visible)
{
    const WPARAM ind = static_cast<WPARAM>(indicator);
    const LPARAM alpha = visible ? 255 : 0;
    Call(sci, SCI_INDICSETSTYLE, ind, INDIC_FULLBOX);
    Call(sci, SCI_INDICSETFORE, ind, colorBgr);
    Call(sci, SCI_INDICSETALPHA, ind, alpha);
    Call(sci, SCI_INDICSETOUTLINEALPHA, ind, alpha);
    // 글자를 먼저 그리고 그 위에 상자를 그린다.
    Call(sci, SCI_INDICSETUNDER, ind, FALSE);
}

void Fill(HWND sci, int indicator, const Range& range)
{
    if (range.Empty())
        return;
    IndicatorScope scope(sci, indicator);
    Call(sci, SCI_INDICATORFILLRANGE, static_cast<WPARAM>(range.start), range.end - range.start);
}

void Clear(HWND sci, int indicator, const Range& range)
{
    if (range.Empty())
        return;
    IndicatorScope scope(sci, indicator);
    Call(sci, SCI_INDICATORCLEARRANGE, static_cast<WPARAM>(range.start), range.end - range.start);
}

void ClearAll(HWND sci, int indicator)
{
    Clear(sci, indicator, {0, Length(sci)});
}

std::vector<Range> IndicatorRanges(HWND sci, int indicator)
{
    std::vector<Range> ranges;
    const WPARAM ind = static_cast<WPARAM>(indicator);
    const Pos length = Length(sci);
    Pos pos = 0;
    while (pos < length)
    {
        // INDICATOREND 는 pos 가 든 같은 값 구간의 끝을 준다. 칠한 적이 없으면 0 을 준다.
        const Pos end = Call(sci, SCI_INDICATOREND, ind, pos);
        if (end <= pos)
            break;
        if (Call(sci, SCI_INDICATORVALUEAT, ind, pos) != 0)
            ranges.push_back({pos, end});
        pos = end;
    }
    return ranges;
}

std::vector<Range> Selections(HWND sci)
{
    std::vector<Range> ranges;
    const LRESULT count = Call(sci, SCI_GETSELECTIONS);
    for (LRESULT i = 0; i < count; ++i)
    {
        const Pos start = Call(sci, SCI_GETSELECTIONNSTART, static_cast<WPARAM>(i));
        const Pos end = Call(sci, SCI_GETSELECTIONNEND, static_cast<WPARAM>(i));
        ranges.push_back({start, end});
    }
    return ranges;
}

bool FindAllRegex(HWND sci, const std::string& pattern, std::vector<Range>& found)
{
    // 문서가 UTF-8 이 아니면 정규식을 문서의 코드 페이지로 바꾼다.
    std::string query = pattern;
    const LRESULT codePage = Call(sci, SCI_GETCODEPAGE);
    if (codePage != SC_CP_UTF8)
    {
        const std::wstring wide = Utf8ToWide(pattern);
        const UINT cp = codePage == 0 ? CP_ACP : static_cast<UINT>(codePage);
        const int n = ::WideCharToMultiByte(cp, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        query.assign(static_cast<size_t>(n), '\0');
        ::WideCharToMultiByte(cp, 0, wide.data(), static_cast<int>(wide.size()), query.data(), n, nullptr, nullptr);
    }

    // 검색 대상 범위와 검색 옵션은 보기에 하나뿐인 상태다. 쓰고 나서 되돌린다.
    const LRESULT oldStart = Call(sci, SCI_GETTARGETSTART);
    const LRESULT oldEnd = Call(sci, SCI_GETTARGETEND);
    const LRESULT oldFlags = Call(sci, SCI_GETSEARCHFLAGS);
    Call(sci, SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_MATCHCASE);

    bool valid = true;
    const Pos length = Length(sci);
    Pos from = 0;
    while (from < length)
    {
        Call(sci, SCI_SETTARGETRANGE, static_cast<WPARAM>(from), length);
        const LRESULT pos = Call(sci, SCI_SEARCHINTARGET, query.size(), reinterpret_cast<LPARAM>(query.c_str()));
        if (pos == -1)
            break;
        if (pos < -1)
        {
            // -2 는 틀린 정규식, -3 은 검색 중 오류다 (Notepad++ BoostRegExSearch.cxx).
            valid = false;
            break;
        }
        const Pos start = Call(sci, SCI_GETTARGETSTART);
        const Pos end = Call(sci, SCI_GETTARGETEND);
        if (end > start)
            found.push_back({start, end});

        // 빈 결과여도 앞으로 나아가게 한다.
        Pos next = end;
        if (next <= from)
            next = Call(sci, SCI_POSITIONAFTER, static_cast<WPARAM>(from));
        if (next <= from)
            break;
        from = next;
    }

    Call(sci, SCI_SETSEARCHFLAGS, static_cast<WPARAM>(oldFlags));
    Call(sci, SCI_SETTARGETRANGE, static_cast<WPARAM>(oldStart), oldEnd);
    return valid;
}

std::optional<std::string> TextRange(HWND sci, Pos start, Pos length)
{
    if (start < 0 || length < 0 || start + length > Length(sci))
        return std::nullopt;
    std::string text(static_cast<size_t>(length) + 1, '\0');
    Sci_TextRangeFull tr{};
    tr.chrg.cpMin = start;
    tr.chrg.cpMax = start + length;
    tr.lpstrText = text.data();
    Call(sci, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<LPARAM>(&tr));
    text.resize(static_cast<size_t>(length));
    return text;
}

Pos Length(HWND sci)
{
    return Call(sci, SCI_GETLENGTH);
}

bool IsModified(HWND sci)
{
    return Call(sci, SCI_GETMODIFY) != 0;
}

LRESULT DocPointer(HWND sci)
{
    return Call(sci, SCI_GETDOCPOINTER);
}
}
