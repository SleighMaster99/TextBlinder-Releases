// SPDX-License-Identifier: GPL-3.0-only
#include "Settings.h"

#include "Notepad_plus_msgs.h"
#include "core/Color.h"
#include "core/Ini.h"

namespace tb
{
namespace
{
// 새로 만드는 설정 파일. UTF-8 (BOM 없음), 줄 끝은 CRLF.
constexpr std::string_view kDefaultSettings =
    "; TextBlinder settings. Save this file in Notepad++ to apply changes.\r\n"
    "; TextBlinder 설정 파일. Notepad++ 에서 저장하면 바로 적용된다.\r\n"
    "\r\n"
    "[Display]\r\n"
    "; Mask color, #RRGGBB. 가림 색.\r\n"
    "MaskColor=#808080\r\n"
    "\r\n"
    "[General]\r\n"
    "; auto (follow Notepad++), ko, en. 메뉴 언어.\r\n"
    "Language=auto\r\n"
    "\r\n"
    "[Rules]\r\n"
    "; Keys that start with Rule hold regular expressions. The matched text is masked.\r\n"
    "; Text before \\K stays visible. Case matters. Put (?i) first to ignore case.\r\n"
    "; Rule 로 시작하는 키에 정규식을 적는다. 맞은 글을 가린다. \\K 앞은 보인다.\r\n"
    "; 대소문자를 가린다. 무시하려면 맨 앞에 (?i) 를 쓴다.\r\n"
    "; Examples. Remove ; to use one. 예시다. 쓰려면 ; 를 지운다.\r\n"
    "; Rule1: case matters (password only). 대소문자를 가린다 (password 만).\r\n"
    "; Rule2: case ignored (password, Password, PASSWORD). 대소문자를 무시한다.\r\n"
    ";Rule1=password\\s*=\\s*\\K.+\r\n"
    ";Rule2=(?i)password\\s*=\\s*\\K.+\r\n";
}

std::wstring PluginConfigDir(HWND npp)
{
    const LRESULT len = ::SendMessage(npp, NPPM_GETPLUGINSCONFIGDIR, 0, 0);
    std::wstring dir(static_cast<size_t>(len) + 1, L'\0');
    ::SendMessage(npp, NPPM_GETPLUGINSCONFIGDIR, dir.size(), reinterpret_cast<LPARAM>(dir.data()));
    dir.resize(static_cast<size_t>(len));
    ::CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

void EnsureSettingsFile(const std::wstring& path)
{
    if (::GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
        WriteFileBytes(path, kDefaultSettings);
}

Settings LoadSettings(const std::wstring& path)
{
    EnsureSettingsFile(path);
    const Ini ini = Ini::Parse(ReadFileBytes(path).value_or(std::string(kDefaultSettings)));

    Settings s;
    s.maskColor = ParseMaskColor(ini.Get("Display", "MaskColor").value_or(""));
    s.language = ini.Get("General", "Language").value_or("auto");
    s.detectedLanguage = ini.Get("General", "DetectedLanguage").value_or("");
    s.rules = ReadRules(ini);
    return s;
}

void SaveDetectedLanguage(const std::wstring& path, Lang lang)
{
    const std::string text = ReadFileBytes(path).value_or(std::string(kDefaultSettings));
    WriteFileBytes(path, IniSetValue(text, "General", "DetectedLanguage", LangCode(lang)));
}

std::optional<std::string> ReadFileBytes(const std::wstring& path)
{
    HANDLE file = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return std::nullopt;

    std::optional<std::string> result;
    LARGE_INTEGER size{};
    if (::GetFileSizeEx(file, &size) && size.QuadPart < (1LL << 30))
    {
        std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
        DWORD read = 0;
        if (bytes.empty() || (::ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) && read == bytes.size()))
            result = std::move(bytes);
    }
    ::CloseHandle(file);
    return result;
}

bool WriteFileBytes(const std::wstring& path, std::string_view bytes)
{
    const std::wstring temp = path + L".tmp";
    HANDLE file = ::CreateFileW(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    DWORD written = 0;
    const bool ok = bytes.empty() || (::WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size());
    ::CloseHandle(file);
    if (!ok || !::MoveFileExW(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        ::DeleteFileW(temp.c_str());
        return false;
    }
    return true;
}
}
