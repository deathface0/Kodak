#pragma once

#include <string>

namespace kodak::core {

/// A local calendar date and time, to the second.
struct DateTime {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
};

/// The file name for a capture taken at `when`: "Kodak_20261010_231500.png".
/// From `attempt` 2 on a suffix is added ("Kodak_20261010_231500_2.png"), for
/// several captures within the same second.
std::string captureFileName(const DateTime& when, int attempt = 1);

} // namespace kodak::core
