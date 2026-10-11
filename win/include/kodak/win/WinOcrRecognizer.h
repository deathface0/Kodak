#pragma once

#include <kodak/core/TextRecognizer.h>

#include <memory>
#include <string>

namespace kodak::win {

/// Reads text with Windows.Media.Ocr, in the first language of the user's
/// profile that has OCR installed (or any installed one).
///
/// The region is enlarged 2x before recognition (screen text is small) and
/// read in overlapping tiles, so large selections stay within the OCR's size
/// limit and a bounded amount of memory.
///
/// recognize() blocks until the OCR finishes: call it from a worker thread in
/// the multi-threaded apartment, never from a UI (single-threaded) thread.
class WinOcrRecognizer final : public core::TextRecognizer {
public:
    /// `maxTileSide`: the largest tile, in image pixels before enlarging; 0 picks
    /// a default within the OCR's limit. Tests pass a small value to force tiling.
    explicit WinOcrRecognizer(int maxTileSide = 0);
    ~WinOcrRecognizer() override;

    WinOcrRecognizer(const WinOcrRecognizer&) = delete;
    WinOcrRecognizer& operator=(const WinOcrRecognizer&) = delete;

    /// False when no OCR language is installed: recognize() then returns nothing.
    bool available() const noexcept;
    /// The BCP-47 tag of the OCR language in use ("en-US"), empty if unavailable.
    std::wstring languageTag() const;

    std::vector<core::OcrLine> recognize(const core::Image& image, const core::Rect& region,
                                         std::stop_token stop) override;

private:
    struct Engine; // keeps C++/WinRT out of this header
    std::unique_ptr<Engine> engine_;
    int maxTileSide_;
};

} // namespace kodak::win
