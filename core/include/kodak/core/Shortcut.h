#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace kodak::core {

/// A keyboard shortcut such as Ctrl+Shift+X.
struct Shortcut {
    bool ctrl = false;
    bool alt = false;
    bool shift = false;
    bool win = false;
    std::string key; // canonical name: "X", "7", "F5", "PrintScreen"...

    friend bool operator==(const Shortcut&, const Shortcut&) = default;
};

/// Reads "Ctrl+Shift+X" (case-insensitive, modifiers in any order, spaces
/// allowed). Keys: A-Z, 0-9, F1-F24, PrintScreen, Insert, Delete, Home, End,
/// PageUp, PageDown, Pause and Space. A key that types text needs a modifier;
/// F-keys, PrintScreen and Pause may stand alone. std::nullopt if invalid.
std::optional<Shortcut> parseShortcut(std::string_view text);

/// "Ctrl+Alt+Shift+Win+Key", with only the modifiers that are set.
std::string formatShortcut(const Shortcut& shortcut);

} // namespace kodak::core
