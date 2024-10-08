#include <fstream>
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

struct KODAK_INFO {
    HWND hwnd;
    HHOOK kbHook;
    HCURSOR hCursor;
    POINT startPoint;
    POINT currentPoint;
    bool isDrawing = false;
} kinfo;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HBITMAP hBitmap = NULL;

    switch (msg) {
    case WM_LBUTTONDOWN: {
        kinfo.startPoint.x = LOWORD(lParam);
        kinfo.startPoint.y = HIWORD(lParam);
        kinfo.isDrawing = IsWindowVisible(hwnd) ? true : false;
        SetCapture(hwnd);
        break;
    }
    case WM_LBUTTONUP: {
        if (!kinfo.isDrawing)
            break;

        ShowWindow(hwnd, FALSE);

        kinfo.currentPoint.x = LOWORD(lParam);
        kinfo.currentPoint.y = HIWORD(lParam);
        kinfo.isDrawing = false;

        // Calculate the top-left corner of the rectangle
        int x = min(kinfo.startPoint.x, kinfo.currentPoint.x);
        int y = min(kinfo.startPoint.y, kinfo.currentPoint.y);

        // Calculate the width and height of the rectangle
        UINT width = abs(kinfo.currentPoint.x - kinfo.startPoint.x);
        UINT height = abs(kinfo.currentPoint.y - kinfo.startPoint.y);

        hBitmap = CaptureScreen(x, y, width, height);

        std::string base64_image = EncodeHBITMAPToBase64(hBitmap, "png");
        if (!base64_image.empty()) {
            std::string html = "<title>Kodak Screenshot</title><img src='" + base64_image + "' />";
            std::string temp_file = std::tmpnam(nullptr);
            temp_file += ".html";

            std::ofstream out(temp_file);
            out << html;
            out.close();

            ShellExecuteA(NULL, "open", temp_file.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }

        ReleaseCapture();
        InvalidateRect(hwnd, NULL, TRUE);

        ShowWindow(hwnd, SW_HIDE);

        break;
    }
    case WM_MOUSEMOVE: {
        if (!kinfo.isDrawing)
            break;

        kinfo.currentPoint.x = LOWORD(lParam);
        kinfo.currentPoint.y = HIWORD(lParam);
        InvalidateRect(hwnd, NULL, TRUE);

        break;
    }
    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(kinfo.hCursor);
            return TRUE;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        HBRUSH hGrayBrush = CreateSolidBrush(RGB(24, 24, 24));
        FillRect(hdc, &rect, hGrayBrush);

        if (kinfo.isDrawing)
            Rectangle(hdc, kinfo.startPoint.x, kinfo.startPoint.y, kinfo.currentPoint.x, kinfo.currentPoint.y);

        EndPaint(hwnd, &ps);        

        break;
    }
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONDOWN) {
            // Create the menu
            HMENU hMenu = CreatePopupMenu();
            if (hMenu) {
                // Add menu items
                InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

                // Get the current cursor position (where the user clicked)
                POINT cursorPos;
                GetCursorPos(&cursorPos);

                // Display the context menu
                SetForegroundWindow(hwnd); // Required to bring the menu to the foreground
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursorPos.x, cursorPos.y, 0, hwnd, NULL);

                DestroyMenu(hMenu); // Destroy the menu after use
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            DestroyWindow(hwnd);
            break;
        }
        break;
    case WM_DESTROY: {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    kinfo.hCursor = LoadCursor(NULL, IDC_CROSS);  // Load the cursor

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"KodakWindow";
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    RegisterClass(&wc);

    kinfo.hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST,  // Extended window styles
        L"KodakWindow",                 // Class name
        L"Kodak",                       // Window title
        WS_POPUP,                       // Window style (borderless)
        0, 0,                           // Position (x, y)
        GetSystemMetrics(SM_CXSCREEN),  // Width (full screen)
        GetSystemMetrics(SM_CYSCREEN),  // Height (full screen)
        NULL, NULL, hInstance, NULL
    );
    
    SetWindowLongW(kinfo.hwnd, GWL_EXSTYLE, GetWindowLong(kinfo.hwnd, GWL_EXSTYLE) | WS_EX_LAYERED); // Set the window to be layered
    SetLayeredWindowAttributes(kinfo.hwnd, 0, 124, LWA_ALPHA); // Set the opacity (128 for 50% transparency)
    HBRUSH hBrush = CreateSolidBrush(RGB(24, 24, 24)); // Create brush
    SetClassLongPtr(kinfo.hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrush); // Set the window background color
    InvalidateRect(kinfo.hwnd, NULL, TRUE); // Force the window to redraw

    UpdateWindow(kinfo.hwnd);

    std::thread([&] {
        while (true) {
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
                (GetAsyncKeyState('S') & 0x8000))
            {
                SetForegroundWindow(kinfo.hwnd);
                ShowWindow(kinfo.hwnd, SW_SHOW);
            }
            else if (GetAsyncKeyState(VK_ESCAPE))
            {
                ShowWindow(kinfo.hwnd, SW_HIDE);
                kinfo.isDrawing = false;
                InvalidateRect(kinfo.hwnd, NULL, TRUE); // Force the window to redraw
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Prevent CPU overuse
        }
        }
    ).detach();

    // Add the tray icon
    AddTrayIcon(kinfo.hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(kinfo.kbHook);

    return 0;
}
