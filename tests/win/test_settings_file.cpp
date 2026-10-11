#include <doctest/doctest.h>

#include "TestFiles.h"

#include <kodak/win/SettingsFile.h>
#include <kodak/win/ShortcutKeys.h>

#include <string>
#include <vector>

using kodak::core::FindingKind;
using kodak::core::Settings;
using kodak::core::Shortcut;
using kodak::test::readText;
using kodak::test::TempFolder;
using kodak::test::writeText;
using kodak::win::HotkeyCode;
using kodak::win::loadSettings;
using kodak::win::saveSettings;
using kodak::win::toHotkey;

TEST_CASE("a missing settings file gives the defaults and is created with them") {
    const TempFolder folder;
    const auto file = folder.path / "settings.json";

    const auto loaded = loadSettings(file);
    CHECK(loaded.settings == Settings{});
    CHECK(loaded.warnings.empty());
    REQUIRE(std::filesystem::exists(file));

    std::vector<std::string> warnings;
    CHECK(kodak::core::fromJson(readText(file), warnings) == Settings{});
}

TEST_CASE("settings saved to a file load back the same") {
    const TempFolder folder;
    const auto file = folder.path / "settings.json";
    Settings settings;
    settings.hotkey = Shortcut{.ctrl = true, .alt = true, .key = "K"};
    settings.saveToFile = false;
    settings.disabled = {FindingKind::Ip};
    settings.userTerms = {U"Jos\u00E9"};

    REQUIRE(saveSettings(file, settings));
    CHECK_FALSE(std::filesystem::exists(folder.path / "settings.json.tmp"));
    const auto loaded = loadSettings(file);
    CHECK(loaded.settings == settings);
    CHECK(loaded.warnings.empty());

    settings.saveToFile = true; // saving again replaces the file
    REQUIRE(saveSettings(file, settings));
    CHECK(loadSettings(file).settings == settings);
}

TEST_CASE("a broken settings file is never overwritten") {
    const TempFolder folder;
    const auto file = folder.path / "settings.json";
    const std::string broken = "{ \"hotkey\": \"Ctrl+Alt+K\", ";
    writeText(file, broken);

    const auto loaded = loadSettings(file);
    CHECK(loaded.settings == Settings{});
    CHECK(loaded.warnings.size() == 1);
    CHECK(readText(file) == broken);
}

TEST_CASE("a byte order mark left by an editor is accepted") {
    const TempFolder folder;
    const auto file = folder.path / "settings.json";
    writeText(file, "\xEF\xBB\xBF{\"saveToFile\": false}");

    const auto loaded = loadSettings(file);
    CHECK_FALSE(loaded.settings.saveToFile);
    CHECK(loaded.warnings.empty());
}

TEST_CASE("shortcuts map to RegisterHotKey codes") {
    CHECK(toHotkey(Shortcut{.ctrl = true, .shift = true, .key = "X"}) ==
          HotkeyCode{MOD_CONTROL | MOD_SHIFT, static_cast<UINT>('X')});
    CHECK(toHotkey(Shortcut{.key = "F9"}) == HotkeyCode{0, VK_F9});
    CHECK(toHotkey(Shortcut{.win = true, .key = "PrintScreen"}) == HotkeyCode{MOD_WIN, VK_SNAPSHOT});
    CHECK(toHotkey(Shortcut{.alt = true, .key = "7"}) == HotkeyCode{MOD_ALT, static_cast<UINT>('7')});
    CHECK_FALSE(toHotkey(Shortcut{.ctrl = true, .key = "Foo"}).has_value());
    CHECK_FALSE(toHotkey(Shortcut{.ctrl = true, .key = "F99"}).has_value());
}
