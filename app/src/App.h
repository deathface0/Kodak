#pragma once

#include "Overlay.h"

#include <kodak/core/Settings.h>
#include <kodak/win/GdiScreenSource.h>
#include <kodak/win/GlobalHotkey.h>
#include <kodak/win/TrayIcon.h>

#include <windows.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace kodak::app {

/// Owns the hidden main window, the tray icon, the hotkey and the overlay.
class App {
public:
    explicit App(HINSTANCE instance);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// False if the main window could not be created.
    bool valid() const noexcept { return window_ != nullptr; }

    /// Runs the message loop until the user picks "Exit"; returns the exit code.
    int run();

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);

    void capture();
    void showMenu(POINT anchor);
    /// Copies the selection to the clipboard and saves it as a PNG in Pictures\Kodak.
    void deliver(const core::Image& image);
    void openCapturesFolder();
    /// The capture shortcut as shown to the user ("Ctrl+Shift+X").
    std::wstring hotkeyName() const;

    /// Opens the settings window, or brings it to the front if it is open.
    void openSettings();
    /// Puts `updated` in use and saves it; a message if the new shortcut is taken
    /// (the old one then stays registered and nothing changes).
    std::optional<std::wstring> applySettings(const core::Settings& updated);
    /// Registers the shortcut of settings_; false if another application owns it.
    bool registerHotkey();

    HINSTANCE instance_;
    HICON trayIcon_ = nullptr; // owned: loaded for the tray's DPI
    HWND window_ = nullptr;
    core::Settings settings_;
    std::optional<std::filesystem::path> settingsPath_;
    bool settingsFileHadErrors_ = false;
    HWND settingsDialog_ = nullptr;
    std::unique_ptr<win::TrayIcon> tray_;
    std::unique_ptr<win::GlobalHotkey> hotkey_;
    win::GdiScreenSource screen_;
    Overlay overlay_;
};

} // namespace kodak::app
