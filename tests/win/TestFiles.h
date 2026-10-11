#pragma once

#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace kodak::test {

/// A fresh, empty folder under %TEMP%, removed with everything in it.
struct TempFolder {
    std::filesystem::path path;

    TempFolder() {
        static int counter = 0;
        path = std::filesystem::temp_directory_path() /
               ("kodak_tests_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(++counter));
        std::error_code error;
        std::filesystem::remove_all(path, error);
        std::filesystem::create_directories(path);
    }
    ~TempFolder() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    TempFolder(const TempFolder&) = delete;
    TempFolder& operator=(const TempFolder&) = delete;
};

inline std::vector<std::uint8_t> readBytes(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline std::string readText(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline void writeText(const std::filesystem::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary) << text;
}

} // namespace kodak::test
