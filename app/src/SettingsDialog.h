#pragma once

#include <kodak/core/Settings.h>

#include <windows.h>

#include <functional>
#include <optional>
#include <string>

namespace kodak::app {

/// The settings window opened from the tray: shortcut, saving, which data to
/// look for and the user's own terms.
class SettingsDialog {
public:
    /// Applies new settings; returns a message to show if they could not be
    /// applied (the window then stays open).
    using Apply = std::function<std::optional<std::wstring>(const core::Settings&)>;

    struct Context {
        core::Settings settings;    // shown when the window opens
        std::wstring ocrLanguage;   // empty when no OCR language is installed
        bool fileHadErrors = false; // settings.json could not be read cleanly
        Apply apply;
    };

    /// Shows the window and returns when it closes. While it is open, `handle`
    /// holds the window, so another request can bring it to the front.
    static void run(HINSTANCE instance, HWND owner, Context context, HWND& handle);
};

} // namespace kodak::app
