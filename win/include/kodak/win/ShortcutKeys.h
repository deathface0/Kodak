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

} // namespace kodak::win
