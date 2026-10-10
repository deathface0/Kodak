#include <doctest/doctest.h>

#include <kodak/core/Redaction.h>

#include <cstddef>
#include <cstdint>
#include <vector>

using kodak::core::Finding;
using kodak::core::FindingKind;
using kodak::core::Image;
using kodak::core::Point;
using kodak::core::Rect;
using kodak::core::redact;

namespace {

// Every pixel a distinct, non-black opaque colour.
Image colourful(int width, int height) {
    Image image(width, height);
    for (std::size_t i = 0; i < image.bgra.size(); i += 4) {
        image.bgra[i + 0] = static_cast<std::uint8_t>(10 + i % 200);
        image.bgra[i + 1] = static_cast<std::uint8_t>(20 + i % 150);
        image.bgra[i + 2] = static_cast<std::uint8_t>(30 + i % 100);
        image.bgra[i + 3] = 255;
    }
    return image;
}

bool isRedacted(const Image& image, int x, int y) {
    const std::size_t i = (static_cast<std::size_t>(y) * image.width + x) * 4;
    return image.bgra[i] == 0 && image.bgra[i + 1] == 0 && image.bgra[i + 2] == 0 && image.bgra[i + 3] == 255;
}

bool samePixel(const Image& a, const Image& b, int x, int y) {
    const std::size_t i = (static_cast<std::size_t>(y) * a.width + x) * 4;
    for (std::size_t c = 0; c < 4; ++c) {
        if (a.bgra[i + c] != b.bgra[i + c]) return false;
    }
    return true;
}

/// Checks the whole image: black inside any of `areas`, untouched elsewhere.
void checkOnly(const Image& before, const Image& after, const std::vector<Rect>& areas) {
    for (int y = 0; y < after.height; ++y) {
        for (int x = 0; x < after.width; ++x) {
            bool inside = false;
            for (const Rect& area : areas) inside = inside || area.contains(Point{x, y});
            INFO("pixel ", x, ",", y);
            if (inside) {
                CHECK(isRedacted(after, x, y));
            } else {
                CHECK(samePixel(before, after, x, y));
            }
        }
    }
}

} // namespace

TEST_CASE("redact paints exactly the area black and leaves every other pixel alone") {
    const Image before = colourful(12, 9);
    Image after = before;
    const std::vector<Finding> findings{{Rect{3, 2, 5, 4}, FindingKind::Manual}};

    redact(after, findings);
    checkOnly(before, after, {Rect{3, 2, 5, 4}});
}

TEST_CASE("areas partly outside the image are clipped") {
    const Image before = colourful(10, 6);
    Image after = before;
    const std::vector<Finding> findings{{Rect{-4, -2, 7, 5}}, {Rect{8, 4, 10, 10}}};

    redact(after, findings);
    checkOnly(before, after, {Rect{0, 0, 3, 3}, Rect{8, 4, 2, 2}});
}

TEST_CASE("disabled findings are not redacted") {
    const Image before = colourful(8, 8);
    Image after = before;
    std::vector<Finding> findings{{Rect{1, 1, 3, 3}}, {Rect{5, 5, 2, 2}}};
    findings[0].enabled = false;

    redact(after, findings);
    checkOnly(before, after, {Rect{5, 5, 2, 2}});
}

TEST_CASE("overlapping boxes cover their union") {
    const Image before = colourful(10, 10);
    Image after = before;
    const std::vector<Finding> findings{{Rect{1, 1, 5, 5}}, {Rect{4, 4, 5, 5}}};

    redact(after, findings);
    checkOnly(before, after, {Rect{1, 1, 5, 5}, Rect{4, 4, 5, 5}});
}

TEST_CASE("no findings, no change") {
    const Image before = colourful(5, 5);
    Image after = before;
    redact(after, {});
    CHECK(after.bgra == before.bgra);
}
