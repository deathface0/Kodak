#include "kodak/core/Utf8.h"

#include <cstddef>

namespace kodak::core {

namespace {

constexpr char32_t replacement = 0xFFFD;

constexpr bool isValidCodePoint(char32_t c) noexcept { return c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF); }

void append(std::string& out, char32_t c) {
    if (!isValidCodePoint(c)) c = replacement;
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
}

} // namespace

std::string toUtf8(std::u32string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char32_t c : text) append(out, c);
    return out;
}

std::u32string fromUtf8(std::string_view text) {
    std::u32string out;
    out.reserve(text.size());
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = 0;
        char32_t c = 0;
        char32_t minimum = 0;
        if (lead < 0x80) {
            length = 1;
            c = lead;
        } else if ((lead & 0xE0) == 0xC0) {
            length = 2;
            c = lead & 0x1F;
            minimum = 0x80;
        } else if ((lead & 0xF0) == 0xE0) {
            length = 3;
            c = lead & 0x0F;
            minimum = 0x800;
        } else if ((lead & 0xF8) == 0xF0) {
            length = 4;
            c = lead & 0x07;
            minimum = 0x10000;
        } else {
            out += replacement; // a continuation byte or an invalid lead byte
            ++i;
            continue;
        }

        bool valid = i + length <= text.size();
        for (std::size_t k = 1; valid && k < length; ++k) {
            const auto next = static_cast<unsigned char>(text[i + k]);
            valid = (next & 0xC0) == 0x80;
            c = (c << 6) | (next & 0x3F);
        }
        if (!valid || c < minimum || !isValidCodePoint(c)) {
            out += replacement;
            ++i; // resynchronise on the next byte
            continue;
        }
        out += c;
        i += length;
    }
    return out;
}

} // namespace kodak::core
