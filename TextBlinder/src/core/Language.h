// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <optional>
#include <string_view>

namespace tb
{
enum class Lang
{
    En,
    Ko
};

// "ko" / "en" 을 읽는다. 대소문자는 가리지 않는다. 그 밖의 값은 nullopt.
std::optional<Lang> ParseLangCode(std::string_view code);

const char* LangCode(Lang lang);

// [General] Language 가 ko / en 이 아니면 auto 로 본다.
bool IsAutoLanguage(std::string_view setting);

// Notepad++ 언어 파일 이름이 korean.xml 이면 한국어, 그 밖이면 영어다.
Lang LangFromNativeFile(std::string_view nativeLangFile);

// 최종 언어. ko / en 고정값이 우선한다. auto 면 Notepad++ 언어 파일을 따른다.
Lang ResolveLanguage(std::string_view setting, std::string_view nativeLangFile);

// 플러그인을 읽을 때 (Notepad++ 언어를 아직 모를 때) 쓸 언어.
// auto 면 지난번에 확인해 둔 언어 (detected) 를 쓰고, 없으면 영어다.
Lang StartupLanguage(std::string_view setting, std::string_view detected);
}
