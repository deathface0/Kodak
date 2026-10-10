#pragma once

#include "kodak/win/ScreenSource.h"

namespace kodak::win {

/// Captures the virtual desktop with BitBlt (SRCCOPY | CAPTUREBLT, so layered
/// windows are included). The process must be per-monitor DPI aware to get
/// physical pixels.
class GdiScreenSource final : public ScreenSource {
public:
    std::optional<ScreenCapture> capture() override;
};

} // namespace kodak::win
