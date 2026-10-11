#include "kodak/core/Settings.h"

#include "kodak/core/Utf8.h"

#include <nlohmann/json.hpp>

#include <array>
#include <utility>

namespace kodak::core {

namespace {

constexpr int formatVersion = 1;

/// The detections the user can turn off, by their name in settings.json.
constexpr std::array<std::pair<const char*, FindingKind>, 9> detections{{
    {"email", FindingKind::Email},
    {"card", FindingKind::Card},
    {"iban", FindingKind::Iban},
    {"dniNie", FindingKind::DniNie},
    {"phone", FindingKind::Phone},
    {"ip", FindingKind::Ip},
    {"secret", FindingKind::Secret},
    {"contextValue", FindingKind::ContextValue},
    {"userTerm", FindingKind::UserTerm},
}};

std::u32string trimmed(std::u32string text) {
    const std::size_t first = text.find_first_not_of(U" \t");
    if (first == std::u32string::npos) return {};
    return text.substr(first, text.find_last_not_of(U" \t") - first + 1);
}

} // namespace

std::string toJson(const Settings& settings) {
    nlohmann::ordered_json json;
    json["version"] = formatVersion;
    json["hotkey"] = formatShortcut(settings.hotkey);
    json["saveToFile"] = settings.saveToFile;
    nlohmann::ordered_json detect = nlohmann::ordered_json::object();
    for (const auto& [name, kind] : detections) detect[name] = !settings.disabled.contains(kind);
    json["detect"] = std::move(detect);
    nlohmann::ordered_json terms = nlohmann::ordered_json::array();
    for (const std::u32string& term : settings.userTerms) terms.push_back(toUtf8(term));
    json["userTerms"] = std::move(terms);
    return json.dump(2) + "\n";
}

Settings fromJson(std::string_view text, std::vector<std::string>& warnings) {
    Settings settings;
    const nlohmann::json json = nlohmann::json::parse(text.begin(), text.end(), nullptr, false, true);
    if (json.is_discarded() || !json.is_object()) {
        warnings.emplace_back(
            "settings.json is not valid JSON; using the defaults and leaving the file as it is");
        return settings;
    }

    if (const auto it = json.find("version"); it != json.end() && *it != formatVersion)
        warnings.emplace_back("settings.json has an unknown version; reading the fields Kodak understands");

    if (const auto it = json.find("hotkey"); it != json.end()) {
        const auto parsed = it->is_string() ? parseShortcut(it->get<std::string>()) : std::nullopt;
        if (parsed) {
            settings.hotkey = *parsed;
        } else {
            warnings.emplace_back("\"hotkey\" is not a valid shortcut; using " +
                                  formatShortcut(settings.hotkey));
        }
    }

    if (const auto it = json.find("saveToFile"); it != json.end()) {
        if (it->is_boolean()) {
            settings.saveToFile = it->get<bool>();
        } else {
            warnings.emplace_back("\"saveToFile\" must be true or false; using true");
        }
    }

    if (const auto it = json.find("detect"); it != json.end()) {
        if (!it->is_object()) {
            warnings.emplace_back("\"detect\" must be an object; every detection stays on");
        } else {
            for (const auto& [name, kind] : detections) {
                const auto entry = it->find(name);
                if (entry == it->end()) continue;
                if (!entry->is_boolean()) {
                    warnings.emplace_back(std::string("\"detect.") + name +
                                          "\" must be true or false; it stays on");
                } else if (!entry->get<bool>()) {
                    settings.disabled.insert(kind);
                }
            }
        }
    }

    if (const auto it = json.find("userTerms"); it != json.end()) {
        if (!it->is_array()) {
            warnings.emplace_back("\"userTerms\" must be a list of texts; ignored");
        } else {
            for (const auto& term : *it) {
                if (!term.is_string()) {
                    warnings.emplace_back("an entry of \"userTerms\" is not a text; ignored");
                    continue;
                }
                std::u32string value = trimmed(fromUtf8(term.get<std::string>()));
                if (!value.empty()) settings.userTerms.push_back(std::move(value));
            }
        }
    }
    return settings;
}

} // namespace kodak::core
