#pragma once

#include "kodak/core/Finding.h"
#include "kodak/core/Image.h"

#include <span>

namespace kodak::core {

/// Paints every enabled finding's area (clipped to the image) opaque black,
/// overwriting the pixels: unlike pixelation or blur, nothing can be recovered.
void redact(Image& image, std::span<const Finding> findings) noexcept;

} // namespace kodak::core
