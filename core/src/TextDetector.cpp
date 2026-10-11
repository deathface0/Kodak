#include "kodak/core/TextDetector.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>

namespace kodak::core {

namespace {

/// A line's words joined by single spaces, and which word each character belongs to.
struct JoinedLine {
    std::u32string text;
    std::vector<std::optional<std::size_t>> wordOf; // none for the separators
};

JoinedLine join(const OcrLine& line) {
    JoinedLine joined;
    for (std::size_t w = 0; w < line.words.size(); ++w) {
        if (w > 0) {
            joined.text += U' ';
            joined.wordOf.emplace_back();
        }
        joined.text += line.words[w].text;
        joined.wordOf.insert(joined.wordOf.end(), line.words[w].text.size(), w);
    }
    return joined;
}

} // namespace

TextDetector::TextDetector(TextRecognizer& recognizer, std::vector<std::unique_ptr<TextRule>> rules)
    : recognizer_(recognizer), rules_(std::move(rules)) {}

int TextDetector::marginFor(int height) noexcept { return std::max(2, height / 5); }

std::vector<Finding> TextDetector::detect(const Image& image, const Rect& region, std::stop_token stop) {
    if (stop.stop_requested()) return {};
    const std::vector<OcrLine> lines = recognizer_.recognize(image, region, stop);

    std::vector<Finding> findings;
    for (const OcrLine& line : lines) {
        if (stop.stop_requested()) return {};
        const JoinedLine joined = join(line);
        for (const auto& rule : rules_) {
            for (const TextMatch& match : rule->find(joined.text)) {
                // Every word the match touches, even partly: better too much than a visible piece.
                Rect covered;
                for (std::size_t c = match.begin; c < match.end && c < joined.wordOf.size(); ++c) {
                    if (joined.wordOf[c]) covered = unite(covered, line.words[*joined.wordOf[c]].box);
                }
                const Rect area = intersect(inflate(covered, marginFor(covered.height)), region);
                if (!covered.empty() && !area.empty()) findings.push_back({area, match.kind});
            }
        }
    }
    return mergeOverlapping(std::move(findings));
}

} // namespace kodak::core
