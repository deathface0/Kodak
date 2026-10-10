#pragma once

#include <algorithm>

namespace kodak::core {

/// A pixel position.
struct Point {
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(Point, Point) noexcept = default;
};

/// An axis-aligned rectangle in pixels.
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr bool empty() const noexcept { return width <= 0 || height <= 0; }

    friend constexpr bool operator==(const Rect&, const Rect&) noexcept = default;
};

/// The overlap of two rectangles; empty (all zero) when they do not overlap.
constexpr Rect intersect(const Rect& a, const Rect& b) noexcept {
    const int left = std::max(a.x, b.x);
    const int top = std::max(a.y, b.y);
    const int right = std::min(a.x + a.width, b.x + b.width);
    const int bottom = std::min(a.y + a.height, b.y + b.height);
    if (right <= left || bottom <= top) return {};
    return {left, top, right - left, bottom - top};
}

/// The rectangle covering both pixels and everything between them, whichever
/// corner each one is (a drag can go in any direction).
constexpr Rect fromCorners(Point a, Point b) noexcept {
    const int left = std::min(a.x, b.x);
    const int top = std::min(a.y, b.y);
    return {left, top, std::max(a.x, b.x) - left + 1, std::max(a.y, b.y) - top + 1};
}

} // namespace kodak::core
