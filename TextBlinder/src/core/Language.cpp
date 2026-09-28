// SPDX-License-Identifier: GPL-3.0-only
#include "Language.h"

#include "Text.h"

namespace tb
{
std::optional<Lang> ParseLangCode(std::string_view code)
{
    if (EqualsNoCase(code, "ko"))
        return Lang::Ko;
    if (EqualsNoCase(code, "en"))
        return Lang::En;
    return std::nullopt;
}

const char* LangCode(Lang lang)
{
    return lang == Lang::Ko ? "ko" : "en";
}

bool IsAutoLanguage(std::string_view setting)
{
    return !ParseLangCode(setting).has_value();
}

Lang LangFromNativeFile(std::string_view nativeLangFile)
{
    return EqualsNoCase(nativeLangFile, "korean.xml") ? Lang::Ko : Lang::En;
}

Lang ResolveLanguage(std::string_view setting, std::string_view nativeLangFile)
{
    if (const auto fixed = ParseLangCode(setting))
        return *fixed;
    return LangFromNativeFile(nativeLangFile);
}

Lang StartupLanguage(std::string_view setting, std::string_view detected)
{
    if (const auto fixed = ParseLangCode(setting))
        return *fixed;
    return ParseLangCode(detected).value_or(Lang::En);
}
}
