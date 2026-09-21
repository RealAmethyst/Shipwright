#include "gameplaystats.h"

#include "soh/SaveManager.h"
#include "functions.h"
#include "macros.h"
#include "soh/cvar_prefixes.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/util.h"

#include <vector>
#include <string>
#include <libultraship/bridge.h>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/OTRGlobals.h"

extern "C" {
#include <z64.h>
#include "variables.h"
extern PlayState* gPlayState;
uint64_t GetUnixTimestamp();
}

const char* const sceneMappings[] = {
    "Inside the Deku Tree",
    "Dodongo's Cavern",
    "Inside Jabu-Jabu's Belly",
    "Forest Temple",
    "Fire Temple",
    "Water Temple",
    "Spirit Temple",
    "Shadow Temple",
    "Bottom of the Well",
    "Ice Cavern",
    "Ganon's Tower",
    "Gerudo Training Ground",
    "Thieves' Hideout",
    "Inside Ganon's Castle",
    "Tower Collapse",
    "Castle Collapse",
    "Treasure Box Shop",
    "Gohma's Lair",
    "King Dodongo's Lair",
    "Barinade's Lair",
    "Phantom Ganon's Lair",
    "Volvagia's Lair",
    "Morpha's Lair",
    "Twinrova's Lair",
    "Bongo Bongo's Lair",
    "Ganondorf's Lair",
    "Ganon's Lair",
    "Market Entrance (Day)",
    "Market Entrance (Night)",
    "Market Entrance (Adult)",
    "Back Alley (Day)",
    "Back Alley (Night)",
    "Market (Day)",
    "Market (Night)",
    "Market (Adult)",
    "Outside ToT (Day)",
    "Outside ToT (Night)",
    "Outside ToT (Adult)",
    "Know-It-All Bros' House",
    "Twins' House",
    "Mido's House",
    "Saria's House",
    "Carpenter Boss's House",
    "Man in Green's House",
    "Bazaar",
    "Kokiri Shop",
    "Goron Shop",
    "Zora Shop",
    "Kakariko Potion Shop",
    "Market Potion Shop",
    "Bombchu Shop",
    "Happy Mask Shop",
    "Link's House",
    "Richard's House",
    "Stable",
    "Impa's House",
    "Lakeside Lab",
    "Carpenters' Tent",
    "Gravekeeper's Hut",
    "Great Fairy",
    "Fairy Fountain",
    "Great Fairy",
    "Grotto",
    "Redead Grave",
    "Fairy Fountain Grave",
    "Royal Family's Tomb",
    "Shooting Gallery",
    "Temple of Time",
    "Chamber of Sages",
    "Castle Maze (Day)",
    "Castle Maze (Night)",
    "Cutscene Map",
    "Dampe's Grave",
    "Fishing Pond",
    "Castle Courtyard",
    "Bombchu Bowling Alley",
    "Ranch House",
    "Guard House",
    "Granny's Potion Shop",
    "Ganon Fight",
    "House of Skulltula",
    "Hyrule Field",
    "Kakariko Village",
    "Graveyard",
    "Zora's River",
    "Kokiri Forest",
    "Sacred Forest Meadow",
    "Lake Hylia",
    "Zora's Domain",
    "Zora's Fountain",
    "Gerudo Valley",
    "Lost Woods",
    "Desert Colossus",
    "Gerudo's Fortress",
    "Haunted Wasteland",
    "Hyrule Castle",
    "Death Mountain Trail",
    "Death Mountain Crater",
    "Goron City",
    "Lon Lon Ranch",
    "Outside Ganon's Castle",
    // Debug Rooms
    "Test Map",
    "Test Room",
    "Depth Test",
    "Stalfos Mini-Boss",
    "Stalfos Boss",
    "Dark Link",
    "Castle Maze (Broken)",
    "SRD Room",
    "Chest Room",
};

const char* const countMappings[] = {
    "Anubis:",             // COUNT_ENEMIES_DEFEATED_ANUBIS
    "Armos:",              // COUNT_ENEMIES_DEFEATED_ARMOS
    "Arwing:",             // COUNT_ENEMIES_DEFEATED_ARWING
    "Bari:",               // COUNT_ENEMIES_DEFEATED_BARI
    "Beamos:",             // COUNT_ENEMIES_DEFEATED_BEAMOS
    "Big Octo:",           // COUNT_ENEMIES_DEFEATED_BIG_OCTO
    "Biri:",               // COUNT_ENEMIES_DEFEATED_BIRI
    "Bubble (Green):",     // COUNT_ENEMIES_DEFEATED_BUBBLE_GREEN
    "Bubble (Blue):",      // COUNT_ENEMIES_DEFEATED_BUBBLE_BLUE
    "Bubble (White):",     // COUNT_ENEMIES_DEFEATED_BUBBLE_WHITE
    "Bubble (Red):",       // COUNT_ENEMIES_DEFEATED_BUBBLE_RED
    "Business Scrub:",     // COUNT_ENEMIES_DEFEATED_BUSINESS_SCRUB
    "Dark Link:",          // COUNT_ENEMIES_DEFEATED_DARK_LINK
    "Dead Hand:",          // COUNT_ENEMIES_DEFEATED_DEAD_HAND
    "Deku Baba:",          // COUNT_ENEMIES_DEFEATED_DEKU_BABA
    "Deku Baba (Big):",    // COUNT_ENEMIES_DEFEATED_DEKU_BABA_BIG
    "Deku Scrub:",         // COUNT_ENEMIES_DEFEATED_DEKU_SCRUB
    "Dinolfos:",           // COUNT_ENEMIES_DEFEATED_DINOLFOS
    "Dodongo:",            // COUNT_ENEMIES_DEFEATED_DODONGO
    "Dodongo (Baby):",     // COUNT_ENEMIES_DEFEATED_DODONGO_BABY
    "Door Mimic:",         // COUNT_ENEMIES_DEFEATED_DOOR_TRAP
    "Flare Dancer:",       // COUNT_ENEMIES_DEFEATED_FLARE_DANCER
    "Floormaster:",        // COUNT_ENEMIES_DEFEATED_FLOORMASTER
    "Flying Pot:",         // COUNT_ENEMIES_DEFEATED_FLYING_POT
    "Flying Floor Tile:",  // COUNT_ENEMIES_DEFEATED_FLOOR_TILE
    "Freezard:",           // COUNT_ENEMIES_DEFEATED_FREEZARD
    "Gerudo Thief:",       // COUNT_ENEMIES_DEFEATED_GERUDO_THIEF
    "Gibdo:",              // COUNT_ENEMIES_DEFEATED_GIBDO
    "Gohma Larva:",        // COUNT_ENEMIES_DEFEATED_GOHMA_LARVA
    "Guay:",               // COUNT_ENEMIES_DEFEATED_GUAY
    "Iron Knuckle:",       // COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE
    "Iron Knuckle (Nab):", // COUNT_ENEMIES_DEFEATED_IRON_KNUCKLE_NABOORU
    "Keese:",              // COUNT_ENEMIES_DEFEATED_KEESE
    "Keese (Fire):",       // COUNT_ENEMIES_DEFEATED_KEESE_FIRE
    "Keese (Ice):",        // COUNT_ENEMIES_DEFEATED_KEESE_ICE
    "Leever:",             // COUNT_ENEMIES_DEFEATED_LEEVER
    "Leever (Big):",       // COUNT_ENEMIES_DEFEATED_LEEVER_BIG
    "Like-Like:",          // COUNT_ENEMIES_DEFEATED_LIKE_LIKE
    "Lizalfos:",           // COUNT_ENEMIES_DEFEATED_LIZALFOS
    "Mad Scrub:",          // COUNT_ENEMIES_DEFEATED_MAD_SCRUB
    "Moblin:",             // COUNT_ENEMIES_DEFEATED_MOBLIN
    "Moblin (Club):",      // COUNT_ENEMIES_DEFEATED_MOBLIN_CLUB
    "Octorok:",            // COUNT_ENEMIES_DEFEATED_OCTOROK
    "Parasitic Tentacle:", // COUNT_ENEMIES_DEFEATED_PARASITIC_TENTACLE
    "Peahat:",             // COUNT_ENEMIES_DEFEATED_PEAHAT
    "Peahat Larva:",       // COUNT_ENEMIES_DEFEATED_PEAHAT_LARVA
    "Poe:",                // COUNT_ENEMIES_DEFEATED_POE
    "Poe (Big):",          // COUNT_ENEMIES_DEFEATED_POE_BIG
    "Poe (Composer):",     // COUNT_ENEMIES_DEFEATED_POE_COMPOSER
    "Poe Sisters:",        // COUNT_ENEMIES_DEFEATED_POE_SISTERS
    "Redead:",             // COUNT_ENEMIES_DEFEATED_REDEAD
    "Shabom:",             // COUNT_ENEMIES_DEFEATED_SHABOM
    "Shell Blade:",        // COUNT_ENEMIES_DEFEATED_SHELLBLADE
    "Skulltula:",          // COUNT_ENEMIES_DEFEATED_SKULLTULA
    "Skulltula (Big):",    // COUNT_ENEMIES_DEFEATED_SKULLTULA_BIG
    "Skulltula (Gold):",   // COUNT_ENEMIES_DEFEATED_SKULLTULA_GOLD
    "Skullwalltula:",      // COUNT_ENEMIES_DEFEATED_SKULLWALLTULA
    "Skull Kid:",          // COUNT_ENEMIES_DEFEATED_SKULL_KID
    "Spike:",              // COUNT_ENEMIES_DEFEATED_SPIKE
    "Stalchild:",          // COUNT_ENEMIES_DEFEATED_STALCHILD
    "Stalfos:",            // COUNT_ENEMIES_DEFEATED_STALFOS
    "Stinger:",            // COUNT_ENEMIES_DEFEATED_STINGER
    "Tailpasaran:",        // COUNT_ENEMIES_DEFEATED_TAILPASARAN
    "Tektite (Blue):",     // COUNT_ENEMIES_DEFEATED_TEKTITE_BLUE
    "Tektite (Red):",      // COUNT_ENEMIES_DEFEATED_TEKTITE_RED
    "Torch Slug:",         // COUNT_ENEMIES_DEFEATED_TORCH_SLUG
    "Wallmaster:",         // COUNT_ENEMIES_DEFEATED_WALLMASTER
    "Withered Deku Baba:", // COUNT_ENEMIES_DEFEATED_WITHERED_DEKU_BABA
    "Wolfos:",             // COUNT_ENEMIES_DEFEATED_WOLFOS
    "Wolfos (White):",     // COUNT_ENEMIES_DEFEATED_WOLFOS_WHITE
    "Deku Sticks:",        // COUNT_AMMO_USED_STICK
    "Deku Nuts:",          // COUNT_AMMO_USED_NUT
    "Bombs:",              // COUNT_AMMO_USED_BOMB
    "Arrows:",             // COUNT_AMMO_USED_ARROW
    "Deku Seeds:",         // COUNT_AMMO_USED_SEED
    "Bombchus:",           // COUNT_AMMO_USED_BOMBCHU
    "Beans:",              // COUNT_AMMO_USED_BEAN
    "A:",                  // COUNT_BUTTON_PRESSES_A
    "B:",                  // COUNT_BUTTON_PRESSES_B
    "L:",                  // COUNT_BUTTON_PRESSES_L
    "R:",                  // COUNT_BUTTON_PRESSES_R
    "Z:",                  // COUNT_BUTTON_PRESSES_Z
    "C-Up:",               // COUNT_BUTTON_PRESSES_CUP
    "C-Right:",            // COUNT_BUTTON_PRESSES_CRIGHT
    "C-Down:",             // COUNT_BUTTON_PRESSES_CDOWN
    "C-Left:",             // COUNT_BUTTON_PRESSES_CLEFT
    "D-Up:",               // COUNT_BUTTON_PRESSES_DUP
    "D-Right:",            // COUNT_BUTTON_PRESSES_DRIGHT
    "D-Down:",             // COUNT_BUTTON_PRESSES_DDOWN
    "D-Left:",             // COUNT_BUTTON_PRESSES_DLEFT
    "Start:",              // COUNT_BUTTON_PRESSES_START
};

static char itemTimestampDisplayName[TIMESTAMP_MAX][21] = { "" };

std::string formatTimestampGameplayStat(uint32_t value) {
    uint32_t sec = value / 10;
    uint32_t hh = sec / 3600;
    uint32_t mm = (sec - hh * 3600) / 60;
    uint32_t ss = sec - hh * 3600 - mm * 60;
    uint32_t ds = value % 10;
    return fmt::format("{}:{:0>2}:{:0>2}.{}", hh, mm, ss, ds);
}

std::string formatIntGameplayStat(uint32_t value) {
    return fmt::format("{}", value);
}

std::string formatHexGameplayStat(uint32_t value) {
    return fmt::format("{:#x} ({:d})", value, value);
}

std::string formatHexOnlyGameplayStat(uint32_t value) {
    return fmt::format("{:#x}", value, value);
}

extern "C" char* GameplayStats_GetCurrentTime() {
    std::string timeString = formatTimestampGameplayStat(GAMEPLAYSTAT_TOTAL_TIME).c_str();
    const size_t stringLength = timeString.length();
    char* timeChar = (char*)malloc(stringLength + 1); // We need to use malloc so we can free this from a C file.
    strcpy(timeChar, timeString.c_str());
    return timeChar;
}

void LoadStatsVersion1() {
    SaveManager::Instance->LoadCharArray("buildVersion", gSaveContext.ship.stats.buildVersion,
                                         ARRAY_COUNT(gSaveContext.ship.stats.buildVersion));
    SaveManager::Instance->LoadData("buildVersionMajor", gSaveContext.ship.stats.buildVersionMajor);
    SaveManager::Instance->LoadData("buildVersionMinor", gSaveContext.ship.stats.buildVersionMinor);
    SaveManager::Instance->LoadData("buildVersionPatch", gSaveContext.ship.stats.buildVersionPatch);

    SaveManager::Instance->LoadData("heartPieces", gSaveContext.ship.stats.heartPieces);
    SaveManager::Instance->LoadData("heartContainers", gSaveContext.ship.stats.heartContainers);
    SaveManager::Instance->LoadArray("dungeonKeys", ARRAY_COUNT(gSaveContext.ship.stats.dungeonKeys), [](size_t i) {
        SaveManager::Instance->LoadData("", gSaveContext.ship.stats.dungeonKeys[i]);
    });
    SaveManager::Instance->LoadData("rtaTiming", gSaveContext.ship.stats.rtaTiming);
    SaveManager::Instance->LoadData("firstInput", gSaveContext.ship.stats.firstInput);
    SaveManager::Instance->LoadData("fileCreatedAt", gSaveContext.ship.stats.fileCreatedAt);
    SaveManager::Instance->LoadData("playTimer", gSaveContext.ship.stats.playTimer);
    SaveManager::Instance->LoadData("pauseTimer", gSaveContext.ship.stats.pauseTimer);
    SaveManager::Instance->LoadArray(
        "itemTimestamps", ARRAY_COUNT(gSaveContext.ship.stats.itemTimestamp),
        [](size_t i) { SaveManager::Instance->LoadData("", gSaveContext.ship.stats.itemTimestamp[i]); });
    SaveManager::Instance->LoadArray(
        "sceneTimestamps", ARRAY_COUNT(gSaveContext.ship.stats.sceneTimestamps), [&](size_t i) {
            SaveManager::Instance->LoadStruct("", [&]() {
                int scene, room, sceneTime, roomTime, isRoom;
                SaveManager::Instance->LoadData("scene", scene);
                SaveManager::Instance->LoadData("room", room);
                SaveManager::Instance->LoadData("sceneTime", sceneTime);
                SaveManager::Instance->LoadData("roomTime", roomTime);
                SaveManager::Instance->LoadData("isRoom", isRoom);
                if (scene == 0 && room == 0 && sceneTime == 0 && roomTime == 0 && isRoom == 0) {
                    return;
                }
                gSaveContext.ship.stats.sceneTimestamps[i].scene = scene;
                gSaveContext.ship.stats.sceneTimestamps[i].room = room;
                gSaveContext.ship.stats.sceneTimestamps[i].sceneTime = sceneTime;
                gSaveContext.ship.stats.sceneTimestamps[i].roomTime = roomTime;
                gSaveContext.ship.stats.sceneTimestamps[i].isRoom = isRoom;
            });
        });
    SaveManager::Instance->LoadData("tsIdx", gSaveContext.ship.stats.tsIdx);
    SaveManager::Instance->LoadArray("counts", ARRAY_COUNT(gSaveContext.ship.stats.count), [](size_t i) {
        SaveManager::Instance->LoadData("", gSaveContext.ship.stats.count[i]);
    });
    SaveManager::Instance->LoadArray(
        "scenesDiscovered", ARRAY_COUNT(gSaveContext.ship.stats.scenesDiscovered),
        [](size_t i) { SaveManager::Instance->LoadData("", gSaveContext.ship.stats.scenesDiscovered[i]); });
    SaveManager::Instance->LoadArray(
        "entrancesDiscovered", ARRAY_COUNT(gSaveContext.ship.stats.entrancesDiscovered),
        [](size_t i) { SaveManager::Instance->LoadData("", gSaveContext.ship.stats.entrancesDiscovered[i]); });
}

void SaveStats(SaveContext* saveContext, int sectionID, bool fullSave) {
    SaveManager::Instance->SaveData("buildVersion", saveContext->ship.stats.buildVersion);
    SaveManager::Instance->SaveData("buildVersionMajor", saveContext->ship.stats.buildVersionMajor);
    SaveManager::Instance->SaveData("buildVersionMinor", saveContext->ship.stats.buildVersionMinor);
    SaveManager::Instance->SaveData("buildVersionPatch", saveContext->ship.stats.buildVersionPatch);

    SaveManager::Instance->SaveData("heartPieces", saveContext->ship.stats.heartPieces);
    SaveManager::Instance->SaveData("heartContainers", saveContext->ship.stats.heartContainers);
    SaveManager::Instance->SaveArray("dungeonKeys", ARRAY_COUNT(saveContext->ship.stats.dungeonKeys), [&](size_t i) {
        SaveManager::Instance->SaveData("", saveContext->ship.stats.dungeonKeys[i]);
    });
    SaveManager::Instance->SaveData("rtaTiming", saveContext->ship.stats.rtaTiming);
    SaveManager::Instance->SaveData("firstInput", saveContext->ship.stats.firstInput);
    SaveManager::Instance->SaveData("fileCreatedAt", saveContext->ship.stats.fileCreatedAt);
    SaveManager::Instance->SaveData("playTimer", saveContext->ship.stats.playTimer);
    SaveManager::Instance->SaveData("pauseTimer", saveContext->ship.stats.pauseTimer);
    SaveManager::Instance->SaveArray(
        "itemTimestamps", ARRAY_COUNT(saveContext->ship.stats.itemTimestamp),
        [&](size_t i) { SaveManager::Instance->SaveData("", saveContext->ship.stats.itemTimestamp[i]); });
    SaveManager::Instance->SaveArray(
        "sceneTimestamps", ARRAY_COUNT(saveContext->ship.stats.sceneTimestamps), [&](size_t i) {
            if (saveContext->ship.stats.sceneTimestamps[i].scene != 254 &&
                saveContext->ship.stats.sceneTimestamps[i].room != 254) {
                SaveManager::Instance->SaveStruct("", [&]() {
                    SaveManager::Instance->SaveData("scene", saveContext->ship.stats.sceneTimestamps[i].scene);
                    SaveManager::Instance->SaveData("room", saveContext->ship.stats.sceneTimestamps[i].room);
                    SaveManager::Instance->SaveData("sceneTime", saveContext->ship.stats.sceneTimestamps[i].sceneTime);
                    SaveManager::Instance->SaveData("roomTime", saveContext->ship.stats.sceneTimestamps[i].roomTime);
                    SaveManager::Instance->SaveData("isRoom", saveContext->ship.stats.sceneTimestamps[i].isRoom);
                });
            }
        });
    SaveManager::Instance->SaveData("tsIdx", saveContext->ship.stats.tsIdx);
    SaveManager::Instance->SaveArray("counts", ARRAY_COUNT(saveContext->ship.stats.count), [&](size_t i) {
        SaveManager::Instance->SaveData("", saveContext->ship.stats.count[i]);
    });
    SaveManager::Instance->SaveArray(
        "scenesDiscovered", ARRAY_COUNT(saveContext->ship.stats.scenesDiscovered),
        [&](size_t i) { SaveManager::Instance->SaveData("", saveContext->ship.stats.scenesDiscovered[i]); });
    SaveManager::Instance->SaveArray(
        "entrancesDiscovered", ARRAY_COUNT(saveContext->ship.stats.entrancesDiscovered),
        [&](size_t i) { SaveManager::Instance->SaveData("", saveContext->ship.stats.entrancesDiscovered[i]); });
}

const char* ResolveSceneID(int sceneID, int roomID) {
    if (sceneID == SCENE_GROTTOS) {
        switch (roomID) {
            case 0:
                return "Generic Grotto";
            case 1:
                return "Lake Hylia Scrub Grotto";
            case 2:
                return "Redead Grotto";
            case 3:
                return "Cow Grotto";
            case 4:
                return "Scrub Trio";
            case 5:
                return "Flooded Grotto";
            case 6:
                return "Scrub Duo (Upgrade)";
            case 7:
                return "Wolfos Grotto";
            case 8:
                return "Hyrule Castle Storms Grotto";
            case 9:
                return "Scrub Duo";
            case 10:
                return "Tektite Grotto";
            case 11:
                return "Forest Stage";
            case 12:
                return "Webbed Grotto";
            case 13:
                return "Big Skulltula Grotto";
        };
    } else if (sceneID == SCENE_WINDMILL_AND_DAMPES_GRAVE) {
        // Only the last room of Dampe's Grave (rm 6) is considered the windmill.
        return roomID == 6 ? "Windmill" : "Dampe's Grave";
    } else if (sceneID >= 0 && sceneID < SCENE_ID_MAX) {
        return sceneMappings[sceneID];
    }

    return "";
}

namespace {
namespace N = NativeOptions;

N::Row StatRow(std::string id, std::string label, std::string value, std::string description = "") {
    const auto last = label.find_last_not_of(" :");
    label = last == std::string::npos ? "" : label.substr(0, last + 1);
    auto row = N::Action(std::move(id), std::move(label), [] { N::ReadCurrentDescription(); }, std::move(description));
    row.value = std::move(value);
    return row;
}

N::PagePtr StatsTimestampsPage() {
    return N::MakePage("stats/timestamps", N::Text("stats_timestamps"), [] {
        std::vector<int> entries;
        for (int i = 0; i < TIMESTAMP_MAX; ++i)
            if (gSaveContext.ship.stats.itemTimestamp[i] > 0 && itemTimestampDisplayName[i][0]) entries.push_back(i);
        std::stable_sort(entries.begin(), entries.end(), [](int a, int b) {
            const auto first = gSaveContext.ship.stats.itemTimestamp[a], second = gSaveContext.ship.stats.itemTimestamp[b];
            return CVarGetInteger(CVAR_GAMEPLAY_STATS("ReverseTimestamps"), 0) ? first > second : first < second;
        });
        std::vector<N::Row> rows;
        for (int i : entries)
            rows.push_back(StatRow(std::to_string(i), itemTimestampDisplayName[i],
                                   formatTimestampGameplayStat(gSaveContext.ship.stats.itemTimestamp[i])));
        return rows;
    });
}

uint32_t StatCount(int index) {
    const auto count = gSaveContext.ship.stats.count[index];
    return index == COUNT_ENEMIES_DEFEATED_FLOORMASTER ? count / 3 : count;
}

N::Row CountGroup(const std::string& id, const std::string& title, const std::string& details, int first, int last) {
    uint32_t total = 0;
    for (int i = first; i <= last; ++i) total += StatCount(i);
    auto row = N::Link(id, title, [=] {
        return N::MakePage("stats/" + id, details, [=] {
            std::vector<N::Row> rows;
            for (int i = first; i <= last; ++i)
                rows.push_back(StatRow(std::to_string(i), countMappings[i], formatIntGameplayStat(StatCount(i))));
            return rows;
        });
    });
    row.value = formatIntGameplayStat(total);
    return row;
}

N::PagePtr StatsCountsPage() {
    return N::MakePage("stats/counts", N::Text("stats_counts"), [] {
        std::vector<N::Row> rows;
        rows.push_back(CountGroup("enemies", N::Text("stats_enemies"), N::Text("stats_enemy_details"),
                                  COUNT_ENEMIES_DEFEATED_ANUBIS, COUNT_ENEMIES_DEFEATED_WOLFOS));
        for (int i : {COUNT_RUPEES_COLLECTED, COUNT_RUPEES_SPENT, COUNT_CHESTS_OPENED})
            rows.push_back(StatRow(std::to_string(i), countMappings[i], formatIntGameplayStat(StatCount(i)),
                                   i == COUNT_RUPEES_COLLECTED ? N::Text("stats_rupees_description") : ""));
        rows.push_back(CountGroup("ammo", N::Text("stats_ammo"), N::Text("stats_ammo_details"),
                                  COUNT_AMMO_USED_STICK, COUNT_AMMO_USED_BEAN));
        for (int i : {COUNT_DAMAGE_TAKEN, COUNT_SWORD_SWINGS, COUNT_STEPS})
            rows.push_back(StatRow(std::to_string(i), countMappings[i], formatIntGameplayStat(StatCount(i))));
        if (CVarGetInteger(CVAR_ENHANCEMENT("MMBunnyHood"), BUNNY_HOOD_VANILLA) != BUNNY_HOOD_VANILLA ||
            gSaveContext.ship.stats.count[COUNT_TIME_BUNNY_HOOD] > 0)
            rows.push_back(StatRow("bunny", N::Text("stats_bunny_time"),
                                   formatTimestampGameplayStat(gSaveContext.ship.stats.count[COUNT_TIME_BUNNY_HOOD] / 2)));
        for (int i : {COUNT_ROLLS, COUNT_BONKS, COUNT_SIDEHOPS, COUNT_BACKFLIPS, COUNT_ICE_TRAPS, COUNT_PAUSES,
                      COUNT_POTS_BROKEN, COUNT_BUSHES_CUT})
            rows.push_back(StatRow(std::to_string(i), countMappings[i], formatIntGameplayStat(StatCount(i))));
        rows.push_back(CountGroup("buttons", N::Text("stats_buttons_pressed"), N::Text("stats_buttons"),
                                  COUNT_BUTTON_PRESSES_A, COUNT_BUTTON_PRESSES_START));
        return rows;
    });
}

std::string StatsSceneName(int scene, int room, bool rooms) {
    const auto* resolved = ResolveSceneID(scene, room);
    if (!resolved || !*resolved) return "";
    auto name = std::string(resolved);
    if (rooms && scene != SCENE_GROTTOS) name += " " + N::Text("stats_room") + " " + std::to_string(room);
    return name;
}

N::PagePtr StatsBreakdownPage() {
    return N::MakePage("stats/breakdown", N::Text("stats_breakdown"), [] {
        std::vector<N::Row> rows;
        const bool rooms = CVarGetInteger(CVAR_GAMEPLAY_STATS("RoomBreakdown"), 0);
        const auto count = std::min<size_t>(gSaveContext.ship.stats.tsIdx, ARRAY_COUNT(gSaveContext.ship.stats.sceneTimestamps));
        for (size_t i = 0; i < count; ++i) {
            const auto& entry = gSaveContext.ship.stats.sceneTimestamps[i];
            const auto time = rooms ? entry.roomTime : entry.sceneTime;
            const auto name = StatsSceneName(entry.scene, entry.room, rooms);
            if (time && (rooms || !entry.isRoom) && !name.empty())
                rows.push_back(StatRow(std::to_string(i), name, formatTimestampGameplayStat(time)));
        }
        const auto current = StatsSceneName(gSaveContext.ship.stats.sceneNum, gSaveContext.ship.stats.roomNum, rooms);
        if (!current.empty()) rows.push_back(StatRow("current", current, formatTimestampGameplayStat(CURRENT_MODE_TIMER / 2)));
        return rows;
    });
}

N::PagePtr StatsOptionsPage() {
    return N::MakePage("stats/options", N::Text("title"), [] {
        return std::vector<N::Row>{
            N::CVarToggle(N::Text("stats_show_timer"), CVAR_GAMEPLAY_STATS("ShowIngameTimer"), false, N::Text("stats_timer_description")),
            N::CVarToggle(N::Text("stats_reverse"), CVAR_GAMEPLAY_STATS("ReverseTimestamps")),
            N::CVarToggle(N::Text("stats_rooms"), CVAR_GAMEPLAY_STATS("RoomBreakdown"), false, N::Text("stats_rooms_description")),
            N::CVarToggle(N::Text("stats_rta"), CVAR_GAMEPLAY_STATS("RTATiming"), false, N::Text("stats_rta_description")),
            N::CVarToggle(N::Text("stats_detail_timers"), CVAR_GAMEPLAY_STATS("ShowAdditionalTimers")),
            N::CVarToggle(N::Text("stats_debug"), CVAR_GAMEPLAY_STATS("ShowDebugInfo"))
        };
    });
}

N::PagePtr GameplayStatsPage() {
    return N::MakePage("stats", N::Text("gameplay_stats"), [] {
        std::vector<N::Row> rows;
        if (gGitCommitTag[0] == 0) {
            rows.push_back(StatRow("branch", N::Text("stats_branch"), gGitBranch));
            rows.push_back(StatRow("hash", N::Text("stats_commit"), gGitCommitHash));
        } else rows.push_back(StatRow("version", N::Text("stats_version"), gBuildVersion));
        if (GameInteractor::IsSaveLoaded()) {
            rows.push_back(StatRow("total", N::Text(gSaveContext.ship.stats.rtaTiming ? "stats_total_rta" : "stats_total_game"),
                                   formatTimestampGameplayStat(GAMEPLAYSTAT_TOTAL_TIME)));
            if (CVarGetInteger(CVAR_GAMEPLAY_STATS("ShowAdditionalTimers"), 0)) {
                rows.push_back(StatRow("play", N::Text("stats_play_time"), formatTimestampGameplayStat(gSaveContext.ship.stats.playTimer / 2)));
                rows.push_back(StatRow("pause", N::Text("stats_pause_time"), formatTimestampGameplayStat(gSaveContext.ship.stats.pauseTimer / 3)));
                rows.push_back(StatRow("scene", N::Text("stats_scene_time"), formatTimestampGameplayStat(gSaveContext.ship.stats.sceneTimer / 2)));
                rows.push_back(StatRow("room", N::Text("stats_room_time"), formatTimestampGameplayStat(gSaveContext.ship.stats.roomTimer / 2)));
            }
            if (gPlayState && CVarGetInteger(CVAR_GAMEPLAY_STATS("ShowDebugInfo"), 0)) {
                rows.push_back(StatRow("scene_id", "play->sceneNum", formatHexGameplayStat(gPlayState->sceneNum)));
                rows.push_back(StatRow("entrance", "gSaveContext.entranceIndex", formatHexGameplayStat(gSaveContext.entranceIndex)));
                rows.push_back(StatRow("cutscene", "gSaveContext.cutsceneIndex", formatHexOnlyGameplayStat(gSaveContext.cutsceneIndex)));
                rows.push_back(StatRow("room_id", "play->roomCtx.curRoom.num", formatIntGameplayStat(gPlayState->roomCtx.curRoom.num)));
            }
            rows.push_back(N::Link("timestamps", N::Text("stats_timestamps"), StatsTimestampsPage));
            rows.push_back(N::Link("counts", N::Text("stats_counts"), StatsCountsPage));
            rows.push_back(N::Link("breakdown", N::Text("stats_breakdown"), StatsBreakdownPage));
        }
        rows.push_back(N::Link("options", N::Text("title"), StatsOptionsPage));
        return rows;
    }, N::Text("stats_save_note"));
}
} // namespace

void InitStats(bool isDebug) {
    gSaveContext.ship.stats.heartPieces = isDebug ? 8 : 0;
    gSaveContext.ship.stats.heartContainers = isDebug ? 8 : 0;
    for (int dungeon = 0; dungeon < ARRAY_COUNT(gSaveContext.ship.stats.dungeonKeys); dungeon++) {
        gSaveContext.ship.stats.dungeonKeys[dungeon] = isDebug ? 8 : 0;
    }
    gSaveContext.ship.stats.rtaTiming = CVarGetInteger(CVAR_GAMEPLAY_STATS("RTATiming"), 0);
    gSaveContext.ship.stats.fileCreatedAt = GetUnixTimestamp();
    gSaveContext.ship.stats.firstInput = 0;
    gSaveContext.ship.stats.playTimer = 0;
    gSaveContext.ship.stats.pauseTimer = 0;
    for (int timestamp = 0; timestamp < ARRAY_COUNT(gSaveContext.ship.stats.itemTimestamp); timestamp++) {
        gSaveContext.ship.stats.itemTimestamp[timestamp] = 0;
    }
    for (int timestamp = 0; timestamp < ARRAY_COUNT(gSaveContext.ship.stats.sceneTimestamps); timestamp++) {
        gSaveContext.ship.stats.sceneTimestamps[timestamp].sceneTime = 0;
        gSaveContext.ship.stats.sceneTimestamps[timestamp].roomTime = 0;
        gSaveContext.ship.stats.sceneTimestamps[timestamp].scene = 254;
        gSaveContext.ship.stats.sceneTimestamps[timestamp].room = 254;
        gSaveContext.ship.stats.sceneTimestamps[timestamp].isRoom = 0;
    }
    gSaveContext.ship.stats.tsIdx = 0;
    for (int count = 0; count < ARRAY_COUNT(gSaveContext.ship.stats.count); count++) {
        gSaveContext.ship.stats.count[count] = 0;
    }
    gSaveContext.ship.stats.gameComplete = false;
    for (int scenesIdx = 0; scenesIdx < ARRAY_COUNT(gSaveContext.ship.stats.scenesDiscovered); scenesIdx++) {
        gSaveContext.ship.stats.scenesDiscovered[scenesIdx] = 0;
    }
    for (int entrancesIdx = 0; entrancesIdx < ARRAY_COUNT(gSaveContext.ship.stats.entrancesDiscovered);
         entrancesIdx++) {
        gSaveContext.ship.stats.entrancesDiscovered[entrancesIdx] = 0;
    }

    SohUtils::CopyStringToCharArray(gSaveContext.ship.stats.buildVersion, std::string((char*)gBuildVersion),
                                    ARRAY_COUNT(gSaveContext.ship.stats.buildVersion));
    gSaveContext.ship.stats.buildVersionMajor = gBuildVersionMajor;
    gSaveContext.ship.stats.buildVersionMinor = gBuildVersionMinor;
    gSaveContext.ship.stats.buildVersionPatch = gBuildVersionPatch;
}

// Entries listed here will have a timestamp shown in the stat window
void SetupDisplayNames() {
    // To add a timestamp for an item or event, add it to this list and ensure
    // it has a corresponding entry in the enum (see gameplaystats.h)

    // clang-format off
    strcpy(itemTimestampDisplayName[ITEM_BOW],              "Fairy Bow:          ");
    strcpy(itemTimestampDisplayName[ITEM_ARROW_FIRE],       "Fire Arrows:        ");
    strcpy(itemTimestampDisplayName[ITEM_DINS_FIRE],        "Din's Fire:         ");
    strcpy(itemTimestampDisplayName[ITEM_SLINGSHOT],        "Slingshot:          ");
    strcpy(itemTimestampDisplayName[ITEM_OCARINA_FAIRY],    "Fairy Ocarina:      ");
    strcpy(itemTimestampDisplayName[ITEM_OCARINA_TIME],     "Ocarina of Time:    ");
    strcpy(itemTimestampDisplayName[ITEM_BOMBCHU],          "Bombchus:           ");
    strcpy(itemTimestampDisplayName[ITEM_HOOKSHOT],         "Hookshot:           ");
    strcpy(itemTimestampDisplayName[ITEM_LONGSHOT],         "Longshot:           ");
    strcpy(itemTimestampDisplayName[ITEM_ARROW_ICE],        "Ice Arrows:         ");
    strcpy(itemTimestampDisplayName[ITEM_FARORES_WIND],     "Farore's Wind:      ");
    strcpy(itemTimestampDisplayName[ITEM_BOOMERANG],        "Boomerang:          ");
    strcpy(itemTimestampDisplayName[ITEM_LENS],             "Lens of Truth:      ");
    strcpy(itemTimestampDisplayName[ITEM_HAMMER],           "Megaton Hammer:     ");
    strcpy(itemTimestampDisplayName[ITEM_ARROW_LIGHT],      "Light Arrows:       ");
    strcpy(itemTimestampDisplayName[ITEM_BOTTLE],           "Bottle:             ");
    strcpy(itemTimestampDisplayName[ITEM_LETTER_ZELDA],     "Zelda's Letter:     ");
    strcpy(itemTimestampDisplayName[ITEM_SWORD_KOKIRI],     "Kokiri Sword:       ");
    strcpy(itemTimestampDisplayName[ITEM_SWORD_MASTER],     "Master Sword:       ");
    strcpy(itemTimestampDisplayName[ITEM_SWORD_BGS],        "Biggoron's Sword:   ");
    strcpy(itemTimestampDisplayName[ITEM_SHIELD_DEKU],      "Deku Shield:        ");
    strcpy(itemTimestampDisplayName[ITEM_SHIELD_HYLIAN],    "Hylian Shield:      ");
    strcpy(itemTimestampDisplayName[ITEM_SHIELD_MIRROR],    "Mirror Shield:      ");
    strcpy(itemTimestampDisplayName[ITEM_TUNIC_GORON],      "Goron Tunic:        ");
    strcpy(itemTimestampDisplayName[ITEM_TUNIC_ZORA],       "Zora Tunic:         ");
    strcpy(itemTimestampDisplayName[ITEM_BOOTS_IRON],       "Iron Boots:         ");
    strcpy(itemTimestampDisplayName[ITEM_BOOTS_HOVER],      "Hover Boots:        ");
    strcpy(itemTimestampDisplayName[ITEM_BOMB_BAG_20],      "Bomb Bag:           ");
    strcpy(itemTimestampDisplayName[ITEM_BRACELET],         "Goron's Bracelet:   ");
    strcpy(itemTimestampDisplayName[ITEM_GAUNTLETS_SILVER], "Silver Gauntlets:   ");
    strcpy(itemTimestampDisplayName[ITEM_GAUNTLETS_GOLD],   "Gold Gauntlets:     ");
    strcpy(itemTimestampDisplayName[ITEM_SCALE_SILVER],     "Silver Scale:       ");
    strcpy(itemTimestampDisplayName[ITEM_SCALE_GOLDEN],     "Gold Scale:         ");
    strcpy(itemTimestampDisplayName[ITEM_WALLET_ADULT],     "Adult's Wallet:     ");
    strcpy(itemTimestampDisplayName[ITEM_WALLET_GIANT],     "Giant's Wallet:     ");
    strcpy(itemTimestampDisplayName[ITEM_WEIRD_EGG],        "Weird Egg:          ");
    strcpy(itemTimestampDisplayName[ITEM_GERUDO_CARD],      "Gerudo's Card:      ");
    strcpy(itemTimestampDisplayName[ITEM_COJIRO],           "Cojiro:             ");
    strcpy(itemTimestampDisplayName[ITEM_POCKET_EGG],       "Pocket Egg:         ");
    strcpy(itemTimestampDisplayName[ITEM_MASK_SKULL],       "Skull Mask:         ");
    strcpy(itemTimestampDisplayName[ITEM_MASK_SPOOKY],      "Spooky Mask:        ");
    strcpy(itemTimestampDisplayName[ITEM_MASK_KEATON],      "Keaton Mask:        ");
    strcpy(itemTimestampDisplayName[ITEM_MASK_BUNNY],       "Bunny Hood:         ");
    strcpy(itemTimestampDisplayName[ITEM_ODD_MUSHROOM],     "Odd Mushroom:       ");
    strcpy(itemTimestampDisplayName[ITEM_ODD_POTION],       "Odd Potion:         ");
    strcpy(itemTimestampDisplayName[ITEM_SAW],              "Poacher's Saw:      ");
    strcpy(itemTimestampDisplayName[ITEM_SWORD_BROKEN],     "Broken Goron Sword: ");
    strcpy(itemTimestampDisplayName[ITEM_PRESCRIPTION],     "Prescription:       ");
    strcpy(itemTimestampDisplayName[ITEM_FROG],             "Eyeball Frog:       ");
    strcpy(itemTimestampDisplayName[ITEM_EYEDROPS],         "Eye Drops:          ");
    strcpy(itemTimestampDisplayName[ITEM_CLAIM_CHECK],      "Claim Check:        ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_MINUET],      "Minuet of Forest:   ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_BOLERO],      "Bolero of Fire:     ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_SERENADE],    "Serenade of Water:  ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_REQUIEM],     "Requiem of Spirit:  ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_NOCTURNE],    "Nocturne of Shadow: ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_PRELUDE],     "Prelude of Light:   ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_LULLABY],     "Zelda's Lullaby:    ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_EPONA],       "Epona's Song:       ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_SARIA],       "Saria's Song:       ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_SUN],         "Sun's Song:         ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_TIME],        "Song of Time:       ");
    strcpy(itemTimestampDisplayName[ITEM_SONG_STORMS],      "Song of Storms:     ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_FOREST], "Forest Medallion:   ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_FIRE],   "Fire Medallion:     ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_WATER],  "Water Medallion:    ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_SPIRIT], "Spirit Medallion:   ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_SHADOW], "Shadow Medallion:   ");
    strcpy(itemTimestampDisplayName[ITEM_MEDALLION_LIGHT],  "Light Medallion:    ");
    strcpy(itemTimestampDisplayName[ITEM_KOKIRI_EMERALD],   "Kokiri's Emerald:   ");
    strcpy(itemTimestampDisplayName[ITEM_GORON_RUBY],       "Goron's Ruby:       ");
    strcpy(itemTimestampDisplayName[ITEM_ZORA_SAPPHIRE],    "Zora's Sapphire:    ");
    strcpy(itemTimestampDisplayName[ITEM_KEY_BOSS],         "Ganon's Boss Key:   ");
    strcpy(itemTimestampDisplayName[ITEM_SINGLE_MAGIC],     "Magic:              ");
    strcpy(itemTimestampDisplayName[ITEM_DOUBLE_DEFENSE],   "Double Defense:     ");

    // Other events
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_GOHMA],         "Gohma Defeated:     ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_KING_DODONGO],  "KD Defeated:        ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_BARINADE],      "Barinade Defeated:  ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_PHANTOM_GANON], "PG Defeated:        ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_VOLVAGIA],      "Volvagia Defeated:  ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_MORPHA],        "Morpha Defeated:    ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_BONGO_BONGO],   "Bongo Defeated:     ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_TWINROVA],      "Twinrova Defeated:  ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_GANONDORF],     "Ganondorf Defeated: ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_DEFEAT_GANON],         "Ganon Defeated:     ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_BOSSRUSH_FINISH],      "Boss Rush Finished: ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_FOUND_GREG],           "Greg Found:         ");
    strcpy(itemTimestampDisplayName[TIMESTAMP_TRIFORCE_COMPLETED],   "Triforce Completed: ");
    // clang-format on
}

void InitializeGameplayStats() {
    NativeOptions::RegisterPage("Gameplay Stats", GameplayStatsPage);
    SetupDisplayNames();

    SaveManager::Instance->AddLoadFunction("sohStats", 1, LoadStatsVersion1);
    // Add main section save, no parent.
    SaveManager::Instance->AddSaveFunction("sohStats", 1, SaveStats, true, SECTION_PARENT_NONE);
    // Add subsections, parent of "sohStats". Not sure how to do this without the redundant references to "SaveStats".
    SaveManager::Instance->AddSaveFunction("entrances", 1, SaveStats, false, SECTION_ID_STATS);
    SaveManager::Instance->AddSaveFunction("scenes", 1, SaveStats, false, SECTION_ID_STATS);
    SaveManager::Instance->AddInitFunction(InitStats);
}
