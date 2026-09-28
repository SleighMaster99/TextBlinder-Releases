// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "Ini.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace IniTests
{
TEST_CLASS(ParseTests)
{
public:
    TEST_METHOD(ReadsValuesIgnoringCaseAndSpaces)
    {
        const auto ini = tb::Ini::Parse(
            "[Display]\r\n"
            "  MaskColor = #112233  \r\n"
            "[general]\r\n"
            "language=ko\r\n");
        Assert::AreEqual(std::string("#112233"), ini.Get("display", "MASKCOLOR").value());
        Assert::AreEqual(std::string("ko"), ini.Get("General", "Language").value());
        Assert::IsFalse(ini.Get("General", "Missing").has_value());
        Assert::IsFalse(ini.Get("Other", "Language").has_value());
    }

    TEST_METHOD(KeepsQuotesEqualsAndUtf8InValues)
    {
        const auto ini = tb::Ini::Parse(
            "[Rules]\n"
            "Rule1=\"quoted\"\n"
            "Rule2=a=b\\s*=\\K.+\n"
            "Rule3=비밀번호\\s*:\\s*\\K.+\n");
        Assert::AreEqual(std::string("\"quoted\""), ini.Get("Rules", "Rule1").value());
        Assert::AreEqual(std::string("a=b\\s*=\\K.+"), ini.Get("Rules", "Rule2").value());
        Assert::AreEqual(std::string("비밀번호\\s*:\\s*\\K.+"), ini.Get("Rules", "Rule3").value());
    }

    TEST_METHOD(SkipsCommentsAndBom)
    {
        const auto ini = tb::Ini::Parse(
            "\xEF\xBB\xBF[General]\n"
            "; Language=en\n"
            "# Language=en\n"
            "Language=ko\n");
        Assert::AreEqual(std::string("ko"), ini.Get("General", "Language").value());
        Assert::AreEqual(size_t{1}, ini.Section("General").size());
    }

    TEST_METHOD(SectionKeepsFileOrder)
    {
        const auto ini = tb::Ini::Parse("[R]\nb=2\na=1\n[X]\nc=3\n[r]\nd=4\n");
        const auto entries = ini.Section("R");
        Assert::AreEqual(size_t{3}, entries.size());
        Assert::AreEqual(std::string("b"), entries[0].key);
        Assert::AreEqual(std::string("a"), entries[1].key);
        Assert::AreEqual(std::string("d"), entries[2].key);
    }
};

TEST_CLASS(SetValueTests)
{
public:
    TEST_METHOD(ReplacesExistingKeyOnly)
    {
        const std::string text = "; top\r\n[General]\r\nLanguage=auto\r\nDetectedLanguage=en\r\n\r\n[Rules]\r\nRule1=x\r\n";
        const std::string out = tb::IniSetValue(text, "general", "detectedlanguage", "ko");
        Assert::AreEqual(std::string("; top\r\n[General]\r\nLanguage=auto\r\nDetectedLanguage=ko\r\n\r\n[Rules]\r\nRule1=x\r\n"), out);
    }

    TEST_METHOD(AddsKeyAtSectionEnd)
    {
        const std::string text = "[General]\r\nLanguage=auto\r\n\r\n[Rules]\r\nRule1=x\r\n";
        const std::string out = tb::IniSetValue(text, "General", "DetectedLanguage", "ko");
        Assert::AreEqual(std::string("[General]\r\nLanguage=auto\r\nDetectedLanguage=ko\r\n\r\n[Rules]\r\nRule1=x\r\n"), out);
    }

    TEST_METHOD(AddsSectionWhenMissing)
    {
        Assert::AreEqual(std::string("[A]\nx=1\n\n[General]\nDetectedLanguage=ko\n"),
            tb::IniSetValue("[A]\nx=1", "General", "DetectedLanguage", "ko"));
        Assert::AreEqual(std::string("[General]\r\nDetectedLanguage=ko\r\n"),
            tb::IniSetValue("", "General", "DetectedLanguage", "ko"));
    }

    TEST_METHOD(ResultParsesBack)
    {
        const std::string out = tb::IniSetValue("[General]\nLanguage=auto\n", "General", "DetectedLanguage", "en");
        const auto ini = tb::Ini::Parse(out);
        Assert::AreEqual(std::string("auto"), ini.Get("General", "Language").value());
        Assert::AreEqual(std::string("en"), ini.Get("General", "DetectedLanguage").value());
    }
};
}
