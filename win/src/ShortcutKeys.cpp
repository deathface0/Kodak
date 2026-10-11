#include "kodak/win/ShortcutKeys.h"

#include <array>
#include <string_view>
#include <utility>

namespace kodak::win {

namespace {

std::optional<UINT> virtualKeyOf(const std::string& key) {
    if (key.size() == 1 && ((key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= '0' && key[0] <= '9')))
        return static_cast<UINT>(key[0]); // VK codes of letters and digits are their ASCII codes

    if (key.size() >= 2 && key.size() <= 3 && key[0] == 'F') {
        int number = 0;
        for (std::size_t i = 1; i < key.size(); ++i) {
            if (key[i] < '0' || key[i] > '9') return std::nullopt;
            number = number * 10 + (key[i] - '0');
        }
        if (number >= 1 && number <= 24) return static_cast<UINT>(VK_F1 + number - 1);
        return std::nullopt;
    }

    static constexpr std::array<std::pair<std::string_view, UINT>, 9> named{{
        {"PrintScreen", VK_SNAPSHOT},
        {"Pause", VK_PAUSE},
        {"Insert", VK_INSERT},
        {"Delete", VK_DELETE},
        {"Home", VK_HOME},
        {"End", VK_END},
        {"PageUp", VK_PRIOR},
        {"PageDown", VK_NEXT},
        {"Space", VK_SPACE},
    }};
    for (const auto& [name, code] : named) {
        if (key == name) return code;
    }
    return std::nullopt;
}

} // namespace

std::optional<HotkeyCode> toHotkey(const core::Shortcut& shortcut) {
    const std::optional<UINT> key = virtualKeyOf(shortcut.key);
    if (!key) return std::nullopt;

    HotkeyCode code{0, *key};
    if (shortcut.ctrl) code.modifiers |= MOD_CONTROL;
    if (shortcut.alt) code.modifiers |= MOD_ALT;
    if (shortcut.shift) code.modifiers |= MOD_SHIFT;
    if (shortcut.win) code.modifiers |= MOD_WIN;
    return code;
}

} // namespace kodak::win
