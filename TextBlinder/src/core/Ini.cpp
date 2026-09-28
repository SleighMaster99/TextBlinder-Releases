// SPDX-License-Identifier: GPL-3.0-only
#include "Ini.h"

#include "Text.h"

namespace tb
{
namespace
{
constexpr std::string_view kUtf8Bom = "\xEF\xBB\xBF";

// 줄 끝 문자를 포함해 줄로 나눈다. 마지막 줄은 줄 끝이 없을 수 있다.
std::vector<std::string> SplitLinesKeepEnds(std::string_view text)
{
    std::vector<std::string> lines;
    while (!text.empty())
    {
        const size_t nl = text.find('\n');
        const size_t len = (nl == std::string_view::npos) ? text.size() : nl + 1;
        lines.emplace_back(text.substr(0, len));
        text.remove_prefix(len);
    }
    return lines;
}

bool IsComment(std::string_view t)
{
    return !t.empty() && (t[0] == ';' || t[0] == '#');
}

// "[이름]" 줄이면 이름을 준다.
std::optional<std::string_view> SectionName(std::string_view t)
{
    if (t.size() >= 2 && t.front() == '[' && t.back() == ']')
        return Trim(t.substr(1, t.size() - 2));
    return std::nullopt;
}

// "키=값" 줄이면 키를 준다.
std::optional<std::string_view> KeyName(std::string_view t)
{
    if (t.empty() || IsComment(t))
        return std::nullopt;
    const size_t eq = t.find('=');
    if (eq == std::string_view::npos)
        return std::nullopt;
    const std::string_view key = Trim(t.substr(0, eq));
    if (key.empty())
        return std::nullopt;
    return key;
}
}

Ini Ini::Parse(std::string_view text)
{
    if (text.substr(0, kUtf8Bom.size()) == kUtf8Bom)
        text.remove_prefix(kUtf8Bom.size());

    Ini ini;
    std::string section;
    for (const std::string& line : SplitLinesKeepEnds(text))
    {
        const std::string_view t = Trim(line);
        if (const auto name = SectionName(t))
        {
            section = std::string(*name);
            continue;
        }
        if (const auto key = KeyName(t))
        {
            const std::string_view value = Trim(t.substr(t.find('=') + 1));
            ini.entries_.push_back({section, std::string(*key), std::string(value)});
        }
    }
    return ini;
}

std::optional<std::string> Ini::Get(std::string_view section, std::string_view key) const
{
    for (const IniEntry& e : entries_)
    {
        if (EqualsNoCase(e.section, section) && EqualsNoCase(e.key, key))
            return e.value;
    }
    return std::nullopt;
}

std::vector<IniEntry> Ini::Section(std::string_view section) const
{
    std::vector<IniEntry> out;
    for (const IniEntry& e : entries_)
    {
        if (EqualsNoCase(e.section, section))
            out.push_back(e);
    }
    return out;
}

std::string IniSetValue(std::string_view text, std::string_view section, std::string_view key, std::string_view value)
{
    const std::string nl = (text.find("\r\n") != std::string_view::npos || text.find('\n') == std::string_view::npos) ? "\r\n" : "\n";
    const std::string newLine = std::string(key) + "=" + std::string(value);

    std::vector<std::string> lines = SplitLinesKeepEnds(text);
    bool inSection = false;
    int sectionLast = -1;  // 섹션의 마지막 내용 줄. 새 키를 이 뒤에 넣는다.
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::string_view t = Trim(lines[i]);
        if (const auto name = SectionName(t))
        {
            if (inSection)
                break;
            inSection = EqualsNoCase(*name, section);
            if (inSection)
                sectionLast = static_cast<int>(i);
            continue;
        }
        if (!inSection)
            continue;
        if (const auto k = KeyName(t); k && EqualsNoCase(*k, key))
        {
            // 사용자가 쓴 키 이름과 줄 끝은 그대로 둔다.
            const size_t endPos = lines[i].find_first_of("\r\n");
            const std::string ending = (endPos == std::string::npos) ? "" : lines[i].substr(endPos);
            lines[i] = std::string(*k) + "=" + std::string(value) + ending;
            std::string out;
            for (const std::string& l : lines)
                out += l;
            return out;
        }
        if (!t.empty())
            sectionLast = static_cast<int>(i);
    }

    auto ensureEnding = [&](std::string& line) {
        if (!line.empty() && line.back() != '\n')
            line += nl;
    };

    if (sectionLast >= 0)
    {
        ensureEnding(lines[static_cast<size_t>(sectionLast)]);
        lines.insert(lines.begin() + sectionLast + 1, newLine + nl);
    }
    else
    {
        if (!lines.empty())
        {
            ensureEnding(lines.back());
            lines.push_back(nl);
        }
        lines.push_back("[" + std::string(section) + "]" + nl);
        lines.push_back(newLine + nl);
    }

    std::string out;
    for (const std::string& l : lines)
        out += l;
    return out;
}
}
