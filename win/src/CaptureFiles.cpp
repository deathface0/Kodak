#include "kodak/win/CaptureFiles.h"

#include "detail/KnownFolder.h"

namespace kodak::win {

namespace {

constexpr int maxAttempts = 100;

struct FileHandle {
    HANDLE handle;
    ~FileHandle() {
        if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
    }
};

} // namespace

std::optional<std::filesystem::path> capturesFolder() {
    return detail::knownSubfolder(FOLDERID_Pictures, L"Kodak");
}

bool writeNewFile(const std::filesystem::path& file, std::span<const std::uint8_t> bytes) {
    bool complete = false;
    {
        const FileHandle out{
            CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
        if (out.handle == INVALID_HANDLE_VALUE) return false; // GetLastError() == ERROR_FILE_EXISTS if taken

        DWORD written = 0;
        complete = WriteFile(out.handle, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
                   written == bytes.size();
    }
    if (!complete) DeleteFileW(file.c_str()); // never leave a truncated capture behind
    return complete;
}

std::optional<std::filesystem::path> saveCapture(const std::filesystem::path& folder,
                                                 std::span<const std::uint8_t> png,
                                                 const core::DateTime& when) {
    for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
        const std::filesystem::path file = folder / core::captureFileName(when, attempt);
        if (writeNewFile(file, png)) return file;
        if (GetLastError() != ERROR_FILE_EXISTS) return std::nullopt; // a real failure, not a taken name
    }
    return std::nullopt;
}

core::DateTime localNow() noexcept {
    SYSTEMTIME now;
    GetLocalTime(&now);
    return {now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond};
}

} // namespace kodak::win
