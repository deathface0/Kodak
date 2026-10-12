#pragma once

#include <kodak/core/Image.h>

#include <windows.h>

#include <wincodec.h>
#include <wrl/client.h>

namespace kodak::win::detail {

using Microsoft::WRL::ComPtr;

/// The WIC factory; null if COM is not initialised on this thread.
inline ComPtr<IWICImagingFactory> wicFactory() {
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return nullptr;
    return factory;
}

/// A WIC copy of `image`'s pixels; null on failure.
inline ComPtr<IWICBitmap> toWicBitmap(IWICImagingFactory* factory, const core::Image& image) {
    ComPtr<IWICBitmap> bitmap;
    if (FAILED(factory->CreateBitmapFromMemory(
            static_cast<UINT>(image.width), static_cast<UINT>(image.height), GUID_WICPixelFormat32bppBGRA,
            static_cast<UINT>(image.stride()), static_cast<UINT>(image.bgra.size()),
            const_cast<BYTE*>(image.bgra.data()), &bitmap)))
        return nullptr;
    return bitmap;
}

} // namespace kodak::win::detail
