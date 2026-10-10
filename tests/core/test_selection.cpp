#include <doctest/doctest.h>

#include <kodak/core/Selection.h>

using kodak::core::isUsableSelection;
using kodak::core::minSelectionSide;
using kodak::core::Rect;

TEST_CASE("selections of at least 4x4 pixels are usable") {
    CHECK(isUsableSelection({0, 0, minSelectionSide, minSelectionSide}));
    CHECK(isUsableSelection({10, 10, 300, 200}));
}

TEST_CASE("smaller selections are treated as a stray click") {
    CHECK_FALSE(isUsableSelection({0, 0, 1, 1}));
    CHECK_FALSE(isUsableSelection({0, 0, minSelectionSide - 1, 100}));
    CHECK_FALSE(isUsableSelection({0, 0, 100, minSelectionSide - 1}));
}
