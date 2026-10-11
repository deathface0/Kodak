#pragma once

#include "kodak/core/Geometry.h"
#include "kodak/core/Image.h"

#include <stop_token>
#include <string>
#include <vector>

namespace kodak::core {

/// A recognised word and where it is, in image coordinates.
struct OcrWord {
    std::u32string text;
    Rect box;
};

/// The words of one line of text, left to right.
struct OcrLine {
    std::vector<OcrWord> words;
};

/// Reads the text in part of an image (Windows.Media.Ocr in kodak_win).
class TextRecognizer {
public:
    virtual ~TextRecognizer() = default;

    /// The lines of text inside `region`. Returns early, possibly with fewer
    /// lines, once `stop` is requested.
    virtual std::vector<OcrLine> recognize(const Image& image, const Rect& region, std::stop_token stop) = 0;
};

} // namespace kodak::core
