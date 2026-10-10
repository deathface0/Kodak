#pragma once

#include <windows.h>

#include <objbase.h>

namespace kodak::win {

/// Initialises COM (single-threaded apartment) on the calling thread for the
/// lifetime of the object. WIC and ShellExecute need it.
class ComApartment {
public:
    ComApartment() noexcept : result_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComApartment() {
        if (SUCCEEDED(result_)) CoUninitialize();
    }

    ComApartment(const ComApartment&) = delete;
    ComApartment& operator=(const ComApartment&) = delete;

    /// True when COM is usable on this thread.
    bool ok() const noexcept { return SUCCEEDED(result_); }

private:
    HRESULT result_;
};

} // namespace kodak::win
