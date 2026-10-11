#include "kodak/core/Preview.h"

#include <cstddef>
#include <string>

namespace kodak::core {

namespace {

constexpr std::u32string_view hidden = U"***";

std::u32string ascii(const std::string& text) { return {text.begin(), text.end()}; }

/// Only the characters that are not separators (spaces, hyphens, dots).
std::u32string compact(std::u32string_view text) {
    std::u32string out;
    for (const char32_t c : text) {
        if (c != U' ' && c != U'-' && c != U'.') out += c;
    }
    return out;
}

std::u32string digitsOf(std::u32string_view text) {
    std::u32string out;
    for (const char32_t c : text) {
        if (c >= U'0' && c <= U'9') out += c;
    }
    return out;
}

/// The first `keep` characters, then how long the whole text is.
std::u32string startAndLength(std::u32string_view text, std::size_t keep) {
    if (text.size() < keep + 3) return std::u32string(hidden);
    return std::u32string(text.substr(0, keep)) + std::u32string(hidden) + U" (" +
           ascii(std::to_string(text.size())) + U" characters)";
}

std::u32string email(std::u32string_view text) {
    const std::size_t at = text.find(U'@');
    if (at == std::u32string_view::npos || at == 0) return std::u32string(hidden);
    const std::size_t keep = at > 2 ? 2 : 1;
    return std::u32string(text.substr(0, keep)) + std::u32string(hidden) + std::u32string(text.substr(at));
}

std::u32string lastDigits(std::u32string_view text, std::u32string_view prefix, std::size_t count) {
    const std::u32string digits = digitsOf(text);
    if (digits.size() < count + 3) return std::u32string(hidden);
    return std::u32string(prefix) + digits.substr(digits.size() - count);
}

std::u32string iban(std::u32string_view text) {
    const std::u32string letters = compact(text);
    if (letters.size() < 12) return std::u32string(hidden);
    return letters.substr(0, 4) + U" **** " + letters.substr(letters.size() - 4);
}

std::u32string dniNie(std::u32string_view text) {
    const std::u32string id = compact(text);
    if (id.size() < 8) return std::u32string(hidden);
    return U"*****" + id.substr(id.size() - 4);
}

std::u32string ip(std::u32string_view text) {
    const std::size_t first = text.find(U'.');
    const std::size_t second = first == std::u32string_view::npos ? first : text.find(U'.', first + 1);
    if (second == std::u32string_view::npos) return std::u32string(hidden);
    return std::u32string(text.substr(0, second)) + U".*.*";
}

} // namespace

std::u32string maskPreview(FindingKind kind, std::u32string_view text) {
    switch (kind) {
    case FindingKind::Email: return email(text);
    case FindingKind::Card: return lastDigits(text, U"**** ", 4);
    case FindingKind::Iban: return iban(text);
    case FindingKind::DniNie: return dniNie(text);
    case FindingKind::Phone: return lastDigits(text, U"*** *** ", 3);
    case FindingKind::Ip: return ip(text);
    case FindingKind::Secret: return startAndLength(text, 4);
    case FindingKind::ContextValue:
    case FindingKind::UserTerm: return startAndLength(text, 1);
    case FindingKind::Manual:
    case FindingKind::Face: break;
    }
    return {};
}

} // namespace kodak::core
