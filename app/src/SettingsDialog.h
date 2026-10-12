#pragma once

#include <kodak/core/Settings.h>

#include <windows.h>

#include <functional>
#include <optional>
#include <string>

namespace kodak::app {

/// What the settings window shows, and how it puts the user's changes in use.
struct SettingsContext {
    core::Settings settings;    // shown when the window opens
    std::wstring ocrLanguage;   // empty when no OCR language is installed
    bool fileHadErrors = false; // settings.json could not be read cleanly
    /// Applies new settings; returns a message to show if they could not be
    /// applied (the window then stays open).
    std::function<std::optional<std::wstring>(const core::Settings&)> apply;
};

/// Shows the settings window opened from the tray (shortcut, saving, which data
/// to look for and the user's own terms) and returns when it closes. While it
/// is open, `handle` holds the window, so another request can bring it to the front.
void runSettingsDialog(HINSTANCE instance, HWND owner, SettingsContext context, HWND& handle);

} // namespace kodak::app
