// SPDX-License-Identifier: GPL-3.0-only
#include "CppUnitTest.h"

#include "Toggle.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace
{
using tb::Range;
using tb::ToggleAction;
using tb::ToggleKind;

void AssertAction(const ToggleAction& a, ToggleKind kind, Range range)
{
    Assert::IsTrue(a.kind == kind, L"kind");
    Assert::AreEqual(range.start, a.range.start, L"start");
    Assert::AreEqual(range.end, a.range.end, L"end");
}
}

namespace ToggleTests
{
TEST_CLASS(DecideToggleTests)
{
public:
    // 규칙 1: 선택 영역이 모두 가려져 있으면 푼다.
    TEST_METHOD(Rule1_FullyMaskedSelectionIsUnmasked)
    {
        auto a = tb::DecideToggle({{12, 15}}, {{10, 20}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Unmask, {12, 15});

        a = tb::DecideToggle({{10, 20}}, {{10, 20}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Unmask, {10, 20});
    }

    TEST_METHOD(Rule1_AdjacentMasksCoverSelection)
    {
        const auto a = tb::DecideToggle({{2, 8}}, {{0, 5}, {5, 10}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Unmask, {2, 8});
    }

    // 규칙 2: 가려져 있지 않거나 일부만 가려져 있으면 전체를 가린다.
    TEST_METHOD(Rule2_UnmaskedSelectionIsMasked)
    {
        const auto a = tb::DecideToggle({{0, 5}}, {});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Mask, {0, 5});
    }

    TEST_METHOD(Rule2_PartlyMaskedSelectionIsMasked)
    {
        auto a = tb::DecideToggle({{5, 15}}, {{10, 20}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Mask, {5, 15});

        a = tb::DecideToggle({{2, 8}}, {{0, 5}, {6, 10}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Mask, {2, 8});
    }

    // 규칙 3: 선택한 글이 없고 커서가 가린 범위 안이면 그 범위를 푼다.
    TEST_METHOD(Rule3_CaretInsideMaskUnmasksWholeRange)
    {
        for (tb::Pos caret : {10, 15, 20})
        {
            const auto a = tb::DecideToggle({{caret, caret}}, {{0, 3}, {10, 20}});
            Assert::AreEqual(size_t{1}, a.size());
            AssertAction(a[0], ToggleKind::Unmask, {10, 20});
        }
    }

    TEST_METHOD(Rule3_CaretOutsideMaskDoesNothing)
    {
        Assert::AreEqual(size_t{0}, tb::DecideToggle({{21, 21}}, {{10, 20}}).size());
        Assert::AreEqual(size_t{0}, tb::DecideToggle({{5, 5}}, {}).size());
    }

    // 규칙 4: 다중 선택이면 선택 영역마다 1 ~ 2 번을 따로 적용한다.
    TEST_METHOD(Rule4_EachSelectionDecidedAlone)
    {
        const auto a = tb::DecideToggle({{0, 3}, {10, 12}, {18, 25}}, {{10, 20}});
        Assert::AreEqual(size_t{3}, a.size());
        AssertAction(a[0], ToggleKind::Mask, {0, 3});
        AssertAction(a[1], ToggleKind::Unmask, {10, 12});
        AssertAction(a[2], ToggleKind::Mask, {18, 25});
    }

    TEST_METHOD(Rule4_EmptySelectionsAreSkipped)
    {
        const auto a = tb::DecideToggle({{0, 3}, {15, 15}}, {{10, 20}});
        Assert::AreEqual(size_t{1}, a.size());
        AssertAction(a[0], ToggleKind::Mask, {0, 3});
    }
};
}
