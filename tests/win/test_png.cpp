#include <doctest/doctest.h>

#include <kodak/win/PngEncoder.h>

#include <windows.h>

#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using kodak::core::Image;
using kodak::win::encodePng;
using Microsoft::WRL::ComPtr;

namespace {

// Every pixel different: B = x * 40, G = y * 60, R = x + y, opaque.
Image testImage() {
    Image image(5, 3);
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * image.width + x) * 4;
            image.bgra[i + 0] = static_cast<std::uint8_t>(x * 40);
            image.bgra[i + 1] = static_cast<std::uint8_t>(y * 60);
            image.bgra[i + 2] = static_cast<std::uint8_t>(x + y);
            image.bgra[i + 3] = 255;
        }
    }
    return image;
}

std::uint32_t readBigEndian(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    return (std::uint32_t{bytes[at]} << 24) | (std::uint32_t{bytes[at + 1]} << 16) |
           (std::uint32_t{bytes[at + 2]} << 8) | std::uint32_t{bytes[at + 3]};
}

struct Chunks {
    std::vector<std::string> types;
    std::size_t endOfIend = 0; // offset just past the IEND chunk, 0 if there is none
};

Chunks readChunks(const std::vector<std::uint8_t>& png) {
    Chunks chunks;
    std::size_t at = 8; // after the signature
    while (at + 12 <= png.size()) {
        const std::uint32_t length = readBigEndian(png, at);
        const std::string type(png.begin() + static_cast<std::ptrdiff_t>(at + 4),
                               png.begin() + static_cast<std::ptrdiff_t>(at + 8));
        chunks.types.push_back(type);
        at += 12 + length; // length + type + data + CRC
        if (type == "IEND") {
            chunks.endOfIend = at;
            break;
        }
    }
    return chunks;
}

/// Decodes a PNG with WIC into 32bppBGRA.
Image decode(const std::vector<std::uint8_t>& png) {
    ComPtr<IWICImagingFactory> factory;
    REQUIRE(SUCCEEDED(
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))));
    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(png.data(), static_cast<UINT>(png.size())));
    REQUIRE(stream);

    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    REQUIRE(SUCCEEDED(
        factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)));
    REQUIRE(SUCCEEDED(decoder->GetFrame(0, &frame)));
    REQUIRE(SUCCEEDED(factory->CreateFormatConverter(&converter)));
    REQUIRE(
        SUCCEEDED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                                        nullptr, 0.0, WICBitmapPaletteTypeCustom)));

    UINT width = 0;
    UINT height = 0;
    REQUIRE(SUCCEEDED(converter->GetSize(&width, &height)));
    Image image(static_cast<int>(width), static_cast<int>(height));
    REQUIRE(SUCCEEDED(
        converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(image.bgra.size()), image.bgra.data())));
    return image;
}

} // namespace

TEST_CASE("the PNG holds only image and colour chunks and nothing after IEND") {
    const auto png = encodePng(testImage());
    REQUIRE(png);

    const std::vector<std::uint8_t> signature{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    REQUIRE(png->size() > signature.size());
    CHECK(std::vector<std::uint8_t>(png->begin(), png->begin() + 8) == signature);

    const Chunks chunks = readChunks(*png);
    REQUIRE_FALSE(chunks.types.empty());
    CHECK(chunks.types.front() == "IHDR");
    CHECK(chunks.types.back() == "IEND");
    // sRGB and gAMA only say how to read the colours; text, time and EXIF chunks
    // (tEXt, zTXt, iTXt, tIME, eXIf) could carry data about the user and must not appear.
    for (const std::string& type : chunks.types) {
        INFO("chunk ", type);
        CHECK((type == "IHDR" || type == "sRGB" || type == "gAMA" || type == "IDAT" || type == "IEND"));
    }
    CHECK(chunks.endOfIend == png->size());
}

TEST_CASE("the PNG decodes back to the same pixels") {
    const Image original = testImage();
    const auto png = encodePng(original);
    REQUIRE(png);

    const Image decoded = decode(*png);
    REQUIRE(decoded.width == original.width);
    REQUIRE(decoded.height == original.height);
    CHECK(decoded.bgra == original.bgra);
}

TEST_CASE("an empty image is not encoded") { CHECK_FALSE(encodePng(Image{})); }
