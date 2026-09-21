#include "Presets.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/NativeOptions/OptionsOverlayGeometry.h"
#include <array>
#include <string>
#include <fstream>
#include <ship/config/Config.h>
#include <libultraship/classes.h>
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include <ship/resource/type/Json.h>
#include "soh/OTRGlobals.h"
#include "soh/SohGui/MenuTypes.h"
#include "soh/SohGui/SohMenu.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/Enhancements/randomizer/randomizer_check_tracker.h"
#include "soh/Enhancements/randomizer/randomizer_entrance_tracker.h"
#include "soh/Enhancements/randomizer/randomizer_item_tracker.h"

namespace fs = std::filesystem;

namespace SohGui {
extern std::shared_ptr<SohMenu> mSohMenu;
} // namespace SohGui

struct PresetInfo {
    nlohmann::json presetValues;
    std::string fileName;
    bool apply[PRESET_SECTION_MAX];
    bool isBuiltIn = false;
};

struct BlockInfo {
    std::vector<std::string> sections;
    const char* icon;
    std::string names[2];
};

static std::map<std::string, PresetInfo> presets;
static std::string presetFolder;

static BlockInfo blockInfo[PRESET_SECTION_MAX] = {
    { { CVAR_PREFIX_SETTING, CVAR_PREFIX_WINDOW, CVAR_PREFIX_GAMEPLAY_STATS },
      ICON_FA_COG,
      { "Settings", "settings" } },
    { { CVAR_PREFIX_ENHANCEMENT, CVAR_PREFIX_RANDOMIZER_ENHANCEMENT, CVAR_PREFIX_CHEAT },
      ICON_FA_PLUS_CIRCLE,
      { "Enhancements", "enhancements" } },
    { { CVAR_PREFIX_AUDIO }, ICON_FA_MUSIC, { "Audio", "audio" } },
    { { CVAR_PREFIX_COSMETIC }, ICON_FA_PAINT_BRUSH, { "Cosmetics", "cosmetics" } },
    { { CVAR_PREFIX_RANDOMIZER_SETTING }, ICON_FA_RANDOM, { "Rando Settings", "rando" } },
    { { CVAR_PREFIX_TRACKER }, ICON_FA_MAP, { "Trackers", "trackers" } },
    { { CVAR_PREFIX_REMOTE }, ICON_FA_WIFI, { "Network", "network" } },
};

std::string FormatPresetPath(std::string name) {
    return fmt::format("{}/{}.json", presetFolder, name);
}

void applyPreset(std::string presetName, std::vector<PresetSection> includeSections) {
    auto& info = presets[presetName];
    for (int i = PRESET_SECTION_SETTINGS; i < PRESET_SECTION_MAX; i++) {
        if (info.apply[i] && info.presetValues["blocks"].contains(blockInfo[i].names[1])) {
            if (!includeSections.empty() &&
                std::find(includeSections.begin(), includeSections.end(), i) == includeSections.end()) {
                continue;
            }
            if (i == PRESET_SECTION_TRACKERS) {
                ItemTracker_LoadFromPreset(info.presetValues["blocks"][blockInfo[i].names[1]]["windows"]);
                if (info.presetValues["blocks"][blockInfo[i].names[1]]["windows"].contains("Check Tracker")) {
                    CheckTracker::LoadFromPreset(
                        info.presetValues["blocks"][blockInfo[i].names[1]]["windows"]["Check Tracker"]);
                }
                if (info.presetValues["blocks"][blockInfo[i].names[1]]["windows"].contains("Entrance Tracker")) {
                    EntranceTracker::LoadFromPreset(
                        info.presetValues["blocks"][blockInfo[i].names[1]]["windows"]["Entrance Tracker"]);
                }
            }
            auto section = info.presetValues["blocks"][blockInfo[i].names[1]];
            std::string sectionStrategy = "overwrite";
            if (info.presetValues.contains("blockStrategy") &&
                info.presetValues["blockStrategy"].contains(blockInfo[i].names[1])) {
                sectionStrategy = info.presetValues["blockStrategy"][blockInfo[i].names[1]];
            }

            for (auto& item : section.items()) {
                if (section[item.key()].is_null()) {
                    CVarClearBlock(item.key().c_str());
                } else {
                    auto block = item.value();
                    if (sectionStrategy == "merge") {
                        auto currentJson = Ship::Context::GetInstance()->GetConfig()->GetNestedJson();
                        if (currentJson.contains("CVars") && currentJson["CVars"].contains(item.key())) {
                            block = currentJson["CVars"][item.key()];
                            // Recursively merge the two json objects
                            block.update(item.value(), true);
                        }
                    }

                    Ship::Context::GetInstance()->GetConfig()->SetBlock(fmt::format("{}.{}", "CVars", item.key()),
                                                                        block);
                    Ship::Context::GetInstance()->GetConsoleVariables()->Load();
                }
            }
            if (i == PRESET_SECTION_RANDOMIZER) {
                Rando::Settings::GetInstance()->UpdateAllOptions();
                SohGui::UpdateMenuTricks();
                SohGui::UpdateMenuLocations();
            }
        }
    }
    ShipInit::InitAll();
    OTRGlobals::Instance->ScaleImGui();
}

void ParsePreset(nlohmann::json& json, std::string name) {
    try {
        presets[json["presetName"]].presetValues = json;
        presets[json["presetName"]].fileName = name;
        if (json.contains("isBuiltIn")) {
            presets[json["presetName"]].isBuiltIn = json["isBuiltIn"];
        }
        for (int i = 0; i < PRESET_SECTION_MAX; i++) {
            if (presets[json["presetName"]].presetValues["blocks"].contains(blockInfo[i].names[1])) {
                presets[json["presetName"]].apply[i] = true;
            }
        }
    } catch (...) {}
}

void LoadPresets() {
    if (!presets.empty()) {
        presets.clear();
    }
    if (fs::exists(presetFolder)) {
        for (auto const& preset : fs::directory_iterator(presetFolder)) {
            std::ifstream ifs(preset.path());

            auto json = nlohmann::json::parse(ifs);
            if (!json.contains("presetName")) {
                spdlog::error(fmt::format("Attempted to load file {} as a preset, but was not a preset file.",
                                          preset.path().filename().string()));
            } else {
                ParsePreset(json, preset.path().filename().stem().string());
            }
            ifs.close();
        }
    }
    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(Ship::ResourceType::Json);
    initData->ResourceVersion = 0;
    std::string folder = "presets/*";
    auto builtIns = Ship::Context::GetInstance()->GetResourceManager()->GetArchiveManager()->ListFiles(folder);
    size_t start = std::string(folder).size() - 1;
    for (size_t i = 0; i < builtIns->size(); i++) {
        std::string filePath = builtIns->at(i);
        auto json = std::static_pointer_cast<Ship::Json>(
            Ship::Context::GetInstance()->GetResourceManager()->LoadResource(filePath, true, initData));

        std::string fileName = filePath.substr(start, filePath.size() - start - 5); // 5 for length of ".json"
        ParsePreset(json->Data, fileName);
    }
}

void SavePreset(std::string& presetName) {
    if (!fs::exists(presetFolder)) {
        fs::create_directory(presetFolder);
    }
    presets[presetName].presetValues["presetName"] = presetName;
    presets[presetName].presetValues["fileType"] = FILE_TYPE_PRESET;
    std::ofstream file(
        fmt::format("{}/{}.json", Ship::Context::GetInstance()->LocateFileAcrossAppDirs("presets"), presetName));
    file << presets[presetName].presetValues.dump(4);
    file.close();
    LoadPresets();
}


namespace {
using namespace NativeOptions;

bool ValidPresetName(const std::string& name) {
    return !name.empty() && name != "." && name != ".." &&
           name.find_first_of("<>:\"/\\|?*") == std::string::npos &&
           name.back() != '.' && name.back() != ' ' &&
           std::none_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; });
}

void CapturePreset(std::string newPresetName, const std::array<bool, PRESET_SECTION_MAX>& saveSection) {
    presets[newPresetName] = {};
    Ship::Context::GetInstance()->GetConsoleVariables()->Save();
    auto config = Ship::Context::GetInstance()->GetConfig()->GetNestedJson();
    for (int i = PRESET_SECTION_SETTINGS; i < PRESET_SECTION_MAX; i++) {
        if (saveSection[i]) {
            for (size_t j = 0; j < blockInfo[i].sections.size(); j++) {
                presets[newPresetName].presetValues["blocks"][blockInfo[i].names[1]][blockInfo[i].sections[j]] =
                    config["CVars"][blockInfo[i].sections[j]];
            }
        }
    }
    if (saveSection[PRESET_SECTION_TRACKERS]) {
        auto windows = itemTrackerWindowIDs;
        windows.push_back("Entrance Tracker");
        windows.push_back("Check Tracker");
        for (const auto* name : windows) {
            if (!ImGui::FindWindowByName(name) && !ImGui::FindWindowSettingsByID(ImHashStr(name))) continue;
            const auto layout = ReadOverlayGeometry(name);
            const auto origin = ImGui::GetMainViewport()->Pos;
            auto& saved = presets[newPresetName].presetValues["blocks"][blockInfo[PRESET_SECTION_TRACKERS].names[1]]["windows"][name];
            saved["size"] = {{"width", layout.size.x}, {"height", layout.size.y}};
            saved["pos"] = {{"x", layout.position.x + origin.x}, {"y", layout.position.y + origin.y}};
        }
    }
    presets[newPresetName].fileName = newPresetName;
    std::fill_n(presets[newPresetName].apply, PRESET_SECTION_MAX, true);
    SavePreset(newPresetName);

}

PagePtr NewPresetPage() {
    struct State {
        std::string name;
        std::array<bool, PRESET_SECTION_MAX> sections;
    };
    auto state = std::make_shared<State>();
    state->sections.fill(true);
    return MakePage("presets/new", NativeOptions::Text("new_preset"), [state] {
        std::vector<Row> rows = {
            String("name", NativeOptions::Text("preset_name"), state->name, [state](std::string value) { state->name = value; }, "", 120),
        };
        for (int i = 0; i < PRESET_SECTION_MAX; ++i)
            rows.push_back(Toggle(std::to_string(i), blockInfo[i].names[0], state->sections[i],
                                  [state, i](bool value) { state->sections[i] = value; }));
        auto save = Action("save", NativeOptions::Text("save"), [state] {
            try {
                CapturePreset(state->name, state->sections);
                GetModel().Back();
            } catch (const std::exception& error) {
                Message(NativeOptions::Text("save_failed"), error.what());
            }
        });
        const bool any = std::any_of(state->sections.begin(), state->sections.end(), [](bool value) { return value; });
        if (!ValidPresetName(state->name) || presets.contains(state->name) || !any ||
            CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) {
            save.enabled = false;
            save.disabledReason = CVarGetInteger(CVAR_SETTING("DisableChanges"), 0) ? NativeOptions::Text("race_lockout") :
                                  !ValidPresetName(state->name) ? NativeOptions::Text("preset_invalid_name") :
                                  presets.contains(state->name) ? NativeOptions::Text("preset_exists") : NativeOptions::Text("preset_no_sections");
        }
        rows.push_back(std::move(save));
        return rows;
    });
}

PagePtr PresetPage(const std::string& name) {
    return MakePage("presets/" + name, name, [name] {
        std::vector<Row> rows;
        auto found = presets.find(name);
        if (found == presets.end())
            return rows;
        auto& info = found->second;
        for (int i = 0; i < PRESET_SECTION_MAX; ++i) {
            if (info.presetValues["blocks"].contains(blockInfo[i].names[1]))
                rows.push_back(Toggle(std::to_string(i), blockInfo[i].names[0], info.apply[i],
                                      [name, i](bool value) { presets.at(name).apply[i] = value; }));
        }
        auto apply = Action("apply", NativeOptions::Text("apply"), [name] {
            if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) return;
            applyPreset(name);
            SaveSettings();
            GetModel().Announce(NativeOptions::Text("preset_applied"));
        });
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) {
            apply.enabled = false;
            apply.disabledReason = NativeOptions::Text("race_lockout");
        }
        rows.push_back(std::move(apply));
        if (!info.isBuiltIn) {
            rows.push_back(Action("delete", NativeOptions::Text("delete"), [name] {
                Confirm(NativeOptions::Text("delete_preset"), name, NativeOptions::Text("delete"), [name] {
                    try {
                        const auto path = FormatPresetPath(presets.at(name).fileName);
                        if (fs::exists(path)) fs::remove(path);
                        presets.erase(name);
                        GetModel().Back();
                    } catch (const std::exception& error) {
                        Message(NativeOptions::Text("delete_failed"), error.what());
                    }
                });
            }));
        }
        return rows;
    });
}

PagePtr PresetsPage() {
    return MakePage("presets", NativeOptions::Text("presets"), [] {
        std::vector<Row> rows = {
            Link("new", NativeOptions::Text("new_preset"), NewPresetPage),
            CVarToggle(NativeOptions::Text("hide_builtin"), CVAR_GENERAL("HideBuiltInPresets")),
        };
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) {
            rows[0].enabled = false;
            rows[0].disabledReason = NativeOptions::Text("race_lockout");
        }
        const bool hide = CVarGetInteger(CVAR_GENERAL("HideBuiltInPresets"), 0);
        for (const auto& [name, info] : presets)
            if (!(hide && info.isBuiltIn))
                rows.push_back(Link(name, name, [name] { return PresetPage(name); }));
        return rows;
    });
}
} // namespace

void RegisterPresetsWidgets() {
    NativeOptions::RegisterPage("Presets", PresetsPage, NativeOptions::Text("presets"));
    SohGui::mSohMenu->AddSidebarEntry("Settings", "Presets", 1);
    WidgetPath path = { "Settings", "Presets", SECTION_COLUMN_1 };
    SohGui::mSohMenu->AddWidget(path, "PresetsWidget", WIDGET_CUSTOM)
        .NativePage(PresetsPage, NativeOptions::Text("presets"));
    presetFolder = Ship::Context::GetInstance()->GetPathRelativeToAppDirectory("presets");
    LoadPresets();
}

static RegisterMenuInitFunc menuInitFunc(RegisterPresetsWidgets);
