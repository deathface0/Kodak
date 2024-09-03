#pragma once

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include "stb_image_write.h"

#define ID_TRAY_APP_ICON    1001
#define ID_TRAY_EXIT        1002
#define WM_TRAYICON         (WM_USER + 1)

NOTIFYICONDATA nid;

void AddTrayIcon(HWND hwnd) {
    memset(&nid, 0, sizeof(NOTIFYICONDATA));

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = ID_TRAY_APP_ICON;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);

#ifdef UNICODE
    wcscpy_s(nid.szTip, L"Kodak"); // For Unicode
#else
    strcpy_s(nid.szTip, "Kodak");  // For ANSI
#endif

    Shell_NotifyIcon(NIM_ADD, &nid);
}


std::string base64_encode(const std::vector<unsigned char>& data) {
    static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::stringstream ss;
    for (size_t i = 0; i < data.size(); i += 3) {
        int b = (data[i] & 0xFC) >> 2;
        ss << encoding[b];
        b = (data[i] & 0x03) << 4;
        if (i + 1 < data.size()) {
            b |= (data[i + 1] & 0xF0) >> 4;
            ss << encoding[b];
            b = (data[i + 1] & 0x0F) << 2;
            if (i + 2 < data.size()) {
                b |= (data[i + 2] & 0xC0) >> 6;
                ss << encoding[b];
                b = data[i + 2] & 0x3F;
                ss << encoding[b];
            }
            else {
                ss << encoding[b];
                ss << '=';
            }
        }
        else {
            ss << encoding[b];
            ss << "==";
        }
    }
    return ss.str();
}

std::string EncodeHBITMAPToBase64(HBITMAP hBitmap, const std::string& format)
{
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    int channels = bmp.bmBitsPixel / 8;

    std::vector<unsigned char> pixels(width * height * channels);

    // Get the bitmap data
    GetBitmapBits(hBitmap, width * height * channels, pixels.data());

    // Convert BGR to RGB
    for (int i = 0; i < width * height * channels; i += channels)
    {
        std::swap(pixels[i], pixels[i + 2]);
    }

    std::vector<unsigned char> output;
    if (format == "png")
    {
        stbi_write_png_to_func([](void* context, void* data, int size) {
            std::vector<unsigned char>* output = static_cast<std::vector<unsigned char>*>(context);
            output->insert(output->end(), static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + size);
            }, &output, width, height, channels, pixels.data(), width * channels);
    }
    else if (format == "jpg" || format == "jpeg")
    {
        stbi_write_jpg_to_func([](void* context, void* data, int size) {
            std::vector<unsigned char>* output = static_cast<std::vector<unsigned char>*>(context);
            output->insert(output->end(), static_cast<unsigned char*>(data), static_cast<unsigned char*>(data) + size);
            }, &output, width, height, channels, pixels.data(), 100);
    }
    else
    {
        return ""; // Unsupported format
    }

    std::string base64 = base64_encode(output);
    std::string mime_type = (format == "png") ? "image/png" : "image/jpeg";
    return "data:" + mime_type + ";base64," + base64;
}

bool SaveHBITMAPToFile(HBITMAP hBitmap, const std::string& filepath, const std::string& format)
{
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    int width = bmp.bmWidth;
    int height = bmp.bmHeight;
    int channels = bmp.bmBitsPixel / 8;

    std::vector<unsigned char> pixels(width * height * channels);

    // Get the bitmap data
    GetBitmapBits(hBitmap, width * height * channels, pixels.data());

    // Convert BGR to RGB
    for (int i = 0; i < width * height * channels; i += channels)
    {
        std::swap(pixels[i], pixels[i + 2]);
    }

    int result;
    if (format == "png")
    {
        result = stbi_write_png(filepath.c_str(), width, height, channels, pixels.data(), width * channels);
    }
    else if (format == "jpg" || format == "jpeg")
    {
        result = stbi_write_jpg(filepath.c_str(), width, height, channels, pixels.data(), 100);
    }
    else
    {
        return false; // Unsupported format
    }

    return result != 0;
}

HBITMAP CaptureScreen(int x, int y, int width, int height) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);

    SelectObject(hdcMem, hbmScreen);

    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY);

    ReleaseDC(NULL, hdcScreen);
    DeleteDC(hdcMem);

    return hbmScreen;
}

