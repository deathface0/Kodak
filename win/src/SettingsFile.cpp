#include "kodak/win/SettingsFile.h"

#include "detail/KnownFolder.h"

#include <fstream>
#include <iterator>
#include <system_error>

namespace kodak::win {

std::optional<std::filesystem::path> settingsPath() {
    const auto folder = detail::knownSubfolder(FOLDERID_RoamingAppData, L"Kodak");
    if (!folder) return std::nullopt;
    return *folder / L"settings.json";
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
    bool written = false;
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        written = static_cast<bool>(out << core::toJson(settings) << std::flush);
    }
    if (written &&
        MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return true;
    std::error_code error;
    std::filesystem::remove(temporary, error);
    return false;
}

} // namespace kodak::win
