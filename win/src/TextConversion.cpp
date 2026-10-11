#include "kodak/win/TextConversion.h"

#include <cstddef>

namespace kodak::win {

namespace {

constexpr bool isHighSurrogate(char32_t c) noexcept { return c >= 0xD800 && c <= 0xDBFF; }
constexpr bool isLowSurrogate(char32_t c) noexcept { return c >= 0xDC00 && c <= 0xDFFF; }
constexpr char32_t replacement = 0xFFFD;

} // namespace

std::u32string toUtf32(std::wstring_view text) {
    std::u32string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<char32_t>(text[i]);
        if (isHighSurrogate(c) && i + 1 < text.size() && isLowSurrogate(static_cast<char32_t>(text[i + 1]))) {
            const auto low = static_cast<char32_t>(text[i + 1]);
            out += 0x10000 + ((c - 0xD800) << 10) + (low - 0xDC00);
            ++i;
        } else if (isHighSurrogate(c) || isLowSurrogate(c)) {
            out += replacement;
        } else {
            out += c;
        }
    }
    return out;
}

std::wstring toUtf16(std::u32string_view text) {
    std::wstring out;
    out.reserve(text.size());
    for (char32_t c : text) {
        if (c > 0x10FFFF || isHighSurrogate(c) || isLowSurrogate(c)) c = replacement;
        if (c >= 0x10000) {
            c -= 0x10000;
            out += static_cast<wchar_t>(0xD800 + (c >> 10));
            out += static_cast<wchar_t>(0xDC00 + (c & 0x3FF));
        } else {
            out += static_cast<wchar_t>(c);
        }
    }
    return out;
}

} // namespace kodak::win
