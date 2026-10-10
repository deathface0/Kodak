#pragma once

namespace kodak::win {

/// Holds a named mutex for the lifetime of the object, so a second Kodak
/// process can tell that one is already running.
class SingleInstance {
public:
    /// `name` should start with "Local\\" (one instance per session).
    explicit SingleInstance(const wchar_t* name) noexcept;
    ~SingleInstance();

    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;

    /// True when no other process held the mutex.
    bool isFirst() const noexcept { return first_; }

private:
    void* mutex_ = nullptr; // HANDLE, kept as void* so the header needs no <windows.h>
    bool first_ = false;
};

} // namespace kodak::win
