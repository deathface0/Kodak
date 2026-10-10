#include <doctest/doctest.h>

#include <kodak/core/Finding.h>

#include <vector>

using kodak::core::Finding;
using kodak::core::Point;
using kodak::core::Rect;
using kodak::core::topmostAt;

TEST_CASE("topmostAt picks the last finding containing the point") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}}, {Rect{5, 5, 10, 10}}, {Rect{20, 20, 2, 2}}};

    CHECK(topmostAt(findings, Point{2, 2}) == 0u);
    CHECK(topmostAt(findings, Point{7, 7}) == 1u); // inside both: the one drawn on top
    CHECK(topmostAt(findings, Point{21, 21}) == 2u);
}

TEST_CASE("topmostAt finds nothing outside every finding") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}}};
    CHECK_FALSE(topmostAt(findings, Point{10, 10}).has_value());
    CHECK_FALSE(topmostAt({}, Point{0, 0}).has_value());
}
