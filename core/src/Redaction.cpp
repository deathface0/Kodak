#include "kodak/core/Redaction.h"

#include <cstdint>

namespace kodak::core {

void redact(Image& image, std::span<const Finding> findings) noexcept {
    for (const Finding& finding : findings) {
        if (!finding.enabled) continue;
        const Rect area = intersect(finding.area, image.bounds());
        for (int y = area.y; y < area.y + area.height; ++y) {
            std::uint8_t* pixel = image.bgra.data() + image.offsetOf({area.x, y});
            for (int x = 0; x < area.width; ++x, pixel += Image::bytesPerPixel) {
                pixel[0] = pixel[1] = pixel[2] = 0;
                pixel[3] = 255;
            }
        }
    }
}

} // namespace kodak::core
