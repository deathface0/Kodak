#pragma once

#include <string>
#include <string_view>

namespace kodak::core {

/// Code points to UTF-8. Invalid code points (surrogates, above U+10FFFF) become U+FFFD.
std::string toUtf8(std::u32string_view text);

/// UTF-8 to code points. Invalid or overlong sequences become U+FFFD.
std::u32string fromUtf8(std::string_view text);

} // namespace kodak::core
