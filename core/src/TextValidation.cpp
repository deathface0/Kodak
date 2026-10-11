#include "kodak/core/TextValidation.h"

#include "TextChars.h"

#include <cmath>
#include <cstddef>
#include <unordered_map>

namespace kodak::core {

using namespace detail;

namespace {

constexpr char32_t dniLetters[] = U"TRWAGMYFPDXBNJZSQVHLCKE";

bool allDigits(std::u32string_view text) noexcept {
    for (const char32_t c : text) {
        if (!isDigit(c)) return false;
    }
    return !text.empty();
}

std::uint32_t parseDigits(std::u32string_view digits) noexcept {
    std::uint32_t value = 0;
    for (const char32_t c : digits) value = value * 10 + static_cast<std::uint32_t>(digitValue(c));
    return value;
}

} // namespace

bool luhnValid(std::u32string_view digits) noexcept {
    if (!allDigits(digits)) return false;
    int sum = 0;
    bool doubled = false;
    for (std::size_t i = digits.size(); i-- > 0;) {
        int d = digitValue(digits[i]);
        if (doubled) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        doubled = !doubled;
    }
    return sum % 10 == 0;
}

bool ibanValid(std::u32string_view iban) noexcept {
    if (iban.size() < 15 || iban.size() > 34) return false;
    if (!isLetter(iban[0]) || !isLetter(iban[1]) || !isDigit(iban[2]) || !isDigit(iban[3])) return false;
    if (toUpper(iban[0]) == U'E' && toUpper(iban[1]) == U'S' && iban.size() != 24) return false;

    // Move the first four characters to the end, letters become 10..35, then mod 97.
    int remainder = 0;
    for (std::size_t n = 0; n < iban.size(); ++n) {
        const char32_t c = toUpper(iban[(n + 4) % iban.size()]);
        if (isDigit(c)) {
            remainder = (remainder * 10 + digitValue(c)) % 97;
        } else if (isUpper(c)) {
            remainder = (remainder * 100 + static_cast<int>(c - U'A') + 10) % 97;
        } else {
            return false;
        }
    }
    return remainder == 1;
}

char32_t dniLetter(std::uint32_t number) noexcept { return dniLetters[number % 23]; }

bool dniValid(std::u32string_view id) noexcept {
    return id.size() == 9 && allDigits(id.substr(0, 8)) &&
           toUpper(id[8]) == dniLetter(parseDigits(id.substr(0, 8)));
}

bool nieValid(std::u32string_view id) noexcept {
    if (id.size() != 9 || !allDigits(id.substr(1, 7))) return false;
    const char32_t prefix = toUpper(id[0]);
    if (prefix != U'X' && prefix != U'Y' && prefix != U'Z') return false;
    const std::uint32_t number =
        static_cast<std::uint32_t>(prefix - U'X') * 10'000'000 + parseDigits(id.substr(1, 7));
    return toUpper(id[8]) == dniLetter(number);
}

bool ipv4Valid(std::u32string_view ip) noexcept {
    int octets = 0;
    std::size_t start = 0;
    while (start <= ip.size()) {
        const std::size_t dot = ip.find(U'.', start);
        const std::u32string_view part =
            ip.substr(start, dot == std::u32string_view::npos ? ip.size() - start : dot - start);
        if (part.empty() || part.size() > 3 || !allDigits(part) || parseDigits(part) > 255) return false;
        ++octets;
        if (dot == std::u32string_view::npos) break;
        start = dot + 1;
    }
    return octets == 4;
}

double shannonEntropy(std::u32string_view text) noexcept {
    if (text.empty()) return 0.0;
    std::unordered_map<char32_t, std::size_t> counts;
    for (const char32_t c : text) ++counts[c];
    double entropy = 0.0;
    for (const auto& [c, count] : counts) {
        const double p = static_cast<double>(count) / static_cast<double>(text.size());
        entropy -= p * std::log2(p);
    }
    return entropy;
}

} // namespace kodak::core
