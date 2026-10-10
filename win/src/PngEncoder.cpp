#include "kodak/win/PngEncoder.h"

#include <windows.h>

#include <wincodec.h>
#include <wrl/client.h>

namespace kodak::win {

using Microsoft::WRL::ComPtr;

std::optional<std::vector<std::uint8_t>> encodePng(const core::Image& image) {
    if (image.empty()) return std::nullopt;

    ComPtr<IWICImagingFactory> factory;
    if (FAILED(
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return std::nullopt;

    const auto width = static_cast<UINT>(image.width);
    const auto height = static_cast<UINT>(image.height);
    ComPtr<IWICBitmap> source;
    if (FAILED(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4,
                                               static_cast<UINT>(image.bgra.size()),
                                               const_cast<BYTE*>(image.bgra.data()), &source)))
        return std::nullopt;

    ComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream))) return std::nullopt;

    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    // The image is opaque, so 24 bits per pixel; WriteSource converts from BGRA.
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
        FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)) ||
        FAILED(encoder->CreateNewFrame(&frame, nullptr)) || FAILED(frame->Initialize(nullptr)) ||
        FAILED(frame->SetSize(width, height)) || FAILED(frame->SetPixelFormat(&format)) ||
        FAILED(frame->WriteSource(source.Get(), nullptr)) || FAILED(frame->Commit()) ||
        FAILED(encoder->Commit()))
        return std::nullopt;

    STATSTG stat{};
    if (FAILED(stream->Stat(&stat, STATFLAG_NONAME))) return std::nullopt;
    std::vector<std::uint8_t> png(static_cast<std::size_t>(stat.cbSize.QuadPart));

    const LARGE_INTEGER start{};
    ULONG read = 0;
    if (FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr)) ||
        FAILED(stream->Read(png.data(), static_cast<ULONG>(png.size()), &read)) || read != png.size())
        return std::nullopt;
    return png;
}

} // namespace kodak::win
