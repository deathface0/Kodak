#include "TextTestSupport.h"

#include <kodak/core/Settings.h>

#include <string>
#include <vector>

using kodak::core::FindingKind;
using kodak::core::fromJson;
using kodak::core::Settings;
using kodak::core::Shortcut;
using kodak::core::toJson;

namespace {

Settings read(const std::string& json, std::vector<std::string>& warnings) {
    return fromJson(json, warnings);
}

} // namespace

TEST_CASE("the defaults: Ctrl+Shift+X, save to file, every detection, no terms") {
    const Settings settings;
    CHECK(settings.hotkey == Shortcut{.ctrl = true, .shift = true, .key = "X"});
    CHECK(settings.saveToFile);
    CHECK(settings.disabled.empty());
    CHECK(settings.userTerms.empty());

    const std::string json = toJson(settings);
    CHECK(json.find("\"hotkey\": \"Ctrl+Shift+X\"") != std::string::npos);
    CHECK(json.find("\"email\": true") != std::string::npos);
}

TEST_CASE("settings survive a round trip through JSON") {
    Settings settings;
    settings.hotkey = Shortcut{.alt = true, .key = "F9"};
    settings.saveToFile = false;
    settings.disabled = {FindingKind::Email, FindingKind::Phone};
    settings.userTerms = {U"Jos\u00E9 P\u00E9rez", U"Calle Mayor 5"};

    std::vector<std::string> warnings;
    CHECK(read(toJson(settings), warnings) == settings);
    CHECK(warnings.empty());
}

TEST_CASE("missing fields keep their defaults, unknown ones are ignored") {
    std::vector<std::string> warnings;
    CHECK(read("{}", warnings) == Settings{});
    CHECK(read(R"({"theme": "dark", "detect": {"faces": false}})", warnings) == Settings{});
    CHECK(warnings.empty());
}

TEST_CASE("fields of the wrong type keep their default and warn") {
    std::vector<std::string> warnings;
    const Settings settings = read(R"({"hotkey": 5, "saveToFile": "yes",
                                       "detect": {"email": "no", "phone": false},
                                       "userTerms": ["Ana", 7, "   "]})",
                                   warnings);
    CHECK(settings.hotkey == Settings{}.hotkey);
    CHECK(settings.saveToFile);
    CHECK(settings.disabled == std::set<FindingKind>{FindingKind::Phone});
    CHECK(settings.userTerms == std::vector<std::u32string>{U"Ana"});
    CHECK(warnings.size() == 4); // hotkey, saveToFile, detect.email, the 7 in userTerms
}

TEST_CASE("an invalid shortcut keeps the default one") {
    std::vector<std::string> warnings;
    CHECK(read(R"({"hotkey": "Ctrl+Foo"})", warnings).hotkey == Settings{}.hotkey);
    CHECK(warnings.size() == 1);
}

TEST_CASE("text that is not JSON gives the defaults and a warning") {
    std::vector<std::string> warnings;
    CHECK(read("{ \"hotkey\": ", warnings) == Settings{});
    CHECK(read("[1, 2]", warnings) == Settings{});
    CHECK(warnings.size() == 2);
}

TEST_CASE("comments are allowed, for hand editing") {
    std::vector<std::string> warnings;
    CHECK_FALSE(read("{ // no PNG files\n \"saveToFile\": false }", warnings).saveToFile);
    CHECK(warnings.empty());
}

TEST_CASE("a newer version is read as far as it is understood") {
    std::vector<std::string> warnings;
    CHECK_FALSE(read(R"({"version": 2, "saveToFile": false})", warnings).saveToFile);
    CHECK(warnings.size() == 1);
}
