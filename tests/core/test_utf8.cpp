#include "TextTestSupport.h"

#include <kodak/core/Utf8.h>

#include <string>

using kodak::core::fromUtf8;
using kodak::core::toUtf8;

namespace {

const char32_t replacement = 0xFFFD;

} // namespace

TEST_CASE("UTF-8 round trip of ASCII, accents and emoji") {
    CHECK(toUtf8(U"Kodak") == "Kodak");
    CHECK(toUtf8(U"Jos\u00E9") == "Jos\xC3\xA9");
    CHECK(toUtf8(U"\U0001F600") == "\xF0\x9F\x98\x80");
    for (const std::u32string& text :
         {std::u32string(U"Kodak 2.0"), std::u32string(U"Calle \u00D1\u00FA\u00F1ez"),
          std::u32string(U"\u4E2D\u6587 \U0001F600")})
        CHECK(fromUtf8(toUtf8(text)) == text);
}

TEST_CASE("invalid UTF-8 becomes the replacement character") {
    CHECK(fromUtf8("\xFF") == std::u32string(1, replacement));
    CHECK(fromUtf8("a\xC0\xAF"
                   "b") == std::u32string{U'a', replacement, replacement, U'b'}); // overlong
    CHECK(fromUtf8("\xE2\x82") == std::u32string(2, replacement));                // truncated
    CHECK(fromUtf8("\xED\xA0\x80") == std::u32string(3, replacement));            // a surrogate
}

TEST_CASE("code points that UTF-8 cannot carry are replaced") {
    CHECK(toUtf8(std::u32string(1, static_cast<char32_t>(0xD800))) == "\xEF\xBF\xBD");
    CHECK(toUtf8(std::u32string(1, static_cast<char32_t>(0x110000))) == "\xEF\xBF\xBD");
}
