#include <doctest/doctest.h>

#include <kodak/win/GdiScreenSource.h>

#include <windows.h>

#include <cstddef>

using kodak::win::GdiScreenSource;

TEST_CASE("the capture covers the whole virtual desktop and is opaque") {
    GdiScreenSource source;
    const auto capture = source.capture();
    if (!capture) {
        MESSAGE("no desktop to capture in this session; skipped");
        return;
    }

    CHECK(capture->origin.x == GetSystemMetrics(SM_XVIRTUALSCREEN));
    CHECK(capture->origin.y == GetSystemMetrics(SM_YVIRTUALSCREEN));
    CHECK(capture->image.width == GetSystemMetrics(SM_CXVIRTUALSCREEN));
    CHECK(capture->image.height == GetSystemMetrics(SM_CYVIRTUALSCREEN));
    REQUIRE(capture->image.bgra.size() == static_cast<std::size_t>(capture->image.width) *
                                              static_cast<std::size_t>(capture->image.height) * 4);

    bool opaque = true;
    for (std::size_t i = 3; i < capture->image.bgra.size(); i += 4)
        opaque = opaque && capture->image.bgra[i] == 255;
    CHECK(opaque);
}
