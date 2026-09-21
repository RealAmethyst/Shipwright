#include "actorViewer.h"
#include "../../util.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "actorParameters.h"
#include <limits>
#include <optional>
#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/nametag.h"
#include "soh/ShipInit.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <unordered_map>
#include <string>
#include <libultraship/bridge.h>
#include <libultraship/libultraship.h>
#include <spdlog/fmt/fmt.h>
#include "soh/OTRGlobals.h"
#include "soh/cvar_prefixes.h"
#include "soh/ObjectExtension/ActorListIndex.h"

extern "C" {
#include <z64.h>
#include "z64math.h"
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;

#include "textures/icon_item_static/icon_item_static.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
}

#define DEKUNUTS_FLOWER 10
#define DEBUG_ACTOR_NAMETAG_TAG "debug_actor_viewer"

#define CVAR_ACTOR_NAME_TAGS(val) CVAR_DEVELOPER_TOOLS("ActorViewer.NameTags." val)
#define CVAR_ACTOR_NAME_TAGS_ENABLED_NAME CVAR_ACTOR_NAME_TAGS("Enabled")
#define CVAR_ACTOR_NAME_TAGS_ENABLED CVarGetInteger(CVAR_ACTOR_NAME_TAGS("Enabled"), 0)

typedef struct {
    u16 id;
    u16 params;
    Vec3f pos;
    Vec3s rot;
} ActorInfo;

std::array<const char*, 12> acMapping = {
    "Switch",      "Background (Prop type 1)",
    "Player",      "Bomb",
    "NPC",         "Enemy",
    "Prop type 2", "Item/Action",
    "Misc.",       "Boss",
    "Door",        "Chest",
};

const std::string GetActorDescription(u16 id) {
    const auto& entry = ActorDB::Instance->RetrieveEntry(id);
    return entry.entry.valid ? entry.desc : "";
}

const std::string GetActorDebugName(u16 id) {
    const auto& entry = ActorDB::Instance->RetrieveEntry(id);
    return entry.entry.valid ? entry.name : "";
}

// actors that don't use params at all
static std::vector<u16> noParamsActors = {
    ACTOR_ARMS_HOOK,
    ACTOR_ARROW_FIRE,
    ACTOR_ARROW_ICE,
    ACTOR_ARROW_LIGHT,
    ACTOR_BG_BOM_GUARD,
    ACTOR_BG_DY_YOSEIZO,
    ACTOR_BG_GATE_SHUTTER,
    ACTOR_BG_GJYO_BRIDGE,
    ACTOR_BG_HIDAN_FSLIFT,
    ACTOR_BG_HIDAN_RSEKIZOU,
    ACTOR_BG_HIDAN_SYOKU,
    ACTOR_BG_JYA_GOROIWA,
    ACTOR_BG_MIZU_UZU,
    ACTOR_BG_MORI_RAKKATENJO,
    ACTOR_BG_PUSHBOX,
    ACTOR_BG_SPOT01_FUSYA,
    ACTOR_BG_SPOT01_IDOHASHIRA,
    ACTOR_BG_SPOT01_IDOMIZU,
    ACTOR_BG_SPOT01_IDOSOKO,
    ACTOR_BG_SPOT11_OASIS,
    ACTOR_BG_SPOT15_SAKU,
    ACTOR_BG_SPOT18_FUTA,
    ACTOR_BG_TOKI_SWD,
    ACTOR_BG_TREEMOUTH,
    ACTOR_BG_VB_SIMA,
    ACTOR_BOSS_DODONGO,
    ACTOR_BOSS_FD,
    ACTOR_BOSS_GOMA,
    ACTOR_DEMO_EXT,
    ACTOR_DEMO_SHD,
    ACTOR_DEMO_TRE_LGT,
    ACTOR_DOOR_TOKI,
    ACTOR_EFC_ERUPC,
    ACTOR_EN_ANI,
    ACTOR_EN_AROW_TRAP,
    ACTOR_EN_BIRD,
    ACTOR_EN_BLKOBJ,
    ACTOR_EN_BOM_BOWL_MAN,
    ACTOR_EN_BOM_BOWL_PIT,
    ACTOR_EN_BOM_CHU,
    ACTOR_EN_BUBBLE,
    ACTOR_EN_DIVING_GAME,
    ACTOR_EN_DNT_DEMO,
    ACTOR_EN_DNT_JIJI,
    ACTOR_EN_DS,
    ACTOR_EN_DU,
    ACTOR_EN_EG,
    ACTOR_EN_FU,
    ACTOR_EN_GB,
    ACTOR_EN_GE3,
    ACTOR_EN_GUEST,
    ACTOR_EN_HATA,
    ACTOR_EN_HORSE_GANON,
    ACTOR_EN_HORSE_LINK_CHILD,
    ACTOR_EN_HORSE_ZELDA,
    ACTOR_EN_HS2,
    ACTOR_EN_JS,
    ACTOR_EN_KAKASI,
    ACTOR_EN_KAKASI3,
    ACTOR_EN_MA1,
    ACTOR_EN_MA2,
    ACTOR_EN_MA3,
    ACTOR_EN_MAG,
    ACTOR_EN_MK,
    ACTOR_EN_MS,
    ACTOR_EN_NIW_LADY,
    ACTOR_EN_NWC,
    ACTOR_EN_OE2,
    ACTOR_EN_OKARINA_EFFECT,
    ACTOR_EN_RR,
    ACTOR_EN_SA,
    ACTOR_EN_SCENE_CHANGE,
    ACTOR_EN_SKJNEEDLE,
    ACTOR_EN_SYATEKI_ITM,
    ACTOR_EN_SYATEKI_MAN,
    ACTOR_EN_TAKARA_MAN,
    ACTOR_EN_TORYO,
    ACTOR_EN_VASE,
    ACTOR_EN_ZL1,
    ACTOR_MAGIC_DARK,
    ACTOR_MAGIC_FIRE,
    ACTOR_OBJ_DEKUJR,
    ACTOR_OCEFF_SPOT,

    ACTOR_UNSET_1,
    ACTOR_UNSET_3,
    ACTOR_UNSET_5,
    ACTOR_UNSET_6,
    ACTOR_UNSET_17,
    ACTOR_UNSET_1A,
    ACTOR_UNSET_1F,
    ACTOR_UNSET_22,
    ACTOR_UNSET_31,
    ACTOR_UNSET_36,
    ACTOR_UNSET_53,
    ACTOR_UNSET_73,
    ACTOR_UNSET_74,
    ACTOR_UNSET_75,
    ACTOR_UNSET_76,
    ACTOR_UNSET_78,
    ACTOR_UNSET_79,
    ACTOR_UNSET_7A,
    ACTOR_UNSET_7B,
    ACTOR_UNSET_7E,
    ACTOR_UNSET_7F,
    ACTOR_UNSET_83,
    ACTOR_UNSET_A0,
    ACTOR_UNSET_B2,
    ACTOR_UNSET_CE,
    ACTOR_UNSET_D8,
    ACTOR_UNSET_EA,
    ACTOR_UNSET_EB,
    ACTOR_UNSET_F2,
    ACTOR_UNSET_F3,
    ACTOR_UNSET_FB,
    ACTOR_UNSET_109,
    ACTOR_UNSET_10D,
    ACTOR_UNSET_10E,
    ACTOR_UNSET_128,
    ACTOR_UNSET_129,
    ACTOR_UNSET_134,
    ACTOR_UNSET_154,
    ACTOR_UNSET_15D,
    ACTOR_UNSET_161,
    ACTOR_UNSET_180,
    ACTOR_UNSET_1AA,
};

void ActorViewer_AddTagForActor(Actor* actor) {
    if (!CVarGetInteger(CVAR_ACTOR_NAME_TAGS("Enabled"), 0)) {
        return;
    }

    std::vector<std::string> parts;

    if (CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayID"), 0)) {
        parts.push_back(GetActorDebugName(actor->id));
    }
    if (CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayDescription"), 0)) {
        parts.push_back(GetActorDescription(actor->id));
    }
    if (CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayCategory"), 0)) {
        parts.push_back(acMapping[actor->category]);
    }
    if (CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayParams"), 0)) {
        parts.push_back(fmt::format("0x{:04X} ({})", (u16)actor->params, actor->params));
    }

    std::string tag = "";
    for (size_t i = 0; i < parts.size(); i++) {
        if (i != 0) {
            tag += "\n";
        }
        tag += parts.at(i);
    }

    bool withZBuffer = CVarGetInteger(CVAR_ACTOR_NAME_TAGS("WithZBuffer"), 0);

    NameTag_RegisterForActorWithOptions(actor, tag.c_str(),
                                        { .tag = DEBUG_ACTOR_NAMETAG_TAG, .noZBuffer = !withZBuffer });
}

void ActorViewer_AddTagForAllActors() {
    if (gPlayState == nullptr) {
        return;
    }

    for (size_t i = 0; i < ARRAY_COUNT(gPlayState->actorCtx.actorLists); i++) {
        ActorListEntry currList = gPlayState->actorCtx.actorLists[i];
        Actor* currAct = currList.head;
        while (currAct != nullptr) {
            ActorViewer_AddTagForActor(currAct);
            currAct = currAct->next;
        }
    }
}


namespace {
namespace N = NativeOptions;
Actor* selectedActor = nullptr;
int category = ACTORCAT_SWITCH;
ActorInfo newActor{};
std::string actorSearch;

bool CanEdit(Actor* actor) {
    return gPlayState && actor && actor == selectedActor;
}

void SelectActorId(int id) {
    newActor.id = static_cast<uint16_t>(id);
    newActor.params = id == ACTOR_EN_TITE ? static_cast<uint16_t>(-2) :
                      id == ACTOR_SHOT_SUN ? 0x40 : id == ACTOR_EN_DEKUNUTS ? 0x100 : 0;
}

template <class Vector> auto& Axis(Vector& value, int axis) {
    return axis == 0 ? value.x : axis == 1 ? value.y : value.z;
}

N::PagePtr PositionPage(std::string title, std::function<std::optional<Vec3f>()> get, std::function<void(Vec3f)> set) {
    return N::MakePage("actor/position", std::move(title), [get, set] {
        std::vector<N::Row> rows;
        const auto value = get();
        if (!value) return rows;
        for (int axis = 0; axis < 3; ++axis) {
            const auto label = std::string(1, 'X' + axis);
            rows.push_back(N::Decimal(label, label, Axis(*value, axis), -std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(), 1, [get, set, axis](float next) {
                    if (auto current = get()) { Axis(*current, axis) = next; set(*current); }
                }));
        }
        return rows;
    });
}

N::PagePtr RotationPage(std::string title, std::function<std::optional<Vec3s>()> get, std::function<void(Vec3s)> set) {
    return N::MakePage("actor/rotation", std::move(title), [get, set] {
        std::vector<N::Row> rows;
        const auto value = get();
        if (!value) return rows;
        for (int axis = 0; axis < 3; ++axis) {
            const auto label = std::string(1, 'X' + axis);
            rows.push_back(N::Integer(label, label, Axis(*value, axis), -32768, 32767, 1, [get, set, axis](int next) {
                if (auto current = get()) { Axis(*current, axis) = static_cast<int16_t>(next); set(*current); }
            }));
        }
        return rows;
    });
}

N::PagePtr FlagsPage(Actor* actor, bool background) {
    return N::MakePage("actor/flags", background ? "bgCheckFlags" : "flags", [actor, background] {
        std::vector<N::Row> rows;
        if (!CanEdit(actor)) return rows;
        const uint32_t value = background ? actor->bgCheckFlags : actor->flags;
        for (int bit = 0; bit < (background ? 16 : 32); ++bit) {
            const uint32_t mask = uint32_t{1} << bit;
            rows.push_back(N::Toggle(std::to_string(bit), std::to_string(bit), (value & mask) != 0,
                [actor, background, mask](bool enabled) {
                    if (!CanEdit(actor)) return;
                    if (background) actor->bgCheckFlags = enabled ? actor->bgCheckFlags | mask : actor->bgCheckFlags & ~mask;
                    else actor->flags = enabled ? actor->flags | mask : actor->flags & ~mask;
                }));
        }
        return rows;
    });
}

N::PagePtr SelectedPage(Actor* actor) {
    selectedActor = actor;
    return N::MakePage("actor/selected", N::Text("actor_selected"), [actor] {
        std::vector<N::Row> rows;
        if (!CanEdit(actor)) return rows;
        auto info = [&](const char* key, std::string value) {
            auto row = N::Action(key, N::Text(key), [] { N::ReadCurrentDescription(); });
            row.value = std::move(value);
            rows.push_back(std::move(row));
        };
        info("actor_name", GetActorDebugName(actor->id));
        info("actor_description", GetActorDescription(actor->id));
        if (actor->category < acMapping.size()) info("actor_category", acMapping[actor->category]);
        info("actor_id", std::to_string(actor->id));
        info("actor_parameters", std::to_string(actor->params));
        info("actor_list_index", std::to_string(GetActorListIndex(actor)));
        rows.push_back(N::Link("position", N::Text("actor_position"), [actor] {
            return PositionPage(N::Text("actor_position"),
                [actor]() -> std::optional<Vec3f> { return CanEdit(actor) ? std::optional{actor->world.pos} : std::nullopt; },
                [actor](Vec3f value) { if (CanEdit(actor)) actor->world.pos = value; });
        }));
        rows.push_back(N::Link("rotation", N::Text("actor_rotation"), [actor] {
            return RotationPage(N::Text("actor_rotation"),
                [actor]() -> std::optional<Vec3s> { return CanEdit(actor) ? std::optional{actor->world.rot} : std::nullopt; },
                [actor](Vec3s value) { if (CanEdit(actor)) actor->world.rot = value; });
        }));
        if (actor->category == ACTORCAT_BOSS || actor->category == ACTORCAT_ENEMY)
            rows.push_back(N::Integer("health", N::Text("actor_enemy_health"), actor->colChkInfo.health, 0, 255, 1,
                [actor](int value) { if (CanEdit(actor)) actor->colChkInfo.health = value; }, N::Text("actor_health_help")));
        rows.push_back(N::Link("flags", "flags", [actor] { return FlagsPage(actor, false); }));
        rows.push_back(N::Link("bgCheckFlags", "bgCheckFlags", [actor] { return FlagsPage(actor, true); }));
        auto go = N::Action("go", N::Text("actor_go"), [actor] {
            if (!CanEdit(actor) || !GET_PLAYER(gPlayState)) return;
            auto* player = GET_PLAYER(gPlayState);
            player->actor.world.pos = actor->world.pos;
            player->actor.home.pos = player->actor.world.pos;
        });
        go.enabled = GET_PLAYER(gPlayState) != nullptr;
        rows.push_back(std::move(go));
        return rows;
    });
}

N::PagePtr ActorsPage() {
    return N::MakePage("actor/list", N::Text("actor_list"), [] {
        std::vector<N::Row> rows;
        std::map<int, std::string> categories;
        for (int i = 0; i < acMapping.size(); ++i) categories[i] = acMapping[i];
        rows.push_back(N::Choice("category", N::Text("actor_type"), category, categories, [](int value) { category = value; }));
        if (!gPlayState) return rows;
        for (auto* actor = gPlayState->actorCtx.actorLists[category].head; actor; actor = actor->next) {
            if (!ActorDB::Instance->RetrieveEntry(actor->id).entry.valid) continue;
            rows.push_back(N::Link(std::to_string(reinterpret_cast<uintptr_t>(actor)), GetActorDebugName(actor->id),
                [actor] { return SelectedPage(actor); }, GetActorDescription(actor->id)));
        }
        return rows;
    });
}

N::PagePtr ActorSearchPage() {
    return N::MakePage("actor/search", N::Text("actor_search"), [] {
        std::vector<N::Row> rows;
        rows.push_back(N::String("search", N::Text("actor_search"), actorSearch,
            [](std::string value) { actorSearch = std::move(value); }));
        ImGuiTextFilter filter(actorSearch.c_str());
        for (int id = 0; id < ActorDB::Instance->GetEntryCount(); ++id) {
            const auto& entry = ActorDB::Instance->RetrieveEntry(id);
            if (!entry.entry.valid || (!filter.PassFilter(entry.name.c_str()) && !filter.PassFilter(entry.desc.c_str()))) continue;
            rows.push_back(N::Action(std::to_string(id), entry.name, [id] {
                N::GetModel().Back([id] { SelectActorId(id); });
            }, entry.desc));
        }
        return rows;
    });
}

N::PagePtr ParametersPage() {
    return N::MakePage("actor/parameters", N::Text("actor_specific"), [] {
        auto rows = ActorParameterRows(newActor.id, newActor.params, [](uint16_t value) { newActor.params = value; });
        if (rows.empty()) rows.push_back(N::Integer("params", N::Text("actor_parameters"), static_cast<int16_t>(newActor.params),
            -32768, 32767, 1, [](int value) { newActor.params = value; }));
        return rows;
    });
}

N::PagePtr NewActorPage() {
    return N::MakePage("actor/new", N::Text("actor_new"), [] {
        std::vector<N::Row> rows;
        rows.push_back(N::Link("search", N::Text("actor_search"), ActorSearchPage));
        rows.push_back(N::Integer("id", N::Text("actor_id"), newActor.id, 0, 65535, 1, SelectActorId,
                                 GetActorDescription(newActor.id)));
        rows.push_back(N::CVarToggle(N::Text("actor_advanced"), CVAR_DEVELOPER_TOOLS("ActorViewer.AdvancedParams"), false,
                                    N::Text("actor_advanced_help")));
        if (CVarGetInteger(CVAR_DEVELOPER_TOOLS("ActorViewer.AdvancedParams"), 0))
            rows.push_back(N::Integer("params", N::Text("actor_parameters"), static_cast<int16_t>(newActor.params),
                -32768, 32767, 1, [](int value) { newActor.params = value; }));
        else if (std::find(noParamsActors.begin(), noParamsActors.end(), newActor.id) == noParamsActors.end())
            rows.push_back(N::Link("params", N::Text("actor_specific"), ParametersPage));
        rows.push_back(N::Link("position", N::Text("actor_new_position"), [] {
            return PositionPage(N::Text("actor_new_position"), [] { return std::optional{newActor.pos}; },
                                [](Vec3f value) { newActor.pos = value; });
        }));
        rows.push_back(N::Link("rotation", N::Text("actor_new_rotation"), [] {
            return RotationPage(N::Text("actor_new_rotation"), [] { return std::optional{newActor.rot}; },
                                [](Vec3s value) { newActor.rot = value; });
        }));
        auto fetch = N::Action("fetch", N::Text("actor_fetch_link"), [] {
            if (!gPlayState || !GET_PLAYER(gPlayState)) return;
            newActor.pos = GET_PLAYER(gPlayState)->actor.world.pos;
            newActor.rot = GET_PLAYER(gPlayState)->actor.world.rot;
        });
        fetch.enabled = gPlayState && GET_PLAYER(gPlayState);
        rows.push_back(std::move(fetch));
        for (bool child : {false, true}) {
            auto spawn = N::Action(child ? "child" : "spawn", N::Text(child ? "actor_spawn_child" : "actor_spawn"), [child] {
                if (!gPlayState || !ActorDB::Instance->RetrieveEntry(newActor.id).entry.valid || (child && !selectedActor)) return;
                Actor* spawned = child
                    ? Actor_SpawnAsChild(&gPlayState->actorCtx, selectedActor, gPlayState, newActor.id, newActor.pos.x,
                        newActor.pos.y, newActor.pos.z, newActor.rot.x, newActor.rot.y, newActor.rot.z, newActor.params)
                    : Actor_Spawn(&gPlayState->actorCtx, gPlayState, newActor.id, newActor.pos.x, newActor.pos.y,
                        newActor.pos.z, newActor.rot.x, newActor.rot.y, newActor.rot.z, newActor.params);
                N::Message(N::Text(child ? "actor_spawn_child" : "actor_spawn"),
                           N::Text(spawned ? "actor_spawned" : "actor_spawn_failed"));
            });
            spawn.enabled = gPlayState && ActorDB::Instance->RetrieveEntry(newActor.id).entry.valid && (!child || selectedActor);
            rows.push_back(std::move(spawn));
        }
        rows.push_back(N::Action("reset", N::Text("reset"), [] { newActor = {}; }));
        return rows;
    });
}

void RefreshNameTags(bool toggled) {
    const bool enabled = CVarGetInteger(CVAR_ACTOR_NAME_TAGS("Enabled"), 0);
    const bool empty = !CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayID"), 0) &&
                       !CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayDescription"), 0) &&
                       !CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayCategory"), 0) &&
                       !CVarGetInteger(CVAR_ACTOR_NAME_TAGS("DisplayParams"), 0);
    if (enabled && empty) CVarSetInteger(toggled ? CVAR_ACTOR_NAME_TAGS("DisplayID") : CVAR_ACTOR_NAME_TAGS("Enabled"), toggled);
    else if (!toggled && !enabled && !empty) CVarSetInteger(CVAR_ACTOR_NAME_TAGS("Enabled"), 1);
    N::ChangedCVar(CVAR_ACTOR_NAME_TAGS("Enabled"));
    NameTag_RemoveAllByTag(DEBUG_ACTOR_NAMETAG_TAG);
    ActorViewer_AddTagForAllActors();
}

N::PagePtr NameTagsPage() {
    return N::MakePage("actor/tags", N::Text("actor_name_tags"), [] {
        return std::vector<N::Row>{
            N::CVarToggle(N::Text("actor_name_tags"), CVAR_ACTOR_NAME_TAGS("Enabled"), false, N::Text("actor_name_tags_help"),
                          [] { RefreshNameTags(true); }),
            N::CVarToggle(N::Text("actor_id"), CVAR_ACTOR_NAME_TAGS("DisplayID"), false, "", [] { RefreshNameTags(false); }),
            N::CVarToggle(N::Text("actor_description"), CVAR_ACTOR_NAME_TAGS("DisplayDescription"), false, "", [] { RefreshNameTags(false); }),
            N::CVarToggle(N::Text("actor_category"), CVAR_ACTOR_NAME_TAGS("DisplayCategory"), false, "", [] { RefreshNameTags(false); }),
            N::CVarToggle(N::Text("actor_parameters"), CVAR_ACTOR_NAME_TAGS("DisplayParams"), false, "", [] { RefreshNameTags(false); }),
            N::CVarToggle(N::Text("actor_zbuffer"), CVAR_ACTOR_NAME_TAGS("WithZBuffer"), false, N::Text("actor_zbuffer_help"),
                          [] { RefreshNameTags(false); })};
    });
}

N::PagePtr ActorViewerPage() {
    return N::MakePage("advanced/actors", N::Text("actor_viewer"), [] {
        std::vector<N::Row> rows{
            N::Link("tags", N::Text("actor_name_tags"), NameTagsPage),
            N::Link("list", N::Text("actor_list"), ActorsPage),
            N::Link("new", N::Text("actor_new"), NewActorPage)};
        auto selected = N::Link("selected", N::Text("actor_selected"), [] { return SelectedPage(selectedActor); });
        selected.enabled = gPlayState && selectedActor;
        rows.push_back(std::move(selected));
        const char* keys[] = {"actor_fetch_target", "actor_fetch_held", "actor_fetch_interaction"};
        for (int which = 0; which < 3; ++which) {
            Actor* actor = nullptr;
            if (gPlayState && GET_PLAYER(gPlayState)) {
                auto* player = GET_PLAYER(gPlayState);
                actor = which == 0 ? player->talkActor : which == 1 ? player->heldActor : player->interactRangeActor;
            }
            auto row = N::Link(keys[which], N::Text(keys[which]), [actor] { return SelectedPage(actor); },
                               N::Text(std::string(keys[which]) + "_help"));
            row.enabled = actor != nullptr;
            rows.push_back(std::move(row));
        }
        if (!gPlayState) N::Disable(rows, N::Text("actor_no_context"));
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
        return rows;
    });
}
}

void InitializeActorViewer() {
    NativeOptions::RegisterPage("Actor Viewer", ActorViewerPage, NativeOptions::Text("actor_viewer"));
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorDestroy>([](void* actor) {
        if (selectedActor == actor) selectedActor = nullptr;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) {
        selectedActor = nullptr;
        category = ACTORCAT_SWITCH;
    });
}

void ActorViewer_RegisterNameTagHooks() {
    COND_HOOK(OnActorInit, CVAR_ACTOR_NAME_TAGS_ENABLED,
              [](void* actor) { ActorViewer_AddTagForActor(static_cast<Actor*>(actor)); });
}

static RegisterShipInitFunc initFunc(ActorViewer_RegisterNameTagHooks, { CVAR_ACTOR_NAME_TAGS_ENABLED_NAME });
