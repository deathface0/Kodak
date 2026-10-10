#include "kodak/win/SingleInstance.h"

#include <windows.h>

namespace kodak::win {

SingleInstance::SingleInstance(const wchar_t* name) noexcept {
    mutex_ = CreateMutexW(nullptr, FALSE, name);
    first_ = mutex_ != nullptr && GetLastError() != ERROR_ALREADY_EXISTS;
}

SingleInstance::~SingleInstance() {
    if (mutex_) CloseHandle(mutex_);
}

} // namespace kodak::win
