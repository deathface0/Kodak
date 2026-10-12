#pragma once

#include <kodak/core/Image.h>

#include <cstdint>
#include <vector>

namespace kodak::win {

/// Encodes `image` as a 24-bit PNG with WIC, in memory. No metadata is written:
/// besides the image chunks there are only WIC's colour chunks (sRGB, gAMA), no
/// text, time or EXIF. Needs COM on the calling thread (ComApartment).
/// Empty if the image is empty or WIC fails.
std::vector<std::uint8_t> encodePng(const core::Image& image);

} // namespace kodak::win
