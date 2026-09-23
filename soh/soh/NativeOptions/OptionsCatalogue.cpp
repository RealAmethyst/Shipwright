#include "NativeOptions.h"
#include "OptionsFormatting.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/SaveManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/audio/spatial/CueActors.h"

extern "C" {
#include "global.h"
#include "overlays/gamestates/ovl_file_choose/file_choose.h"
}

namespace NativeOptions {
namespace {
struct Group {
    std::string id;
    std::string name;
    std::vector<std::shared_ptr<Group>> children;
    std::vector<std::pair<std::string, WidgetInfo*>> widgets;
    RowsProvider nativeRows;
    std::function<void()> onClose;
    std::string hints;
    std::vector<SearchWidget*> searchWidgets;
};

std::shared_ptr<Group> Child(const std::shared_ptr<Group>& parent, const std::string& name) {
    for (const auto& child : parent->children)
        if (child->name == name)
            return child;
    auto child = std::make_shared<Group>();
    child->id = parent->id + "/" + name;
    child->name = name;
    parent->children.push_back(child);
    return child;
}

std::string Category(const std::string& menu, const std::string& sidebar, const WidgetInfo& widget,
                     const std::string& section = "") {
    const std::string cvar = widget.cVar ? widget.cVar : "";
    if (cvar.starts_with(CVAR_SETTING("A11y"))) return "accessibility";
    if (cvar == CVAR_SETTING("ImGuiScale")) return "trackers";
    if (menu == "Dev Tools") return "advanced";
    if (menu == "Network") return "network";
    if (cvar == CVAR_ALLOW_BACKGROUND_INPUTS || cvar == "gSettings.ResetBtn" ||
        section == "Controls" || section == "Camera Fixes") return "controls";
    if (section == "Audio Fixes") return "audio";
    if (section == "Graphical Fixes" || section == "Graphical Restorations") return "display";
    if (sidebar == "Graphics") return "display";
    if (sidebar == "Controls" || sidebar == "Input Viewer") return "controls";
    if (sidebar == "Cosmetics Editor") return "cosmetics";
    if (sidebar == "Audio" || sidebar == "Audio Editor") return "audio";
    if (sidebar.find("Tracker") != std::string::npos || sidebar == "Time Splits" ||
        sidebar == "Timers" || sidebar == "Gameplay Stats") return "trackers";
    if (menu == "Randomizer") return "randomizer";
    if (menu == "Enhancements") return "gameplay";
    return "system";
}

PagePtr GroupPage(const std::shared_ptr<Group>& group) {
    auto page = MakePage(group->id, group->name, [group] {
        std::vector<Row> rows = group->nativeRows ? group->nativeRows() : std::vector<Row>{};
        for (const auto& child : group->children)
            rows.push_back(Link(child->id, child->name, [child] { return GroupPage(child); }));
        for (const auto& [id, widget] : group->widgets)
            AppendWidget(rows, *widget, id);
        return rows;
    });
    page->onClose = group->onClose;
    if (!group->hints.empty()) page->hints += " " + group->hints;
    return page;
}

std::string SearchKey(const std::string& text) {
    std::string result;
    for (unsigned char c : PrepareOptionsText(text)) {
        if (c <= ' ') continue;
        result += static_cast<char>(c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c);
    }
    return result;
}

void SearchGroup(const std::shared_ptr<Group>& group, const std::string& query,
                 std::string path, std::vector<Row>& result) {
    path += (path.empty() ? "" : " / ") + group->name;
    auto append = [&](Row row, const std::string& location, const std::string& terms) {
        if (SearchKey(row.label + " " + row.value + " " + row.description + " " + location + " " + terms).find(query) ==
            std::string::npos) return;
        row.description += (row.description.empty() ? "" : "\n") + location;
        result.push_back(std::move(row));
    };
    if (group->nativeRows)
        for (auto row : group->nativeRows()) append(std::move(row), path, "");
    for (const auto& [id, widget] : group->widgets) {
        std::vector<Row> rows;
        AppendWidget(rows, *widget, id);
        for (auto row : rows) append(std::move(row), path, "");
    }
    for (auto* extra : group->searchWidgets) {
        std::vector<Row> rows;
        AppendWidget(rows, extra->info, extra->menuName + "/" + extra->sidebarName + "/" + extra->info.name);
        std::string location = path;
        if (extra->sidebarName != group->name) location += " / " + extra->sidebarName;
        if (!extra->location.empty()) location += " / " + extra->location;
        for (auto row : rows) append(std::move(row), location, extra->extraTerms);
    }
    for (const auto& child : group->children) SearchGroup(child, query, path, result);
}

PagePtr SearchPage(const std::shared_ptr<Group>& catalogue) {
    auto query = std::make_shared<std::string>();
    auto page = MakePage("search", Text("search"), [query, catalogue] {
        std::vector<Row> rows{String("query", Text("search_query"), *query,
            [query](std::string value) { *query = std::move(value); }, Text("search_help"), 128)};
        const auto key = SearchKey(*query);
        if (!key.empty())
            for (const auto& category : catalogue->children) SearchGroup(category, key, "", rows);
        if (!key.empty() && rows.size() == 1)
            rows.push_back(Action("empty", Text("no_results"), [] {}));
        return rows;
    });
    page->onClose = SpatialAudio::StopCuePreview;
    return page;
}
} // namespace

static Row OriginalChoice(const std::string& id, int value, const std::map<int, std::string>& source,
                          std::function<void(int)> changed) {
    std::string label;
    std::map<int, std::string> choices;
    for (const auto& [number, key] : source) {
        const auto caption = OriginalOptionText(key);
        const auto separator = caption.find(" - ");
        if (separator == std::string::npos)
            continue;
        label = caption.substr(0, separator);
        choices.emplace(number, caption.substr(separator + 3));
    }
    return Choice(id, label, value, choices, changed);
}

PagePtr BuildRoot(const std::string& requestedCategory) {
    auto root = std::make_shared<Group>();
    root->id = "options";
    root->name = Text("title");
    std::map<std::string, std::shared_ptr<Group>> categories;
    for (const char* key : { "accessibility", "audio", "display", "controls", "gameplay", "cosmetics",
                             "randomizer", "trackers", "network", "system", "advanced" })
        categories[key] = Child(root, Text(key));
#ifdef SOH_PRISM
    Child(categories.at("accessibility"), Text("tts_menu"))->nativeRows = [] {
        return std::vector<Row>{
            CVarToggle(Text("tts_enabled"), CVAR_SETTING("A11yTTS"), true, Text("tts_enabled_help")),
            CVarToggle(Text("tts_compass"), CVAR_SETTING("A11yTTSCompass"), true, Text("tts_compass_help")),
            CVarChoice(Text("tts_compass_directions"), CVAR_SETTING("A11yTTSCompassDirections"), 4,
                       {{4, Text("tts_compass_four")}, {8, Text("tts_compass_eight")}}, Text("tts_compass_directions_help")),
            Action("tts/reset", Text("tts_reset"), [] {
                for (const char* setting : {CVAR_SETTING("A11yTTS"), CVAR_SETTING("A11yTTSCompass"),
                                            CVAR_SETTING("A11yTTSCompassDirections")}) {
                    CVarClear(setting);
                    ChangedCVar(setting);
                }
            }, Text("tts_reset_help"))};
    };
#endif
    auto cueGroup = Child(categories.at("accessibility"), Text("audio"));
    cueGroup->onClose = SpatialAudio::StopCuePreview;
    cueGroup->hints = Text("cue_preview_hint");
    cueGroup->nativeRows = [] {
        std::vector<Row> rows;
        for (size_t i = 0; i < SpatialAudio::CueNames.size(); ++i) {
            const char* cue = SpatialAudio::CueNames[i];
            const std::string key = std::string("cue_") + cue;
            const std::string cvar = std::string(CVAR_SETTING("A11yAudio.")) + cue;
            auto preview = [cvar, i] {
                SpatialAudio::PreviewCue(static_cast<SpatialAudio::Cue>(i),
                                         CVarGetInteger((cvar + ".Volume").c_str(), 50));
            };
            auto toggle = CVarToggle(Text(key), cvar + ".Enabled", true, Text(key + "_help"));
            toggle.preview = preview;
            rows.push_back(std::move(toggle));
            auto volume = CVarInteger(Text(key + "_volume"), cvar + ".Volume", 50, 0, 100, 1,
                                      Text("cue_volume_help"), preview);
            volume.preview = preview;
            rows.push_back(std::move(volume));
        }
        const std::string aimCvar = CVAR_SETTING("A11yAudio.aim");
        auto previewAim = [] {
            SpatialAudio::PreviewCue(SpatialAudio::Cue::Pathfinder,
                                     CVarGetInteger(CVAR_SETTING("A11yAudio.aim.Volume"), 50));
        };
        auto aimToggle = CVarToggle(Text("cue_aim"), aimCvar + ".Enabled", true, Text("cue_aim_help"));
        aimToggle.preview = previewAim;
        rows.push_back(std::move(aimToggle));
        auto aimVolume = CVarInteger(Text("cue_aim_volume"), aimCvar + ".Volume", 50, 0, 100, 1,
                                     Text("cue_volume_help"), previewAim);
        aimVolume.preview = previewAim;
        rows.push_back(std::move(aimVolume));
        rows.push_back(Action("cue/reset_all", Text("cue_reset_all"), [] {
            for (const char* cue : SpatialAudio::CueNames) {
                const std::string cvar = std::string(CVAR_SETTING("A11yAudio.")) + cue;
                for (const char* suffix : {".Enabled", ".Volume"}) {
                    const auto setting = cvar + suffix;
                    CVarClear(setting.c_str());
                    ChangedCVar(setting);
                }
            }
            for (const char* suffix : {".Enabled", ".Volume"}) {
                const auto setting = std::string(CVAR_SETTING("A11yAudio.aim")) + suffix;
                CVarClear(setting.c_str());
                ChangedCVar(setting);
            }
        }, Text("cue_reset_all_help")));
        return rows;
    };
    categories.at("audio")->nativeRows = [] {
        return std::vector<Row>{OriginalChoice("original/sound", gSaveContext.audioSetting,
            {{FS_AUDIO_STEREO, "audio_stereo"}, {FS_AUDIO_MONO, "audio_mono"},
             {FS_AUDIO_HEADSET, "audio_headset"}, {FS_AUDIO_SURROUND, "audio_surround"}}, [](int value) {
                gSaveContext.audioSetting = static_cast<uint8_t>(value);
                func_800F6700(gSaveContext.audioSetting);
                if (gSaveContext.audioSetting != value) {
                    Message(Text("unavailable"), Text(value == FS_AUDIO_SURROUND ? "spatial_output_unavailable" : "hrtf_unavailable"));
                    return;
                }
                SaveManager::Instance->SaveGlobal();
            })};
    };
    categories.at("controls")->nativeRows = [] {
        return std::vector<Row>{OriginalChoice("original/target", gSaveContext.zTargetSetting,
            {{FS_TARGET_SWITCH, "target_switch"}, {FS_TARGET_HOLD, "target_hold"}}, [](int value) {
                gSaveContext.zTargetSetting = static_cast<uint8_t>(value);
                SaveManager::Instance->SaveGlobal();
            })};
    };
    categories.at("system")->nativeRows = [] {
        return std::vector<Row>{
            Action("reset", Text("reset"), [] {
                GetModel().Close();
                std::static_pointer_cast<Ship::ConsoleWindow>(
                    Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"))->Dispatch("reset");
            }),
            Action("quit", Text("quit_soh"), [] {
                Confirm(Text("quit_soh"), Text("quit_soh_help"), Text("quit"), [] {
                    GetModel().Close();
                    Ship::Context::GetInstance()->GetWindow()->Close();
                });
            })};
    };
    auto menu = SohGui::GetSohMenu();
    for (const auto& menuName : menu->GetEntryOrder()) {
        auto& entry = menu->GetEntries().at(menuName);
        for (const auto& sidebarName : entry.sidebarOrder) {
            if (sidebarName == "Search")
                continue;
            auto& sidebar = entry.sidebars.at(sidebarName);
            for (auto& column : sidebar.columnWidgets) {
                std::string section;
                for (auto& widget : column) {
                    if (widget.type == WIDGET_SEPARATOR_TEXT) {
                        section = widget.name;
                        continue;
                    }
                    if (widget.type == WIDGET_SEPARATOR || widget.type == WIDGET_SEARCH)
                        continue;
                    auto category = categories.at(Category(menuName, sidebarName, widget, section));
                    auto parent = category;
                    if (sidebarName != "General" && sidebarName != "Audio" && sidebarName != "Graphics" &&
                        sidebarName != "Controls" && sidebarName != "Cosmetics Editor" && sidebarName != "Audio Editor")
                        parent = Child(parent, sidebarName);
                    if (!section.empty() && section != category->name && section != sidebarName &&
                        section != "EXPERIMENTAL" && widget.type != WIDGET_WINDOW_BUTTON)
                        parent = Child(parent, section);
                    parent->widgets.emplace_back(menuName + "/" + sidebarName + "/" + widget.name, &widget);
                }
            }
        }
    }
    for (auto& extra : menu->GetExtraWidgets()) {
        categories.at(Category(extra.menuName, extra.sidebarName, extra.info))->searchWidgets.push_back(&extra);
    }
    std::weak_ptr<Group> weakRoot = root;
    root->nativeRows = [weakRoot] {
        return std::vector<Row>{Link("search", Text("search"), [weakRoot] {
            auto root = weakRoot.lock();
            return root ? SearchPage(root) : nullptr;
        }, Text("search_help"))};
    };
    auto page = GroupPage(requestedCategory.empty() ? root : categories.at(requestedCategory));
    return page;
}

} // namespace NativeOptions
