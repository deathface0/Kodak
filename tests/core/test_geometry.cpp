#include <doctest/doctest.h>

#include <kodak/core/Geometry.h>

using kodak::core::fromCorners;
using kodak::core::intersect;
using kodak::core::Point;
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

TEST_CASE("fromCorners covers both pixels whichever way the drag goes") {
    const Rect expected{2, 3, 5, 4}; // pixels x 2..6, y 3..6
    CHECK(fromCorners(Point{2, 3}, Point{6, 6}) == expected);
    CHECK(fromCorners(Point{6, 6}, Point{2, 3}) == expected);
    CHECK(fromCorners(Point{6, 3}, Point{2, 6}) == expected);
    CHECK(fromCorners(Point{2, 6}, Point{6, 3}) == expected);
}

TEST_CASE("fromCorners of a single pixel is 1x1") {
    CHECK(fromCorners(Point{7, 7}, Point{7, 7}) == Rect{7, 7, 1, 1});
}
