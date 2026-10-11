#pragma once

#include <string>
#include <string_view>

namespace kodak::win {

/// UTF-16 (what Windows returns) to code points for kodak_core. Surrogate pairs
/// become one character; a lone surrogate becomes U+FFFD.
std::u32string toUtf32(std::wstring_view text);

} // namespace kodak::win
