#include "NativeSaveEditor.h"
#include "soh/OTRGlobals.h"
#include "soh/SaveManager.h"
#include <algorithm>

extern "C" {
#include "global.h"
#include "message_data_static.h"
extern MessageTableEntry* sGerMessageEntryTablePtr;
extern MessageTableEntry* sFraMessageEntryTablePtr;
extern MessageTableEntry* sJpnMessageEntryTablePtr;
}

extern char z2ASCII(int code);
extern std::string decodeNTSCPlayerNameChar(int code);

namespace NativeSaveEditor {
namespace {
std::string PlayerName() {
    std::string name;
    for (uint8_t value : gSaveContext.playerName)
        name += gSaveContext.ship.filenameLanguage == NAME_LANGUAGE_PAL ?
                    std::string(1, z2ASCII(value)) : decodeNTSCPlayerNameChar(value);
    return name;
}

N::PagePtr NamePage() {
    return N::MakePage("save/name", Text("name"), [] {
        std::vector<N::Row> rows;
        auto name = N::Action("name", Text("name"), [] { N::ReadCurrentDescription(); });
        name.value = PlayerName();
        rows.push_back(std::move(name));
        for (int i = 0; i < 8; ++i)
            rows.push_back(Scalar(std::to_string(i), Text("character") + " " + std::to_string(i + 1), gSaveContext.playerName[i]));
        const bool hasPAL = sGerMessageEntryTablePtr && sFraMessageEntryTablePtr;
        const bool hasNTSC = sJpnMessageEntryTablePtr;
        std::map<int, std::string> languages{{NAME_LANGUAGE_PAL, "PAL"}, {NAME_LANGUAGE_NTSC_JPN, "NTSC JPN"},
                                            {NAME_LANGUAGE_NTSC_ENG, "NTSC ENG"}};
        if (!hasPAL && hasNTSC && gSaveContext.ship.filenameLanguage != NAME_LANGUAGE_PAL) languages.erase(NAME_LANGUAGE_PAL);
        auto language = N::Choice("language", Text("name_language"), gSaveContext.ship.filenameLanguage, languages,
            [](int value) { gSaveContext.ship.filenameLanguage = value; }, Text("name_language_help"));
        language.enabled = hasNTSC && (hasPAL || gSaveContext.ship.filenameLanguage != NAME_LANGUAGE_PAL);
        rows.push_back(std::move(language));
        return rows;
    });
}

N::PagePtr HealthPage() {
    return N::MakePage("save/health", Text("health_magic"), [] {
        std::vector<N::Row> rows;
        rows.push_back(Scalar("maximum", Text("max_health"), gSaveContext.healthCapacity, Text("max_health_help"), {}, [] {
            gSaveContext.health = std::clamp<int16_t>(gSaveContext.health, 0, std::max<int16_t>(0, gSaveContext.healthCapacity));
        }));
        rows.push_back(N::Integer("health", Text("health"), gSaveContext.health, 0, std::max<int>(0, gSaveContext.healthCapacity),
            1, [](int value) { gSaveContext.health = value; }, Text("health_help")));
        rows.push_back(N::Toggle("defense", Text("double_defense"), gSaveContext.isDoubleDefenseAcquired != 0, [](bool value) {
            gSaveContext.isDoubleDefenseAcquired = value;
            gSaveContext.inventory.defenseHearts = value ? 20 : 0;
        }, Text("double_defense_help")));
        rows.push_back(N::Choice("magic_level", Text("magic_level"), gSaveContext.magicLevel,
            {{0, N::Text("none")}, {1, Text("single")}, {2, Text("double")}}, [](int value) {
                gSaveContext.magicLevel = value;
                gSaveContext.isMagicAcquired = value > 0;
                gSaveContext.isDoubleMagicAcquired = value == 2;
                gSaveContext.magicCapacity = value * 0x30;
                gSaveContext.magic = std::clamp<int>(gSaveContext.magic, 0, gSaveContext.magicCapacity);
            }, Text("magic_level_help")));
        rows.push_back(N::Integer("magic", Text("magic"), gSaveContext.magic, 0, std::max<int>(0, gSaveContext.magicCapacity),
            1, [](int value) { gSaveContext.magic = value; }, Text("magic_help")));
        rows.push_back(Scalar("rupees", Text("rupees"), gSaveContext.rupees, Text("rupees_help")));
        return rows;
    });
}

N::PagePtr TimePage() {
    return N::MakePage("save/time", Text("time_timers"), [] {
        std::vector<N::Row> rows{Scalar("time", Text("time"), gSaveContext.dayTime, Text("time_help"))};
        const std::pair<const char*, uint16_t> times[] = {{"dawn", 0x4000}, {"noon", 0x8000}, {"sunset", 0xC001}, {"midnight", 0}};
        for (const auto& [key, value] : times)
            rows.push_back(N::Action(key, Text(key), [value] { gSaveContext.dayTime = value; }));
        rows.push_back(Scalar("days", Text("total_days"), gSaveContext.totalDays, Text("total_days_help")));
        rows.push_back(Scalar("bgs_days", Text("bgs_days"), gSaveContext.bgsDayCount, Text("bgs_days_help")));
        rows.push_back(Scalar("navi", Text("navi_timer"), gSaveContext.naviTimer, Text("navi_timer_help")));
        rows.push_back(Scalar("timer_state", Text("timer_state"), gSaveContext.timerState, Text("timer_state_help")));
        rows.push_back(Scalar("timer_seconds", Text("timer_seconds"), gSaveContext.timerSeconds, Text("seconds_help")));
        rows.push_back(Scalar("sub_state", Text("sub_timer_state"), gSaveContext.subTimerState, Text("sub_timer_state_help")));
        rows.push_back(Scalar("sub_seconds", Text("sub_timer_seconds"), gSaveContext.subTimerSeconds, Text("seconds_help")));
        return rows;
    });
}

N::PagePtr FishingPage() {
    return N::MakePage("save/fishing", Text("fishing"), [] {
        std::vector<N::Row> rows;
        auto& score = gSaveContext.highScores[HS_FISHING];
        for (bool adult : {false, true}) {
            const int shift = adult ? 24 : 0;
            const uint32_t mask = uint32_t{0x7F} << shift;
            const int size = (static_cast<uint32_t>(score) & mask) >> shift;
            rows.push_back(N::Integer(adult ? "adult_size" : "child_size", Text(adult ? "adult_size" : "child_size"), size,
                0, 127, 1, [shift, mask](int value) {
                    auto& score = gSaveContext.highScores[HS_FISHING];
                    score = (static_cast<uint32_t>(score) & ~mask) | (static_cast<uint32_t>(value) << shift);
                }, fmt::format("{} {:.0f} {}", Text("weight"), size * size * 0.0036 + 0.5, Text("pounds"))));
            rows.push_back(Flag(adult ? "adult_cheated" : "child_cheated", Text(adult ? "adult_cheated" : "child_cheated"),
                                score, uint32_t{0x80} << shift, Text("cheated_help")));
        }
        const std::pair<const char*, uint32_t> flags[] = {
            {"child_played", 0x100}, {"adult_played", 0x200}, {"child_prize", 0x400},
            {"adult_prize", 0x800}, {"stole_hat", 0x1000}};
        for (auto [key, mask] : flags)
            rows.push_back(Flag(key, Text(key), score, mask, Text((std::string(key) + "_help").c_str())));
        rows.push_back(N::Integer("times", Text("times_played"), (static_cast<uint32_t>(score) >> 16) & 255, 0, 255, 1,
            [](int value) {
                auto& score = gSaveContext.highScores[HS_FISHING];
                score = (static_cast<uint32_t>(score) & ~uint32_t{0xFF0000}) | (static_cast<uint32_t>(value) << 16);
            }, Text("times_played_help")));
        return rows;
    });
}

N::PagePtr MinigamesPage() {
    return N::MakePage("save/minigames", Text("minigames"), [] {
        std::vector<N::Row> rows;
        const char* names[] = {"horseback_archery", "big_poe_points", "fishing", "obstacle_course",
                              "running_man", "", "dampe_race"};
        for (int i = 0; i < 7; ++i) {
            if (i == HS_FISHING) rows.push_back(N::Link("fishing", Text("fishing"), FishingPage));
            else if (i != 5) rows.push_back(Scalar(names[i], Text(names[i]), gSaveContext.highScores[i]));
        }
        return rows;
    });
}

} // namespace

N::PagePtr InfoPage() {
    return N::MakePage("save/info", Text("info"), [] {
        std::vector<N::Row> rows;
        if (gPlayState && gSaveContext.gameMode == GAMEMODE_NORMAL && gSaveContext.fileNum <= 2)
            rows.push_back(N::Choice("file", Text("file_number"), gSaveContext.fileNum,
                {{0, Text("file_1")}, {1, Text("file_2")}, {2, Text("file_3")}},
                [](int value) { gSaveContext.fileNum = value; }, Text("file_number_help")));
        else {
            const char* state = gSaveContext.gameMode == GAMEMODE_TITLE_SCREEN ? "title_screen" :
                gSaveContext.gameMode == GAMEMODE_FILE_SELECT ? "file_select" : !gPlayState ? "game_inactive" : "debug_file";
            rows.push_back(N::Action("state", Text(state), [] { N::ReadCurrentDescription(); }));
        }
        rows.push_back(N::Link("name", Text("name"), NamePage, PlayerName()));
        rows.push_back(N::Link("health", Text("health_magic"), HealthPage));
        rows.push_back(N::Link("time", Text("time_timers"), TimePage));
        rows.push_back(Scalar("deaths", Text("deaths"), gSaveContext.deaths, Text("deaths_help")));
        rows.push_back(N::Toggle("bgs", Text("has_bgs"), gSaveContext.bgsFlag != 0,
            [](bool value) { gSaveContext.bgsFlag = value; }, Text("has_bgs_help")));
        rows.push_back(Scalar("sword", Text("sword_health"), gSaveContext.swordHealth, Text("sword_health_help")));
        rows.push_back(Scalar("entrance", Text("entrance_index"), gSaveContext.entranceIndex, Text("entrance_index_help")));
        rows.push_back(Scalar("cutscene", Text("cutscene_index"), gSaveContext.cutsceneIndex, Text("cutscene_index_help")));
        rows.push_back(N::Choice("audio", Text("audio"), gSaveContext.audioSetting,
            {{0, Text("stereo")}, {1, Text("mono")}, {2, Text("headset")}, {3, Text("surround")}},
            [](int value) { gSaveContext.audioSetting = value; func_800F6700(value); }, Text("audio_help")));
        rows.push_back(N::Toggle("64dd", Text("64dd"), gSaveContext.n64ddFlag != 0,
            [](bool value) { gSaveContext.n64ddFlag = value; }, Text("64dd_help")));
        rows.push_back(N::Choice("target", Text("target_mode"), gSaveContext.zTargetSetting,
            {{0, Text("switch")}, {1, Text("hold")}}, [](int value) { gSaveContext.zTargetSetting = value; }, Text("target_help")));
        if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_TRIFORCE_HUNT) != RO_TRIFORCE_HUNT_OFF)
            rows.push_back(Scalar("triforce", Text("triforce_pieces"), gSaveContext.ship.quest.data.randomizer.triforcePiecesCollected,
                                  Text("triforce_pieces_help")));
        rows.push_back(N::Link("minigames", Text("minigames"), MinigamesPage));
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
        return rows;
    });
}
}
