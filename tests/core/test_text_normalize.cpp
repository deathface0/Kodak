#include "TextTestSupport.h"

#include <kodak/core/TextNormalize.h>

using kodak::core::foldForSearch;
using kodak::core::normaliseDigits;

TEST_CASE("OCR slips next to digits become digits") {
    CHECK(normaliseDigits(U"4111 l111 O000") == U"4111 1111 0000");
    CHECK(normaliseDigits(U"12|4") == U"1214");
    CHECK(normaliseDigits(U"lO11") == U"1011"); // a slip next to a slip next to a digit
}

TEST_CASE("letters away from digits are left alone") {
    CHECK(normaliseDigits(U"IT60 X054") == U"IT60 X054");
    CHECK(normaliseDigits(U"Hello World") == U"Hello World");
    CHECK(normaliseDigits(U"Room 12") == U"Room 12");
}

TEST_CASE("folding lowers case and drops accents, keeping the length") {
    const std::u32string text =
        U"Contrase\u00F1a \u00C1\u00C9\u00CD\u00D3\u00DA \u00FC \u00C7 \u0141\u00F3d\u017A \u00DF";
    const std::u32string folded = foldForSearch(text);
    CHECK(folded == U"contrasena aeiou u c lodz s");
    CHECK(folded.size() == text.size());
}

TEST_CASE("folding keeps symbols and other scripts") {
    CHECK(foldForSearch(U"2\u00D73 = 6") == U"2\u00D73 = 6");
    CHECK(foldForSearch(U"\u0416\u4E2D") == U"\u0416\u4E2D");
}
