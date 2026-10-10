#pragma once

#include <windows.h>

#include <memory>
#include <type_traits>

namespace kodak::win::detail {

struct ScreenDcDeleter {
    void operator()(HDC dc) const noexcept { ReleaseDC(nullptr, dc); }
};
struct MemoryDcDeleter {
    void operator()(HDC dc) const noexcept { DeleteDC(dc); }
};
struct BitmapDeleter {
    void operator()(HBITMAP bitmap) const noexcept { DeleteObject(bitmap); }
};

/// The screen DC from GetDC(nullptr).
using ScreenDc = std::unique_ptr<std::remove_pointer_t<HDC>, ScreenDcDeleter>;
/// A DC from CreateCompatibleDC.
using MemoryDc = std::unique_ptr<std::remove_pointer_t<HDC>, MemoryDcDeleter>;
using Bitmap = std::unique_ptr<std::remove_pointer_t<HBITMAP>, BitmapDeleter>;

/// Selects an object into a DC and puts the previous one back on destruction.
class SelectedObject {
public:
    SelectedObject(HDC dc, HGDIOBJ object) noexcept : dc_(dc), previous_(SelectObject(dc, object)) {}
    ~SelectedObject() { SelectObject(dc_, previous_); }

    SelectedObject(const SelectedObject&) = delete;
    SelectedObject& operator=(const SelectedObject&) = delete;

private:
    HDC dc_;
    HGDIOBJ previous_;
};

} // namespace kodak::win::detail
