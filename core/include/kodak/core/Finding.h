#pragma once

#include "kodak/core/Geometry.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace kodak::core {

/// What a finding covers. Manual boxes are drawn by the user; the rest come
/// from the detectors (phase 4).
enum class FindingKind { Manual, Email, Card, Iban, DniNie, Phone, Ip, Secret, UserTerm, ContextValue, Face };

/// An area of the image proposed for redaction.
struct Finding {
    Rect area;
    FindingKind kind = FindingKind::Manual;
    bool enabled = true; // the user can turn a proposal off before confirming
};

/// The index of the last finding whose area contains `p` (the one drawn on top), if any.
constexpr std::optional<std::size_t> topmostAt(std::span<const Finding> findings, Point p) noexcept {
    for (std::size_t i = findings.size(); i-- > 0;) {
        if (findings[i].area.contains(p)) return i;
    }
    return std::nullopt;
}

/// Joins findings whose areas overlap into one covering both, until none
/// overlap; the result keeps the kind of the larger of the two. Avoids two
/// boxes over the same text when several rules match it.
std::vector<Finding> mergeOverlapping(std::vector<Finding> findings);

} // namespace kodak::core
