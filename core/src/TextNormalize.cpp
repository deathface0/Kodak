#include "kodak/core/TextNormalize.h"

#include "TextChars.h"

#include <cstddef>
#include <string_view>

namespace kodak::core {

using namespace detail;

namespace {

// Base letters for U+00C0..U+00FF; '*' keeps the character (multiplication and division signs).
constexpr std::string_view latin1 = "aaaaaaaceeeeiiiidnooooo*ouuuuytsaaaaaaaceeeeiiiidnooooo*ouuuuyty";
// Base letters for U+0100..U+017F (Latin Extended-A), already lower case.
constexpr std::string_view latinExtendedA =
    "aaaaaaccccccccddddeeeeeeeeeegggggggghhhhiiiiiiiiiiiijjkkkllllllllll"
    "nnnnnnnnnoooooooorrrrrrssssssssttttttuuuuuuuuuuuuwwyyyzzzzzzs";
static_assert(latin1.size() == 64);
static_assert(latinExtendedA.size() == 128);

char32_t slipToDigit(char32_t c) noexcept {
    switch (c) {
    case U'O':
    case U'o': return U'0';
    case U'I':
    case U'l':
    case U'|': return U'1';
    default: return 0;
    }
}

} // namespace

std::u32string normaliseDigits(std::u32string_view line) {
    std::u32string out(line);
    // Two passes, so a slip next to another slip next to a digit ("4lO1") is caught too.
    for (int pass = 0; pass < 2; ++pass) {
        for (std::size_t i = 0; i < out.size(); ++i) {
            const char32_t digit = slipToDigit(out[i]);
            if (!digit) continue;
            const bool before = i > 0 && isDigit(out[i - 1]);
            const bool after = i + 1 < out.size() && isDigit(out[i + 1]);
            if (before || after) out[i] = digit;
        }
    }
    return out;
}

std::u32string foldForSearch(std::u32string_view line) {
    std::u32string out(line);
    for (char32_t& c : out) {
        if (isUpper(c)) {
            c = c - U'A' + U'a';
        } else if (c >= 0xC0 && c <= 0xFF && latin1[c - 0xC0] != '*') {
            c = static_cast<char32_t>(latin1[c - 0xC0]);
        } else if (c >= 0x100 && c <= 0x17F) {
            c = static_cast<char32_t>(latinExtendedA[c - 0x100]);
        }
    }
    return out;
}

} // namespace kodak::core
