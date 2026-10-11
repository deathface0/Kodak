#pragma once

#include <doctest/doctest.h>

#include <kodak/core/TextRule.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace doctest {

// Readable failure messages for UTF-32 text (non-ASCII shown as \u{...}).
template <> struct StringMaker<std::u32string> {
    static String convert(const std::u32string& text) {
        std::string out = "U\"";
        for (const char32_t c : text) {
            if (c >= 0x20 && c < 0x7F) {
                out += static_cast<char>(c);
            } else {
                out += "\\u{" + std::to_string(static_cast<unsigned long>(c)) + "}";
            }
        }
        out += "\"";
        return out.c_str();
    }
};

template <> struct StringMaker<std::vector<std::u32string>> {
    static String convert(const std::vector<std::u32string>& texts) {
        String out = "{";
        for (std::size_t i = 0; i < texts.size(); ++i) {
            if (i > 0) out += ", ";
            out += StringMaker<std::u32string>::convert(texts[i]);
        }
        out += "}";
        return out;
    }
};

} // namespace doctest

namespace kodak::test {

using Texts = std::vector<std::u32string>;

/// The pieces of `line` that the default rules flag as `kind`.
inline Texts found(std::u32string_view line, core::FindingKind kind, Texts userTerms = {}) {
    Texts out;
    for (const auto& rule : core::defaultRules(std::move(userTerms))) {
        for (const core::TextMatch& match : rule->find(line)) {
            if (match.kind == kind) out.emplace_back(line.substr(match.begin, match.end - match.begin));
        }
    }
    return out;
}

inline bool contains(const Texts& texts, std::u32string_view text) {
    return std::find(texts.begin(), texts.end(), text) != texts.end();
}

} // namespace kodak::test
