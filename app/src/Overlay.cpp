#include "Overlay.h"

#include <kodak/core/Redaction.h>
#include <kodak/core/Selection.h>

#include <shellscalingapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace kodak::app {

namespace {

constexpr wchar_t className[] = L"KodakOverlay";
constexpr wchar_t helpText[] = L"Drag to redact · Right-click to remove · Enter to save · Esc to cancel";

const D2D1_COLOR_F veilColour = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f);
const D2D1_COLOR_F borderColour = D2D1::ColorF(D2D1::ColorF::White);
const D2D1_COLOR_F boxColour = D2D1::ColorF(D2D1::ColorF::Black); // what the output will show
const D2D1_COLOR_F boxOutlineColour = D2D1::ColorF(0.6f, 0.6f, 0.6f);
const D2D1_COLOR_F helpBackColour = D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.85f);
const D2D1_COLOR_F helpTextColour = D2D1::ColorF(D2D1::ColorF::White);

D2D1_RECT_F toRectF(const core::Rect& r) noexcept {
    return D2D1::RectF(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.x + r.width),
                       static_cast<float>(r.y + r.height));
}

/// A 1-pixel outline drawn just inside `r`.
D2D1_RECT_F insideOutline(const D2D1_RECT_F& r) noexcept {
    return D2D1::RectF(r.left + 0.5f, r.top + 0.5f, r.right - 0.5f, r.bottom - 0.5f);
}

} // namespace

Overlay::Overlay(HINSTANCE instance, SelectHandler onSelect) : onSelect_(std::move(onSelect)) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf()));

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_DBLCLKS; // double-click confirms
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
    redraw();
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

    case WM_LBUTTONDOWN: onButtonDown(clampToImage(lParam)); return 0;
    case WM_LBUTTONUP: onButtonUp(clampToImage(lParam)); return 0;
    case WM_RBUTTONUP: onRightClick(clampToImage(lParam)); return 0;

    case WM_LBUTTONDBLCLK:
        if (mode_ == Mode::Reviewing) confirm();
        return 0;

    case WM_MOUSEMOVE:
        if (drag_ != Drag::None) {
            current_ = clampToImage(lParam);
            redraw();
        }
        return 0;

    case WM_CAPTURECHANGED:
        if (drag_ != Drag::None) { // the drag was interrupted
            drag_ = Drag::None;
            redraw();
        }
        return 0;

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_ESCAPE: close(); break;
        case VK_RETURN:
            if (mode_ == Mode::Reviewing) confirm();
            break;
        case VK_BACK: removeLastBox(); break;
        case 'Z':
            if (GetKeyState(VK_CONTROL) < 0) removeLastBox();
            break;
        }
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) close(); // the user switched away
        return 0;

    case WM_DISPLAYCHANGE: close(); return 0; // the capture no longer matches the monitors

    case WM_DPICHANGED: return 0; // keep covering the virtual desktop in physical pixels
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

void Overlay::onButtonDown(core::Point p) {
    // Inside the selection under review: a redaction box. Anywhere else: a new
    // selection, which replaces the current one only once it is usable.
    drag_ = mode_ == Mode::Reviewing && selection_.contains(p) ? Drag::Box : Drag::Selection;
    anchor_ = p;
    current_ = p;
    SetCapture(window_);
    redraw();
}

void Overlay::onButtonUp(core::Point p) {
    if (drag_ == Drag::None) return;
    const Drag finished = drag_;
    drag_ = Drag::None; // before ReleaseCapture, so WM_CAPTURECHANGED ignores it
    ReleaseCapture();

    const core::Rect dragged = core::fromCorners(anchor_, p);
    if (finished == Drag::Selection && core::isUsableSelection(dragged)) {
        selection_ = dragged;
        boxes_.clear();
        mode_ = Mode::Reviewing;
    } else if (finished == Drag::Box) {
        const core::Rect box = core::intersect(dragged, selection_);
        if (core::isUsableSelection(box)) boxes_.push_back({box, core::FindingKind::Manual});
    }
    redraw();
}

void Overlay::onRightClick(core::Point p) {
    if (mode_ == Mode::Selecting) {
        close();
        return;
    }
    if (const auto hit = core::topmostAt(boxes_, p)) {
        boxes_.erase(boxes_.begin() + static_cast<std::ptrdiff_t>(*hit));
        redraw();
    }
}

void Overlay::removeLastBox() {
    if (mode_ != Mode::Reviewing || boxes_.empty()) return;
    boxes_.pop_back();
    redraw();
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
    brush_->SetColor(veilColour);
    target_->FillRectangle(whole, brush_.Get());

    const bool selecting = drag_ == Drag::Selection;
    if (selecting || mode_ == Mode::Reviewing) {
        const D2D1_RECT_F selection = toRectF(selecting ? core::fromCorners(anchor_, current_) : selection_);
        target_->DrawBitmap(bitmap_.Get(), selection, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
                            selection);
        brush_->SetColor(borderColour);
        target_->DrawRectangle(insideOutline(selection), brush_.Get(), 1.0f);
    }

    if (mode_ == Mode::Reviewing && !selecting) {
        std::vector<core::Rect> shown;
        for (const core::Finding& box : boxes_) shown.push_back(box.area);
        if (drag_ == Drag::Box)
            shown.push_back(core::intersect(core::fromCorners(anchor_, current_), selection_));
        for (const core::Rect& box : shown) {
            const D2D1_RECT_F rect = toRectF(box);
            brush_->SetColor(boxColour);
            target_->FillRectangle(rect, brush_.Get());
            brush_->SetColor(boxOutlineColour);
            target_->DrawRectangle(insideOutline(rect), brush_.Get(), 1.0f);
        }
        paintHelp();
    }

    if (target_->EndDraw() == D2DERR_RECREATE_TARGET) {
        releaseDeviceResources();
        redraw();
    }
    EndPaint(window_, &ps);
}

void Overlay::paintHelp() {
    if (!writeFactory_) return;

    // Direct2D runs at 96 DPI, so size the text for the monitor under the selection.
    RECT onScreen{capture_.origin.x + selection_.x, capture_.origin.y + selection_.y,
                  capture_.origin.x + selection_.x + selection_.width,
                  capture_.origin.y + selection_.y + selection_.height};
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (FAILED(GetDpiForMonitor(MonitorFromRect(&onScreen, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI,
                                &dpiX, &dpiY)))
        dpiX = 96;
    const float scale = static_cast<float>(dpiX) / 96.0f;

    if (!helpFormat_ || helpDpi_ != dpiX) {
        helpFormat_.Reset();
        if (FAILED(writeFactory_->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
                                                   DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                                   13.0f * scale, L"en-us", &helpFormat_)))
            return;
        helpDpi_ = dpiX;
    }

    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(writeFactory_->CreateTextLayout(helpText, static_cast<UINT32>(std::size(helpText) - 1),
                                               helpFormat_.Get(), 10000.0f, 1000.0f, &layout)))
        return;
    DWRITE_TEXT_METRICS metrics{};
    layout->GetMetrics(&metrics);

    const float padX = 10.0f * scale;
    const float padY = 5.0f * scale;
    const float gap = 6.0f * scale;
    const float width = metrics.width + 2 * padX;
    const float height = metrics.height + 2 * padY;
    const auto imageWidth = static_cast<float>(capture_.image.width);
    const auto imageHeight = static_cast<float>(capture_.image.height);

    // Under the selection; above it if there is no room; inside it as a last resort.
    float top = static_cast<float>(selection_.y + selection_.height) + gap;
    if (top + height > imageHeight) top = static_cast<float>(selection_.y) - gap - height;
    if (top < 0) top = static_cast<float>(selection_.y) + gap;
    const float left = std::clamp(static_cast<float>(selection_.x), 0.0f, std::max(0.0f, imageWidth - width));

    brush_->SetColor(helpBackColour);
    target_->FillRoundedRectangle(
        D2D1::RoundedRect(D2D1::RectF(left, top, left + width, top + height), 4 * scale, 4 * scale),
        brush_.Get());
    brush_->SetColor(helpTextColour);
    target_->DrawTextLayout(D2D1::Point2F(left + padX, top + padY), layout.Get(), brush_.Get());
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
        SUCCEEDED(target_->CreateSolidColorBrush(veilColour, brush_.GetAddressOf()));
    if (!created) releaseDeviceResources();
    return created;
}

void Overlay::releaseDeviceResources() noexcept {
    brush_.Reset();
    bitmap_.Reset();
    target_.Reset();
}

core::Point Overlay::clampToImage(LPARAM lParam) const noexcept {
    // Signed coordinates: with mouse capture the cursor can leave the window.
    return {std::clamp(GET_X_LPARAM(lParam), 0, capture_.image.width - 1),
            std::clamp(GET_Y_LPARAM(lParam), 0, capture_.image.height - 1)};
}

void Overlay::redraw() noexcept {
    if (window_) InvalidateRect(window_, nullptr, FALSE);
}

void Overlay::confirm() {
    core::Image part = core::crop(capture_.image, selection_);
    std::vector<core::Finding> boxes = boxes_;
    for (core::Finding& box : boxes) box.area = core::offset(box.area, -selection_.x, -selection_.y);
    core::redact(part, boxes);

    close();
    if (onSelect_) onSelect_(part);
}

void Overlay::close() noexcept {
    mode_ = Mode::Selecting;
    drag_ = Drag::None;
    selection_ = {};
    boxes_.clear();
    if (window_ && IsWindowVisible(window_)) ShowWindow(window_, SW_HIDE);
    releaseDeviceResources();
    if (!capture_.image.bgra.empty())
        SecureZeroMemory(capture_.image.bgra.data(), capture_.image.bgra.size());
    capture_ = {};
}

} // namespace kodak::app
