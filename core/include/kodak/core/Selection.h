#pragma once

#include "kodak/core/Geometry.h"

namespace kodak::core {

/// Selections narrower or shorter than this are treated as a stray click.
inline constexpr int minSelectionSide = 4;

constexpr bool isUsableSelection(const Rect& selection) noexcept {
    return selection.width >= minSelectionSide && selection.height >= minSelectionSide;
}

} // namespace kodak::core
