#include "kodak/core/Shortcut.h"

#include <array>
#include <cctype>
#include <utility>

namespace kodak::core {

namespace {

std::string lower(std::string_view text) {
    std::string out(text);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && text.front() == ' ') text.remove_prefix(1);
    while (!text.empty() && text.back() == ' ') text.remove_suffix(1);
    return text;
}

/// The canonical name of a key, and whether it may be used without a modifier.
struct KeyName {
    std::string name;
    bool standalone = false;
};

std::optional<KeyName> canonicalKey(std::string_view text) {
    const std::string key = lower(text);
    if (key.size() == 1 && std::isalnum(static_cast<unsigned char>(key[0])))
        return KeyName{std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(key[0])))),
                       false};

    if (key.size() >= 2 && key.size() <= 3 && key[0] == 'f') {
        int number = 0;
        for (std::size_t i = 1; i < key.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(key[i]))) return std::nullopt;
            number = number * 10 + (key[i] - '0');
        }
        if (number >= 1 && number <= 24) return KeyName{"F" + std::to_string(number), true};
        return std::nullopt;
    }

    static const std::array<std::pair<std::string_view, KeyName>, 15> named{{
        {"printscreen", {"PrintScreen", true}},
        {"prtsc", {"PrintScreen", true}},
        {"print", {"PrintScreen", true}},
        {"pause", {"Pause", true}},
        {"insert", {"Insert", false}},
        {"ins", {"Insert", false}},
        {"delete", {"Delete", false}},
        {"del", {"Delete", false}},
        {"home", {"Home", false}},
        {"end", {"End", false}},
        {"pageup", {"PageUp", false}},
        {"pgup", {"PageUp", false}},
        {"pagedown", {"PageDown", false}},
        {"pgdn", {"PageDown", false}},
        {"space", {"Space", false}},
    }};
    for (const auto& [alias, name] : named) {
        if (key == alias) return name;
    }
    return std::nullopt;
}

} // namespace

std::optional<Shortcut> parseShortcut(std::string_view text) {
    Shortcut shortcut;
    std::optional<KeyName> key;
    while (true) {
        const std::size_t plus = text.find('+');
        const std::string_view part = trim(text.substr(0, plus));
        if (part.empty()) return std::nullopt;

        const std::string name = lower(part);
        if (name == "ctrl" || name == "control") {
            shortcut.ctrl = true;
        } else if (name == "alt") {
            shortcut.alt = true;
        } else if (name == "shift") {
            shortcut.shift = true;
        } else if (name == "win" || name == "windows") {
            shortcut.win = true;
        } else if (!key) {
            key = canonicalKey(part);
            if (!key) return std::nullopt;
        } else {
            return std::nullopt; // a second key
        }

        if (plus == std::string_view::npos) break;
        text.remove_prefix(plus + 1);
    }

    if (!key) return std::nullopt;
    const bool modified = shortcut.ctrl || shortcut.alt || shortcut.shift || shortcut.win;
    if (!modified && !key->standalone) return std::nullopt; // it would swallow a typed character
    shortcut.key = key->name;
    return shortcut;
}

std::string formatShortcut(const Shortcut& shortcut) {
    std::string text;
    if (shortcut.ctrl) text += "Ctrl+";
    if (shortcut.alt) text += "Alt+";
    if (shortcut.shift) text += "Shift+";
    if (shortcut.win) text += "Win+";
    return text + shortcut.key;
}

} // namespace kodak::core
