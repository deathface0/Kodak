#pragma once

#include <kodak/core/Settings.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace kodak::win {

/// The settings read from disk and what was wrong with the file, if anything.
struct LoadedSettings {
    core::Settings settings;
    std::vector<std::string> warnings;
};

/// %APPDATA%\Kodak\settings.json for the current user; the folder is created if missing.
std::optional<std::filesystem::path> settingsPath();

/// Reads `file`. A missing file gives the defaults and is created with them, so it
/// can be edited by hand. A file that cannot be read or parsed is left untouched.
LoadedSettings loadSettings(const std::filesystem::path& file);

/// Writes `settings` to `file` atomically: a temporary file replaces it, so a
/// crash never leaves half a settings file.
bool saveSettings(const std::filesystem::path& file, const core::Settings& settings);

} // namespace kodak::win
