#include <doctest/doctest.h>

#include <kodak/win/TextConversion.h>

#include <string>

using kodak::win::toUtf32;

TEST_CASE("ASCII and accented text convert one to one") {
    CHECK(toUtf32(L"Kodak 2.0") == U"Kodak 2.0");
    CHECK(toUtf32(L"Jos\u00E9 P\u00E9rez") == U"Jos\u00E9 P\u00E9rez");
    CHECK(toUtf32(L"").empty());
}

TEST_CASE("a surrogate pair becomes one character") {
    const std::wstring smiley{static_cast<wchar_t>(0xD83D), static_cast<wchar_t>(0xDE00)};
    CHECK(toUtf32(smiley) == std::u32string(1, static_cast<char32_t>(0x1F600)));
    CHECK(toUtf32(L"a" + smiley + L"b").size() == 3);
}

TEST_CASE("a lone surrogate becomes the replacement character") {
    const std::wstring lone{static_cast<wchar_t>(0xD83D), L'a'};
    CHECK(toUtf32(lone) == std::u32string{static_cast<char32_t>(0xFFFD), U'a'});
    const std::wstring trailing{L'a', static_cast<wchar_t>(0xDE00)};
    CHECK(toUtf32(trailing) == std::u32string{U'a', static_cast<char32_t>(0xFFFD)});
}

TEST_CASE("code points convert back to UTF-16") {
    using kodak::win::toUtf16;
    CHECK(toUtf16(U"Jos\u00E9") == L"Jos\u00E9");
    const std::wstring smiley{static_cast<wchar_t>(0xD83D), static_cast<wchar_t>(0xDE00)};
    CHECK(toUtf16(std::u32string(1, static_cast<char32_t>(0x1F600))) == smiley);
    CHECK(toUtf32(toUtf16(U"a\U0001F600b")) == U"a\U0001F600b");
    CHECK(toUtf16(std::u32string(1, static_cast<char32_t>(0xD800))) ==
          std::wstring(1, static_cast<wchar_t>(0xFFFD)));
}
