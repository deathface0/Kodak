#include <doctest/doctest.h>

#include <kodak/win/CaptureFiles.h>

#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using kodak::core::DateTime;
using kodak::win::saveCapture;
using kodak::win::writeNewFile;

namespace {

/// A fresh, empty folder under %TEMP%, removed with everything in it.
struct TempFolder {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("kodak_tests_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(GetTickCount64()));
    TempFolder() { std::filesystem::create_directories(path); }
    ~TempFolder() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

std::vector<std::uint8_t> readFile(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("writeNewFile writes a new file") {
    const TempFolder folder;
    const std::vector<std::uint8_t> bytes{1, 2, 3, 4};
    const auto file = folder.path / "new.png";

    CHECK(writeNewFile(file, bytes));
    CHECK(readFile(file) == bytes);
}

TEST_CASE("writeNewFile never touches an existing file") {
    const TempFolder folder;
    const auto file = folder.path / "taken.png";
    const std::vector<std::uint8_t> original{9, 9, 9, 9, 9, 9, 9, 9};
    REQUIRE(writeNewFile(file, original));

    CHECK_FALSE(writeNewFile(file, std::vector<std::uint8_t>{1, 2}));
    CHECK(readFile(file) == original);
}

TEST_CASE("saveCapture adds a suffix when the name is taken") {
    const TempFolder folder;
    const DateTime when{2026, 10, 11, 9, 30, 5};
    const std::vector<std::uint8_t> png{7, 7, 7};

    const auto first = saveCapture(folder.path, png, when);
    const auto second = saveCapture(folder.path, png, when);
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->filename() == "Kodak_20261011_093005.png");
    CHECK(second->filename() == "Kodak_20261011_093005_2.png");
    CHECK(readFile(*second) == png);
}
