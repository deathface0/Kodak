#include "App.h"

#include <kodak/win/ComApartment.h>

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // One Kodak per session: the mutex lives until the process exits, and a
    // second process finds it already there.
    const HANDLE running = CreateMutexW(nullptr, FALSE, L"Local\\Kodak.SingleInstance");
    if (!running || GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    const kodak::win::ComApartment com; // WIC (PNG) and ShellExecute
    if (!com.ok()) return 1;

    kodak::app::App app(instance);
    if (!app.valid()) return 1;
    return app.run();
}
