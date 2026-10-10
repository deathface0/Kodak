#pragma once

#include <windows.h>

namespace kodak::win {

/// A system-wide shortcut registered for the lifetime of the object. Windows
/// sends WM_HOTKEY (wParam = id) to the window's thread, so no polling thread
/// is needed.
class GlobalHotkey {
public:
    /// `modifiers` are MOD_* flags (MOD_NOREPEAT is always added); `virtualKey` a VK_* or 'A'..'Z'.
    GlobalHotkey(HWND window, int id, UINT modifiers, UINT virtualKey) noexcept;
    ~GlobalHotkey();

    GlobalHotkey(const GlobalHotkey&) = delete;
    GlobalHotkey& operator=(const GlobalHotkey&) = delete;

    /// False when another application already owns the combination.
    bool registered() const noexcept { return registered_; }

private:
    HWND window_;
    int id_;
    bool registered_;
};

} // namespace kodak::win
