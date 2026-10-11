#include "kodak/win/SettingsFile.h"

#include <windows.h>

#include <shlobj.h>

#include <fstream>
#include <iterator>
#include <system_error>

namespace kodak::win {

std::optional<std::filesystem::path> settingsPath() {
    PWSTR appData = nullptr;
    const HRESULT result = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &appData);
    if (FAILED(result)) {
        CoTaskMemFree(appData);
        return std::nullopt;
    }
    const std::filesystem::path folder = std::filesystem::path(appData) / L"Kodak";
    CoTaskMemFree(appData);

    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (error || !std::filesystem::is_directory(folder, error)) return std::nullopt;
    return folder / L"settings.json";
}

LoadedSettings loadSettings(const std::filesystem::path& file) {
    LoadedSettings loaded;
    std::error_code error;
    if (!std::filesystem::exists(file, error)) {
        if (!saveSettings(file, loaded.settings))
            loaded.warnings.emplace_back("could not create settings.json; using the defaults");
        return loaded;
    }

    std::ifstream in(file, std::ios::binary);
    std::string text{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    if (!in.good() && !in.eof()) {
        loaded.warnings.emplace_back("could not read settings.json; using the defaults");
        return loaded;
    }
    if (text.starts_with("\xEF\xBB\xBF")) text.erase(0, 3); // a BOM left by some editors

    loaded.settings = core::fromJson(text, loaded.warnings);
    return loaded;
}

bool saveSettings(const std::filesystem::path& file, const core::Settings& settings) {
    std::filesystem::path temporary = file;
    temporary += L".tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out << core::toJson(settings);
        out.flush();
        if (!out) {
            std::error_code error;
            std::filesystem::remove(temporary, error);
            return false;
        }
    }
    if (MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return true;
    std::error_code error;
    std::filesystem::remove(temporary, error);
    return false;
}

} // namespace kodak::win
