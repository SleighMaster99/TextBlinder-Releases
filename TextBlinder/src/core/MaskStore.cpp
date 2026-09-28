// SPDX-License-Identifier: GPL-3.0-only
#include "MaskStore.h"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <charconv>

#include "Text.h"

namespace tb
{
namespace
{
std::optional<Pos> ParsePos(std::string_view s)
{
    Pos value = 0;
    const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || end != s.data() + s.size() || value < 0)
        return std::nullopt;
    return value;
}

bool IsHashText(std::string_view s)
{
    if (s.size() != 64)
        return false;
    return std::all_of(s.begin(), s.end(), [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); });
}

// 탭으로 나눈 칸이 정확히 4 개일 때만 채운다.
bool SplitFields(std::string_view line, std::string_view (&fields)[4])
{
    for (int i = 0; i < 3; ++i)
    {
        const size_t tab = line.find('\t');
        if (tab == std::string_view::npos)
            return false;
        fields[i] = line.substr(0, tab);
        line.remove_prefix(tab + 1);
    }
    if (line.find('\t') != std::string_view::npos)
        return false;
    fields[3] = line;
    return true;
}
}

std::vector<MaskRecord> ParseMaskRecords(std::string_view utf8)
{
    std::vector<MaskRecord> records;
    while (!utf8.empty())
    {
        const size_t nl = utf8.find('\n');
        std::string_view line = utf8.substr(0, nl);
        utf8.remove_prefix(nl == std::string_view::npos ? utf8.size() : nl + 1);
        if (!line.empty() && line.back() == '\r')
            line.remove_suffix(1);

        std::string_view f[4];
        if (!SplitFields(line, f) || f[0].empty())
            continue;
        const auto start = ParsePos(f[1]);
        const auto length = ParsePos(f[2]);
        if (!start || !length || *length == 0 || !IsHashText(f[3]))
            continue;
        records.push_back({Utf8ToWide(f[0]), *start, *length, std::string(f[3])});
    }
    return records;
}

std::string SerializeMaskRecords(const std::vector<MaskRecord>& records)
{
    std::string out;
    for (const MaskRecord& r : records)
    {
        out += WideToUtf8(r.path);
        out += '\t';
        out += std::to_string(r.start);
        out += '\t';
        out += std::to_string(r.length);
        out += '\t';
        out += r.hash;
        out += "\r\n";
    }
    return out;
}

bool SamePath(std::wstring_view a, std::wstring_view b)
{
    return ::CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
}

std::vector<MaskRecord> RecordsFor(const std::vector<MaskRecord>& all, std::wstring_view path)
{
    std::vector<MaskRecord> out;
    for (const MaskRecord& r : all)
    {
        if (SamePath(r.path, path))
            out.push_back(r);
    }
    return out;
}

void ReplaceRecords(std::vector<MaskRecord>& all, std::wstring_view path, const std::vector<MaskRecord>& records)
{
    all.erase(std::remove_if(all.begin(), all.end(), [&](const MaskRecord& r) { return SamePath(r.path, path); }), all.end());
    all.insert(all.end(), records.begin(), records.end());
}

void RenameRecords(std::vector<MaskRecord>& all, std::wstring_view oldPath, std::wstring_view newPath)
{
    // 새 경로에 남아 있던 옛 기록은 이제 다른 파일의 것이라 버린다.
    all.erase(std::remove_if(all.begin(), all.end(), [&](const MaskRecord& r) { return SamePath(r.path, newPath); }), all.end());
    for (MaskRecord& r : all)
    {
        if (SamePath(r.path, oldPath))
            r.path = newPath;
    }
}

std::string Sha256Hex(std::string_view bytes)
{
    UCHAR digest[32] = {};
    const NTSTATUS status = ::BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
        reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())), static_cast<ULONG>(bytes.size()),
        digest, sizeof(digest));
    if (!BCRYPT_SUCCESS(status))
        return {};

    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (UCHAR b : digest)
    {
        out += kHex[b >> 4];
        out += kHex[b & 0x0F];
    }
    return out;
}

std::vector<MaskRecord> MakeRecords(std::wstring_view path, const std::vector<Range>& ranges, const TextReader& read)
{
    std::vector<MaskRecord> out;
    for (const Range& r : ranges)
    {
        if (r.Empty())
            continue;
        const auto text = read(r.start, r.end - r.start);
        if (!text)
            continue;
        out.push_back({std::wstring(path), r.start, r.end - r.start, Sha256Hex(*text)});
    }
    return out;
}

RestoreResult MatchRecords(const std::vector<MaskRecord>& records, const TextReader& read)
{
    RestoreResult result;
    for (const MaskRecord& r : records)
    {
        const auto text = read(r.start, r.length);
        if (text && Sha256Hex(*text) == r.hash)
            result.ranges.push_back({r.start, r.start + r.length});
        else
            ++result.skipped;
    }
    return result;
}
}
