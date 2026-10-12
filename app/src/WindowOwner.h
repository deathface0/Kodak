#pragma once

#include <windows.h>

namespace kodak::app {

/// Routes the messages of a window to the object that owns it. `Owner` derives
/// from WindowOwner<Owner> (and befriends it), registers its window class with
/// windowProc, passes itself as the creation parameter of CreateWindowExW and
/// handles the messages in handle(). window_ holds the window from WM_NCCREATE
/// until WM_NCDESTROY.
template <typename Owner> class WindowOwner {
protected:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto* owner = static_cast<Owner*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            owner->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
        }
        auto* owner = reinterpret_cast<Owner*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (owner && message == WM_NCDESTROY) {
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            owner->window_ = nullptr;
        } else if (owner) {
            return owner->handle(message, wParam, lParam);
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    HWND window_ = nullptr;
};

} // namespace kodak::app
