#include "NativeSaveEditor.h"
#include "debugSaveEditor.h"
#include "soh/OTRGlobals.h"
#include "soh/util.h"
#include <span>
#include <bit>

extern "C" {
#include "global.h"
extern u8 gAreaGsFlags[];
}
extern std::vector<const char*> gsMapping;

namespace NativeSaveEditor {
namespace {
uint64_t sceneRevision = 0;
bool keepGsCountUpdated = true;

std::span<uint16_t> TableEntries(FlagTableType type) {
    switch (type) {
        case EVENT_CHECK_INF: return gSaveContext.eventChkInf;
        case ITEM_GET_INF: return gSaveContext.itemGetInf;
        case INF_TABLE: return gSaveContext.infTable;
        case EVENT_INF: return gSaveContext.eventInf;
        case RANDOMIZER_INF: return gSaveContext.ship.randomizerInf;
        default: return {};
    }
}

N::PagePtr TablePage(size_t tableIndex) {
    auto filterText = std::make_shared<std::string>();
    return N::MakePage("save/table/" + std::to_string(tableIndex), flagTables.at(tableIndex).name, [tableIndex, filterText] {
        const auto& table = flagTables.at(tableIndex);
        std::vector<N::Row> rows{N::String("filter", N::Text("filter"), *filterText,
                                         [filterText](std::string value) { *filterText = std::move(value); })};
        if (table.flagTableType == RANDOMIZER_INF) {
            const auto mode = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY);
            if (mode != RO_FISHSANITY_OFF && mode != RO_FISHSANITY_OVERWORLD) {
                for (bool adult : {false, true}) for (bool caught : {true, false}) {
                    const auto key = adult ? (caught ? "catch_adult" : "uncatch_adult") : (caught ? "catch_child" : "uncatch_child");
                    rows.push_back(N::Action(key, Text(key), [adult, caught] {
                        for (int bit = adult ? RAND_INF_ADULT_FISH_1 : RAND_INF_CHILD_FISH_1;
                             bit <= (adult ? RAND_INF_ADULT_LOACH : RAND_INF_CHILD_LOACH_2); ++bit) {
                            if (caught) Flags_SetRandomizerInf(static_cast<RandomizerInf>(bit));
                            else Flags_UnsetRandomizerInf(static_cast<RandomizerInf>(bit));
                        }
                    }));
                }
            }
        }
        ImGuiTextFilter filter(filterText->c_str());
        const auto entries = TableEntries(table.flagTableType);
        for (size_t row = 0; row < entries.size(); ++row) {
            for (int bit = 15; bit >= 0; --bit) {
                const uint16_t index = row * 16 + bit;
                const auto found = table.flagDescriptions.find(index);
                const std::string label = fmt::format("0x{:02X}", index) +
                    (found == table.flagDescriptions.end() ? "" : std::string(": ") + found->second);
                if (filter.PassFilter(label.c_str()))
                    rows.push_back(Flag(std::to_string(index), label, entries[row], uint32_t{1} << bit));
            }
        }
        return rows;
    });
}

bool TokensMatchFlags() {
    return !(IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TOKENS) != RO_TOKENSANITY_OFF);
}

void UpdateGoldCount() {
    if (!keepGsCountUpdated || !TokensMatchFlags()) return;
    int count = 0;
    for (auto flags : gSaveContext.gsFlags) count += std::popcount(static_cast<uint32_t>(flags));
    gSaveContext.inventory.gsTokens = count;
}

N::PagePtr GoldPage() {
    auto area = std::make_shared<int>(0);
    UpdateGoldCount();
    return N::MakePage("save/gold", Text("gold_skulltulas"), [area] {
        std::vector<N::Row> rows;
        std::map<int, std::string> areas;
        for (int i = 0; i < gsMapping.size(); ++i) areas[i] = gsMapping[i];
        rows.push_back(N::Choice("area", Text("map"), *area, areas, [area](int value) { *area = value; }));
        const int current = *area;
        const uint32_t flags = GET_GS_FLAGS(current);
        for (int bit = 0; bit < 8; ++bit) {
            const uint32_t mask = uint32_t{1} << bit;
            if (!(gAreaGsFlags[current] & mask)) continue;
            rows.push_back(N::Toggle(std::to_string(bit), std::to_string(bit), (flags & mask) != 0, [current, mask](bool enabled) {
                const uint32_t old = GET_GS_FLAGS(current);
                gSaveContext.gsFlags[current >> 2] &= ~gGsFlagsMasks[current & 3];
                SET_GS_FLAGS(current, enabled ? old | mask : old & ~mask);
                UpdateGoldCount();
            }));
        }
        if (TokensMatchFlags())
            rows.push_back(N::Toggle("count", Text("keep_gs_count"), keepGsCountUpdated,
                [](bool value) { keepGsCountUpdated = value; UpdateGoldCount(); }, Text("keep_gs_count_help")));
        return rows;
    });
}

N::PagePtr ScenePage() {
    return N::MakePage("save/scene", Text("current_scene"), [] {
        std::vector<N::Row> rows;
        if (!gPlayState) return rows;
        auto* play = gPlayState;
        const auto guard = CurrentSceneGuard();
        auto& flags = play->actorCtx.flags;
        const auto add = [&](const char* key, auto& field, const char* description) {
            auto* pointer = &field;
            rows.push_back(N::Link(key, Text(key), [key, pointer, guard] {
                return guard() ? Bits(Text(key), *pointer, true, guard) : nullptr;
            }, Text(description)));
        };
        add("switch", flags.swch, "scene_switch_help");
        add("temp_switch", flags.tempSwch, "scene_temp_switch_help");
        add("clear", flags.clear, "scene_clear_help");
        add("temp_clear", flags.tempClear, "scene_temp_clear_help");
        add("collect", flags.collect, "scene_collect_help");
        add("temp_collect", flags.tempCollect, "scene_temp_collect_help");
        add("chest", flags.chest, "scene_chest_help");
        if (play->sceneNum >= 0 && play->sceneNum < std::size(gSaveContext.sceneFlags)) {
            rows.push_back(N::Action("reload", Text("reload_flags"), [play, guard] {
                if (!guard()) return;
                auto& current = play->actorCtx.flags;
                const auto& saved = gSaveContext.sceneFlags[play->sceneNum];
                current.swch = saved.swch; current.clear = saved.clear;
                current.collect = saved.collect; current.chest = saved.chest;
            }, Text("reload_flags_help")));
            rows.push_back(N::Action("save", Text("save_flags"), [play, guard] {
                if (!guard()) return;
                const auto& current = play->actorCtx.flags;
                auto& saved = gSaveContext.sceneFlags[play->sceneNum];
                saved.swch = current.swch; saved.clear = current.clear;
                saved.collect = current.collect; saved.chest = current.chest;
            }, Text("save_flags_help")));
        }
        rows.push_back(N::Action("clear_all", Text("clear_flags"), [play, guard] {
            if (!guard()) return;
            auto& flags = play->actorCtx.flags;
            flags.swch = flags.clear = flags.collect = flags.chest = 0;
        }, Text("clear_flags_help")));
        return rows;
    });
}

N::PagePtr SavedScenePage() {
    auto scene = std::make_shared<int>(0);
    return N::MakePage("save/saved_scene", Text("saved_scene_flags"), [scene] {
        std::map<int, std::string> scenes;
        for (int i = 0; i < SCENE_ID_MAX && i < std::size(gSaveContext.sceneFlags); ++i) scenes[i] = SohUtils::GetSceneName(i);
        std::vector<N::Row> rows{N::Choice("map", Text("map"), *scene, scenes, [scene](int value) { *scene = value; })};
        if (gPlayState && scenes.contains(gPlayState->sceneNum))
            rows.push_back(N::Action("current", Text("current"), [scene] { if (gPlayState) *scene = gPlayState->sceneNum; }));
        auto& flags = gSaveContext.sceneFlags[*scene];
        const auto add = [&](const char* key, auto& field) {
            auto* pointer = &field;
            rows.push_back(N::Link(key, Text(key), [key, pointer] { return Bits(Text(key), *pointer); }));
        };
        add("switch", flags.swch); add("clear", flags.clear); add("collect", flags.collect);
        add("chest", flags.chest); add("rooms", flags.rooms); add("floors", flags.floors);
        return rows;
    });
}
}

void SceneChanged() {
    ++sceneRevision;
}

Guard CurrentSceneGuard() {
    auto* play = gPlayState;
    const auto revision = sceneRevision;
    return [play, revision] { return play && play == gPlayState && revision == sceneRevision; };
}

N::PagePtr PlayerFlagsPage() {
    return N::MakePage("save/player_flags", Text("player_state"), [] {
        std::vector<N::Row> rows;
        if (!gPlayState || !GET_PLAYER(gPlayState)) return rows;
        auto* player = GET_PLAYER(gPlayState);
        auto scene = CurrentSceneGuard();
        Guard guard = [scene, player] { return scene() && GET_PLAYER(gPlayState) == player; };
        rows.push_back(N::Link("state1", "stateFlags1", [player, guard] { return guard() ? Bits("stateFlags1", player->stateFlags1, false, guard, state1) : nullptr; }));
        rows.push_back(N::Link("state2", "stateFlags2", [player, guard] { return guard() ? Bits("stateFlags2", player->stateFlags2, false, guard, state2) : nullptr; }));
        rows.push_back(N::Link("state3", "stateFlags3", [player, guard] { return guard() ? Bits("stateFlags3", player->stateFlags3, false, guard, state3) : nullptr; }));
        rows.push_back(N::Link("rotation", "unk_6AE_rotFlags", [player, guard] { return guard() ? Bits("unk_6AE_rotFlags", player->unk_6AE_rotFlags, false, guard) : nullptr; }));
        return rows;
    });
}

N::PagePtr FlagsPage() {
    return N::MakePage("save/flags", Text("flags"), [] {
        std::vector<N::Row> rows{
            N::Link("player", Text("player_state"), PlayerFlagsPage),
            N::Link("scene", Text("current_scene"), ScenePage),
            N::Link("saved", Text("saved_scene_flags"), SavedScenePage),
            N::Link("gold", Text("gold_skulltulas"), GoldPage)};
        rows[0].enabled = gPlayState && GET_PLAYER(gPlayState);
        rows[1].enabled = gPlayState != nullptr;
        for (size_t i = 0; i < flagTables.size(); ++i) {
            if (flagTables[i].flagTableType == RANDOMIZER_INF && !IS_RANDO && !IS_BOSS_RUSH) continue;
            rows.push_back(N::Link("table/" + std::to_string(i), flagTables[i].name, [i] { return TablePage(i); }));
        }
        return rows;
    });
}
}
