// SPDX-License-Identifier: GPL-3.0-only
// This file is part of TextBlinder, based on PluginDefinition.cpp of the Notepad++ plugin template.
// Copyright (C)2022 Don HO <don.h@free.fr>
// Copyright (C)2026 SleighMaster99
#include "PluginDefinition.h"

#include <shlwapi.h>

#include <algorithm>
#include <cwchar>
#include <string>
#include <unordered_map>
#include <vector>

#include "Editor.h"
#include "Settings.h"
#include "Strings.h"
#include "core/Language.h"
#include "core/MaskStore.h"
#include "core/Rules.h"
#include "core/Text.h"
#include "core/Toggle.h"
#include "menuCmdID.h"

FuncItem funcItem[nbFunc];
NppData nppData;

namespace
{
using namespace tb;

// 기본 단축키. Notepad++ 는 이 포인터를 계속 쓰므로 정적 수명이어야 한다.
ShortcutKey g_toggleMaskKey{true, false, true, 'C'};
ShortcutKey g_toggleVisibleKey{true, false, true, 'V'};

// NPPM_ALLOCATEINDICATOR 로 받는다. 받기 전 (NPPN_READY 전) 에는 -1 이다.
// 선택 가림과 규칙 가림에 따로 쓴다. 그래서 규칙을 다시 찾아도 선택 가림은 지워지지 않는다.
int g_selectIndicator = -1;
int g_ruleIndicator = -1;

bool g_ready = false;
bool g_masksVisible = true;
Lang g_menuLang = Lang::En;
Settings g_settings;
std::wstring g_settingsPath;
std::wstring g_masksPath;

// TextBlinder.masks 의 내용. 시작할 때 읽고, 바뀔 때마다 파일 전체를 다시 쓴다.
std::vector<MaskRecord> g_records;

// 설정을 다시 읽을 때마다 올린다. 버퍼의 규칙 가림이 낡았는지 볼 때 쓴다.
int g_rulesVersion = 1;
// 이미 상태 표시줄에 알린 틀린 규칙. 글을 고칠 때마다 다시 알리지 않는다.
std::vector<std::string> g_reportedBadRules;
// 글을 고친 뒤 규칙을 다시 찾을 때까지 기다리는 타이머. 타자마다 찾지 않게 한다.
constexpr UINT kRuleSearchDelayMs = 300;
UINT_PTR g_ruleTimer = 0;
bool g_rulesPending[2] = {false, false};

// 편집 화면 두 개 밖에서 우리 문서를 보여 주는 Scintilla (문서 구조 창 등). 가림 모양을 설정해 둔 창이다.
// 이런 창은 사용자가 나중에 열 수 있어서 주기적으로 찾는다.
std::vector<HWND> g_sharedViews;
constexpr UINT kViewScanMs = 1000;
UINT_PTR g_viewScanTimer = 0;

// 조금 뒤에 상태 표시줄에 쓸 알림 (ShowStatus).
constexpr UINT kStatusDelayMs = 200;
UINT_PTR g_statusTimer = 0;
std::wstring g_pendingStatus;

struct BufferState
{
    // 이 버퍼에 기록을 적용했다. 적용 전에는 기록을 덮어쓰지 않는다.
    bool restored = false;
    // 마지막으로 본 선택 가림 (경로는 비움). Save All 은 숨은 버퍼도 저장하는데,
    // 숨은 버퍼의 indicator 는 읽을 수 없어서 이 값을 쓴다.
    std::vector<MaskRecord> cache;
    // NPPN_FILEBEFORERENAME 때의 경로.
    std::wstring oldPath;
    // 마지막으로 규칙을 찾았을 때의 g_rulesVersion. 0 이면 아직 찾지 않았다.
    int rulesVersion = 0;
};
std::unordered_map<UINT_PTR, BufferState> g_buffers;

// 보기마다 "글이 바뀌어 캐시가 낡았다" 표시. SCN_UPDATEUI 에서 한 번에 새로 만든다.
bool g_cacheStale[2] = {false, false};

HWND Npp()
{
    return nppData._nppHandle;
}

HWND ViewHandle(int view)
{
    return view == MAIN_VIEW ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

int ViewOf(HWND sci)
{
    if (sci == nppData._scintillaMainHandle)
        return MAIN_VIEW;
    if (sci == nppData._scintillaSecondHandle)
        return SUB_VIEW;
    return -1;
}

int CurrentView()
{
    int view = -1;
    ::SendMessage(Npp(), NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&view));
    return view;
}

// 보기에 걸린 버퍼. 보기가 숨어 있으면 0.
UINT_PTR BufferOfView(int view)
{
    const LRESULT index = ::SendMessage(Npp(), NPPM_GETCURRENTDOCINDEX, 0, view);
    if (index < 0)
        return 0;
    return static_cast<UINT_PTR>(::SendMessage(Npp(), NPPM_GETBUFFERIDFROMPOS, static_cast<WPARAM>(index), view));
}

// 버퍼를 보여 주는 보기. 어느 보기에도 걸려 있지 않으면 nullptr.
HWND ViewShowing(UINT_PTR buffer)
{
    for (int view : {MAIN_VIEW, SUB_VIEW})
    {
        if (buffer != 0 && BufferOfView(view) == buffer)
            return ViewHandle(view);
    }
    return nullptr;
}

std::wstring BufferPath(UINT_PTR buffer)
{
    const LRESULT len = ::SendMessage(Npp(), NPPM_GETFULLPATHFROMBUFFERID, buffer, 0);
    if (len <= 0)
        return {};
    std::wstring path(static_cast<size_t>(len) + 1, L'\0');
    ::SendMessage(Npp(), NPPM_GETFULLPATHFROMBUFFERID, buffer, reinterpret_cast<LPARAM>(path.data()));
    path.resize(static_cast<size_t>(len));
    return path;
}

// 저장한 적 없는 새 문서 ("new 1" 등) 는 경로가 없다.
bool HasFilePath(const std::wstring& path)
{
    return !path.empty() && !::PathIsRelativeW(path.c_str());
}

TextReader ReaderFor(HWND sci)
{
    return [sci](Pos start, Pos length) { return editor::TextRange(sci, start, length); };
}

void CALLBACK StatusTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    ::KillTimer(nullptr, g_statusTimer);
    g_statusTimer = 0;
    if (g_ready && !g_pendingStatus.empty())
        ::SendMessage(Npp(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, reinterpret_cast<LPARAM>(g_pendingStatus.c_str()));
    g_pendingStatus.clear();
}

// 파일을 여는 도중에는 Notepad++ 가 상태 표시줄 첫 칸을 다시 쓴다. 그래서 조금 뒤에 표시한다.
// 짧은 사이에 알림이 여러 개 오면 이어 붙인다.
void ShowStatus(const std::wstring& text)
{
    if (!g_pendingStatus.empty())
        g_pendingStatus += L" / ";
    g_pendingStatus += text;
    if (g_statusTimer != 0)
        ::KillTimer(nullptr, g_statusTimer);
    g_statusTimer = ::SetTimer(nullptr, 0, kStatusDelayMs, StatusTimerProc);
}

void SaveRecordsFile()
{
    WriteFileBytes(g_masksPath, SerializeMaskRecords(g_records));
}

void RefreshCache(UINT_PTR buffer, HWND sci)
{
    g_buffers[buffer].cache = MakeRecords(L"", editor::IndicatorRanges(sci, g_selectIndicator), ReaderFor(sci));
}

// 버퍼의 선택 가림을 기록 파일에 쓴다. 보기에 있으면 지금 범위를, 없으면 캐시를 쓴다.
void WriteRecords(UINT_PTR buffer)
{
    const auto it = g_buffers.find(buffer);
    if (it == g_buffers.end() || !it->second.restored)
        return;
    const std::wstring path = BufferPath(buffer);
    if (!HasFilePath(path))
        return;

    if (HWND sci = ViewShowing(buffer))
        RefreshCache(buffer, sci);
    std::vector<MaskRecord> records = it->second.cache;
    for (MaskRecord& r : records)
        r.path = path;
    ReplaceRecords(g_records, path, records);
    SaveRecordsFile();
}

// 기록 가운데 해시가 맞는 범위만 다시 가린다.
void Restore(UINT_PTR buffer, HWND sci)
{
    const std::wstring path = BufferPath(buffer);
    if (!HasFilePath(path))
        return;
    const RestoreResult result = MatchRecords(RecordsFor(g_records, path), ReaderFor(sci));
    for (const Range& r : result.ranges)
        editor::Fill(sci, g_selectIndicator, r);
    if (result.skipped > 0)
    {
        wchar_t text[256];
        swprintf_s(text, Text(g_menuLang, TextId::StatusSkipped), result.skipped);
        ShowStatus(text);
    }
}

// 규칙에 맞는 곳을 모두 다시 가린다. 틀린 규칙은 건너뛰고 한 번만 알린다.
void SearchRules(UINT_PTR buffer, HWND sci)
{
    editor::ClearAll(sci, g_ruleIndicator);
    std::wstring newBad;
    for (const Rule& rule : g_settings.rules)
    {
        std::vector<Range> found;
        if (!editor::FindAllRegex(sci, rule.pattern, found))
        {
            if (std::find(g_reportedBadRules.begin(), g_reportedBadRules.end(), rule.name) == g_reportedBadRules.end())
            {
                g_reportedBadRules.push_back(rule.name);
                if (!newBad.empty())
                    newBad += L", ";
                newBad += Utf8ToWide(rule.name);
            }
            continue;
        }
        for (const Range& r : found)
            editor::Fill(sci, g_ruleIndicator, r);
    }
    g_buffers[buffer].rulesVersion = g_rulesVersion;

    if (!newBad.empty())
    {
        wchar_t text[512];
        swprintf_s(text, Text(g_menuLang, TextId::StatusBadRules), newBad.c_str());
        ShowStatus(text);
    }
}

// 보기에 걸린 버퍼를 준비한다. 처음 보면 기록을 적용하고, 규칙 가림이 낡았으면 다시 찾는다.
// searchRules 가 true 면 낡지 않았어도 규칙을 다시 찾는다.
void PrepareView(int view, bool searchRules)
{
    if (!g_ready || view < 0)
        return;
    const UINT_PTR buffer = BufferOfView(view);
    if (buffer == 0)
        return;
    HWND sci = ViewHandle(view);
    BufferState& state = g_buffers[buffer];
    if (!state.restored)
    {
        state.restored = true;
        Restore(buffer, sci);
        RefreshCache(buffer, sci);
    }
    if (searchRules || state.rulesVersion != g_rulesVersion)
        SearchRules(buffer, sci);
}

void CALLBACK RuleTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    ::KillTimer(nullptr, g_ruleTimer);
    g_ruleTimer = 0;
    if (!g_ready)
        return;

    // 두 보기가 같은 문서를 보여 주면 한 번만 찾는다. indicator 범위는 문서에 있다.
    UINT_PTR searched = 0;
    for (int view : {MAIN_VIEW, SUB_VIEW})
    {
        if (!g_rulesPending[view])
            continue;
        g_rulesPending[view] = false;
        const UINT_PTR buffer = BufferOfView(view);
        if (buffer != 0 && buffer != searched)
        {
            SearchRules(buffer, ViewHandle(view));
            searched = buffer;
        }
    }
}

void ScheduleRuleSearch(int view)
{
    if (g_settings.rules.empty())
        return;
    g_rulesPending[view] = true;
    if (g_ruleTimer != 0)
        ::KillTimer(nullptr, g_ruleTimer);
    g_ruleTimer = ::SetTimer(nullptr, 0, kRuleSearchDelayMs, RuleTimerProc);
}

void StyleView(HWND sci)
{
    editor::SetupIndicator(sci, g_selectIndicator, g_settings.maskColor, g_masksVisible);
    editor::SetupIndicator(sci, g_ruleIndicator, g_settings.maskColor, g_masksVisible);
}

BOOL CALLBACK CollectScintilla(HWND hwnd, LPARAM lParam)
{
    wchar_t cls[32] = {};
    ::GetClassNameW(hwnd, cls, 32);
    if (std::wcscmp(cls, L"Scintilla") == 0)
        reinterpret_cast<std::vector<HWND>*>(lParam)->push_back(hwnd);
    return TRUE;
}

BOOL CALLBACK CollectFromTopWindow(HWND hwnd, LPARAM lParam)
{
    CollectScintilla(hwnd, lParam);
    ::EnumChildWindows(hwnd, CollectScintilla, lParam);
    return TRUE;
}

// 문서 구조 창처럼 편집 화면과 같은 문서를 보여 주는 Scintilla 에도 가림 모양을 설정한다.
// 가림 범위는 문서에 있어서 이런 창에도 있다. 하지만 모양은 창마다 따로라서, 설정하지 않으면 기본 모양 (밑줄) 으로 그린다.
// 플러그인 API 로는 이런 창의 핸들을 받을 수 없어서 Notepad++ 의 창을 뒤진다.
// 다른 문서를 쓰는 창 (찾기 결과 창, 다른 플러그인의 창) 은 건드리지 않는다.
void StyleSharedViews()
{
    const HWND mainView = ViewHandle(MAIN_VIEW);
    const HWND subView = ViewHandle(SUB_VIEW);
    const LRESULT docs[2] = {editor::DocPointer(mainView), editor::DocPointer(subView)};

    std::vector<HWND> found;
    ::EnumThreadWindows(::GetCurrentThreadId(), CollectFromTopWindow, reinterpret_cast<LPARAM>(&found));

    std::vector<HWND> styled;
    for (HWND h : found)
    {
        if (h == mainView || h == subView)
            continue;
        const bool known = std::find(g_sharedViews.begin(), g_sharedViews.end(), h) != g_sharedViews.end();
        if (!known)
        {
            const LRESULT doc = editor::DocPointer(h);
            if (doc == 0 || (doc != docs[0] && doc != docs[1]))
                continue;
            StyleView(h);
        }
        styled.push_back(h);
    }
    g_sharedViews = styled;  // 없어진 창은 뺀다
}

void CALLBACK ViewScanTimerProc(HWND, UINT, UINT_PTR, DWORD)
{
    if (g_ready)
        StyleSharedViews();
}

void ApplyIndicatorStyles()
{
    for (int view : {MAIN_VIEW, SUB_VIEW})
        StyleView(ViewHandle(view));
    // 색이나 보이기 상태가 바뀌면 이미 찾은 창도 다시 설정한다.
    for (HWND h : g_sharedViews)
        StyleView(h);
    StyleSharedViews();
}

void SetMenuTexts(Lang lang)
{
    g_menuLang = lang;
    HMENU menu = reinterpret_cast<HMENU>(::SendMessage(Npp(), NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0));
    if (!menu)
        return;
    for (int i = 0; i < nbFunc; ++i)
    {
        const UINT id = static_cast<UINT>(funcItem[i]._cmdID);
        // 메뉴 글 뒤의 "\t단축키" 는 Notepad++ 가 붙인 것이라 그대로 둔다.
        wchar_t current[256] = {};
        ::GetMenuStringW(menu, id, current, 256, MF_BYCOMMAND);
        const wchar_t* tab = std::wcschr(current, L'\t');
        std::wstring text = Text(lang, static_cast<TextId>(i));
        if (tab)
            text += tab;

        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING;
        mii.dwTypeData = text.data();
        ::SetMenuItemInfoW(menu, id, FALSE, &mii);
    }
}

std::string NativeLangFile()
{
    char name[MAX_PATH] = {};
    if (::SendMessage(Npp(), NPPM_GETNATIVELANGFILENAME, MAX_PATH, reinterpret_cast<LPARAM>(name)) <= 0)
        return {};
    return name;
}

// Notepad++ 언어를 확인해 메뉴 글을 바꾼다. auto 면 확인한 언어를 다음 시작 때를 위해 기억한다.
void ApplyMenuLanguage()
{
    const std::string native = NativeLangFile();
    if (IsAutoLanguage(g_settings.language))
    {
        const char* detected = LangCode(LangFromNativeFile(native));
        if (g_settings.detectedLanguage != detected)
        {
            SaveDetectedLanguage(g_settingsPath, LangFromNativeFile(native));
            g_settings.detectedLanguage = detected;
        }
    }
    SetMenuTexts(ResolveLanguage(g_settings.language, native));
}

void ReloadSettings()
{
    g_settings = LoadSettings(g_settingsPath);
    ++g_rulesVersion;
    g_reportedBadRules.clear();
    ApplyIndicatorStyles();
    ApplyMenuLanguage();
    PrepareView(MAIN_VIEW, true);
    PrepareView(SUB_VIEW, true);
}

void OnReady()
{
    int start = 0;
    if (!::SendMessage(Npp(), NPPM_ALLOCATEINDICATOR, 2, reinterpret_cast<LPARAM>(&start)))
        return;  // indicator 번호를 못 받으면 아무것도 하지 않는다 (Notepad++ 8.5.6 미만).
    g_selectIndicator = start;
    g_ruleIndicator = start + 1;

    if (const auto bytes = ReadFileBytes(g_masksPath))
        g_records = ParseMaskRecords(*bytes);
    ApplyIndicatorStyles();
    ApplyMenuLanguage();
    g_ready = true;
    PrepareView(MAIN_VIEW, true);
    PrepareView(SUB_VIEW, true);
    g_viewScanTimer = ::SetTimer(nullptr, 0, kViewScanMs, ViewScanTimerProc);
}

void OnFileSaved(UINT_PTR buffer)
{
    // 저장이 실패해도 알림이 온다. 아직 수정 상태면 디스크와 위치가 다를 수 있어서 쓰지 않는다.
    if (HWND sci = ViewShowing(buffer); sci && editor::IsModified(sci))
        return;
    if (SamePath(BufferPath(buffer), g_settingsPath))
        ReloadSettings();
    WriteRecords(buffer);
}

void OnBeforeClose(UINT_PTR buffer)
{
    // 저장 안 된 변경이 있으면 쓰지 않는다. 기록은 마지막으로 저장한 파일과 맞아야 한다.
    HWND sci = ViewShowing(buffer);
    if (!sci || editor::IsModified(sci))
        return;
    WriteRecords(buffer);
}

void OnFileClosed(UINT_PTR buffer)
{
    // 다른 보기에 복제본이 남아 있으면 상태를 지우지 않는다.
    if (::SendMessage(Npp(), NPPM_GETPOSFROMBUFFERID, buffer, MAIN_VIEW) == -1)
        g_buffers.erase(buffer);
}

void OnFileRenamed(UINT_PTR buffer)
{
    BufferState& state = g_buffers[buffer];
    const std::wstring newPath = BufferPath(buffer);
    if (HasFilePath(state.oldPath) && HasFilePath(newPath))
    {
        RenameRecords(g_records, state.oldPath, newPath);
        SaveRecordsFile();
    }
    state.oldPath.clear();
}

// 가림을 바꾼 뒤: 캐시를 새로 만들고, 저장 안 된 변경이 없으면 바로 기록한다.
void AfterMaskChange(UINT_PTR buffer, HWND sci)
{
    RefreshCache(buffer, sci);
    if (!editor::IsModified(sci))
        WriteRecords(buffer);
}

// ---- 메뉴 명령 ----

void ToggleMask()
{
    const int view = CurrentView();
    if (!g_ready || view < 0)
        return;
    PrepareView(view, false);
    HWND sci = ViewHandle(view);

    const auto actions = DecideToggle(editor::Selections(sci), editor::IndicatorRanges(sci, g_selectIndicator));
    for (const ToggleAction& a : actions)
    {
        if (a.kind == ToggleKind::Mask)
            editor::Fill(sci, g_selectIndicator, a.range);
        else
            editor::Clear(sci, g_selectIndicator, a.range);
    }
    if (!actions.empty())
        AfterMaskChange(BufferOfView(view), sci);
}

void ToggleVisible()
{
    if (!g_ready)
        return;
    g_masksVisible = !g_masksVisible;
    ApplyIndicatorStyles();
}

void UnmaskAll()
{
    const int view = CurrentView();
    if (!g_ready || view < 0)
        return;
    PrepareView(view, false);
    HWND sci = ViewHandle(view);
    const UINT_PTR buffer = BufferOfView(view);

    editor::ClearAll(sci, g_selectIndicator);
    RefreshCache(buffer, sci);
    const std::wstring path = BufferPath(buffer);
    if (HasFilePath(path))
    {
        ReplaceRecords(g_records, path, {});
        SaveRecordsFile();
    }
}

void OpenSettings()
{
    EnsureSettingsFile(g_settingsPath);
    ::SendMessage(Npp(), NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(g_settingsPath.c_str()));
}

void OpenShortcutMapper()
{
    ::SendMessage(Npp(), NPPM_MENUCOMMAND, 0, IDM_SETTING_SHORTCUT_MAPPER);
}

void SetCommand(int index, PFUNCPLUGINCMD func, ShortcutKey* key)
{
    lstrcpynW(funcItem[index]._itemName, Text(g_menuLang, static_cast<TextId>(index)), menuItemSize);
    funcItem[index]._pFunc = func;
    funcItem[index]._init2Check = false;
    funcItem[index]._pShKey = key;
}
}

void pluginInit(HANDLE /*hModule*/)
{
}

void pluginCleanUp()
{
}

void commandMenuInit()
{
    const std::wstring dir = PluginConfigDir(Npp());
    g_settingsPath = dir + L"\\TextBlinder.ini";
    g_masksPath = dir + L"\\TextBlinder.masks";
    g_settings = LoadSettings(g_settingsPath);

    // 이때는 Notepad++ 언어를 물을 수 없다. 지난번에 확인해 둔 언어로 메뉴를 만든다.
    g_menuLang = StartupLanguage(g_settings.language, g_settings.detectedLanguage);

    SetCommand(kCmdToggleMask, ToggleMask, &g_toggleMaskKey);
    SetCommand(kCmdToggleVisible, ToggleVisible, &g_toggleVisibleKey);
    SetCommand(kCmdUnmaskAll, UnmaskAll, nullptr);
    SetCommand(kCmdOpenSettings, OpenSettings, nullptr);
    SetCommand(kCmdShortcutMapper, OpenShortcutMapper, nullptr);
}

void onNotify(SCNotification* notifyCode)
{
    const UINT code = notifyCode->nmhdr.code;

    // Scintilla 알림은 두 편집 보기에서 온 것만 본다.
    const int view = ViewOf(static_cast<HWND>(notifyCode->nmhdr.hwndFrom));
    if (view >= 0)
    {
        switch (code)
        {
        case SCN_MODIFIED:
            if (notifyCode->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
            {
                g_cacheStale[view] = true;
                if (g_ready)
                    ScheduleRuleSearch(view);
            }
            break;
        case SCN_UPDATEUI:
            if (g_cacheStale[view] && g_ready)
            {
                g_cacheStale[view] = false;
                const UINT_PTR buffer = BufferOfView(view);
                if (buffer != 0 && g_buffers[buffer].restored)
                    RefreshCache(buffer, ViewHandle(view));
            }
            break;
        case SCN_FOCUSIN:
            // 현재 보기가 아닌 쪽에 버퍼가 걸리면 NPPN_BUFFERACTIVATED 가 오지 않는다.
            PrepareView(view, false);
            break;
        default:
            break;
        }
        return;
    }

    const UINT_PTR buffer = notifyCode->nmhdr.idFrom;
    switch (code)
    {
    case NPPN_READY:
        OnReady();
        break;
    case NPPN_BUFFERACTIVATED:
        PrepareView(CurrentView(), true);
        break;
    case NPPN_FILEBEFOREOPEN:
        // 버퍼 ID 는 메모리 주소라서 다시 쓰일 수 있다. 빈 새 문서는 NPPN_FILECLOSED 없이 닫히기도 해서
        // 그 ID 의 옛 상태가 남는다. 새로 여는 파일은 옛 상태를 버리고 처음부터 복원한다.
        g_buffers.erase(buffer);
        break;
    case NPPN_FILEOPENED:
        // 보통은 아직 보기에 걸리기 전이다. 걸려 있으면 바로 준비한다.
        for (int v : {MAIN_VIEW, SUB_VIEW})
        {
            if (buffer != 0 && BufferOfView(v) == buffer)
                PrepareView(v, true);
        }
        break;
    case NPPN_FILESAVED:
        if (g_ready)
            OnFileSaved(buffer);
        break;
    case NPPN_FILEBEFORECLOSE:
        if (g_ready)
            OnBeforeClose(buffer);
        break;
    case NPPN_FILECLOSED:
        OnFileClosed(buffer);
        break;
    case NPPN_FILEBEFORERENAME:
        g_buffers[buffer].oldPath = BufferPath(buffer);
        break;
    case NPPN_FILERENAMED:
        if (g_ready)
            OnFileRenamed(buffer);
        break;
    case NPPN_NATIVELANGCHANGED:
        ApplyMenuLanguage();
        break;
    case NPPN_SHUTDOWN:
        // DLL 이 내려간 뒤에 타이머가 부르지 않게 한다.
        g_ready = false;
        if (g_ruleTimer != 0)
            ::KillTimer(nullptr, g_ruleTimer);
        g_ruleTimer = 0;
        if (g_statusTimer != 0)
            ::KillTimer(nullptr, g_statusTimer);
        g_statusTimer = 0;
        if (g_viewScanTimer != 0)
            ::KillTimer(nullptr, g_viewScanTimer);
        g_viewScanTimer = 0;
        break;
    case NPPN_SHORTCUTREMAPPED:
        // Notepad++ 가 처음 이름으로 메뉴 글을 다시 쓴다. 지금 언어로 되돌린다.
        SetMenuTexts(g_menuLang);
        break;
    default:
        break;
    }
}
