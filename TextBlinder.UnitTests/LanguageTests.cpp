// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "Language.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft::VisualStudio::CppUnitTestFramework
{
template <>
std::wstring ToString<tb::Lang>(const tb::Lang& lang)
{
    return lang == tb::Lang::Ko ? L"Ko" : L"En";
}
}

namespace LanguageTests
{
TEST_CLASS(ResolveLanguageTests)
{
public:
    TEST_METHOD(AutoFollowsKoreanXml)
    {
        Assert::AreEqual(tb::Lang::Ko, tb::ResolveLanguage("auto", "korean.xml"));
        Assert::AreEqual(tb::Lang::Ko, tb::ResolveLanguage("auto", "KOREAN.XML"));
    }

    TEST_METHOD(AutoIsEnglishForOtherFiles)
    {
        Assert::AreEqual(tb::Lang::En, tb::ResolveLanguage("auto", "english.xml"));
        Assert::AreEqual(tb::Lang::En, tb::ResolveLanguage("auto", "japanese.xml"));
        Assert::AreEqual(tb::Lang::En, tb::ResolveLanguage("auto", ""));
    }

    TEST_METHOD(FixedValueWins)
    {
        Assert::AreEqual(tb::Lang::En, tb::ResolveLanguage("en", "korean.xml"));
        Assert::AreEqual(tb::Lang::Ko, tb::ResolveLanguage("ko", "english.xml"));
        Assert::AreEqual(tb::Lang::Ko, tb::ResolveLanguage("KO", ""));
    }

    TEST_METHOD(UnknownValueActsAsAuto)
    {
        Assert::IsTrue(tb::IsAutoLanguage("fr"));
        Assert::IsTrue(tb::IsAutoLanguage(""));
        Assert::IsFalse(tb::IsAutoLanguage("ko"));
        Assert::AreEqual(tb::Lang::Ko, tb::ResolveLanguage("fr", "korean.xml"));
    }
};

TEST_CLASS(StartupLanguageTests)
{
public:
    TEST_METHOD(AutoUsesDetectedLanguage)
    {
        Assert::AreEqual(tb::Lang::Ko, tb::StartupLanguage("auto", "ko"));
        Assert::AreEqual(tb::Lang::En, tb::StartupLanguage("auto", "en"));
    }

    TEST_METHOD(AutoWithoutDetectedIsEnglish)
    {
        Assert::AreEqual(tb::Lang::En, tb::StartupLanguage("auto", ""));
        Assert::AreEqual(tb::Lang::En, tb::StartupLanguage("auto", "xx"));
    }

    TEST_METHOD(FixedValueWins)
    {
        Assert::AreEqual(tb::Lang::En, tb::StartupLanguage("en", "ko"));
        Assert::AreEqual(tb::Lang::Ko, tb::StartupLanguage("ko", ""));
    }
};
}
