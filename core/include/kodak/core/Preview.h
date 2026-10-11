#pragma once

#include "kodak/core/Finding.h"

#include <string>
#include <string_view>

namespace kodak::core {

/// A short, masked version of what a finding covers, enough to tell which data
/// it is without showing it in full on screen: "ju***@gmail.com", "**** 0366",
/// "ES91 **** 1332", "192.168.*.*", "h*** (7 characters)". Text too short to
/// mask safely becomes "***"; manual boxes have no preview.
std::u32string maskPreview(FindingKind kind, std::u32string_view text);

} // namespace kodak::core
