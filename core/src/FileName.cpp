#include "kodak/core/FileName.h"

#include <cstdio>

namespace kodak::core {

std::string captureFileName(const DateTime& when, int attempt) {
    char stamp[48];
    std::snprintf(stamp, sizeof(stamp), "%04d%02d%02d_%02d%02d%02d", when.year, when.month, when.day,
                  when.hour, when.minute, when.second);

    std::string name = "Kodak_";
    name += stamp;
    if (attempt >= 2) name += "_" + std::to_string(attempt);
    return name + ".png";
}

} // namespace kodak::core
