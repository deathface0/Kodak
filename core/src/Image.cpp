#include "kodak/core/Image.h"

#include <algorithm>
#include <cstddef>

namespace kodak::core {

namespace {

constexpr std::size_t bytesPerPixel = 4;

std::size_t byteCount(int width, int height) {
    if (width <= 0 || height <= 0) return 0;
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * bytesPerPixel;
}

} // namespace

Image::Image(int width, int height)
    : width(std::max(width, 0)), height(std::max(height, 0)), bgra(byteCount(width, height)) {}

Image crop(const Image& image, const Rect& area) {
    const Rect clipped = intersect(area, image.bounds());
    if (clipped.empty()) return {};

    Image result(clipped.width, clipped.height);
    const std::size_t srcStride = static_cast<std::size_t>(image.width) * bytesPerPixel;
    const std::size_t rowBytes = static_cast<std::size_t>(clipped.width) * bytesPerPixel;
    const std::uint8_t* src = image.bgra.data() + static_cast<std::size_t>(clipped.y) * srcStride +
                              static_cast<std::size_t>(clipped.x) * bytesPerPixel;
    std::uint8_t* dst = result.bgra.data();
    for (int row = 0; row < clipped.height; ++row, src += srcStride, dst += rowBytes) {
        std::copy_n(src, rowBytes, dst);
    }
    return result;
}

} // namespace kodak::core
