#include "kodak/win/GdiScreenSource.h"

#include "detail/GdiHandles.h"

#include <windows.h>

#include <algorithm>
#include <cstdint>

namespace kodak::win {

std::optional<ScreenCapture> GdiScreenSource::capture() {
    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (width <= 0 || height <= 0) return std::nullopt;

    const detail::ScreenDc screen(GetDC(nullptr));
    if (!screen) return std::nullopt;
    const detail::MemoryDc memory(CreateCompatibleDC(screen.get()));
    if (!memory) return std::nullopt;

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height; // top-down, like core::Image
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    const detail::Bitmap bitmap(CreateDIBSection(screen.get(), &info, DIB_RGB_COLORS, &bits, nullptr, 0));
    if (!bitmap || !bits) return std::nullopt;

    {
        const detail::SelectedObject selected(memory.get(), bitmap.get());
        if (!BitBlt(memory.get(), 0, 0, width, height, screen.get(), left, top, SRCCOPY | CAPTUREBLT))
            return std::nullopt;
    }
    GdiFlush(); // finish GDI's writes before reading the DIB section

    ScreenCapture capture{core::Image(width, height), core::Point{left, top}};
    std::copy_n(static_cast<const std::uint8_t*>(bits), capture.image.bgra.size(),
                capture.image.bgra.begin());
    core::makeOpaque(capture.image);
    return capture;
}

} // namespace kodak::win
