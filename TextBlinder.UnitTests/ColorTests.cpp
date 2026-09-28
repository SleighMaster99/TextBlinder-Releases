// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "Color.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ColorTests
{
TEST_CLASS(ParseMaskColorTests)
{
public:
    TEST_METHOD(GrayStaysGray)
    {
        Assert::AreEqual(0x808080, tb::ParseMaskColor("#808080"));
    }

    TEST_METHOD(RgbBecomesBgr)
    {
        Assert::AreEqual(0x0000FF, tb::ParseMaskColor("#FF0000"));
        Assert::AreEqual(0x00FF00, tb::ParseMaskColor("#00FF00"));
        Assert::AreEqual(0xFF0000, tb::ParseMaskColor("#0000FF"));
        Assert::AreEqual(0x563412, tb::ParseMaskColor("#123456"));
    }

    TEST_METHOD(LowerCaseHexWorks)
    {
        Assert::AreEqual(0xEFCDAB, tb::ParseMaskColor("#abcdef"));
    }

    TEST_METHOD(BadFormatGivesDefault)
    {
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor(""));
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor("808080"));
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor("#80808"));
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor("#8080800"));
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor("#GG0000"));
        Assert::AreEqual(tb::kDefaultMaskColor, tb::ParseMaskColor("red"));
    }
};
}
