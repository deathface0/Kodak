#pragma once

#include <string>
#include <string_view>

namespace kodak::win {

/// UTF-16 (what Windows returns) to code points for kodak_core. Surrogate pairs
/// become one character; a lone surrogate becomes U+FFFD.
std::u32string toUtf32(std::wstring_view text);

/// Code points to UTF-16 for Windows. Characters above U+FFFF become surrogate
/// pairs; invalid code points become U+FFFD.
std::wstring toUtf16(std::u32string_view text);

} // namespace kodak::win
