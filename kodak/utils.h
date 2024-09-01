#pragma once

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <windows.h>
#include <vector>
#include <string>
#include "stb_image_write.h"

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

