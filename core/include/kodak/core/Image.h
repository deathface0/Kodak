#pragma once

#include "kodak/core/Geometry.h"

#include <cstdint>
#include <vector>

namespace kodak::core {

/// 32-bit BGRA pixels, rows top to bottom with no padding (stride = width * 4).
struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> bgra;

    Image() = default;
    /// A width x height image with every byte set to zero.
    Image(int width, int height);

    bool empty() const noexcept { return width <= 0 || height <= 0; }
    Rect bounds() const noexcept { return {0, 0, width, height}; }
};

/// A copy of the part of `area` that lies inside `image`; empty if they do not overlap.
Image crop(const Image& image, const Rect& area);

} // namespace kodak::core
