#include "kodak/core/TextNormalize.h"
#include "kodak/core/TextRule.h"
#include "kodak/core/TextValidation.h"

#include "TextChars.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace kodak::core {

using namespace detail;

namespace {

using Matches = std::vector<TextMatch>;
constexpr auto npos = std::u32string_view::npos;

bool startsWord(std::u32string_view s, std::size_t i) noexcept { return i == 0 || !isWordChar(s[i - 1]); }
bool endsWord(std::u32string_view s, std::size_t i) noexcept { return i >= s.size() || !isWordChar(s[i]); }

std::size_t skipSpaces(std::u32string_view s, std::size_t i) noexcept {
    while (i < s.size() && isSpace(s[i])) ++i;
    return i;
}

// --- Runs of digit groups ("4111 1111 1111 1111", "612.345.678") ---------------

struct Group {
    std::size_t begin;
    std::size_t end;
};

/// Digit groups from `start`, separated by exactly one character of `separators`.
std::vector<Group> digitGroups(std::u32string_view s, std::size_t start, std::u32string_view separators) {
    std::vector<Group> groups;
    std::size_t i = start;
    while (i < s.size() && isDigit(s[i])) {
        const std::size_t begin = i;
        while (i < s.size() && isDigit(s[i])) ++i;
        groups.push_back({begin, i});
        if (i + 1 < s.size() && separators.find(s[i]) != npos && isDigit(s[i + 1])) {
            ++i;
        } else {
            break;
        }
    }
    return groups;
}

std::u32string digitsOf(std::u32string_view s, const std::vector<Group>& groups, std::size_t first,
                        std::size_t last) {
    std::u32string digits;
    for (std::size_t g = first; g <= last; ++g)
        digits += s.substr(groups[g].begin, groups[g].end - groups[g].begin);
    return digits;
}

/// Calls `visit(groups, endsAtBoundary)` for each run of digit groups that starts a word.
template <typename Visit>
void forEachDigitRun(std::u32string_view s, std::u32string_view separators, Visit visit) {
    std::size_t i = 0;
    while (i < s.size()) {
        if (isDigit(s[i]) && startsWord(s, i)) {
            const std::vector<Group> groups = digitGroups(s, i, separators);
            visit(groups, endsWord(s, groups.back().end));
            i = groups.back().end;
        } else {
            ++i;
        }
    }
}

/// A match from each word start where `endAt(s, i)` finds one, skipping what it covers.
template <typename EndAt> Matches matchWordStarts(std::u32string_view s, FindingKind kind, EndAt endAt) {
    Matches out;
    for (std::size_t i = 0; i < s.size();) {
        const std::optional<std::size_t> end = startsWord(s, i) ? endAt(s, i) : std::nullopt;
        if (end) {
            out.push_back({i, *end, kind});
            i = *end;
        } else {
            ++i;
        }
    }
    return out;
}

// --- Rules ------------------------------------------------------------------------

Matches findEmails(std::u32string_view line) {
    auto isLocal = [](char32_t c) {
        return isAlnum(c) || c == U'.' || c == U'_' || c == U'%' || c == U'+' || c == U'-';
    };
    auto isDomain = [](char32_t c) { return isAlnum(c) || c == U'.' || c == U'-'; };

    Matches out;
    for (std::size_t at = line.find(U'@'); at != npos; at = line.find(U'@', at + 1)) {
        std::size_t begin = at;
        while (begin > 0 && isLocal(line[begin - 1])) --begin;
        while (begin < at && line[begin] == U'.') ++begin;
        std::size_t end = at + 1;
        while (end < line.size() && isDomain(line[end])) ++end;
        while (end > at + 1 && (line[end - 1] == U'.' || line[end - 1] == U'-')) --end;
        if (begin == at || end == at + 1 || line[at - 1] == U'.') continue;

        const std::u32string_view domain = line.substr(at + 1, end - at - 1);
        const std::size_t dot = domain.rfind(U'.');
        if (dot == npos || dot == 0) continue;
        const std::u32string_view tld = domain.substr(dot + 1);
        if (tld.size() < 2 || !std::all_of(tld.begin(), tld.end(), isLetter)) continue;
        out.push_back({begin, end, FindingKind::Email});
    }
    return out;
}

Matches findCards(std::u32string_view line) {
    const std::u32string text = normaliseDigits(line);
    Matches out;
    forEachDigitRun(text, U" -", [&](const std::vector<Group>& groups, bool endsAtBoundary) {
        // The longest run of whole groups with 13-19 digits that passes Luhn, so
        // "4111 1111 1111 1111 12/25" still finds the card before the expiry date.
        for (std::size_t first = 0; first < groups.size();) {
            std::optional<std::size_t> best;
            for (std::size_t last = first; last < groups.size(); ++last) {
                if (last + 1 == groups.size() && !endsAtBoundary) break;
                const std::u32string digits = digitsOf(text, groups, first, last);
                if (digits.size() > 19) break;
                if (digits.size() >= 13 && luhnValid(digits)) best = last;
            }
            if (best) {
                out.push_back({groups[first].begin, groups[*best].end, FindingKind::Card});
                first = *best + 1;
            } else {
                ++first;
            }
        }
    });
    return out;
}

/// Where the longest valid IBAN of whole groups starting at `i` ends.
std::optional<std::size_t> ibanEndingAt(std::u32string_view s, std::size_t i) {
    if (i + 4 > s.size() || !isLetter(s[i]) || !isLetter(s[i + 1]) || !isDigit(s[i + 2]) ||
        !isDigit(s[i + 3]))
        return std::nullopt;
    // Alphanumeric groups separated by single spaces ("ES91 2100 0418 ...").
    std::vector<Group> groups;
    std::u32string compact;
    std::size_t k = i;
    while (k < s.size() && isAlnum(s[k]) && compact.size() < 34) {
        const std::size_t begin = k;
        while (k < s.size() && isAlnum(s[k])) ++k;
        groups.push_back({begin, k});
        compact += s.substr(begin, k - begin);
        if (k + 1 < s.size() && s[k] == U' ' && isAlnum(s[k + 1])) ++k;
    }
    std::size_t length = compact.size();
    for (std::size_t g = groups.size(); g-- > 0;) {
        if (endsWord(s, groups[g].end) && ibanValid(std::u32string_view(compact).substr(0, length)))
            return groups[g].end;
        length -= groups[g].end - groups[g].begin;
    }
    return std::nullopt;
}

Matches findIbans(std::u32string_view line) {
    return matchWordStarts(normaliseDigits(line), FindingKind::Iban, ibanEndingAt);
}

/// "12345678Z", "12345678-Z", "X1234567L", "X-1234567-L": where the ID starting at `i` ends.
std::optional<std::size_t> dniNieEndingAt(std::u32string_view s, std::size_t i) {
    auto digitsAt = [&](std::size_t at, std::size_t count) {
        if (at + count > s.size()) return false;
        for (std::size_t k = at; k < at + count; ++k) {
            if (!isDigit(s[k])) return false;
        }
        return true;
    };
    auto skipDash = [&](std::size_t at) {
        return at < s.size() && (s[at] == U'-' || s[at] == U' ') ? at + 1 : at;
    };
    auto letterEnd = [&](std::size_t at, std::u32string id) -> std::optional<std::size_t> {
        at = skipDash(at);
        if (at >= s.size() || !isLetter(s[at]) || !endsWord(s, at + 1)) return std::nullopt;
        id += s[at];
        if (dniValid(id) || nieValid(id)) return at + 1;
        return std::nullopt;
    };

    if (digitsAt(i, 8)) return letterEnd(i + 8, std::u32string(s.substr(i, 8)));
    const char32_t prefix = toUpper(s[i]);
    if (prefix == U'X' || prefix == U'Y' || prefix == U'Z') {
        const std::size_t at = skipDash(i + 1);
        if (digitsAt(at, 7))
            return letterEnd(at + 7, std::u32string(1, prefix) + std::u32string(s.substr(at, 7)));
    }
    return std::nullopt;
}

Matches findDniNies(std::u32string_view line) {
    return matchWordStarts(normaliseDigits(line), FindingKind::DniNie, dniNieEndingAt);
}

/// Spanish numbers: 9 digits starting 6-9, optionally after +34 or 0034.
bool isSpanishPhone(std::u32string_view d, bool plus) noexcept {
    auto national = [](char32_t c) { return c >= U'6' && c <= U'9'; };
    if (d.size() == 9) return national(d[0]);
    if (plus && d.size() == 11 && d.starts_with(U"34")) return national(d[2]);
    if (d.size() == 13 && d.starts_with(U"0034")) return national(d[4]);
    return false;
}

Matches findPhones(std::u32string_view line) {
    const std::u32string text = normaliseDigits(line);
    Matches out;
    forEachDigitRun(text, U" .-", [&](const std::vector<Group>& groups, bool endsAtBoundary) {
        const bool plus = groups.front().begin > 0 && text[groups.front().begin - 1] == U'+';
        for (std::size_t first = 0; first < groups.size();) {
            bool found = false;
            for (std::size_t last = first; last < groups.size(); ++last) {
                if (last + 1 == groups.size() && !endsAtBoundary) break;
                const std::u32string digits = digitsOf(text, groups, first, last);
                if (digits.size() > 13) break;
                const bool withPlus = plus && first == 0;
                if (isSpanishPhone(digits, withPlus)) {
                    const std::size_t begin =
                        withPlus && digits.size() == 11 ? groups[first].begin - 1 : groups[first].begin;
                    out.push_back({begin, groups[last].end, FindingKind::Phone});
                    first = last + 1;
                    found = true;
                    break;
                }
            }
            if (!found) ++first;
        }
    });
    return out;
}

Matches findIpv4s(std::u32string_view line) {
    const std::u32string text = normaliseDigits(line);
    Matches out;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!isDigit(text[i]) || !(i == 0 || (!isWordChar(text[i - 1]) && text[i - 1] != U'.'))) {
            ++i;
            continue;
        }
        std::size_t end = i;
        while (end < text.size() && (isDigit(text[end]) || text[end] == U'.')) ++end;
        const std::size_t next = end;
        while (end > i && text[end - 1] == U'.') --end; // a full stop after the address
        if (endsWord(text, end) && ipv4Valid(std::u32string_view(text).substr(i, end - i)))
            out.push_back({i, end, FindingKind::Ip});
        i = next;
    }
    return out;
}

// --- Secrets with a known shape ---------------------------------------------------

bool upperOrDigit(char32_t c) noexcept { return isUpper(c) || isDigit(c); }
bool alnumOrUnderscore(char32_t c) noexcept { return isAlnum(c) || c == U'_'; }
bool base64Url(char32_t c) noexcept { return isAlnum(c) || c == U'_' || c == U'-'; }

/// A token made of a fixed prefix and `minRest` to `maxRest` `allowed` characters.
struct Prefixed {
    std::u32string_view prefix;
    std::size_t minRest;
    std::size_t maxRest;
    bool (*allowed)(char32_t);
};

constexpr std::array<Prefixed, 11> prefixedSecrets{{
    {U"AKIA", 16, 16, upperOrDigit}, // AWS access key ID
    {U"ASIA", 16, 16, upperOrDigit}, // AWS temporary access key ID
    {U"ghp_", 36, 36, isAlnum},      // GitHub tokens
    {U"gho_", 36, 36, isAlnum},
    {U"ghu_", 36, 36, isAlnum},
    {U"ghs_", 36, 36, isAlnum},
    {U"ghr_", 36, 36, isAlnum},
    {U"github_pat_", 22, 255, alnumOrUnderscore},
    {U"AIza", 35, 35, base64Url},    // Google API key
    {U"sk_live_", 24, 255, isAlnum}, // Stripe
    {U"rk_live_", 24, 255, isAlnum},
}};

void findPrefixed(std::u32string_view line, const Prefixed& pattern, Matches& out) {
    for (std::size_t at = line.find(pattern.prefix); at != npos; at = line.find(pattern.prefix, at + 1)) {
        if (at > 0 && isAlnum(line[at - 1])) continue;
        std::size_t end = at + pattern.prefix.size();
        while (end < line.size() && pattern.allowed(line[end])) ++end;
        const std::size_t rest = end - at - pattern.prefix.size();
        if (rest >= pattern.minRest && rest <= pattern.maxRest && endsWord(line, end))
            out.push_back({at, end, FindingKind::Secret});
    }
}

/// "eyJ<header>.eyJ<payload>.<signature>"
void findJwts(std::u32string_view line, Matches& out) {
    for (std::size_t at = line.find(U"eyJ"); at != npos; at = line.find(U"eyJ", at + 1)) {
        if (at > 0 && base64Url(line[at - 1])) continue;
        std::size_t end = at;
        std::array<std::size_t, 3> partBegin{};
        std::size_t parts = 0;
        while (parts < 3) {
            partBegin[parts] = end;
            while (end < line.size() && base64Url(line[end])) ++end;
            if (end == partBegin[parts]) break;
            ++parts;
            if (parts < 3 && end < line.size() && line[end] == U'.') {
                ++end;
            } else {
                break;
            }
        }
        if (parts == 3 && line.substr(partBegin[1]).starts_with(U"eyJ") && endsWord(line, end)) {
            out.push_back({at, end, FindingKind::Secret});
            at = end - 1;
        }
    }
}

/// "-----BEGIN RSA PRIVATE KEY-----": the key lines below are caught by findRandomTokens.
void findPrivateKeyHeaders(std::u32string_view line, Matches& out) {
    constexpr std::u32string_view begin = U"-----BEGIN ";
    constexpr std::u32string_view tail = U"PRIVATE KEY-----";
    for (std::size_t at = line.find(begin); at != npos; at = line.find(begin, at + 1)) {
        const std::size_t t = line.find(tail, at + begin.size());
        if (t == npos) break;
        out.push_back({at, t + tail.size(), FindingKind::Secret});
        at = t;
    }
}

/// API keys and tokens with a known shape.
Matches findKnownSecrets(std::u32string_view line) {
    Matches out;
    for (const Prefixed& pattern : prefixedSecrets) findPrefixed(line, pattern, out);
    findJwts(line, out);
    findPrivateKeyHeaders(line, out);
    return out;
}

// --- Other rules ------------------------------------------------------------------

/// Passwords and keys with no known shape: tokens of 20 characters or more that
/// mix upper case, lower case and digits and look random. A token of `length`
/// characters needs min(4.5, 0.85 * log2(length)) bits of entropy per character:
/// random keys reach it (about 4.55 bits for 32 base-62 characters), readable
/// identifiers of that length stay below (about 4.1).
bool looksRandom(std::u32string_view token) noexcept {
    if (token.size() < 20) return false;
    const bool upper = std::any_of(token.begin(), token.end(), isUpper);
    const bool lower = std::any_of(token.begin(), token.end(), isLower);
    const bool digit = std::any_of(token.begin(), token.end(), isDigit);
    const double threshold = std::min(4.5, 0.85 * std::log2(static_cast<double>(token.size())));
    return upper && lower && digit && shannonEntropy(token) >= threshold;
}

Matches findRandomTokens(std::u32string_view line) {
    auto isTokenChar = [](char32_t c) {
        return isAlnum(c) || c == U'+' || c == U'/' || c == U'_' || c == U'-';
    };
    Matches out;
    std::size_t i = 0;
    while (i < line.size()) {
        if (!isTokenChar(line[i])) {
            ++i;
            continue;
        }
        const std::size_t begin = i;
        while (i < line.size() && isTokenChar(line[i])) ++i;
        std::size_t end = i;
        while (end < line.size() && end - i < 2 && line[end] == U'=') ++end; // base64 padding
        if (looksRandom(line.substr(begin, end - begin))) out.push_back({begin, end, FindingKind::Secret});
        i = end;
    }
    return out;
}

/// The user's own `terms` (already folded), ignoring case and accents.
Matches findTerms(std::u32string_view line, const std::vector<std::u32string>& terms) {
    const std::u32string folded = foldForSearch(line);
    Matches out;
    for (const std::u32string& term : terms) {
        for (std::size_t at = folded.find(term); at != npos; at = folded.find(term, at + 1)) {
            if (startsWord(folded, at) && endsWord(folded, at + term.size()))
                out.push_back({at, at + term.size(), FindingKind::UserTerm});
        }
    }
    return out;
}

/// "Password: hunter2", "PIN = 1234": the rest of the line after a label.
Matches findContextValues(std::u32string_view line) {
    // Folded (lower case, no accents), so "contrasena" also matches the accented spelling.
    static constexpr std::array<std::u32string_view, 11> labels{U"password",   U"pass", U"passwd", U"pwd",
                                                                U"contrasena", U"pin",  U"token",  U"clave",
                                                                U"api key",    U"cvv",  U"secret"};

    const std::u32string folded = foldForSearch(line);
    std::size_t valueEnd = line.size();
    while (valueEnd > 0 && isSpace(line[valueEnd - 1])) --valueEnd;

    Matches out;
    for (const std::u32string_view label : labels) {
        for (std::size_t at = folded.find(label); at != npos; at = folded.find(label, at + 1)) {
            if (!startsWord(folded, at) || !endsWord(folded, at + label.size())) continue;
            std::size_t k = skipSpaces(line, at + label.size());
            if (k >= line.size() || (line[k] != U':' && line[k] != U'=')) continue;
            k = skipSpaces(line, k + 1);
            if (k < valueEnd) out.push_back({k, valueEnd, FindingKind::ContextValue});
        }
    }
    return out;
}

} // namespace

std::vector<TextRule> defaultRules(const std::vector<std::u32string>& userTerms,
                                   const std::set<FindingKind>& disabled) {
    std::vector<TextRule> rules;
    auto add = [&](FindingKind kind, TextRule rule) {
        if (!disabled.contains(kind)) rules.push_back(std::move(rule));
    };
    add(FindingKind::Email, findEmails);
    add(FindingKind::Card, findCards);
    add(FindingKind::Iban, findIbans);
    add(FindingKind::DniNie, findDniNies);
    add(FindingKind::Phone, findPhones);
    add(FindingKind::Ip, findIpv4s);
    add(FindingKind::Secret, findKnownSecrets);
    add(FindingKind::Secret, findRandomTokens);
    if (!userTerms.empty()) {
        std::vector<std::u32string> folded;
        for (const std::u32string& term : userTerms) {
            if (const auto trimmed = trimSpaces(term); !trimmed.empty())
                folded.push_back(foldForSearch(trimmed));
        }
        add(FindingKind::UserTerm,
            [terms = std::move(folded)](std::u32string_view line) { return findTerms(line, terms); });
    }
    add(FindingKind::ContextValue, findContextValues);
    return rules;
}

} // namespace kodak::core
