#include "kodak/win/Clipboard.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

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

/// A movable memory block, freed unless ownership passes to the clipboard (release()).
struct GlobalFreer {
    void operator()(HGLOBAL block) const noexcept { GlobalFree(block); }
};
using GlobalBlock = std::unique_ptr<void, GlobalFreer>;

/// A block of `size` bytes filled by `fill(bytes)`; null if memory runs out.
template <typename Fill> GlobalBlock globalBlock(std::size_t size, Fill fill) {
    GlobalBlock block(GlobalAlloc(GMEM_MOVEABLE, size));
    auto* bytes = block ? static_cast<std::uint8_t*>(GlobalLock(block.get())) : nullptr;
    if (!bytes) return nullptr;
    fill(bytes);
    GlobalUnlock(block.get());
    return block;
}

/// A BITMAPV5HEADER followed by the pixels, bottom-up: the layout every reader understands.
GlobalBlock dibV5(const core::Image& image) {
    const std::size_t rowBytes = image.stride();
    const std::size_t pixelBytes = rowBytes * static_cast<std::size_t>(image.height);
    return globalBlock(sizeof(BITMAPV5HEADER) + pixelBytes, [&](std::uint8_t* bytes) {
        BITMAPV5HEADER header{};
        header.bV5Size = sizeof(BITMAPV5HEADER);
        header.bV5Width = image.width;
        header.bV5Height = image.height;
        header.bV5Planes = 1;
        header.bV5BitCount = 32;
        header.bV5Compression = BI_BITFIELDS;
        header.bV5SizeImage = static_cast<DWORD>(pixelBytes);
        header.bV5RedMask = 0x00FF0000;
        header.bV5GreenMask = 0x0000FF00;
        header.bV5BlueMask = 0x000000FF;
        header.bV5AlphaMask = 0xFF000000;
        header.bV5CSType = LCS_sRGB;
        header.bV5Intent = LCS_GM_IMAGES;
        std::memcpy(bytes, &header, sizeof(header));

        std::uint8_t* pixels = bytes + sizeof(header);
        for (int row = 0; row < image.height; ++row) {
            const std::size_t srcRow = static_cast<std::size_t>(image.height - 1 - row);
            std::memcpy(pixels + static_cast<std::size_t>(row) * rowBytes,
                        image.bgra.data() + srcRow * rowBytes, rowBytes);
        }
    });
}

} // namespace

bool copyImageToClipboard(HWND owner, const core::Image& image, std::span<const std::uint8_t> png) {
    if (image.empty()) return false;

    GlobalBlock dib = dibV5(image);
    if (!dib) return false;
    const UINT pngFormat = png.empty() ? 0 : RegisterClipboardFormatW(L"PNG");
    GlobalBlock pngBlock;
    if (pngFormat) {
        pngBlock =
            globalBlock(png.size(), [&](std::uint8_t* bytes) { std::memcpy(bytes, png.data(), png.size()); });
        if (!pngBlock) return false;
    }

    if (!openClipboard(owner)) return false;
    bool copied = false;
    if (EmptyClipboard() && SetClipboardData(CF_DIBV5, dib.get())) {
        dib.release(); // the clipboard owns it now
        copied = true;
        if (pngFormat && SetClipboardData(pngFormat, pngBlock.get())) pngBlock.release();
    }
    CloseClipboard();
    return copied;
}

} // namespace kodak::win
