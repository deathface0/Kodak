#pragma once

#include "kodak/core/Detector.h"
#include "kodak/core/TextRecognizer.h"
#include "kodak/core/TextRule.h"

#include <vector>

namespace kodak::core {

/// Proposes redactions for sensitive text: recognises the words, runs the
/// rules on each line and covers every word a match touches, with a margin.
class TextDetector final : public Detector {
public:
    TextDetector(TextRecognizer& recognizer, std::vector<TextRule> rules);

    std::vector<Finding> detect(const Image& image, const Rect& region, std::stop_token stop) override;

    /// The margin added around a box `height` pixels tall: it grows with the text.
    static int marginFor(int height) noexcept;

private:
    TextRecognizer& recognizer_;
    std::vector<TextRule> rules_;
};

} // namespace kodak::core
