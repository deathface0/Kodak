#include "kodak/win/Clipboard.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kodak::win {

namespace {

/// Another application may hold the clipboard for a moment; retry briefly.
bool openClipboard(HWND owner) {
    for (int attempt = 0; attempt < 10; ++attempt) {
        if (OpenClipboard(owner)) return true;
        Sleep(20);
    }
    return false;
}

/// Frees the memory block unless ownership passed to the clipboard.
struct GlobalBlock {
    HGLOBAL handle;
    ~GlobalBlock() {
        if (handle) GlobalFree(handle);
    }
};

} // namespace

bool copyImageToClipboard(HWND owner, const core::Image& image) {
    if (image.empty()) return false;

    const std::size_t rowBytes = static_cast<std::size_t>(image.width) * 4;
    const std::size_t pixelBytes = rowBytes * static_cast<std::size_t>(image.height);
    GlobalBlock block{GlobalAlloc(GMEM_MOVEABLE, sizeof(BITMAPV5HEADER) + pixelBytes)};
    if (!block.handle) return false;

    auto* header = static_cast<BITMAPV5HEADER*>(GlobalLock(block.handle));
    if (!header) return false;
    *header = {};
    header->bV5Size = sizeof(BITMAPV5HEADER);
    header->bV5Width = image.width;
    header->bV5Height = image.height; // bottom-up: the layout every reader understands
    header->bV5Planes = 1;
    header->bV5BitCount = 32;
    header->bV5Compression = BI_BITFIELDS;
    header->bV5SizeImage = static_cast<DWORD>(pixelBytes);
    header->bV5RedMask = 0x00FF0000;
    header->bV5GreenMask = 0x0000FF00;
    header->bV5BlueMask = 0x000000FF;
    header->bV5AlphaMask = 0xFF000000;
    header->bV5CSType = LCS_sRGB;
    header->bV5Intent = LCS_GM_IMAGES;

    auto* pixels = reinterpret_cast<std::uint8_t*>(header + 1);
    for (int row = 0; row < image.height; ++row) {
        const std::size_t srcRow = static_cast<std::size_t>(image.height - 1 - row);
        std::memcpy(pixels + static_cast<std::size_t>(row) * rowBytes, image.bgra.data() + srcRow * rowBytes,
                    rowBytes);
    }
    GlobalUnlock(block.handle);

    if (!openClipboard(owner)) return false;
    bool copied = false;
    if (EmptyClipboard() && SetClipboardData(CF_DIBV5, block.handle)) {
        block.handle = nullptr; // the clipboard owns it now
        copied = true;
    }
    CloseClipboard();
    return copied;
}

} // namespace kodak::win
