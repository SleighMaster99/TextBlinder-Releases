// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "Rules.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace RulesTests
{
TEST_CLASS(ReadRulesTests)
{
public:
    TEST_METHOD(ReadsRuleKeysInFileOrder)
    {
        const auto rules = tb::ReadRules(tb::Ini::Parse(
            "[Rules]\r\n"
            "Rule2=api_key\\s*=\\s*\\K.+\r\n"
            "Rule1=password\\s*=\\s*\\K.+\r\n"
            "RuleToken=token:\\s*\\K\\S+\r\n"));
        Assert::AreEqual(size_t{3}, rules.size());
        Assert::AreEqual(std::string("Rule2"), rules[0].name);
        Assert::AreEqual(std::string("api_key\\s*=\\s*\\K.+"), rules[0].pattern);
        Assert::AreEqual(std::string("Rule1"), rules[1].name);
        Assert::AreEqual(std::string("password\\s*=\\s*\\K.+"), rules[1].pattern);
        Assert::AreEqual(std::string("RuleToken"), rules[2].name);
    }

    TEST_METHOD(IgnoresOtherKeysSectionsCommentsAndEmptyValues)
    {
        const auto rules = tb::ReadRules(tb::Ini::Parse(
            "[General]\n"
            "Rule1=not-a-rule-section\n"
            "[Rules]\n"
            ";Rule2=commented\n"
            "Pattern3=wrong-prefix\n"
            "Rule4=\n"
            "rule5=lower-case-key\n"
            "[rules]\n"
            "Rule6=same-section-again\n"));
        Assert::AreEqual(size_t{2}, rules.size());
        Assert::AreEqual(std::string("rule5"), rules[0].name);
        Assert::AreEqual(std::string("Rule6"), rules[1].name);
    }

    TEST_METHOD(NoRulesSectionGivesNoRules)
    {
        Assert::AreEqual(size_t{0}, tb::ReadRules(tb::Ini::Parse("[Display]\nMaskColor=#808080\n")).size());
    }
};
}
