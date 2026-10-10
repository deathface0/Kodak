#include "kodak/win/TrayIcon.h"

#include <cwchar>

namespace kodak::win {

TrayIcon::TrayIcon(HWND window, UINT callbackMessage, HICON icon, const wchar_t* tooltip) noexcept {
    data_.cbSize = sizeof(data_);
    data_.hWnd = window;
    data_.uID = 1;
    data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data_.uCallbackMessage = callbackMessage;
    data_.hIcon = icon;
    wcsncpy_s(data_.szTip, tooltip, _TRUNCATE);
    added_ = add();
}

TrayIcon::~TrayIcon() {
    if (added_) Shell_NotifyIconW(NIM_DELETE, &data_);
}

void TrayIcon::restore() noexcept { added_ = add(); }

void TrayIcon::showWarning(const wchar_t* title, const wchar_t* text) noexcept {
    NOTIFYICONDATAW info = data_;
    info.uFlags = NIF_INFO;
    info.dwInfoFlags = NIIF_WARNING;
    wcsncpy_s(info.szInfoTitle, title, _TRUNCATE);
    wcsncpy_s(info.szInfo, text, _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &info);
}

UINT TrayIcon::taskbarCreatedMessage() noexcept {
    static const UINT message = RegisterWindowMessageW(L"TaskbarCreated");
    return message;
}

bool TrayIcon::add() noexcept {
    if (!Shell_NotifyIconW(NIM_ADD, &data_)) return false;
    NOTIFYICONDATAW version = data_;
    version.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &version);
    return true;
}

} // namespace kodak::win
