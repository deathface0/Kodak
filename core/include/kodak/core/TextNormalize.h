#pragma once

#include <string>
#include <string_view>

namespace kodak::core {

/// Undoes common OCR slips inside numbers: O/o become 0 and I/l/| become 1, but
/// only next to a digit, so "4111 l111" reads as digits while words and country
/// codes ("IT60...") stay intact. Same length as `line`, so ranges carry over.
std::u32string normaliseDigits(std::u32string_view line);

/// Lower case without accents (U+00E1 -> a, U+00F1 -> n, U+00E7 -> c... for Latin-1 and Latin Extended-A),
/// for case- and accent-insensitive search. Same length as `line`.
std::u32string foldForSearch(std::u32string_view line);

} // namespace kodak::core
