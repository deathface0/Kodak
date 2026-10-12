#include "kodak/win/WinOcrRecognizer.h"

#include "kodak/win/TextConversion.h"

#include "detail/Wic.h"

#include <kodak/core/Tiling.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Globalization.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Storage.Streams.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>

namespace kodak::win {

namespace {

using winrt::Windows::Graphics::Imaging::BitmapAlphaMode;
using winrt::Windows::Graphics::Imaging::BitmapPixelFormat;
using winrt::Windows::Graphics::Imaging::SoftwareBitmap;
using winrt::Windows::Media::Ocr::OcrEngine;

constexpr int scale = 2;
// Enlarged pixels per tile side: within OcrEngine::MaxImageDimension and at most
// 64 MB of pixels per tile, whatever the size of the selection.
constexpr int maxEnlargedTile = 4096;
// A word cut by a tile edge has to fit whole inside the neighbouring tile.
constexpr int maxOverlap = 256;

/// `image` enlarged `scale` times with high-quality cubic interpolation; empty if WIC fails.
core::Image enlarge(const core::Image& image) {
    const detail::ComPtr<IWICImagingFactory> factory = detail::wicFactory();
    if (!factory) return {};
    const detail::ComPtr<IWICBitmap> source = detail::toWicBitmap(factory.Get(), image);
    detail::ComPtr<IWICBitmapScaler> scaler;
    core::Image enlarged(image.width * scale, image.height * scale);
    if (!source || FAILED(factory->CreateBitmapScaler(&scaler)) ||
        FAILED(scaler->Initialize(source.Get(), static_cast<UINT>(enlarged.width),
                                  static_cast<UINT>(enlarged.height),
                                  WICBitmapInterpolationModeHighQualityCubic)) ||
        FAILED(scaler->CopyPixels(nullptr, static_cast<UINT>(enlarged.stride()),
                                  static_cast<UINT>(enlarged.bgra.size()), enlarged.bgra.data())))
        return {};
    return enlarged;
}

SoftwareBitmap toSoftwareBitmap(const core::Image& image) {
    const auto bytes = static_cast<std::uint32_t>(image.bgra.size());
    winrt::Windows::Storage::Streams::Buffer buffer(bytes);
    std::memcpy(buffer.data(), image.bgra.data(), bytes);
    buffer.Length(bytes);
    // Our pixels are opaque, so straight and premultiplied alpha are the same.
    return SoftwareBitmap::CreateCopyFromBuffer(buffer, BitmapPixelFormat::Bgra8, image.width, image.height,
                                                BitmapAlphaMode::Premultiplied);
}

/// A recognised line, with the tile it came from and its vertical extent.
struct TileLine {
    core::OcrLine line;
    std::size_t row = 0;
    std::size_t column = 0;
    int top = INT_MAX;
    int bottom = INT_MIN;
};

/// True when two lines sit at the same height (they overlap by half the shorter one).
bool sameHeight(const TileLine& a, const TileLine& b) noexcept {
    const int overlap = std::min(a.bottom, b.bottom) - std::max(a.top, b.top);
    return overlap * 2 >= std::min(a.bottom - a.top, b.bottom - b.top);
}

/// Joins the pieces of a line that a vertical tile edge cut in two, so a match
/// spanning the edge (a card number) is still seen as one line.
std::vector<core::OcrLine> joinCutLines(std::vector<TileLine> lines) {
    bool joined = true;
    while (joined) {
        joined = false;
        for (std::size_t i = 0; i < lines.size() && !joined; ++i) {
            for (std::size_t j = i + 1; j < lines.size(); ++j) {
                if (lines[i].row != lines[j].row || lines[i].column == lines[j].column ||
                    !sameHeight(lines[i], lines[j]))
                    continue;
                auto& words = lines[i].line.words;
                words.insert(words.end(), lines[j].line.words.begin(), lines[j].line.words.end());
                lines[i].top = std::min(lines[i].top, lines[j].top);
                lines[i].bottom = std::max(lines[i].bottom, lines[j].bottom);
                lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(j));
                joined = true;
                break;
            }
        }
    }

    std::vector<core::OcrLine> result;
    for (TileLine& line : lines) {
        std::sort(line.line.words.begin(), line.line.words.end(),
                  [](const core::OcrWord& a, const core::OcrWord& b) { return a.box.x < b.box.x; });
        result.push_back(std::move(line.line));
    }
    return result;
}

} // namespace

struct WinOcrRecognizer::Engine {
    OcrEngine ocr{nullptr};
};

WinOcrRecognizer::WinOcrRecognizer(int maxTileSide) : engine_(std::make_unique<Engine>()) {
    int limit = maxEnlargedTile / scale;
    try {
        engine_->ocr = OcrEngine::TryCreateFromUserProfileLanguages();
        if (!engine_->ocr) {
            // No profile language has OCR installed: use any installed one.
            const auto languages = OcrEngine::AvailableRecognizerLanguages();
            if (languages.Size() > 0) engine_->ocr = OcrEngine::TryCreateFromLanguage(languages.GetAt(0));
        }
        limit = static_cast<int>(std::min<std::uint32_t>(OcrEngine::MaxImageDimension(), maxEnlargedTile)) /
                scale;
    } catch (const winrt::hresult_error&) {
        engine_->ocr = nullptr;
    }
    maxTileSide_ = maxTileSide > 0 ? std::min(maxTileSide, limit) : limit;
}

WinOcrRecognizer::~WinOcrRecognizer() = default;

bool WinOcrRecognizer::available() const noexcept { return engine_->ocr != nullptr; }

std::wstring WinOcrRecognizer::languageTag() const {
    if (!available()) return {};
    try {
        return std::wstring(engine_->ocr.RecognizerLanguage().LanguageTag());
    } catch (const winrt::hresult_error&) {
        return {};
    }
}

std::vector<core::OcrLine> WinOcrRecognizer::recognize(const core::Image& image, const core::Rect& region,
                                                       std::stop_token stop) {
    const core::Rect area = core::intersect(region, image.bounds());
    if (!available() || area.empty()) return {};

    const int overlap = std::min(maxOverlap, maxTileSide_ / 2);
    const std::vector<core::Tile> columns = core::splitIntoTiles(area.width, maxTileSide_, overlap);
    const std::vector<core::Tile> rows = core::splitIntoTiles(area.height, maxTileSide_, overlap);

    std::vector<TileLine> collected;
    try {
        for (std::size_t r = 0; r < rows.size(); ++r) {
            for (std::size_t c = 0; c < columns.size(); ++c) {
                if (stop.stop_requested()) return joinCutLines(std::move(collected));
                const core::Tile& tx = columns[c];
                const core::Tile& ty = rows[r];
                const core::Rect tile{area.x + tx.begin, area.y + ty.begin, tx.end - tx.begin,
                                      ty.end - ty.begin};

                const core::Image enlarged = enlarge(core::crop(image, tile));
                if (enlarged.empty()) continue;
                const auto result = engine_->ocr.RecognizeAsync(toSoftwareBitmap(enlarged)).get();

                for (const auto& line : result.Lines()) {
                    TileLine piece{{}, r, c};
                    for (const auto& word : line.Words()) {
                        // Back to image pixels, rounding outwards so the box covers the whole word.
                        const auto box = word.BoundingRect();
                        const int left = tile.x + static_cast<int>(std::floor(box.X / scale));
                        const int top = tile.y + static_cast<int>(std::floor(box.Y / scale));
                        const int right = tile.x + static_cast<int>(std::ceil((box.X + box.Width) / scale));
                        const int bottom = tile.y + static_cast<int>(std::ceil((box.Y + box.Height) / scale));

                        // Keep a word only in the tile that owns its centre: read once, never cut.
                        const int centreX = (left + right) / 2 - area.x;
                        const int centreY = (top + bottom) / 2 - area.y;
                        if (centreX < tx.ownBegin || centreX >= tx.ownEnd || centreY < ty.ownBegin ||
                            centreY >= ty.ownEnd)
                            continue;

                        piece.line.words.push_back(
                            {toUtf32(word.Text()), core::Rect{left, top, right - left, bottom - top}});
                        piece.top = std::min(piece.top, top);
                        piece.bottom = std::max(piece.bottom, bottom);
                    }
                    if (!piece.line.words.empty()) collected.push_back(std::move(piece));
                }
            }
        }
    } catch (const winrt::hresult_error&) {
        // Keep what was read before the failure.
    }
    return joinCutLines(std::move(collected));
}

} // namespace kodak::win
