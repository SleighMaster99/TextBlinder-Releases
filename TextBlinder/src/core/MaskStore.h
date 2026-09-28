// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Range.h"

namespace tb
{
// TextBlinder.masks 의 한 줄. 원문은 두지 않고 해시만 둔다 (계획 5.3 절).
struct MaskRecord
{
    std::wstring path;
    Pos start = 0;
    Pos length = 0;
    std::string hash;  // 가린 글 바이트의 SHA-256, 소문자 16 진수 64 자
};

// 기록 파일 내용 (UTF-8) 을 읽는다. 형식이 틀린 줄은 건너뛴다.
std::vector<MaskRecord> ParseMaskRecords(std::string_view utf8);

// 한 줄에 기록 하나: 경로 TAB 시작 TAB 길이 TAB 해시. 줄 끝은 CRLF.
std::string SerializeMaskRecords(const std::vector<MaskRecord>& records);

// 경로는 Windows 규칙대로 대소문자를 가리지 않는다.
bool SamePath(std::wstring_view a, std::wstring_view b);

std::vector<MaskRecord> RecordsFor(const std::vector<MaskRecord>& all, std::wstring_view path);

// path 의 기록을 모두 지우고 records 로 바꾼다.
void ReplaceRecords(std::vector<MaskRecord>& all, std::wstring_view path, const std::vector<MaskRecord>& records);

// 파일 이름을 바꾸면 기록의 경로도 바꾼다.
void RenameRecords(std::vector<MaskRecord>& all, std::wstring_view oldPath, std::wstring_view newPath);

std::string Sha256Hex(std::string_view bytes);

// 문서에서 [start, start + length) 의 바이트를 읽는 함수. 문서 밖이면 nullopt.
using TextReader = std::function<std::optional<std::string>(Pos start, Pos length)>;

// 가린 범위들로 기록을 만든다.
std::vector<MaskRecord> MakeRecords(std::wstring_view path, const std::vector<Range>& ranges, const TextReader& read);

struct RestoreResult
{
    std::vector<Range> ranges;  // 해시가 맞아서 다시 가릴 범위
    int skipped = 0;            // 해시가 맞지 않거나 문서 밖이라 건너뛴 기록 수
};

// 기록 가운데 지금 문서의 글과 해시가 맞는 것만 고른다.
RestoreResult MatchRecords(const std::vector<MaskRecord>& records, const TextReader& read);
}
