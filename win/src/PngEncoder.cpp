#include "kodak/win/PngEncoder.h"

#include "detail/Wic.h"

namespace kodak::win {

using detail::ComPtr;

std::vector<std::uint8_t> encodePng(const core::Image& image) {
    if (image.empty()) return {};

    const ComPtr<IWICImagingFactory> factory = detail::wicFactory();
    if (!factory) return {};
    const ComPtr<IWICBitmap> source = detail::toWicBitmap(factory.Get(), image);
    if (!source) return {};

    ComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream))) return {};

    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    // The image is opaque, so 24 bits per pixel; WriteSource converts from BGRA.
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
        FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) ||
        FAILED(encoder->CreateNewFrame(&frame, nullptr)) || FAILED(frame->Initialize(nullptr)) ||
        FAILED(frame->SetSize(static_cast<UINT>(image.width), static_cast<UINT>(image.height))) ||
        FAILED(frame->SetPixelFormat(&format)) || FAILED(frame->WriteSource(source.Get(), nullptr)) ||
        FAILED(frame->Commit()) || FAILED(encoder->Commit()))
        return {};

    STATSTG stat{};
    if (FAILED(stream->Stat(&stat, STATFLAG_NONAME))) return {};
    std::vector<std::uint8_t> png(static_cast<std::size_t>(stat.cbSize.QuadPart));

    const LARGE_INTEGER start{};
    ULONG read = 0;
    if (FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr)) ||
        FAILED(stream->Read(png.data(), static_cast<ULONG>(png.size()), &read)) || read != png.size())
        return {};
    return png;
}

} // namespace kodak::win
