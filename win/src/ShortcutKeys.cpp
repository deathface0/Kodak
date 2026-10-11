#include "kodak/win/ShortcutKeys.h"

#include <array>
#include <string_view>
#include <utility>

namespace kodak::win {

namespace {

/// Keys known by name, with their virtual-key codes.
constexpr std::array<std::pair<std::string_view, UINT>, 9> namedKeys{{
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

constexpr bool isLetterOrDigit(UINT code) noexcept {
    return (code >= 'A' && code <= 'Z') || (code >= '0' && code <= '9');
}

std::optional<UINT> virtualKeyOf(const std::string& key) {
    if (key.size() == 1 && isLetterOrDigit(static_cast<unsigned char>(key[0])))
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

    for (const auto& [name, code] : namedKeys) {
        if (key == name) return code;
    }
    return std::nullopt;
}

std::optional<std::string> keyNameOf(UINT code) {
    if (isLetterOrDigit(code)) return std::string(1, static_cast<char>(code));
    if (code >= VK_F1 && code <= VK_F24) return "F" + std::to_string(code - VK_F1 + 1);
    for (const auto& [name, known] : namedKeys) {
        if (code == known) return std::string(name);
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

std::optional<core::Shortcut> fromHotkey(UINT modifiers, UINT virtualKey) {
    std::optional<std::string> key = keyNameOf(virtualKey);
    if (!key) return std::nullopt;
    return core::Shortcut{.ctrl = (modifiers & MOD_CONTROL) != 0,
                          .alt = (modifiers & MOD_ALT) != 0,
                          .shift = (modifiers & MOD_SHIFT) != 0,
                          .win = (modifiers & MOD_WIN) != 0,
                          .key = std::move(*key)};
}

} // namespace kodak::win
