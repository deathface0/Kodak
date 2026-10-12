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

TextDetector::TextDetector(TextRecognizer& recognizer, std::vector<TextRule> rules)
    : recognizer_(recognizer), rules_(std::move(rules)) {}

int TextDetector::marginFor(int height) noexcept { return std::max(2, height / 5); }

std::vector<Finding> TextDetector::detect(const Image& image, const Rect& region, std::stop_token stop) {
    if (stop.stop_requested()) return {};
    const std::vector<OcrLine> lines = recognizer_.recognize(image, region, stop);

    std::vector<Finding> findings;
    auto propose = [&](const Rect& words, const TextMatch& match, std::u32string_view text) {
        const Rect area = intersect(inflate(words, marginFor(words.height)), region);
        if (!words.empty() && !area.empty())
            findings.push_back(
                {area, match.kind, true, std::u32string(text.substr(match.begin, match.end - match.begin))});
    };

    for (const OcrLine& line : lines) {
        if (stop.stop_requested()) return {};
        const JoinedLine joined = join(line);
        std::vector<bool> matched(line.words.size(), false);
        for (const auto& rule : rules_) {
            for (const TextMatch& match : rule(joined.text)) {
                // Every word the match touches, even partly: better too much than a visible piece.
                Rect covered;
                for (std::size_t c = match.begin; c < match.end && c < joined.wordOf.size(); ++c) {
                    if (!joined.wordOf[c]) continue;
                    covered = unite(covered, line.words[*joined.wordOf[c]].box);
                    matched[*joined.wordOf[c]] = true;
                }
                propose(covered, match, joined.text);
            }
        }

        // The OCR sometimes splits one token in two ("juan.perez@gm" "ail.com"). Try each pair
        // of close neighbours that nothing matched, joined without the space; a match has to
        // cross the join, so a word next to correctly read data is never swallowed.
        for (std::size_t w = 0; w + 1 < line.words.size(); ++w) {
            const OcrWord& left = line.words[w];
            const OcrWord& right = line.words[w + 1];
            const int gap = right.box.x - (left.box.x + left.box.width);
            if (matched[w] || matched[w + 1] || gap > std::max(left.box.height, right.box.height) / 2)
                continue;
            const std::u32string pair = left.text + right.text;
            for (const auto& rule : rules_) {
                for (const TextMatch& match : rule(pair)) {
                    if (match.begin < left.text.size() && match.end > left.text.size())
                        propose(unite(left.box, right.box), match, pair);
                }
            }
        }
    }
    return mergeOverlapping(std::move(findings));
}

} // namespace kodak::core
