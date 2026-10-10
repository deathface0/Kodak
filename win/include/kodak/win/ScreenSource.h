#pragma once

#include <kodak/core/Geometry.h>
#include <kodak/core/Image.h>

#include <optional>

namespace kodak::win {

/// A frozen copy of the whole virtual desktop (every monitor), in physical pixels.
struct ScreenCapture {
    core::Image image;
    /// Virtual-desktop position of the image's top-left pixel; negative when a
    /// monitor sits to the left of or above the primary one.
    core::Point origin;
};

/// Where captures come from, so BitBlt can be swapped for Windows.Graphics.Capture.
class ScreenSource {
public:
    virtual ~ScreenSource() = default;

    /// Captures every monitor; std::nullopt if the capture failed.
    virtual std::optional<ScreenCapture> capture() = 0;
};

} // namespace kodak::win
