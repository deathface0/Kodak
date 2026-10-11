#pragma once

#include <windows.h>

#include <objbase.h>

namespace kodak::win {

/// Initialises COM on the calling thread for the lifetime of the object. WIC
/// and ShellExecute need it; the OCR needs the multi-threaded apartment.
class ComApartment {
public:
    enum class Model { SingleThreaded, MultiThreaded };

    explicit ComApartment(Model model = Model::SingleThreaded) noexcept
        : result_(CoInitializeEx(nullptr, model == Model::MultiThreaded ? COINIT_MULTITHREADED
                                                                        : COINIT_APARTMENTTHREADED)) {}
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
