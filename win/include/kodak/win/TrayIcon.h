#pragma once

#include <windows.h>

#include <shellapi.h>

namespace kodak::win {

/// The notification-area icon, shown for the lifetime of the object.
///
/// Uses the NOTIFYICON_VERSION_4 callback layout: `window` receives
/// `callbackMessage` with LOWORD(lParam) = the event (WM_CONTEXTMENU,
/// WM_LBUTTONDBLCLK…) and GET_X_LPARAM/GET_Y_LPARAM(wParam) = the anchor point.
class TrayIcon {
public:
    TrayIcon(HWND window, UINT callbackMessage, HICON icon, const wchar_t* tooltip) noexcept;
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    /// Adds the icon again; call it on taskbarCreatedMessage() (Explorer restarted).
    void restore() noexcept;

    /// Shows a warning notification next to the icon.
    void showWarning(const wchar_t* title, const wchar_t* text) noexcept;

    /// The message Explorer broadcasts to every top-level window when the taskbar is recreated.
    static UINT taskbarCreatedMessage() noexcept;

private:
    bool add() noexcept;

    NOTIFYICONDATAW data_{};
    bool added_ = false;
};

} // namespace kodak::win
