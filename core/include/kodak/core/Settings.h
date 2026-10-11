#pragma once

#include "kodak/core/Finding.h"
#include "kodak/core/Shortcut.h"

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace kodak::core {

/// What the user can configure. The defaults are what Kodak does out of the box.
struct Settings {
    Shortcut hotkey{.ctrl = true, .shift = true, .key = "X"};
    bool saveToFile = true;                // also write a PNG to Pictures\Kodak
    std::set<FindingKind> disabled;        // detections turned off
    std::vector<std::u32string> userTerms; // name, address... always proposed

    friend bool operator==(const Settings&, const Settings&) = default;
};

/// The settings as the JSON stored in settings.json, indented for hand editing.
std::string toJson(const Settings& settings);

/// Reads settings.json leniently: anything missing or of the wrong type keeps
/// its default and adds a warning; unknown fields are ignored; text that is not
/// valid JSON gives the defaults and a warning. Comments are allowed.
Settings fromJson(std::string_view json, std::vector<std::string>& warnings);

/// The user's terms as an editable text, one per line.
std::u32string joinTerms(const std::vector<std::u32string>& terms, std::u32string_view newline = U"\n");

/// The lines of `text` (LF or CRLF) as terms: trimmed, without empty lines or
/// repeats (ignoring case and accents, as the search does).
std::vector<std::u32string> splitTerms(std::u32string_view text);

} // namespace kodak::core
