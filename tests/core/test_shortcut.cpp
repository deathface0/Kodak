#include <doctest/doctest.h>

#include <kodak/core/Shortcut.h>

#include <string>

using kodak::core::formatShortcut;
using kodak::core::parseShortcut;
using kodak::core::Shortcut;

namespace {

/// The canonical form of `text`, or "invalid".
std::string canonical(const std::string& text) {
    const auto shortcut = parseShortcut(text);
    return shortcut ? formatShortcut(*shortcut) : "invalid";
}

} // namespace

TEST_CASE("a shortcut is read with its modifiers and key") {
    const auto shortcut = parseShortcut("Ctrl+Shift+X");
    REQUIRE(shortcut);
    CHECK(*shortcut == Shortcut{.ctrl = true, .shift = true, .key = "X"});
    CHECK(formatShortcut(*shortcut) == "Ctrl+Shift+X");
}

TEST_CASE("case, spaces, order and aliases do not matter") {
    CHECK(canonical(" shift + ctrl + x ") == "Ctrl+Shift+X");
    CHECK(canonical("control+alt+del") == "Ctrl+Alt+Delete");
    CHECK(canonical("Win+Shift+S") == "Shift+Win+S");
    CHECK(canonical("windows+pgup") == "Win+PageUp");
    CHECK(canonical("Ctrl+7") == "Ctrl+7");
}

TEST_CASE("F-keys, PrintScreen and Pause may stand alone") {
    CHECK(canonical("F9") == "F9");
    CHECK(canonical("f24") == "F24");
    CHECK(canonical("PrtSc") == "PrintScreen");
    CHECK(canonical("Pause") == "Pause");
    CHECK(canonical("Alt+F4") == "Alt+F4");
}

TEST_CASE("invalid shortcuts are rejected") {
    CHECK(canonical("") == "invalid");
    CHECK(canonical("X") == "invalid");     // would swallow a typed letter
    CHECK(canonical("Space") == "invalid"); // likewise
    CHECK(canonical("Ctrl+") == "invalid");
    CHECK(canonical("Ctrl+Shift") == "invalid");
    CHECK(canonical("Ctrl+X+Y") == "invalid");
    CHECK(canonical("Ctrl+Foo") == "invalid");
    CHECK(canonical("F0") == "invalid");
    CHECK(canonical("F25") == "invalid");
    CHECK(canonical("Ctrl++") == "invalid");
}
