#include "App.h"

#include <kodak/win/Clipboard.h>

#include <windowsx.h>

#include <utility>

namespace kodak::app {

namespace {

constexpr wchar_t className[] = L"KodakMain";
constexpr UINT trayMessage = WM_APP + 1;
constexpr int hotkeyId = 1;
constexpr UINT_PTR captureTimerId = 1;
// Lets the tray menu finish fading out so it is not in the capture.
constexpr UINT menuFadeDelayMs = 250;

enum MenuCommand : UINT { menuCapture = 1, menuExit };

} // namespace

App::App(HINSTANCE instance)
    : overlay_(instance, [this](const core::Image& image) { copyToClipboard(image); }) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    RegisterClassExW(&windowClass);

    // A hidden top-level window rather than a message-only one: only top-level
    // windows receive "TaskbarCreated" when Explorer restarts.
    CreateWindowExW(0, className, L"Kodak", WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, instance, this);
    if (!window_) return;

    tray_ =
        std::make_unique<win::TrayIcon>(window_, trayMessage, LoadIconW(nullptr, IDI_APPLICATION), L"Kodak");
    hotkey_ = std::make_unique<win::GlobalHotkey>(window_, hotkeyId, MOD_CONTROL | MOD_SHIFT, 'X');
    if (!hotkey_->registered()) {
        tray_->showWarning(L"Ctrl+Shift+X is taken",
                           L"Another application uses that shortcut. Capture from the Kodak tray icon menu.");
    }
}

App::~App() {
    if (window_) DestroyWindow(window_);
}

int App::run() {
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK App::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* self = static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
        self->window_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    auto* self = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCDESTROY && self) {
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        self->window_ = nullptr;
    } else if (self) {
        return self->handle(message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT App::handle(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == win::TrayIcon::taskbarCreatedMessage()) {
        if (tray_) tray_->restore();
        return 0;
    }

    switch (message) {
    case WM_HOTKEY:
        if (wParam == hotkeyId) capture();
        return 0;

    case trayMessage:
        switch (LOWORD(lParam)) {
        case WM_CONTEXTMENU: showMenu({GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)}); break;
        case WM_LBUTTONDBLCLK: capture(); break;
        }
        return 0;

    case WM_TIMER:
        if (wParam == captureTimerId) {
            KillTimer(window_, captureTimerId);
            capture();
        }
        return 0;

    case WM_DESTROY:
        hotkey_.reset();
        tray_.reset();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

void App::capture() {
    if (overlay_.visible()) return;
    // Capture before showing anything, so the overlay never ends up in the image.
    auto screen = screen_.capture();
    if (!screen) {
        tray_->showWarning(L"Could not capture the screen", L"Please try again.");
        return;
    }
    overlay_.show(std::move(*screen));
}

void App::showMenu(POINT anchor) {
    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING, menuCapture, L"Capture\tCtrl+Shift+X");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, menuExit, L"Exit");
    SetMenuDefaultItem(menu, menuCapture, FALSE);

    SetForegroundWindow(window_); // otherwise the menu does not close when clicking elsewhere
    const UINT command = static_cast<UINT>(
        TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, anchor.x,
                         anchor.y, window_, nullptr));
    DestroyMenu(menu);
    PostMessageW(window_, WM_NULL, 0, 0);

    switch (command) {
    case menuCapture: SetTimer(window_, captureTimerId, menuFadeDelayMs, nullptr); break;
    case menuExit: DestroyWindow(window_); break;
    }
}

void App::copyToClipboard(const core::Image& image) {
    if (!win::copyImageToClipboard(window_, image))
        tray_->showWarning(L"Could not copy the capture", L"The clipboard is busy. Please try again.");
}

} // namespace kodak::app
