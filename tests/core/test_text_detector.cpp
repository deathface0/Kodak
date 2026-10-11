#include <doctest/doctest.h>

#include <kodak/core/TextDetector.h>

#include <stop_token>
#include <string>
#include <vector>

using namespace kodak::core;

namespace {

class FakeRecognizer final : public TextRecognizer {
public:
    std::vector<OcrLine> lines;
    int calls = 0;

    std::vector<OcrLine> recognize(const Image&, const Rect&, std::stop_token) override {
        ++calls;
        return lines;
    }
};

/// Words laid out left to right from `x`: 10 px per character, 10 px between words.
OcrLine lineOf(const std::vector<std::u32string>& words, int x = 0, int y = 100, int height = 20) {
    OcrLine line;
    for (const std::u32string& word : words) {
        const int width = 10 * static_cast<int>(word.size());
        line.words.push_back({word, Rect{x, y, width, height}});
        x += width + 10;
    }
    return line;
}

constexpr Rect everything{0, 0, 1000, 1000};

std::vector<Finding> detect(FakeRecognizer& recognizer, const Rect& region = everything,
                            std::stop_token stop = {}) {
    static const Image image(1000, 1000); // the fake recognizer never looks at it
    TextDetector detector(recognizer, defaultRules());
    return detector.detect(image, region, stop);
}

} // namespace

TEST_CASE("the margin grows with the text height") {
    CHECK(TextDetector::marginFor(8) == 2);
    CHECK(TextDetector::marginFor(20) == 4);
    CHECK(TextDetector::marginFor(60) == 12);
}

TEST_CASE("an e-mail word becomes its box plus the margin") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"Contact", U"juan@mail.com", U"today"})};

    const auto findings = detect(ocr);
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].kind == FindingKind::Email);
    CHECK(findings[0].area == Rect{76, 96, 138, 28}); // word {80, 100, 130, 20}, margin 4
    CHECK(findings[0].enabled);
}

TEST_CASE("a card split into four OCR words gets one box over all of them") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"Card", U"4111", U"1111", U"1111", U"1111"})};

    const auto findings = detect(ocr);
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].kind == FindingKind::Card);
    CHECK(findings[0].area == Rect{46, 96, 198, 28}); // words from x 50 to 240
}

TEST_CASE("a match inside a longer OCR word covers the whole word") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"user:juan@mail.com"}, 100)};

    const auto findings = detect(ocr);
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].area == Rect{96, 96, 188, 28});
}

TEST_CASE("a labelled value covers the value words, not the label") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"Password:", U"hunter2", U"extra"})};

    const auto findings = detect(ocr);
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].kind == FindingKind::ContextValue);
    CHECK(findings[0].area == Rect{96, 96, 138, 28}); // "hunter2 extra" from x 100 to 230
}

TEST_CASE("nothing sensitive, no findings") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"Hello", U"world"}), lineOf({U"Nothing", U"to", U"see"}, 0, 200)};
    CHECK(detect(ocr).empty());
}

TEST_CASE("each line is searched on its own") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"a@mail.com"}, 0, 100), lineOf({U"b@mail.com"}, 0, 300)};
    CHECK(detect(ocr).size() == 2);
}

TEST_CASE("findings are clipped to the selected region") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"Contact", U"juan@mail.com"})};

    const auto findings = detect(ocr, Rect{0, 0, 1000, 110});
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].area == Rect{76, 96, 138, 14});
}

TEST_CASE("text matched by several rules gives a single finding") {
    FakeRecognizer ocr;
    // A JWT: a secret format, and its parts also look random to the entropy rule.
    ocr.lines = {
        lineOf({U"Bearer",
                U"eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjM0In0.dozjgNryP4J3jVmNHl0w5N_XgL0n3I9PlFUP0THsR8U"})};

    const auto findings = detect(ocr);
    REQUIRE(findings.size() == 1);
    CHECK(findings[0].kind == FindingKind::Secret);
}

TEST_CASE("a stop requested before detecting skips the OCR") {
    FakeRecognizer ocr;
    ocr.lines = {lineOf({U"juan@mail.com"})};
    std::stop_source source;
    source.request_stop();

    CHECK(detect(ocr, everything, source.get_token()).empty());
    CHECK(ocr.calls == 0);
}
