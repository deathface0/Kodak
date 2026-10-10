#include "kodak/win/GlobalHotkey.h"

namespace kodak::win {

GlobalHotkey::GlobalHotkey(HWND window, int id, UINT modifiers, UINT virtualKey) noexcept
    : window_(window), id_(id),
      registered_(RegisterHotKey(window, id, modifiers | MOD_NOREPEAT, virtualKey) != FALSE) {}

GlobalHotkey::~GlobalHotkey() {
    if (registered_) UnregisterHotKey(window_, id_);
}

} // namespace kodak::win
