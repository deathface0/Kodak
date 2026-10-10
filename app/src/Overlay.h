#pragma once

#include <kodak/core/Geometry.h>
#include <kodak/core/Image.h>
#include <kodak/win/ScreenSource.h>

#include <windows.h>

#include <d2d1.h>
#include <wrl/client.h>

#include <functional>
#include <optional>

namespace kodak::app {

/// The full-screen window over the frozen capture where the user drags a
/// selection. Escape, a right click or switching away cancels.
class Overlay {
public:
    /// Receives the selected pixels once the user releases the mouse.
    using SelectHandler = std::function<void(const core::Image&)>;

    Overlay(HINSTANCE instance, SelectHandler onSelect);
    ~Overlay();

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    /// Shows `capture` over the whole virtual desktop and waits for a selection.
    void show(win::ScreenCapture capture);
    bool visible() const noexcept;

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);

    void paint();
    bool createDeviceResources();
    void releaseDeviceResources() noexcept;
    core::Point clampToImage(LPARAM lParam) const noexcept;
    void select(const core::Rect& selection);
    /// Hides the window and wipes the capture from memory.
    void close() noexcept;

    HWND window_ = nullptr;
    SelectHandler onSelect_;
    win::ScreenCapture capture_;
    std::optional<core::Point> anchor_; // set while the left button is down
    core::Point current_;

    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> veil_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> border_;
};

} // namespace kodak::app
