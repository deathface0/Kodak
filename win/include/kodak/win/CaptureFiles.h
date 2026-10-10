#pragma once

#include <kodak/core/FileName.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>

namespace kodak::win {

/// Pictures\Kodak for the current user, created if missing.
std::optional<std::filesystem::path> capturesFolder();

/// Writes `bytes` to `file` only if it does not exist yet. Never overwriting
/// means an older, longer file can never leave its tail behind (aCropalypse).
bool writeNewFile(const std::filesystem::path& file, std::span<const std::uint8_t> bytes);

/// Saves `png` in `folder` under captureFileName(when), adding a suffix if the
/// name is taken. Returns the file written, or std::nullopt.
std::optional<std::filesystem::path> saveCapture(const std::filesystem::path& folder,
                                                 std::span<const std::uint8_t> png,
                                                 const core::DateTime& when);

/// The current local date and time.
core::DateTime localNow() noexcept;

} // namespace kodak::win
