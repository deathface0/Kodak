#include "kodak/core/Redaction.h"

#include <cstddef>
#include <cstdint>

namespace kodak::core {

void redact(Image& image, std::span<const Finding> findings) noexcept {
    const std::size_t stride = static_cast<std::size_t>(image.width) * 4;
    for (const Finding& finding : findings) {
        if (!finding.enabled) continue;
        const Rect area = intersect(finding.area, image.bounds());
        for (int y = area.y; y < area.y + area.height; ++y) {
            std::uint8_t* pixel = image.bgra.data() + static_cast<std::size_t>(y) * stride +
                                  static_cast<std::size_t>(area.x) * 4;
            for (int x = 0; x < area.width; ++x, pixel += 4) {
                pixel[0] = 0;
                pixel[1] = 0;
                pixel[2] = 0;
                pixel[3] = 255;
            }
        }
    }
}

} // namespace kodak::core
