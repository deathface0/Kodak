#pragma once

#include <cstdint>
#include <string_view>

namespace kodak::core {

/// Luhn checksum (payment cards) over a string of digits; false if anything else is in it.
bool luhnValid(std::u32string_view digits) noexcept;

/// IBAN without spaces: 2 letters, 2 check digits, then letters/digits; 15 to 34
/// characters (24 for Spain) and remainder 1 modulo 97. Lower case is accepted.
bool ibanValid(std::u32string_view iban) noexcept;

/// The control letter of a Spanish DNI number.
char32_t dniLetter(std::uint32_t number) noexcept;

/// "12345678Z": 8 digits and their control letter (either case).
bool dniValid(std::u32string_view id) noexcept;

/// "X1234567L": X, Y or Z (read as 0, 1, 2), 7 digits and the DNI control letter.
bool nieValid(std::u32string_view id) noexcept;

/// Four dot-separated decimal octets, each 0 to 255.
bool ipv4Valid(std::u32string_view ip) noexcept;

/// Shannon entropy of the characters of `text`, in bits per character.
double shannonEntropy(std::u32string_view text) noexcept;

} // namespace kodak::core
