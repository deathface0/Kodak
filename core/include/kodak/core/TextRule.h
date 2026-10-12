#pragma once

#include "kodak/core/Finding.h"

#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace kodak::core {

/// A sensitive piece of a line of text: characters [begin, end).
struct TextMatch {
    std::size_t begin = 0;
    std::size_t end = 0;
    FindingKind kind = FindingKind::Manual;

    friend bool operator==(const TextMatch&, const TextMatch&) = default;
};

/// Finds one kind of sensitive data in a line of recognised text.
using TextRule = std::function<std::vector<TextMatch>(std::u32string_view line)>;

/// Every rule of the plan (e-mail, card, IBAN, DNI/NIE, phone, IPv4, known
/// secret formats, high-entropy tokens, labelled values) plus one for
/// `userTerms` when there are any, leaving out the kinds in `disabled`
/// (FindingKind::Secret covers both the known formats and the entropy rule).
std::vector<TextRule> defaultRules(const std::vector<std::u32string>& userTerms = {},
                                   const std::set<FindingKind>& disabled = {});

} // namespace kodak::core
