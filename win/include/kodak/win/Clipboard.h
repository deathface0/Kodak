#pragma once

#include <kodak/core/Image.h>

#include <windows.h>

#include <cstdint>
#include <span>

namespace kodak::win {

/// Replaces the clipboard contents with `image` as CF_DIBV5 (Windows derives
/// CF_DIB and CF_BITMAP from it for older applications) and, when `png` is not
/// empty, also as the registered "PNG" format that browsers and Office prefer.
/// `owner` must be a window: with no owner, SetClipboardData fails. Returns
/// false if the image is empty, the clipboard stays busy or memory runs out.
bool copyImageToClipboard(HWND owner, const core::Image& image, std::span<const std::uint8_t> png = {});

} // namespace kodak::win
