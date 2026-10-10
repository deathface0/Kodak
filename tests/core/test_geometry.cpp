#include <doctest/doctest.h>

#include <kodak/core/Geometry.h>

using kodak::core::intersect;
using kodak::core::Rect;

TEST_CASE("intersect returns the overlap") {
    CHECK(intersect({0, 0, 10, 10}, {5, 5, 10, 10}) == Rect{5, 5, 5, 5});
    CHECK(intersect({-20, -5, 30, 10}, {0, 0, 100, 100}) == Rect{0, 0, 10, 5});
}

TEST_CASE("intersect is empty when rectangles only touch or are apart") {
    CHECK(intersect({0, 0, 10, 10}, {10, 0, 5, 5}).empty());
    CHECK(intersect({0, 0, 10, 10}, {50, 50, 5, 5}) == Rect{});
}

TEST_CASE("a rectangle with no width or height is empty") {
    CHECK(Rect{0, 0, 0, 5}.empty());
    CHECK(Rect{0, 0, 5, -1}.empty());
    CHECK_FALSE(Rect{0, 0, 1, 1}.empty());
}
