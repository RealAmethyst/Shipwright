#include "SohMenu.h"
#include "soh/NativeOptions/NativeOptions.h"
#include <charconv>
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/SohGui.hpp"

extern "C" {
#include "variables.h"
}

namespace SohGui {

extern std::shared_ptr<SohMenu> mSohMenu;
using namespace UIWidgets;

static const std::map<int32_t, const char*> skipGetItemAnimationOptions = {
    { SGIA_DISABLED, "Disabled" },
    { SGIA_JUNK, "Junk Items" },
    { SGIA_ALL, "All Items" },
};

static char seedString[MAX_SEED_STRING_SIZE];
static std::set<RandomizerCheck> excludedLocations;
static std::set<RandomizerTrick> enabledTricks;

void SaveEnabledTricks() {
    std::string enabledTrickString = "";
    for (auto enabledTrickIt : enabledTricks) {
        enabledTrickString += Rando::Settings::GetInstance()->GetTrickSetting(enabledTrickIt).GetNameTag();
        enabledTrickString += ",";
    }
    if (enabledTricks.size() == 0) {
        CVarClear(CVAR_RANDOMIZER_SETTING("EnabledTricks"));
    } else {
        CVarSetString(CVAR_RANDOMIZER_SETTING("EnabledTricks"), enabledTrickString.c_str());
    }
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    return;
}

void UpdateMenuLocations() {
    RandomizerCheckObjects::UpdateImGuiVisibility();
    // todo: this efficiently when we build out cvar array support
    std::stringstream excludedLocationStringStream(CVarGetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"), ""));
    std::string excludedLocationString;
    excludedLocations.clear();
    while (getline(excludedLocationStringStream, excludedLocationString, ',')) {
        if (!excludedLocationString.empty()) {
            int value = 0;
            const auto parsed = std::from_chars(excludedLocationString.data(), excludedLocationString.data() + excludedLocationString.size(), value);
            if (parsed.ec == std::errc{} && parsed.ptr == excludedLocationString.data() + excludedLocationString.size() &&
                value > RC_UNKNOWN_CHECK && value < RC_MAX)
                excludedLocations.insert(static_cast<RandomizerCheck>(value));
        }
    }
}

void UpdateMenuTricks() {
    // RandomizerTricks::UpdateImGuiVisibility();
    //  todo: this efficiently when we build out cvar array support
    std::stringstream enabledTrickStringStream(CVarGetString(CVAR_RANDOMIZER_SETTING("EnabledTricks"), ""));
    std::string enabledTrickString;
    enabledTricks.clear();
    while (getline(enabledTrickStringStream, enabledTrickString, ',')) {
        if (Rando::StaticData::trickToEnum.contains(enabledTrickString)) {
            enabledTricks.insert(Rando::StaticData::trickToEnum[enabledTrickString]);
        }
    }

}


namespace {
namespace N = NativeOptions;

void GuardRandomizerRows(std::vector<N::Row>& rows, bool requireLogic = false) {
    if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0))
        N::Disable(rows, N::Text("race_lockout"));
    else if (CVarGetInteger(CVAR_GENERAL("RandoGenerating"), 0) || CVarGetInteger(CVAR_GENERAL("OnFileSelectNameEntry"), 0))
        N::Disable(rows, N::Text("randomizer_busy"));
    else if (requireLogic && CVarGetInteger(CVAR_RANDOMIZER_SETTING("LogicRules"), RO_LOGIC_GLITCHLESS) == RO_LOGIC_NO_LOGIC)
        N::Disable(rows, N::Text("requires_logic"));
}

void SaveExcludedLocations() {
    std::string value;
    for (auto location : excludedLocations)
        value += (value.empty() ? "" : ",") + std::to_string(location);
    if (value.empty()) CVarClear(CVAR_RANDOMIZER_SETTING("ExcludedLocations"));
    else CVarSetString(CVAR_RANDOMIZER_SETTING("ExcludedLocations"), value.c_str());
    N::SaveSettings();
}

N::PagePtr LocationsPage() {
    UpdateMenuLocations();
    auto search = std::make_shared<std::string>();
    return N::MakePage("randomizer/locations", N::Text("excluded_locations"), [=] {
        std::vector<N::Row> rows;
        rows.push_back(N::String("filter", N::Text("filter"), *search, [=](std::string value) { *search = value; }));
        for (bool excluded : {false, true}) {
            const auto section = excluded ? "excluded" : "included";
            rows.push_back(N::Link(section, N::Text(section), [=] {
                return N::MakePage(std::string("randomizer/locations/") + section, N::Text(section), [=] {
                    RandomizerCheckObjects::UpdateImGuiVisibility();
                    ImGuiTextFilter filter(search->c_str());
                    std::vector<N::Row> areas;
                    for (const auto& [area, locations] : RandomizerCheckObjects::GetAllRCObjectsByArea()) {
                        const bool hasItems = std::any_of(locations.begin(), locations.end(), [&](auto rc) {
                            return OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->IsVisible() &&
                                   excludedLocations.contains(rc) == excluded &&
                                   filter.PassFilter(Rando::StaticData::GetLocation(rc)->GetName().c_str());
                        });
                        if (!hasItems) continue;
                        areas.push_back(N::Link(std::to_string(area), RandomizerCheckObjects::GetRCAreaName(area), [=] {
                            return N::MakePage("randomizer/location-area/" + std::to_string(area),
                                RandomizerCheckObjects::GetRCAreaName(area), [=] {
                                    RandomizerCheckObjects::UpdateImGuiVisibility();
                                    ImGuiTextFilter filter(search->c_str());
                                    std::vector<N::Row> result;
                                    for (auto rc : locations) {
                                        const auto location = Rando::StaticData::GetLocation(rc);
                                        if (!OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->IsVisible() ||
                                            excludedLocations.contains(rc) != excluded || !filter.PassFilter(location->GetName().c_str()))
                                            continue;
                                        result.push_back(N::Action(std::to_string(rc), location->GetShortName(), [=] {
                                            if (excluded) excludedLocations.erase(rc);
                                            else excludedLocations.insert(rc);
                                            SaveExcludedLocations();
                                        }, N::Text(excluded ? "include_location" : "exclude_location")));
                                    }
                                    GuardRandomizerRows(result);
                                    return result;
                                });
                        }));
                    }
                    GuardRandomizerRows(areas);
                    return areas;
                });
            }));
        }
        GuardRandomizerRows(rows);
        return rows;
    });
}

struct TrickFilters {
    std::string search;
    std::map<Rando::Tricks::Tag, bool> tags{
        {Rando::Tricks::Tag::NOVICE, true}, {Rando::Tricks::Tag::INTERMEDIATE, true},
        {Rando::Tricks::Tag::ADVANCED, true}, {Rando::Tricks::Tag::EXPERT, true},
        {Rando::Tricks::Tag::EXTREME, true}, {Rando::Tricks::Tag::EXPERIMENTAL, true},
        {Rando::Tricks::Tag::GLITCH, false}};
    std::map<RandomizerArea, bool> areas;
};

bool MatchesTrick(RandomizerTrick id, const TrickFilters& filters, const ImGuiTextFilter& search) {
    const auto option = Rando::Settings::GetInstance()->GetTrickSetting(id);
    const auto area = filters.areas.find(option.GetArea());
    return !option.IsHidden() && (area == filters.areas.end() || area->second) &&
           search.PassFilter(option.GetName().c_str()) && Rando::Tricks::CheckTags(filters.tags, option.GetTags());
}

N::PagePtr TricksPage() {
    UpdateMenuTricks();
    auto filters = std::make_shared<TrickFilters>();
    for (const auto& [area, tricks] : Rando::Settings::GetInstance()->mTricksByArea)
        filters->areas.emplace(area, true);
    return N::MakePage("randomizer/tricks", N::Text("tricks_glitches"), [=] {
        std::vector<N::Row> rows;
        rows.push_back(N::String("filter", N::Text("filter"), filters->search,
                                  [=](std::string value) { filters->search = value; }));
        rows.push_back(N::Link("tags", N::Text("trick_tags"), [=] {
            return N::MakePage("randomizer/trick-tags", N::Text("trick_tags"), [=] {
                std::vector<N::Row> tags;
                for (const auto& [tag, shown] : filters->tags)
                    tags.push_back(N::Toggle(std::to_string(static_cast<int>(tag)), Rando::Tricks::GetTagName(tag), shown,
                                             [=](bool value) { filters->tags[tag] = value; }));
                return tags;
            });
        }));
        rows.push_back(N::Link("areas", N::Text("trick_areas"), [=] {
            return N::MakePage("randomizer/trick-areas", N::Text("trick_areas"), [=] {
                std::vector<N::Row> areas;
                for (const auto& [area, shown] : filters->areas)
                    areas.push_back(N::Toggle(std::to_string(area), Rando::Tricks::GetAreaName(area), shown,
                                              [=](bool value) { filters->areas[area] = value; }));
                for (bool show : {true, false})
                    areas.push_back(N::Action(show ? "show_all" : "hide_all", N::Text(show ? "show_all" : "hide_all"), [=] {
                        for (auto& [area, shown] : filters->areas) shown = show;
                    }));
                return areas;
            });
        }));
        for (bool enabled : {false, true}) {
            const auto section = enabled ? "enabled_tricks" : "disabled_tricks";
            rows.push_back(N::Link(section, N::Text(section), [=] {
                return N::MakePage(std::string("randomizer/") + section, N::Text(section), [=] {
                    std::vector<N::Row> areas;
                    ImGuiTextFilter search(filters->search.c_str());
                    for (const auto& [area, tricks] : Rando::Settings::GetInstance()->mTricksByArea) {
                        if (!std::any_of(tricks.begin(), tricks.end(), [&](auto id) {
                            return enabledTricks.contains(id) == enabled && MatchesTrick(id, *filters, search);
                        })) continue;
                        areas.push_back(N::Link(std::to_string(area), Rando::Tricks::GetAreaName(area), [=] {
                            return N::MakePage("randomizer/trick-area/" + std::to_string(area), Rando::Tricks::GetAreaName(area), [=] {
                                std::vector<N::Row> result;
                                ImGuiTextFilter search(filters->search.c_str());
                                for (auto id : tricks) {
                                    if (enabledTricks.contains(id) != enabled || !MatchesTrick(id, *filters, search)) continue;
                                    const auto option = Rando::Settings::GetInstance()->GetTrickSetting(id);
                                    auto row = N::Toggle(std::to_string(id), option.GetName(), enabled, [=](bool value) {
                                        if (value) enabledTricks.insert(id); else enabledTricks.erase(id);
                                        SaveEnabledTricks();
                                    }, option.GetDescription());
                                    for (auto tag : option.GetTags())
                                        row.description += " " + Rando::Tricks::GetTagName(tag);
                                    result.push_back(std::move(row));
                                }
                                GuardRandomizerRows(result, true);
                                return result;
                            });
                        }));
                    }
                    GuardRandomizerRows(areas, true);
                    return areas;
                });
            }));
        }
        for (bool visibleOnly : {true, false})
            for (bool enable : {true, false}) {
                const auto key = std::string(enable ? "enable_" : "disable_") + (visibleOnly ? "visible" : "all");
                rows.push_back(N::Action(key, N::Text(key), [=] {
                    ImGuiTextFilter search(filters->search.c_str());
                    for (int i = 0; i < RT_MAX; ++i) {
                        const auto id = static_cast<RandomizerTrick>(i);
                        if (visibleOnly && !MatchesTrick(id, *filters, search)) continue;
                        if (enable) enabledTricks.insert(id); else enabledTricks.erase(id);
                    }
                    SaveEnabledTricks();
                }));
            }
        GuardRandomizerRows(rows, true);
        return rows;
    });
}

N::PagePtr SeedPage() {
    return N::MakePage("randomizer/seed", N::Text("seed"), [] {
        auto validate = [](const std::string& text) {
            return std::all_of(text.begin(), text.end(), [](unsigned char ch) {
                return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
            }) ? std::string{} : N::Text("seed_characters");
        };
        std::vector<N::Row> rows{
            N::String("value", N::Text("seed"), seedString, [](std::string value) {
                SohUtils::CopyStringToCharArray(seedString, value, MAX_SEED_STRING_SIZE);
            }, N::Text("seed_help"), MAX_SEED_STRING_SIZE - 1, validate),
            N::Action("random", N::Text("random_seed"), [] {
                SohUtils::CopyStringToCharArray(seedString, std::to_string(rand() & 0xFFFFFFFF), MAX_SEED_STRING_SIZE);
            }),
            N::Action("clear", N::Text("clear"), [] { memset(seedString, 0, MAX_SEED_STRING_SIZE); })};
        GuardRandomizerRows(rows);
        return rows;
    });
}

N::PagePtr SpoilerPage() {
    return N::MakePage("randomizer/spoiler", N::Text("spoiler_file"), [] {
        auto row = N::Action("path", N::Text("spoiler_file"), [] { N::ReadCurrentDescription(); });
        row.value = CVarGetString(CVAR_GENERAL("SpoilerLog"), "");
        if (row.value.empty()) row.value = N::Text("none");
        return std::vector<N::Row>{row};
    });
}
}

void SohMenu::AddMenuRandomizer() {
    // Add Randomizer Menu
    AddMenuEntry("Randomizer", CVAR_SETTING("Menu.RandomizerSidebarSection"));

    // Seed Settings
    WidgetPath path = { "Randomizer", "General", SECTION_COLUMN_1 };
    AddSidebarEntry("Randomizer", path.sidebarName, 2);
    AddWidget(path,
              "Be sure to explore the Presets and Enhancements Menus for various Speedups and Quality of life changes!",
              WIDGET_TEXT)
        .Options(TextOptions().Color(UIWidgets::Colors::Gray));
    AddWidget(path, "Seed Entry", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Manual seed entry", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"))
        .Options(CheckboxOptions().DefaultValue(true));
    AddWidget(path, "Seed", WIDGET_CUSTOM)
        .NativePage(SeedPage, NativeOptions::Text("seed"))
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 1); });
    AddWidget(path, "Generate Randomizer", WIDGET_BUTTON)
        .Callback([](WidgetInfo& info) {
            OTRGlobals::Instance->gRandoContext->SetSpoilerLoaded(false);
            GenerateRandomizer(CVarGetInteger(CVAR_RANDOMIZER_SETTING("ManualSeedEntry"), 0) ? seedString : "");
        })
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled = (gSaveContext.gameMode != GAMEMODE_FILE_SELECT) || GameInteractor::IsSaveLoaded();
        })
        .Options(ButtonOptions()
                     .Size(ImVec2(250.f, 0.f))
                     .DisabledTooltip("Must be on File Select to generate a randomizer seed."));
    AddWidget(path, "Spoiler File", WIDGET_CUSTOM)
        .NativePage(SpoilerPage, NativeOptions::Text("spoiler_file"))
        .PreFunc([](WidgetInfo& info) { info.isHidden = CVarGetInteger(CVAR_RANDOMIZER_SETTING("DontGenerateSpoiler"), 0); });

    // Enhancements
    AddWidget(path, "Enhancements", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "These enhancements are only useful in the Randomizer mode but do not affect the randomizer logic.",
              WIDGET_TEXT)
        .Options(TextOptions().Color(UIWidgets::Colors::Gray));
    AddWidget(path, "Rando-Relevant Navi Hints", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("RandoRelevantNavi"))
        .Options(CheckboxOptions()
                     .Tooltip("Replace Navi's overworld quest hints with rando-related gameplay hints.")
                     .DefaultValue(true));
    AddWidget(path, "Random Rupee Names", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"))
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("When obtaining Rupees, randomize what the Rupee is called in the textbox.")
                     .DefaultValue(true));
    AddWidget(path, "Use Custom Key Models", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("CustomKeyModels"))
        .Options(
            CheckboxOptions()
                .Tooltip("Use Custom graphics for Dungeon Keys, Big and Small, so that they can be easily told apart.")
                .DefaultValue(true));
    AddWidget(path, "Map & Compass Colors Match Dungeon", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("ColoredMapsAndCompasses"))
        .Options(
            CheckboxOptions()
                .Tooltip("Matches the color of maps & compasses to the dungeon they belong to. "
                         "This helps identify maps & compasses from afar and adds a little bit of flair.\n\nThis only "
                         "applies to seeds with maps & compasses shuffled to \"Any Dungeon\", \"Overworld\", or "
                         "\"Anywhere\".")
                .DefaultValue(true));
    AddWidget(path, "Jabber Nut Colors Match Kind", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("GenericJabberNutModel"))
        .RaceDisable(false)
        .Options(CheckboxOptions()
                     .Tooltip("With Shuffle Speak, jabber nut model & color will be generic.")
                     .DefaultValue(true));
    AddWidget(path, "Quest Item Fanfares", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("QuestItemFanfares"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "Play unique fanfares when obtaining quest items (medallions/stones/songs). Note that these "
            "fanfares can be longer than usual."));
    AddWidget(path, "Mysterious Shuffled Items", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("MysteriousShuffle"))
        .Options(CheckboxOptions().Tooltip(
            "Displays a \"Mystery Item\" model in place of any freestanding/GS/shop items that were shuffled, "
            "and replaces item names for them and scrubs and merchants, regardless of hint settings, "
            "so you never know what you're getting."));
    AddWidget(path, "Simpler Boss Soul Models", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("SimplerBossSoulModels"))
        .RaceDisable(false)
        .Options(CheckboxOptions().Tooltip(
            "When shuffling boss souls, they'll appear as a simpler model instead of showing the boss' models."
            "This might make boss souls more distinguishable from a distance, and can help with performance."));
    AddWidget(path, "Skip Get Item Animations", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimation"))
        .Options(ComboboxOptions().ComboMap(skipGetItemAnimationOptions).DefaultIndex(SGIA_JUNK));
    AddWidget(path, "Item Scale: %.2f", WIDGET_CVAR_SLIDER_FLOAT)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimationScale"))
        .PreFunc([](WidgetInfo& info) {
            info.options->disabled =
                !CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("TimeSavers.SkipGetItemAnimation"), SGIA_JUNK);
            info.options->disabledTooltip =
                "This slider only applies when using the \"Skip Get Item Animations\" option.";
        })
        .Options(FloatSliderOptions().Min(5.0f).Max(15.0f).Format("%.2f").DefaultValue(10.0f).Tooltip(
            "The size of the item when it is picked up."));
    AddWidget(path, "Signs Hint Entrances", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_RANDOMIZER_ENHANCEMENT("EntrancesOnSigns"))
        .Options(CheckboxOptions().Tooltip("If enabled, signs near loading zones will tell you where they lead to."));

    auto randoSettings = Rando::Settings::GetInstance();
    randoSettings->CreateOptions();
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_LOGIC_ACCESS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_DUNGEONS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_SHUFFLES).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_HINTS_TRAPS).AddWidgets(path);
    randoSettings->GetOptionGroup(RSG_MENU_SIDEBAR_STARTING_ITEMS).AddWidgets(path);
    path.sidebarName = "Locations";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Excluded Locations", WIDGET_CUSTOM).NativePage(LocationsPage, NativeOptions::Text("excluded_locations"));
    path.sidebarName = "Tricks/Glitches";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Tricks/Glitches", WIDGET_CUSTOM).NativePage(TricksPage, NativeOptions::Text("tricks_glitches"));

    // Plandomizer
    path.sidebarName = "Plandomizer";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);
    AddWidget(path, "Popout Plandomizer Window", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("PlandomizerEditor"))
        .RaceDisable(false)
        .WindowName("Plandomizer Editor")
        .Options(WindowButtonOptions().Tooltip("Enables the separate Randomizer Settings Window."));

    // Item Tracker
    path.sidebarName = "Item Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Item Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Item Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ItemTracker"))
        .RaceDisable(false)
        .WindowName("Item Tracker")
        .Options(WindowButtonOptions().Tooltip("Toggles the Item Tracker.").EmbedWindow(false));

    AddWidget(path, "Item Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Item Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ItemTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Item Tracker Settings")
        .Options(WindowButtonOptions().Tooltip("Enables the separate Item Tracker Settings Window."));

    // Entrance Tracker
    path.sidebarName = "Entrance Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Entrance Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Entrance Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("EntranceTracker"))
        .RaceDisable(false)
        .WindowName("Entrance Tracker")
        .Options(WindowButtonOptions().Tooltip("Toggles the Entrance Tracker.").EmbedWindow(false));

    AddWidget(path, "Entrance Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Entrance Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("EntranceTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Entrance Tracker Settings")
        .Options(WindowButtonOptions().Tooltip("Enables the separate Entrance Tracker Settings Window."));

    // Check Tracker
    path.sidebarName = "Check Tracker";
    AddSidebarEntry("Randomizer", path.sidebarName, 1);

    AddWidget(path, "Check Tracker", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Toggle Check Tracker", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("CheckTracker"))
        .RaceDisable(false)
        .WindowName("Check Tracker")
        .Options(WindowButtonOptions().Tooltip("Toggles the Check Tracker.").EmbedWindow(false));

    AddWidget(path, "Check Tracker Settings", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Popout Check Tracker Settings", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("CheckTrackerSettings"))
        .RaceDisable(false)
        .WindowName("Check Tracker Settings")
        .Options(WindowButtonOptions().Tooltip("Enables the separate Check Tracker Settings Window."));
}

} // namespace SohGui
