#include "Overlay.h"

#include <kodak/core/Selection.h>

#include <windowsx.h>

#include <algorithm>
#include <utility>

namespace kodak::app {

namespace {

constexpr wchar_t className[] = L"KodakOverlay";

D2D1_RECT_F toRectF(const core::Rect& r) noexcept {
    return D2D1::RectF(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.x + r.width),
                       static_cast<float>(r.y + r.height));
}

} // namespace

Overlay::Overlay(HINSTANCE instance, SelectHandler onSelect) : onSelect_(std::move(onSelect)) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    windowClass.lpszClassName = className;
    RegisterClassExW(&windowClass);

    window_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, className, L"Kodak", WS_POPUP, 0, 0, 0, 0,
                              nullptr, nullptr, instance, this);
}

Overlay::~Overlay() {
    close();
    if (window_) DestroyWindow(window_);
}

void Overlay::show(win::ScreenCapture capture) {
    close();
    if (!window_ || !factory_ || capture.image.empty()) return;

    capture_ = std::move(capture);
    SetWindowPos(window_, HWND_TOPMOST, capture_.origin.x, capture_.origin.y, capture_.image.width,
                 capture_.image.height, SWP_SHOWWINDOW);
    SetForegroundWindow(window_); // allowed: we are handling the hotkey or the tray menu
    SetFocus(window_);
    InvalidateRect(window_, nullptr, FALSE);
}

bool Overlay::visible() const noexcept { return window_ && IsWindowVisible(window_); }

LRESULT CALLBACK Overlay::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* self = static_cast<Overlay*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        self->window_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (auto* self = reinterpret_cast<Overlay*>(GetWindowLongPtrW(window, GWLP_USERDATA)))
        return self->handle(message, wParam, lParam);
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Overlay::handle(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: paint(); return 0;
    case WM_ERASEBKGND: return 1; // Direct2D paints every pixel

    case WM_LBUTTONDOWN:
        anchor_ = clampToImage(lParam);
        current_ = *anchor_;
        SetCapture(window_);
        InvalidateRect(window_, nullptr, FALSE);
        return 0;

    case WM_MOUSEMOVE:
        if (anchor_) {
            current_ = clampToImage(lParam);
            InvalidateRect(window_, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONUP:
        if (anchor_) {
            const core::Rect selection = core::fromCorners(*anchor_, clampToImage(lParam));
            anchor_.reset();
            ReleaseCapture();
            if (core::isUsableSelection(selection)) {
                select(selection);
            } else {
                InvalidateRect(window_, nullptr, FALSE);
            }
        }
        return 0;

    case WM_CAPTURECHANGED:
        if (anchor_) { // the drag was interrupted
            anchor_.reset();
            InvalidateRect(window_, nullptr, FALSE);
        }
        return 0;

    case WM_RBUTTONUP: close(); return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) close();
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) close(); // the user switched away
        return 0;

    case WM_DISPLAYCHANGE: close(); return 0; // the capture no longer matches the monitors

    case WM_DPICHANGED: return 0; // keep covering the virtual desktop in physical pixels
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

void Overlay::paint() {
    PAINTSTRUCT ps;
    BeginPaint(window_, &ps);
    if (!createDeviceResources()) {
        EndPaint(window_, &ps);
        close(); // never leave a frozen screen the user cannot see through
        return;
    }

    const D2D1_RECT_F whole = toRectF(capture_.image.bounds());
    target_->BeginDraw();
    target_->DrawBitmap(bitmap_.Get(), whole, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    target_->FillRectangle(whole, veil_.Get());
    if (anchor_) {
        const D2D1_RECT_F selection = toRectF(core::fromCorners(*anchor_, current_));
        target_->DrawBitmap(bitmap_.Get(), selection, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                            selection);
        const D2D1_RECT_F outline = D2D1::RectF(selection.left + 0.5f, selection.top + 0.5f,
                                                selection.right - 0.5f, selection.bottom - 0.5f);
        target_->DrawRectangle(outline, border_.Get(), 1.0f);
    }
    if (target_->EndDraw() == D2DERR_RECREATE_TARGET) {
        releaseDeviceResources();
        InvalidateRect(window_, nullptr, FALSE);
    }
    EndPaint(window_, &ps);
}

bool Overlay::createDeviceResources() {
    if (target_) return true;
    if (!factory_ || capture_.image.empty()) return false;

    const auto size =
        D2D1::SizeU(static_cast<UINT32>(capture_.image.width), static_cast<UINT32>(capture_.image.height));
    // 96 DPI: one Direct2D unit is one physical pixel, whatever the monitor scaling.
    const auto targetProperties =
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(), 96.0f, 96.0f);
    const auto bitmapProperties = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);

    const bool created =
        SUCCEEDED(factory_->CreateHwndRenderTarget(
            targetProperties, D2D1::HwndRenderTargetProperties(window_, size), target_.GetAddressOf())) &&
        SUCCEEDED(target_->CreateBitmap(size, capture_.image.bgra.data(), size.width * 4, bitmapProperties,
                                        bitmap_.GetAddressOf())) &&
        SUCCEEDED(
            target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f), veil_.GetAddressOf())) &&
        SUCCEEDED(target_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), border_.GetAddressOf()));
    if (!created) releaseDeviceResources();
    return created;
}

void Overlay::releaseDeviceResources() noexcept {
    border_.Reset();
    veil_.Reset();
    bitmap_.Reset();
    target_.Reset();
}

core::Point Overlay::clampToImage(LPARAM lParam) const noexcept {
    // Signed coordinates: with mouse capture the cursor can leave the window.
    return {std::clamp(GET_X_LPARAM(lParam), 0, capture_.image.width - 1),
            std::clamp(GET_Y_LPARAM(lParam), 0, capture_.image.height - 1)};
}

void Overlay::select(const core::Rect& selection) {
    const core::Image part = core::crop(capture_.image, selection);
    close();
    if (onSelect_) onSelect_(part);
}

void Overlay::close() noexcept {
    anchor_.reset();
    if (window_ && IsWindowVisible(window_)) ShowWindow(window_, SW_HIDE);
    releaseDeviceResources();
    if (!capture_.image.bgra.empty())
        SecureZeroMemory(capture_.image.bgra.data(), capture_.image.bgra.size());
    capture_ = {};
}

} // namespace kodak::app
