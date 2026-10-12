#pragma once

#include <windows.h>

#include <shlobj.h>

#include <filesystem>
#include <optional>
#include <system_error>

namespace kodak::win::detail {

/// `subfolder` of one of the current user's known folders (Pictures, AppData...),
/// created if missing.
inline std::optional<std::filesystem::path> knownSubfolder(REFKNOWNFOLDERID id, const wchar_t* subfolder) {
    PWSTR base = nullptr;
    const bool found = SUCCEEDED(SHGetKnownFolderPath(id, KF_FLAG_CREATE, nullptr, &base));
    const std::filesystem::path folder =
        found ? std::filesystem::path(base) / subfolder : std::filesystem::path();
    CoTaskMemFree(base); // even when the call failed
    if (!found) return std::nullopt;

    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (error || !std::filesystem::is_directory(folder, error)) return std::nullopt;
    return folder;
}

} // namespace kodak::win::detail
