#include "Overlay.h"

#include <kodak/core/Redaction.h>
#include <kodak/core/Selection.h>
#include <kodak/core/TextDetector.h>
#include <kodak/win/ComApartment.h>
#include <kodak/win/WinOcrRecognizer.h>

#include <shellscalingapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace kodak::app {

namespace {

constexpr wchar_t className[] = L"KodakOverlay";
// Posted by the worker when the analysis is done; wParam is its generation.
constexpr UINT detectedMessage = WM_APP + 1;

const D2D1_COLOR_F veilColour = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.5f);
const D2D1_COLOR_F borderColour = D2D1::ColorF(D2D1::ColorF::White);
const D2D1_COLOR_F boxColour = D2D1::ColorF(D2D1::ColorF::Black); // what the output will show
const D2D1_COLOR_F boxOutlineColour = D2D1::ColorF(0.6f, 0.6f, 0.6f);
const D2D1_COLOR_F proposalColour = D2D1::ColorF(1.0f, 0.75f, 0.0f); // amber
const D2D1_COLOR_F helpBackColour = D2D1::ColorF(0.1f, 0.1f, 0.1f, 0.85f);
const D2D1_COLOR_F helpTextColour = D2D1::ColorF(D2D1::ColorF::White);

D2D1_RECT_F toRectF(const core::Rect& r) noexcept {
    return D2D1::RectF(static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.x + r.width),
                       static_cast<float>(r.y + r.height));
}

/// An outline `width` pixels wide drawn just inside `r`.
D2D1_RECT_F insideOutline(const D2D1_RECT_F& r, float width = 1.0f) noexcept {
    const float half = width / 2;
    return D2D1::RectF(r.left + half, r.top + half, r.right - half, r.bottom - half);
}

/// " . " with a middle dot, between the parts of the help line.
const std::wstring separator = std::wstring(L" ") + static_cast<wchar_t>(0xB7) + L" ";

} // namespace

Overlay::Overlay(HINSTANCE instance, SelectHandler onSelect) : onSelect_(std::move(onSelect)) {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_.GetAddressOf());
    if (factory_) {
        factory_->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
                                                                D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER,
                                                                10.0f, D2D1_DASH_STYLE_DASH),
                                    nullptr, 0, dashed_.GetAddressOf());
    }
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

void Overlay::configureDetection(std::vector<std::u32string> userTerms,
                                 std::set<core::FindingKind> disabled) {
    userTerms_ = std::move(userTerms);
    disabledKinds_ = std::move(disabled);
}

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
    case WM_LBUTTONDBLCLK: onDoubleClick(clampToImage(lParam)); return 0;
    case WM_RBUTTONUP: onRightClick(clampToImage(lParam)); return 0;

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
        case VK_RETURN: requestConfirm(); break;
        case VK_BACK: removeLastBox(); break;
        case 'Z':
            if (GetKeyState(VK_CONTROL) < 0) removeLastBox();
            break;
        }
        return 0;

    case detectedMessage: onDetectionDone(wParam); return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) close(); // the user switched away
        return 0;

    case WM_DISPLAYCHANGE: close(); return 0; // the capture no longer matches the monitors

    case WM_DPICHANGED: return 0; // keep covering the virtual desktop in physical pixels
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

void Overlay::onButtonDown(core::Point p) {
    // Inside the selection under review: a redaction box or a click on a
    // proposal. Anywhere else: a new selection, which replaces the current one
    // only once it is usable.
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
        startDetection();
    } else if (finished == Drag::Box) {
        const core::Rect box = core::intersect(dragged, selection_);
        if (core::isUsableSelection(box)) {
            boxes_.push_back({box, core::FindingKind::Manual});
        } else {
            core::toggleAt(proposals_, p); // a click, not a drag
        }
    }
    redraw();
}

void Overlay::onDoubleClick(core::Point p) {
    if (mode_ != Mode::Reviewing) return;
    // The first click of the double-click already toggled a proposal under the
    // cursor; undo that so confirming does not change it.
    if (selection_.contains(p)) core::toggleAt(proposals_, p);
    requestConfirm();
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

void Overlay::startDetection() {
    stopDetection();
    proposals_.clear();
    detection_ = Detection::Running;
    const unsigned generation = ++generation_;

    // The worker gets its own copy of the selected pixels and wipes it when done.
    worker_ = std::jthread([this, generation, part = core::crop(capture_.image, selection_),
                            origin = core::Point{selection_.x, selection_.y}, terms = userTerms_,
                            disabled = disabledKinds_](std::stop_token stop) mutable {
        std::vector<core::Finding> found;
        bool available = false;
        {
            const win::ComApartment com(win::ComApartment::Model::MultiThreaded); // the OCR needs the MTA
            if (com.ok()) {
                win::WinOcrRecognizer ocr;
                available = ocr.available();
                if (available) {
                    core::TextDetector detector(ocr, core::defaultRules(std::move(terms), disabled));
                    found = detector.detect(part, part.bounds(), stop);
                }
            }
        }
        if (!part.bgra.empty()) SecureZeroMemory(part.bgra.data(), part.bgra.size());
        if (stop.stop_requested()) return;

        for (core::Finding& finding : found) finding.area = core::offset(finding.area, origin.x, origin.y);
        {
            const std::lock_guard lock(resultMutex_);
            result_ = std::move(found);
            resultAvailable_ = available;
        }
        PostMessageW(window_, detectedMessage, generation, 0);
    });
}

void Overlay::stopDetection() noexcept {
    if (worker_.joinable()) {
        worker_.request_stop();
        worker_.join(); // at most the OCR tile in progress
    }
    ++generation_; // a result already posted is now stale
    detection_ = Detection::Idle;
    confirmPending_ = false;
}

void Overlay::onDetectionDone(WPARAM generation) {
    if (generation != generation_ || mode_ != Mode::Reviewing) return;
    if (worker_.joinable()) worker_.join(); // posting was its last step

    bool available = true;
    {
        const std::lock_guard lock(resultMutex_);
        proposals_ = std::move(result_);
        result_.clear();
        available = resultAvailable_;
    }
    detection_ = available ? Detection::Done : Detection::Unavailable;
    redraw();
    if (confirmPending_) confirm();
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
        // Proposals: black as the output will be, with an amber outline; turned
        // off, only a dashed outline, so what stays visible is still marked.
        for (const core::Finding& proposal : proposals_) {
            const D2D1_RECT_F rect = toRectF(proposal.area);
            if (proposal.enabled) {
                brush_->SetColor(boxColour);
                target_->FillRectangle(rect, brush_.Get());
                brush_->SetColor(proposalColour);
                target_->DrawRectangle(insideOutline(rect, 2.0f), brush_.Get(), 2.0f);
            } else {
                brush_->SetColor(proposalColour);
                target_->DrawRectangle(insideOutline(rect, 2.0f), brush_.Get(), 2.0f, dashed_.Get());
            }
        }

        std::vector<core::Rect> manual;
        for (const core::Finding& box : boxes_) manual.push_back(box.area);
        if (drag_ == Drag::Box)
            manual.push_back(core::intersect(core::fromCorners(anchor_, current_), selection_));
        for (const core::Rect& box : manual) {
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

std::wstring Overlay::helpLine() const {
    const std::wstring actions = L"drag to redact" + separator + L"right-click to remove" + separator +
                                 L"Enter to save" + separator + L"Esc to cancel";
    switch (detection_) {
    case Detection::Running:
        return confirmPending_ ? L"Looking for sensitive text" + separator + L"will save when done"
                               : L"Looking for sensitive text..." + separator + actions;
    case Detection::Done:
        if (proposals_.empty()) return L"No sensitive text found" + separator + actions;
        return std::to_wstring(proposals_.size()) +
               (proposals_.size() == 1 ? L" suggestion" : L" suggestions") + separator + L"click to toggle" +
               separator + actions;
    case Detection::Unavailable:
        return L"Text detection unavailable (no OCR language installed)" + separator + actions;
    case Detection::Idle: break;
    }
    return actions;
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

    const std::wstring text = helpLine();
    Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
    if (FAILED(writeFactory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()),
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

void Overlay::requestConfirm() {
    if (mode_ != Mode::Reviewing) return;
    if (detection_ == Detection::Running) { // nothing leaves before it has been analysed
        confirmPending_ = true;
        redraw();
        return;
    }
    confirm();
}

void Overlay::confirm() {
    core::Image part = core::crop(capture_.image, selection_);
    std::vector<core::Finding> covered = boxes_;
    covered.insert(covered.end(), proposals_.begin(), proposals_.end()); // redact() skips disabled ones
    for (core::Finding& finding : covered)
        finding.area = core::offset(finding.area, -selection_.x, -selection_.y);
    core::redact(part, covered);

    close();
    if (onSelect_) onSelect_(part);
}

void Overlay::close() noexcept {
    stopDetection(); // before wiping: the worker must be done with the capture
    mode_ = Mode::Selecting;
    drag_ = Drag::None;
    selection_ = {};
    boxes_.clear();
    proposals_.clear();
    if (window_ && IsWindowVisible(window_)) ShowWindow(window_, SW_HIDE);
    releaseDeviceResources();
    if (!capture_.image.bgra.empty())
        SecureZeroMemory(capture_.image.bgra.data(), capture_.image.bgra.size());
    capture_ = {};
}

} // namespace kodak::app
