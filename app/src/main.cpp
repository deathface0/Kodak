#include "App.h"

#include <kodak/win/ComApartment.h>
#include <kodak/win/SingleInstance.h>

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const kodak::win::SingleInstance singleInstance(L"Local\\Kodak.SingleInstance");
    if (!singleInstance.isFirst()) return 0;

    const kodak::win::ComApartment com; // WIC (PNG) and ShellExecute
    if (!com.ok()) return 1;

    kodak::app::App app(instance);
    if (!app.valid()) return 1;
    return app.run();
}
