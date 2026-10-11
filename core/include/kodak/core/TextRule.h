#pragma once

#include "kodak/core/Finding.h"

#include <cstddef>
#include <memory>
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
class TextRule {
public:
    virtual ~TextRule() = default;
    virtual std::vector<TextMatch> find(std::u32string_view line) const = 0;
};

/// Every rule of the plan (e-mail, card, IBAN, DNI/NIE, phone, IPv4, known
/// secret formats, high-entropy tokens, labelled values) plus one for
/// `userTerms` when there are any.
std::vector<std::unique_ptr<TextRule>> defaultRules(std::vector<std::u32string> userTerms = {});

} // namespace kodak::core
