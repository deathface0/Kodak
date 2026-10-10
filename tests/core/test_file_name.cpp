#include <doctest/doctest.h>

#include <kodak/core/FileName.h>

using kodak::core::captureFileName;
using kodak::core::DateTime;

TEST_CASE("the capture file name carries the date and time, zero-padded") {
    CHECK(captureFileName(DateTime{2026, 10, 10, 23, 15, 0}) == "Kodak_20261010_231500.png");
    CHECK(captureFileName(DateTime{2027, 1, 2, 3, 4, 5}) == "Kodak_20270102_030405.png");
    CHECK(captureFileName(DateTime{2026, 12, 31, 0, 0, 0}) == "Kodak_20261231_000000.png");
}

TEST_CASE("from the second attempt the name gets a suffix") {
    const DateTime when{2026, 10, 10, 23, 15, 0};
    CHECK(captureFileName(when, 1) == "Kodak_20261010_231500.png");
    CHECK(captureFileName(when, 2) == "Kodak_20261010_231500_2.png");
    CHECK(captureFileName(when, 17) == "Kodak_20261010_231500_17.png");
}
