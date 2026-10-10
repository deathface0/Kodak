#include "kodak/win/SingleInstance.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const kodak::win::SingleInstance instance(L"Local\\Kodak.SingleInstance");
    if (!instance.isFirst()) return 0;

    // Phase 1 adds the hotkey, the tray icon and the overlay here.
    return 0;
}
