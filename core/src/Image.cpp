#include "kodak/core/Image.h"

#include <algorithm>

namespace kodak::core {

Image::Image(int width, int height)
    : width(std::max(width, 0)), height(std::max(height, 0)),
      bgra(static_cast<std::size_t>(this->width) * static_cast<std::size_t>(this->height) * bytesPerPixel) {}

Image crop(const Image& image, const Rect& area) {
    const Rect clipped = intersect(area, image.bounds());
    if (clipped.empty()) return {};

    Image result(clipped.width, clipped.height);
    const std::uint8_t* src = image.bgra.data() + image.offsetOf({clipped.x, clipped.y});
    std::uint8_t* dst = result.bgra.data();
    for (int row = 0; row < clipped.height; ++row, src += image.stride(), dst += result.stride()) {
        std::copy_n(src, result.stride(), dst);
    }
    return result;
}

void makeOpaque(Image& image) noexcept {
    for (std::size_t i = 3; i < image.bgra.size(); i += Image::bytesPerPixel) image.bgra[i] = 255;
}

} // namespace kodak::core
