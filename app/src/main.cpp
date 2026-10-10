#include "App.h"

#include <kodak/win/SingleInstance.h>

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    const kodak::win::SingleInstance singleInstance(L"Local\\Kodak.SingleInstance");
    if (!singleInstance.isFirst()) return 0;

    kodak::app::App app(instance);
    if (!app.valid()) return 1;
    return app.run();
}
