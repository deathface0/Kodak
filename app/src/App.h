#pragma once

#include "Overlay.h"

#include <kodak/win/GdiScreenSource.h>
#include <kodak/win/GlobalHotkey.h>
#include <kodak/win/TrayIcon.h>

#include <windows.h>

#include <memory>

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

    HWND window_ = nullptr;
    std::unique_ptr<win::TrayIcon> tray_;
    std::unique_ptr<win::GlobalHotkey> hotkey_;
    win::GdiScreenSource screen_;
    Overlay overlay_;
};

} // namespace kodak::app
