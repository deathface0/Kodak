#pragma once

#include <kodak/core/Shortcut.h>

#include <windows.h>

#include <optional>

namespace kodak::win {

/// What RegisterHotKey takes for a shortcut.
struct HotkeyCode {
    UINT modifiers = 0; // MOD_CONTROL, MOD_ALT, MOD_SHIFT, MOD_WIN
    UINT virtualKey = 0;

    friend bool operator==(const HotkeyCode&, const HotkeyCode&) = default;
};

/// The RegisterHotKey codes for `shortcut`; std::nullopt for an unknown key name.
std::optional<HotkeyCode> toHotkey(const core::Shortcut& shortcut);

/// The shortcut for RegisterHotKey codes (MOD_* flags and a VK_* key), as read
/// from a hotkey control; std::nullopt for keys Kodak does not support (arrows,
/// numeric keypad, punctuation...).
std::optional<core::Shortcut> fromHotkey(UINT modifiers, UINT virtualKey);

} // namespace kodak::win
