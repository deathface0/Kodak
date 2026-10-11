#include "kodak/core/Finding.h"

namespace kodak::core {

namespace {

long long areaOf(const Rect& r) noexcept { return static_cast<long long>(r.width) * r.height; }

} // namespace

bool toggleAt(std::vector<Finding>& findings, Point p) noexcept {
    const std::optional<std::size_t> hit = topmostAt(findings, p);
    if (!hit) return false;
    findings[*hit].enabled = !findings[*hit].enabled;
    return true;
}

std::vector<Finding> mergeOverlapping(std::vector<Finding> findings) {
    bool merged = true;
    while (merged) {
        merged = false;
        for (std::size_t i = 0; i < findings.size() && !merged; ++i) {
            for (std::size_t j = i + 1; j < findings.size(); ++j) {
                if (intersect(findings[i].area, findings[j].area).empty()) continue;
                if (areaOf(findings[j].area) > areaOf(findings[i].area)) findings[i].kind = findings[j].kind;
                findings[i].area = unite(findings[i].area, findings[j].area);
                findings[i].enabled = findings[i].enabled || findings[j].enabled;
                findings.erase(findings.begin() + static_cast<std::ptrdiff_t>(j));
                merged = true; // the grown area may now reach findings already checked
                break;
            }
        }
    }
    return findings;
}

} // namespace kodak::core
