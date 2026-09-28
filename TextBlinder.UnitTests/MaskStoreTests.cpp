// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "MaskStore.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
// 문서 글을 흉내 낸다. 범위가 문서 밖이면 nullopt.
tb::TextReader ReaderFor(const std::string& doc)
{
    return [doc](tb::Pos start, tb::Pos length) -> std::optional<std::string> {
        if (start < 0 || length < 0 || static_cast<size_t>(start + length) > doc.size())
            return std::nullopt;
        return doc.substr(static_cast<size_t>(start), static_cast<size_t>(length));
    };
}

const std::string kHashA(64, 'a');
const std::string kHashB(64, 'b');
}

namespace MaskStoreTests
{
TEST_CLASS(Sha256Tests)
{
public:
    TEST_METHOD(KnownValues)
    {
        Assert::AreEqual(std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), tb::Sha256Hex(""));
        Assert::AreEqual(std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), tb::Sha256Hex("abc"));
    }
};

TEST_CLASS(RecordFileTests)
{
public:
    TEST_METHOD(WriteThenReadGivesSameRecords)
    {
        const std::vector<tb::MaskRecord> records = {
            {L"C:\\Work\\비밀\\keys.txt", 9, 12, kHashA},
            {L"D:\\a b\\c.ini", 0, 1, kHashB},
        };
        const auto read = tb::ParseMaskRecords(tb::SerializeMaskRecords(records));
        Assert::AreEqual(records.size(), read.size());
        for (size_t i = 0; i < records.size(); ++i)
        {
            Assert::AreEqual(records[i].path, read[i].path);
            Assert::AreEqual(records[i].start, read[i].start);
            Assert::AreEqual(records[i].length, read[i].length);
            Assert::AreEqual(records[i].hash, read[i].hash);
        }
    }

    TEST_METHOD(LineFormatIsTabSeparated)
    {
        const auto text = tb::SerializeMaskRecords({{L"C:\\x.txt", 3, 4, kHashA}});
        Assert::AreEqual("C:\\x.txt\t3\t4\t" + kHashA + "\r\n", text);
    }

    TEST_METHOD(BadLinesAreSkipped)
    {
        const std::string text =
            "C:\\ok.txt\t1\t2\t" + kHashA + "\n"
            "C:\\three-fields.txt\t1\t2\n"
            "C:\\five-fields.txt\t1\t2\t" + kHashA + "\textra\n"
            "\t1\t2\t" + kHashA + "\n"
            "C:\\neg.txt\t-1\t2\t" + kHashA + "\n"
            "C:\\zero.txt\t1\t0\t" + kHashA + "\n"
            "C:\\nan.txt\t1x\t2\t" + kHashA + "\n"
            "C:\\shorthash.txt\t1\t2\tabc\n"
            "C:\\upperhash.txt\t1\t2\t" + std::string(64, 'A') + "\n"
            "\n";
        const auto read = tb::ParseMaskRecords(text);
        Assert::AreEqual(size_t{1}, read.size());
        Assert::AreEqual(std::wstring(L"C:\\ok.txt"), read[0].path);
    }

    TEST_METHOD(ReplaceAndRenameIgnorePathCase)
    {
        std::vector<tb::MaskRecord> all = {
            {L"C:\\A.txt", 0, 1, kHashA},
            {L"C:\\B.txt", 0, 1, kHashA},
            {L"c:\\a.TXT", 5, 1, kHashA},
        };
        tb::ReplaceRecords(all, L"c:\\a.txt", {{L"C:\\A.txt", 7, 2, kHashB}});
        Assert::AreEqual(size_t{2}, all.size());
        Assert::AreEqual(size_t{1}, tb::RecordsFor(all, L"C:\\A.TXT").size());
        Assert::AreEqual(tb::Pos{7}, tb::RecordsFor(all, L"C:\\a.txt")[0].start);

        tb::RenameRecords(all, L"C:\\a.txt", L"C:\\B.txt");
        Assert::AreEqual(size_t{0}, tb::RecordsFor(all, L"C:\\A.txt").size());
        const auto b = tb::RecordsFor(all, L"C:\\B.txt");
        Assert::AreEqual(size_t{1}, b.size());
        Assert::AreEqual(tb::Pos{7}, b[0].start);
    }
};

TEST_CLASS(RestoreTests)
{
public:
    TEST_METHOD(SameTextIsRestored)
    {
        const std::string doc = "password=secret123\r\nuser=kim";
        const auto records = tb::MakeRecords(L"C:\\x.txt", {{9, 18}, {25, 28}}, ReaderFor(doc));
        Assert::AreEqual(size_t{2}, records.size());
        Assert::AreEqual(tb::Sha256Hex("secret123"), records[0].hash);

        const auto result = tb::MatchRecords(records, ReaderFor(doc));
        Assert::AreEqual(0, result.skipped);
        Assert::AreEqual(size_t{2}, result.ranges.size());
        Assert::IsTrue(result.ranges[0] == tb::Range{9, 18});
        Assert::IsTrue(result.ranges[1] == tb::Range{25, 28});
    }

    TEST_METHOD(ChangedTextIsSkipped)
    {
        const auto records = tb::MakeRecords(L"C:\\x.txt", {{9, 18}}, ReaderFor("password=secret123"));
        const auto result = tb::MatchRecords(records, ReaderFor("password=Secret123"));
        Assert::AreEqual(1, result.skipped);
        Assert::AreEqual(size_t{0}, result.ranges.size());
    }

    TEST_METHOD(RangeOutsideDocumentIsSkipped)
    {
        const auto records = tb::MakeRecords(L"C:\\x.txt", {{9, 18}}, ReaderFor("password=secret123"));
        const auto result = tb::MatchRecords(records, ReaderFor("password="));
        Assert::AreEqual(1, result.skipped);
        Assert::AreEqual(size_t{0}, result.ranges.size());
    }

    TEST_METHOD(RecordsKeepNoPlainText)
    {
        const auto records = tb::MakeRecords(L"C:\\x.txt", {{9, 18}}, ReaderFor("password=secret123"));
        const auto text = tb::SerializeMaskRecords(records);
        Assert::AreEqual(std::string::npos, text.find("secret123"));
    }
};
}
