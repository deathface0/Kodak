#include <doctest/doctest.h>

#include <kodak/win/Clipboard.h>

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using kodak::core::Image;
using kodak::win::copyImageToClipboard;

namespace {

/// A message-only window to own the clipboard (SetClipboardData needs an owner).
struct ClipboardOwner {
    HWND window = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
    ~ClipboardOwner() {
        if (window) DestroyWindow(window);
    }
};

} // namespace

TEST_CASE("an image copied to the clipboard reads back as the same CF_DIBV5 pixels") {
    // 3x2, every byte different, so a flipped or shifted row shows up.
    Image image(3, 2);
    for (std::size_t i = 0; i < image.bgra.size(); ++i) image.bgra[i] = static_cast<std::uint8_t>(i + 1);

    const ClipboardOwner owner;
    REQUIRE(owner.window != nullptr);
    if (!copyImageToClipboard(owner.window, image)) {
        MESSAGE("clipboard unavailable in this session; skipped");
        return;
    }

    REQUIRE(OpenClipboard(owner.window));
    HANDLE data = GetClipboardData(CF_DIBV5);
    REQUIRE(data != nullptr);
    const auto* header = static_cast<const BITMAPV5HEADER*>(GlobalLock(data));
    REQUIRE(header != nullptr);

    CHECK(header->bV5Width == 3);
    CHECK(header->bV5Height == 2); // bottom-up
    CHECK(header->bV5BitCount == 32);

    const auto* pixels = reinterpret_cast<const std::uint8_t*>(header + 1);
    const std::size_t rowBytes = 3 * 4;
    for (std::size_t row = 0; row < 2; ++row) {
        for (std::size_t i = 0; i < rowBytes; ++i)
            CHECK(pixels[(1 - row) * rowBytes + i] == image.bgra[row * rowBytes + i]);
    }

    GlobalUnlock(data);
    CloseClipboard();
}

TEST_CASE("PNG bytes go on the clipboard as the \"PNG\" format, unchanged") {
    const Image image(4, 4);
    const std::vector<std::uint8_t> png{0x89, 'P', 'N', 'G', 1, 2, 3};

    const ClipboardOwner owner;
    REQUIRE(owner.window != nullptr);
    if (!copyImageToClipboard(owner.window, image, png)) {
        MESSAGE("clipboard unavailable in this session; skipped");
        return;
    }

    REQUIRE(OpenClipboard(owner.window));
    CHECK(IsClipboardFormatAvailable(CF_DIBV5));
    HANDLE data = GetClipboardData(RegisterClipboardFormatW(L"PNG"));
    REQUIRE(data != nullptr);
    const auto* bytes = static_cast<const std::uint8_t*>(GlobalLock(data));
    REQUIRE(bytes != nullptr);
    REQUIRE(GlobalSize(data) >= png.size());
    CHECK(std::vector<std::uint8_t>(bytes, bytes + png.size()) == png);
    GlobalUnlock(data);
    CloseClipboard();
}

TEST_CASE("an empty image is not copied") {
    const ClipboardOwner owner;
    CHECK_FALSE(copyImageToClipboard(owner.window, Image{}));
}
