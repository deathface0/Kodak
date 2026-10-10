#include <doctest/doctest.h>

#include <kodak/core/Image.h>

#include <cstddef>
#include <cstdint>

using kodak::core::crop;
using kodak::core::Image;
using kodak::core::Rect;

namespace {

// Every pixel encodes its own position: B = x, G = y, R = 0, A = 255.
Image gradient(int width, int height) {
    Image image(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * width + x) * 4;
            image.bgra[i + 0] = static_cast<std::uint8_t>(x);
            image.bgra[i + 1] = static_cast<std::uint8_t>(y);
            image.bgra[i + 2] = 0;
            image.bgra[i + 3] = 255;
        }
    }
    return image;
}

std::uint8_t blue(const Image& image, int x, int y) {
    return image.bgra[(static_cast<std::size_t>(y) * image.width + x) * 4];
}
std::uint8_t green(const Image& image, int x, int y) {
    return image.bgra[(static_cast<std::size_t>(y) * image.width + x) * 4 + 1];
}

} // namespace

TEST_CASE("a new image is zero-filled with 4 bytes per pixel") {
    const Image image(3, 2);
    CHECK(image.bgra.size() == 3 * 2 * 4);
    for (const auto byte : image.bgra) CHECK(byte == 0);
}

TEST_CASE("crop copies exactly the requested pixels") {
    const Image image = gradient(10, 8);
    const Image part = crop(image, {2, 3, 4, 2});

    REQUIRE(part.width == 4);
    REQUIRE(part.height == 2);
    for (int y = 0; y < part.height; ++y) {
        for (int x = 0; x < part.width; ++x) {
            CHECK(blue(part, x, y) == 2 + x);
            CHECK(green(part, x, y) == 3 + y);
        }
    }
}

TEST_CASE("crop clips the area to the image") {
    const Image part = crop(gradient(10, 8), {-3, 6, 5, 10});
    CHECK(part.width == 2);
    CHECK(part.height == 2);
    CHECK(blue(part, 0, 0) == 0);
    CHECK(green(part, 0, 0) == 6);
}

TEST_CASE("crop outside the image or with no size is empty") {
    const Image image = gradient(10, 8);
    CHECK(crop(image, {20, 20, 5, 5}).empty());
    CHECK(crop(image, {2, 2, 0, 5}).empty());
}
