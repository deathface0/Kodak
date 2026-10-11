#pragma once

// Character classes used by the text rules. ASCII is what the patterns look
// for; anything above it counts as part of a word so a rule never fires inside
// a longer non-ASCII word.

namespace kodak::core::detail {

constexpr bool isDigit(char32_t c) noexcept { return c >= U'0' && c <= U'9'; }
constexpr bool isUpper(char32_t c) noexcept { return c >= U'A' && c <= U'Z'; }
constexpr bool isLower(char32_t c) noexcept { return c >= U'a' && c <= U'z'; }
constexpr bool isLetter(char32_t c) noexcept { return isUpper(c) || isLower(c); }
constexpr bool isAlnum(char32_t c) noexcept { return isLetter(c) || isDigit(c); }
constexpr bool isSpace(char32_t c) noexcept {
    return c == U' ' || c == U'\t' || c == 0xA0; // 0xA0: no-break space
}

constexpr bool isWordChar(char32_t c) noexcept {
    return isAlnum(c) || c == U'_' || (c >= 0xC0 && c != 0xD7 && c != 0xF7);
}

constexpr char32_t toUpper(char32_t c) noexcept { return isLower(c) ? c - U'a' + U'A' : c; }
constexpr int digitValue(char32_t c) noexcept { return static_cast<int>(c - U'0'); }

} // namespace kodak::core::detail
