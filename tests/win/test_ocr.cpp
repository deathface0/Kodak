#include <doctest/doctest.h>

#include <kodak/core/TextDetector.h>
#include <kodak/win/WinOcrRecognizer.h>

#include <windows.h>

#include <winrt/base.h>

#include <cstring>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using kodak::core::Finding;
using kodak::core::FindingKind;
using kodak::core::Image;
using kodak::core::OcrLine;
using kodak::core::Rect;
using kodak::win::WinOcrRecognizer;

namespace {

struct Piece {
    std::wstring text;
    int x;
};

struct Rendered {
    Image image;
    std::vector<Rect> boxes; // where each piece was drawn
};

/// Black Segoe UI text on white, drawn with GDI, as a capture would show it.
Rendered render(int width, int height, int fontPx, const std::vector<Piece>& pieces, int y = 20) {
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    HDC dc = CreateCompatibleDC(nullptr);
    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    PatBlt(dc, 0, 0, width, height, WHITENESS);

    HFONT font =
        CreateFontW(-fontPx, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));

    Rendered rendered{Image(width, height), {}};
    for (const Piece& piece : pieces) {
        const int length = static_cast<int>(piece.text.size());
        TextOutW(dc, piece.x, y, piece.text.c_str(), length);
        SIZE extent{};
        GetTextExtentPoint32W(dc, piece.text.c_str(), length, &extent);
        rendered.boxes.push_back(Rect{piece.x, y, extent.cx, extent.cy});
    }
    GdiFlush();
    std::memcpy(rendered.image.bgra.data(), bits, rendered.image.bgra.size());
    kodak::core::makeOpaque(rendered.image);

    SelectObject(dc, oldFont);
    DeleteObject(font);
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return rendered;
}

/// Runs `work` on a thread in the multi-threaded apartment, as the app's worker will.
template <typename Work> auto onWorkerThread(Work work) {
    std::optional<decltype(work())> result;
    std::thread worker([&] {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        result = work();
        winrt::uninit_apartment();
    });
    worker.join();
    return std::move(*result);
}

std::u32string allText(const std::vector<OcrLine>& lines) {
    std::u32string text;
    for (const OcrLine& line : lines) {
        for (const auto& word : line.words) text += word.text + U" ";
        text += U"\n";
    }
    return text;
}

/// For failure messages: the recognised text, non-ASCII shown as '?'.
std::string printable(const std::u32string& text) {
    std::string out;
    for (const char32_t c : text) out += c < 0x80 ? static_cast<char>(c) : '?';
    return out;
}

int countOccurrences(const std::u32string& text, const std::u32string& part) {
    int count = 0;
    for (auto at = text.find(part); at != std::u32string::npos; at = text.find(part, at + 1)) ++count;
    return count;
}

/// The share of the dark (ink) pixels inside `text` that `area` covers, from 0 to 1:
/// 1 means nothing of the text would stay visible.
double inkCovered(const Image& image, const Rect& text, const Rect& area) {
    int ink = 0;
    int covered = 0;
    for (int y = text.y; y < text.y + text.height; ++y) {
        for (int x = text.x; x < text.x + text.width; ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * image.width + x) * 4;
            if (image.bgra[i] > 128) continue; // background
            ++ink;
            if (area.contains(kodak::core::Point{x, y})) ++covered;
        }
    }
    return ink == 0 ? 0.0 : static_cast<double>(covered) / ink;
}

/// How much of `target` the box covers, from 0 to 1.
double coverage(const Rect& box, const Rect& target) {
    const Rect common = kodak::core::intersect(box, target);
    return static_cast<double>(common.width) * common.height /
           (static_cast<double>(target.width) * target.height);
}

} // namespace

TEST_CASE("the OCR reads the words and where they are") {
    WinOcrRecognizer recognizer; // created here, used from the worker thread: the app does the same
    if (!recognizer.available()) {
        MESSAGE("no OCR language installed in this session; skipped");
        return;
    }
    CHECK_FALSE(recognizer.languageTag().empty());

    const Rendered page = render(700, 80, 24, {{L"Contact juan.perez@gmail.com today", 20}});
    const auto lines =
        onWorkerThread([&] { return recognizer.recognize(page.image, page.image.bounds(), {}); });

    const std::u32string text = allText(lines);
    CHECK(text.find(U"juan.perez@gmail.com") != std::u32string::npos);

    std::optional<Rect> email;
    for (const OcrLine& line : lines) {
        for (const auto& word : line.words) {
            if (word.text.find(U"gmail") != std::u32string::npos) email = word.box;
        }
    }
    REQUIRE(email.has_value());
    // The word lies inside the drawn line, in its middle part.
    CHECK(coverage(*email, page.boxes[0]) > 0.3);
    CHECK(email->x > page.boxes[0].x);
    CHECK(email->x + email->width < page.boxes[0].x + page.boxes[0].width);
}

TEST_CASE("with tiling, a word near a tile edge is read once and whole") {
    WinOcrRecognizer recognizer(400); // tiles of 400 px own [0,300), [300,500), [500,650), [650,900)
    if (!recognizer.available()) {
        MESSAGE("no OCR language installed in this session; skipped");
        return;
    }

    const Rendered page = render(900, 60, 16, {{L"Hello", 20}, {L"juan.perez@gmail.com", 320}});
    const auto lines =
        onWorkerThread([&] { return recognizer.recognize(page.image, page.image.bounds(), {}); });

    const std::u32string text = allText(lines);
    CHECK(countOccurrences(text, U"juan.perez@gmail.com") == 1);
    CHECK(countOccurrences(text, U"Hello") == 1);
}

TEST_CASE("a line cut by a tile edge is joined again, so a card across it is found") {
    WinOcrRecognizer recognizer(400);
    if (!recognizer.available()) {
        MESSAGE("no OCR language installed in this session; skipped");
        return;
    }

    // Not the usual 4111 1111 1111 1111: Windows OCR drops lines of repeated 1s as noise.
    const Rendered page = render(900, 60, 16, {{L"Card", 220}, {L"4532 0151 1283 0366", 260}});
    INFO("OCR read: ", printable(allText(onWorkerThread(
                           [&] { return recognizer.recognize(page.image, page.image.bounds(), {}); }))));
    const auto findings = onWorkerThread([&] {
        kodak::core::TextDetector detector(recognizer, kodak::core::defaultRules());
        return detector.detect(page.image, page.image.bounds(), {});
    });

    bool cardCovered = false;
    for (const Finding& finding : findings) {
        if (finding.kind == FindingKind::Card && inkCovered(page.image, page.boxes[1], finding.area) == 1.0)
            cardCovered = true;
    }
    CHECK(cardCovered);
}

TEST_CASE("end to end: an e-mail in a capture becomes a finding over it") {
    WinOcrRecognizer recognizer;
    if (!recognizer.available()) {
        MESSAGE("no OCR language installed in this session; skipped");
        return;
    }

    const Rendered page = render(700, 80, 20, {{L"Write to", 20}, {L"juan.perez@gmail.com", 120}});
    INFO("OCR read: ", printable(allText(onWorkerThread(
                           [&] { return recognizer.recognize(page.image, page.image.bounds(), {}); }))));
    const auto findings = onWorkerThread([&] {
        kodak::core::TextDetector detector(recognizer, kodak::core::defaultRules());
        return detector.detect(page.image, page.image.bounds(), {});
    });

    REQUIRE(findings.size() == 1);
    CHECK(findings[0].kind == FindingKind::Email);
    CHECK(inkCovered(page.image, page.boxes[1], findings[0].area) == 1.0); // nothing of it stays visible
}
