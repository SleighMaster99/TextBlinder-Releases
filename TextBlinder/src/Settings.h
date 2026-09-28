// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <windows.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/Language.h"
#include "core/Rules.h"

namespace tb
{
// TextBlinder.ini 에서 읽은 값 (계획 5.4 절).
struct Settings
{
    int maskColor = 0;
    std::string language;          // [General] Language: auto, ko, en
    std::string detectedLanguage;  // [General] DetectedLanguage: 지난번에 확인한 Notepad++ 언어
    std::vector<Rule> rules;       // [Rules]
};

// NPPM_GETPLUGINSCONFIGDIR 폴더. 없으면 만든다.
std::wstring PluginConfigDir(HWND npp);

// 설정 파일이 없으면 기본 내용으로 만든다.
void EnsureSettingsFile(const std::wstring& path);

// 파일이 없으면 기본 파일을 만든 뒤 읽는다.
Settings LoadSettings(const std::wstring& path);

// [General] DetectedLanguage 만 고쳐 쓴다. 다른 줄은 그대로 둔다.
void SaveDetectedLanguage(const std::wstring& path, Lang lang);

std::optional<std::string> ReadFileBytes(const std::wstring& path);

// 임시 파일에 쓴 뒤 바꿔 넣는다. 쓰다가 멈춰도 원래 파일이 깨지지 않는다.
bool WriteFileBytes(const std::wstring& path, std::string_view bytes);
}
