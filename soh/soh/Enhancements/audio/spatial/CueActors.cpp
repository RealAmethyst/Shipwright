#include "CueActors.h"
#include "CueLocations.h"
#include "WallProbes.h"
#include "WorldCompass.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/ResourceManagerHelpers.h"
#include <libultraship/libultraship.h>
#include <algorithm>
#include <cmath>
#include <map>

extern "C" {
#include "global.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "overlays/actors/ovl_En_G_Switch/z_en_g_switch.h"
#include "overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "overlays/actors/ovl_En_Ge1/z_en_ge1.h"
#include "overlays/actors/ovl_En_Wood02/z_en_wood02.h"
#include "overlays/actors/ovl_En_Kusa/z_en_kusa.h"
void EnBox_WaitOpen(EnBox*, PlayState*);
void EnGSwitch_SilverRupeeIdle(EnGSwitch*, PlayState*);
void EnKarebaba_DeadItemDrop(EnKarebaba*, PlayState*);
void EnDekubaba_DeadStickDrop(EnDekubaba*, PlayState*);
void func_8001E5C8(EnItem00*, PlayState*);
void EnGe1_WatchForPlayerFrontOnly(EnGe1*, PlayState*);
void EnGe1_WatchForAndSensePlayer(EnGe1*, PlayState*);
void EnGe1_KickPlayer(EnGe1*, PlayState*);
void EnKusa_Main(EnKusa*, PlayState*);
void CollisionPoly_GetVertices(CollisionPoly*, Vec3s*, Vec3f*);
}

namespace SpatialAudio {
namespace {
struct Policy { Cue cue; float range = 500, height = 80; };
struct Tracked {
    uint64_t identity;
    Policy policy;
    uint32_t frame;
};
struct Exit {
    Tracked tracked;
    Vec3f position;
    int destination;
};
struct SurfaceCue { Tracked tracked; Vec3f position; };
struct LocationCue { Tracked tracked; const CueLocation* location; };
CueMixer cueMixer;
std::map<int16_t, Policy> policies;
std::map<Actor*, Tracked> actors;
std::vector<Exit> exits;
std::vector<SurfaceCue> climbs;
std::vector<LocationCue> locations;
int locationsScene = -1;
const int16_t* exitList = nullptr;
size_t exitCount = 0;
CollisionHeader* collision = nullptr;
uint64_t nextIdentity = uint64_t{1} << 63;
std::array<float, static_cast<size_t>(Cue::Count)> levels{};
float masterGain = 0;
bool running = false;
bool reportedCapacity = false;
std::array<uint64_t, 3> wallIdentities{};

float Distance(const Vec3f& a, const Vec3f& b) {
    return std::sqrt(SQ(a.x - b.x) + SQ(a.y - b.y) + SQ(a.z - b.z));
}
float DistanceXZ(const Vec3f& a, const Vec3f& b) {
    return std::sqrt(SQ(a.x - b.x) + SQ(a.z - b.z));
}

Tracked NewTracked(Policy policy) {
    const auto id = ++nextIdentity;
    // Stagger the special-item pulses without consuming the game's random stream.
    return {id, policy, static_cast<uint32_t>(id & 31)};
}

void ResetScene() {
    cueMixer.StopAll();
    actors.clear();
    exits.clear();
    climbs.clear();
    locations.clear();
    locationsScene = -1;
    collision = nullptr;
    exitList = nullptr;
    exitCount = 0;
}

bool Available(Actor* actor, PlayState* play) {
    if (actor->init) return false;
    if (!actor->update || !actor->draw) return actor->id == ACTOR_EN_HOLL && actor->update;
    // isDrawn is frustum-dependent. The native draw function and these state
    // consumers verify availability even for sources behind the camera.
    switch (actor->id) {
        case ACTOR_EN_ITEM00:
            return reinterpret_cast<EnItem00*>(actor)->actionFunc != func_8001E5C8 && !actor->parent;
        case ACTOR_EN_GE1: {
            const auto action = reinterpret_cast<EnGe1*>(actor)->actionFunc;
            return action != EnGe1_WatchForPlayerFrontOnly && action != EnGe1_WatchForAndSensePlayer && action != EnGe1_KickPlayer;
        }
        case ACTOR_EN_BOX:
            return reinterpret_cast<EnBox*>(actor)->actionFunc == EnBox_WaitOpen &&
                   !Flags_GetTreasure(play, actor->params & 0x1f) && (actor->params & 0x1f) < 20;
        case ACTOR_EN_ELF:
            return actor->category == ACTORCAT_ITEMACTION &&
                   (actor->params == FAIRY_HEAL || actor->params == FAIRY_HEAL_TIMED || actor->params == FAIRY_HEAL_BIG);
        case ACTOR_EN_ICE_HONO: return actor->params == -1;
        case ACTOR_EN_KAREBABA:
            return reinterpret_cast<EnKarebaba*>(actor)->actionFunc == EnKarebaba_DeadItemDrop && !actor->parent;
        case ACTOR_EN_DEKUBABA:
            return reinterpret_cast<EnDekubaba*>(actor)->actionFunc == EnDekubaba_DeadStickDrop && !actor->parent;
        case ACTOR_EN_G_SWITCH:
            return reinterpret_cast<EnGSwitch*>(actor)->actionFunc == EnGSwitch_SilverRupeeIdle;
        case ACTOR_DOOR_ANA: return (actor->params & 0x300) == 0;
        case ACTOR_EN_WOOD02: return actor->params >= 0 && actor->params < WOOD_LEAF_GREEN;
        case ACTOR_BG_YDAN_MARUTA: return actor->params == 1;
        case ACTOR_BG_BDAN_OBJECTS: return actor->params == 2 && DistanceXZ(actor->world.pos, GET_PLAYER(play)->actor.world.pos) > 50;
        case ACTOR_EN_KUSA: return !actor->parent && reinterpret_cast<EnKusa*>(actor)->actionFunc == EnKusa_Main;
        case ACTOR_OBJ_KIBAKO:
        case ACTOR_OBJ_KIBAKO2:
        case ACTOR_OBJ_TSUBO: return !actor->parent;
        default: return true;
    }
}

void UpdateSound(Tracked& tracked, const Vec3f& position, float distance, float range, float scale,
                 bool pulsed = false, bool trigger = false) {
    const float level = levels[static_cast<size_t>(tracked.policy.cue)] * scale;
    if (level <= 0 || !std::isfinite(distance) || range <= 0 || distance > range) {
        cueMixer.Stop(tracked.identity);
        return;
    }
    auto source = SpatialAudio_WorldSource(tracked.identity, position.x, position.y, position.z);
    if (!source.identity) { cueMixer.Stop(tracked.identity); return; }
    const bool playing = pulsed ? (!trigger || cueMixer.Play(tracked.identity, tracked.policy.cue)) :
                                 cueMixer.KeepPlaying(tracked.identity, tracked.policy.cue);
    if (!playing) {
        cueMixer.Stop(tracked.identity);
        if (!reportedCapacity) {
            SPDLOG_ERROR("Accessibility cue rejected: missing recording or exhausted voice capacity");
            reportedCapacity = true;
        }
        return;
    }
    // PR 5435's attenuation: 35 dB reduction at the detection limit.
    const float attenuation = std::pow(10.0f, -35.0f * distance / range / 20.0f);
    cueMixer.Update(tracked.identity, source, level * attenuation);
}

void UpdateWalls(PlayState* play) {
    const size_t first = static_cast<size_t>(Cue::WallNorth);
    const bool enabled = std::any_of(levels.begin() + first, levels.end(), [](float level) { return level > 0; });
    if (!enabled || !HasMapCompass(play->sceneNum)) {
        for (auto id : wallIdentities) cueMixer.Stop(id);
        return;
    }
    const auto hits = ScanWalls(play);
    const auto& origin = GET_PLAYER(play)->actor.world.pos;
    const bool mirrored = CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) != 0;
    for (size_t i = 0; i < hits.size(); ++i) {
        const auto& hit = hits[i];
        const auto identity = wallIdentities[i];
        const int direction = hit.found ? MapDirection(hit.position.x - origin.x, hit.position.z - origin.z, mirrored) : -1;
        if (direction < 0 || levels[first + direction] <= 0) { cueMixer.Stop(identity); continue; }
        const auto source = SpatialAudio_WorldSource(identity, hit.position.x, hit.position.y, hit.position.z);
        if (!source.identity) { cueMixer.Stop(identity); continue; }
        const Cue cue = static_cast<Cue>(first + direction);
        if (!cueMixer.KeepPlaying(identity, cue)) {
            cueMixer.Stop(identity);
            if (!reportedCapacity) {
                SPDLOG_ERROR("Accessibility wall cue rejected: missing recording or exhausted voice capacity");
                reportedCapacity = true;
            }
            continue;
        }
        const float gain = levels[first + direction] * std::pow(10.0f, -35.0f * DistanceXZ(hit.position, origin) / 500 / 20);
        cueMixer.Update(identity, source, gain);
    }
}

void FindGeometryCues(PlayState* play) {
    if (play->colCtx.colHeader == collision) return;
    for (const auto& exit : exits) cueMixer.Stop(exit.tracked.identity);
    for (const auto& climb : climbs) cueMixer.Stop(climb.tracked.identity);
    exits.clear();
    climbs.clear();
    collision = play->colCtx.colHeader;
    if (!collision || !collision->polyList || !collision->vtxList) return;
    const bool genericExit = play->sceneNum == SCENE_GROTTOS || play->sceneNum == SCENE_FAIRYS_FOUNTAIN;
    const bool boundedExits = genericExit || (play->setupExitList == exitList && exitList && exitCount);
    if (!boundedExits) {
        SPDLOG_WARN("Accessibility scene exits unavailable: no matching bounded native exit list");
    }
    for (size_t i = 0; i < collision->numPolygons; ++i) {
        auto* poly = &collision->polyList[i];
        const auto wallFlags = func_80041DB8(&play->colCtx, poly, BGCHECK_SCENE);
        const bool climbable = wallFlags == 8 || wallFlags == 3;
        const auto index = SurfaceType_GetSceneExitIndex(&play->colCtx, poly, BGCHECK_SCENE);
        if (!climbable && (!index || !boundedExits)) continue;
        int destination = -1;
        if (!climbable && !genericExit) {
            if (index > exitCount || exitList[index - 1] < 0 || exitList[index - 1] >= ENTR_MAX) {
                SPDLOG_WARN("Accessibility scene exit rejected: polygon {}, exit index {}", i, index);
                continue;
            }
            destination = gEntranceTable[exitList[index - 1]].scene;
        }
        Vec3f vertices[3];
        CollisionPoly_GetVertices(poly, collision->vtxList, vertices);
        Vec3f position{
            (std::min({vertices[0].x, vertices[1].x, vertices[2].x}) + std::max({vertices[0].x, vertices[1].x, vertices[2].x})) / 2,
            std::min({vertices[0].y, vertices[1].y, vertices[2].y}),
            (std::min({vertices[0].z, vertices[1].z, vertices[2].z}) + std::max({vertices[0].z, vertices[1].z, vertices[2].z})) / 2};
        if (climbable) {
            const bool duplicate = std::any_of(climbs.begin(), climbs.end(), [&](const SurfaceCue& cue) {
                return Distance(position, cue.position) < 1;
            });
            if (!duplicate) climbs.push_back({NewTracked({Cue::Ladder}), position});
            continue;
        }
        const bool duplicate = std::any_of(exits.begin(), exits.end(), [&](const Exit& exit) {
            return exit.destination == destination && Distance(position, exit.position) < 1;
        });
        if (!duplicate) exits.push_back({NewTracked({Cue::Transition, 1500, 2000}), position, destination});
    }
}

// Visibility/range conditions from PR 5435's accessible_area_change. Recording
// selection is deliberately separate: every approved exit uses transition.wav.
bool ExitRange(PlayState* play, const Exit& exit, float xz, float y, float distance, float& range, float& gain) {
    const int scene = play->sceneNum, target = exit.destination;
    range = 1500;
    gain = 1;
    if (y > 500 && target != SCENE_DEATH_MOUNTAIN_TRAIL && scene != SCENE_HYRULE_FIELD &&
        scene != SCENE_KAKARIKO_VILLAGE && scene != SCENE_LOST_WOODS) return false;
    if (scene == SCENE_HYRULE_FIELD) {
        if (xz > 8000) return false;
        if (xz > 700) range = xz * 1.2f;
    } else if (scene == SCENE_KAKARIKO_VILLAGE) {
        if (target == SCENE_GRAVEYARD || target == SCENE_HYRULE_FIELD || target == SCENE_DEATH_MOUNTAIN_TRAIL) {
            if (y > 5000 || xz > 8000) return false;
            if (xz > 700) range = distance * (target == SCENE_HYRULE_FIELD ? 1.4f : 1.2f);
        } else if (target == SCENE_BOTTOM_OF_THE_WELL) {
            if (!Flags_GetEventChkInf(EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO)) return false;
        } else { range = 1000; if (y > 500 || xz > 1000) return false; }
    } else if (scene == SCENE_LOST_WOODS || scene == SCENE_CASTLE_COURTYARD_GUARDS_DAY ||
               scene == SCENE_CASTLE_COURTYARD_GUARDS_NIGHT) { range = 1000; if (xz > 1000) return false; }
    else if (xz > 1500) return false;
    if (target == SCENE_KOKIRI_FOREST || target == SCENE_LOST_WOODS) {
        return scene != SCENE_LOST_WOODS || gSaveContext.entranceIndex == ENTR_LOST_WOODS_BRIDGE_EAST_EXIT ||
               gSaveContext.entranceIndex == ENTR_LOST_WOODS_BRIDGE_WEST_EXIT;
    }
    if (scene >= SCENE_DEKU_TREE_BOSS && scene <= SCENE_GANONDORF_BOSS) return false;
    if (scene == SCENE_GROTTOS || scene == SCENE_FAIRYS_FOUNTAIN) { gain = 0.1f; return true; }
    const bool namedDestination = target == SCENE_HYRULE_FIELD || target <= SCENE_GERUDO_TRAINING_GROUND ||
        (target >= SCENE_MARKET_ENTRANCE_DAY && target <= SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS) ||
        scene == SCENE_TEMPLE_OF_TIME || target == SCENE_CASTLE_COURTYARD_GUARDS_DAY ||
        target == SCENE_CASTLE_COURTYARD_GUARDS_NIGHT || target == SCENE_KAKARIKO_VILLAGE ||
        target == SCENE_GRAVEYARD || target == SCENE_ZORAS_RIVER || target == SCENE_ZORAS_DOMAIN ||
        target == SCENE_ZORAS_FOUNTAIN || target == SCENE_LAKE_HYLIA || target == SCENE_GERUDO_VALLEY ||
        target == SCENE_GERUDOS_FORTRESS || target == SCENE_DESERT_COLOSSUS || target == SCENE_HAUNTED_WASTELAND ||
        target == SCENE_OUTSIDE_GANONS_CASTLE || target == SCENE_HYRULE_CASTLE ||
        target == SCENE_DEATH_MOUNTAIN_TRAIL || target == SCENE_DEATH_MOUNTAIN_CRATER ||
        target == SCENE_GORON_CITY || target == SCENE_LON_LON_RANCH;
    if (!namedDestination) {
        if (target == SCENE_SACRED_FOREST_MEADOW || (target >= SCENE_DEKU_TREE_BOSS && target <= SCENE_GANONDORF_BOSS))
            return false;
        range = 500;
    }
    return true;
}

bool LocationPosition(PlayState* play, const CueLocation& location, Vec3f& position) {
    if (location.scene != play->sceneNum || location.room != play->roomCtx.curRoom.num) return false;
    // These fixed dungeon points describe original layouts, not Master Quest rooms.
    if (play->sceneNum <= SCENE_GERUDO_TRAINING_GROUND && ResourceMgr_IsSceneMasterQuest(play->sceneNum)) return false;
    position = location.position;
    if (location.rule == PointRule::ForestBasementLeft || location.rule == PointRule::ForestBasementRight) {
        Actor* walls = Actor_Find(&play->actorCtx, ACTOR_BG_MORI_KAITENKABE, ACTORCAT_BG);
        if (!walls) return false;
        const float offset = location.rule == PointRule::ForestBasementLeft ? -300.0f : 300.0f;
        position.x = walls->world.pos.x + Math_CosS(-walls->world.rot.y) * offset;
        position.z = walls->world.pos.z + Math_SinS(-walls->world.rot.y) * offset;
    } else if (location.rule == PointRule::ForestTwistedHallway) {
        Actor* twisted = Actor_Find(&play->actorCtx, ACTOR_BG_MORI_HINERI, ACTORCAT_BG);
        if (!twisted) return false;
        if ((twisted->params == 0 && position.x != -1835) || (twisted->params == 1 && position.x != -1760) ||
            (twisted->params == 2 && position.x != 1877)) return false;
    }
    return true;
}
}

void InitCues() {
    for (auto& id : wallIdentities) id = ++nextIdentity;
    for (size_t i = 0; i < CueNames.size(); ++i) {
        std::string error;
        const auto path = Ship::Context::GetPathRelativeToAppDirectory(std::string("accessibility/audio/") + CueNames[i] + ".wav");
        if (!cueMixer.Load(static_cast<Cue>(i), path, error)) SPDLOG_ERROR("Accessibility cue {}: {}", CueNames[i], error);
    }
    auto add = [](Policy policy, std::initializer_list<int16_t> ids) { for (auto id : ids) policies[id] = policy; };
    add({Cue::Person}, {ACTOR_EN_MA1, ACTOR_EN_TA, ACTOR_EN_KZ, ACTOR_EN_DIVING_GAME, ACTOR_EN_ZL4,
        ACTOR_EN_IN, ACTOR_EN_NIW_LADY, ACTOR_EN_FU, ACTOR_EN_DU, ACTOR_EN_NB, ACTOR_EN_KO, ACTOR_EN_ZO,
        ACTOR_EN_GO2, ACTOR_EN_SA, ACTOR_EN_MK, ACTOR_EN_CS, ACTOR_EN_TK, ACTOR_EN_GUEST, ACTOR_EN_MM,
        ACTOR_EN_MM2, ACTOR_EN_XC, ACTOR_EN_HY, ACTOR_EN_NIW_GIRL, ACTOR_EN_TG, ACTOR_EN_MU, ACTOR_EN_SKJ,
        ACTOR_EN_TORYO, ACTOR_EN_HS, ACTOR_EN_HS2, ACTOR_EN_MS, ACTOR_EN_DAIKU_KAKARIKO, ACTOR_EN_ANI, ACTOR_EN_STH,
        ACTOR_EN_GE1, ACTOR_EN_GS, ACTOR_EN_SSH, ACTOR_EN_HEISHI2, ACTOR_EN_HEISHI4, ACTOR_EN_OWL,
        ACTOR_EN_SYATEKI_MAN, ACTOR_EN_BOM_BOWL_MAN, ACTOR_EN_OSSAN, ACTOR_EN_DS, ACTOR_EN_JS, ACTOR_EN_RU1});
    add({Cue::Person, 1000}, {ACTOR_EN_MD});
    add({Cue::Item}, {ACTOR_EN_ITEM00, ACTOR_EN_ELF, ACTOR_EN_SI, ACTOR_ITEM_B_HEART, ACTOR_EN_KAREBABA, ACTOR_EN_DEKUBABA});
    add({Cue::Item, 1000}, {ACTOR_EN_BOX, ACTOR_EN_G_SWITCH});
    add({Cue::Item, 900}, {ACTOR_EN_ICE_HONO});
    add({Cue::Item, 1500, 500}, {ACTOR_EN_EX_RUPPY, ACTOR_ITEM_ETCETERA, ACTOR_ITEM_OCARINA});
    add({Cue::Door, 1000}, {ACTOR_EN_DOOR, ACTOR_DOOR_SHUTTER});
    add({Cue::Door}, {ACTOR_BG_SPOT18_SHUTTER, ACTOR_BG_ICE_SHUTTER, ACTOR_BG_MIZU_SHUTTER});
    add({Cue::Transition}, {ACTOR_EN_HOLL, ACTOR_DOOR_ANA});
    add({Cue::Destructible}, {ACTOR_EN_KUSA, ACTOR_OBJ_KIBAKO, ACTOR_OBJ_KIBAKO2, ACTOR_OBJ_TSUBO});
    add({Cue::Destructible, 1000}, {ACTOR_EN_WOOD02});
    add({Cue::Ladder}, {ACTOR_BG_YDAN_MARUTA});
    add({Cue::Elevator, 500, 50}, {ACTOR_BG_BDAN_OBJECTS});
    add({Cue::Elevator, 300, 1}, {ACTOR_BG_MORI_ELEVATOR});
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* raw) {
        auto* actor = static_cast<Actor*>(raw);
        auto policy = policies.find(actor->id);
        if (policy != policies.end()) actors[actor] = NewTracked(policy->second);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorDestroy>([](void* raw) {
        auto found = actors.find(static_cast<Actor*>(raw));
        if (found != actors.end()) { cueMixer.Stop(found->second.identity); actors.erase(found); }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ResetScene);
    running = true;
}

void UpdateCues(PlayState* play) {
    if (!running) return;
    for (size_t i = 0; i < CueNames.size(); ++i) {
        const std::string prefix = std::string(CVAR_SETTING("A11yAudio.")) + CueNames[i];
        levels[i] = CVarGetInteger((prefix + ".Enabled").c_str(), 1) ?
            std::clamp(CVarGetInteger((prefix + ".Volume").c_str(), 10), 0, 100) / 100.0f : 0;
    }
    masterGain = std::clamp(CVarGetInteger(CVAR_SETTING("Volume.Master"), 40), 0, 100) / 100.0f;
    if (!play || !GameInteractor::IsSaveLoaded() || NativeOptions_IsOpen() || play->pauseCtx.state ||
        !GET_PLAYER(play) || (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_IN_CUTSCENE)) {
        cueMixer.StopAll();
        return;
    }
    FindGeometryCues(play);
    UpdateWalls(play);
    if (locationsScene != play->sceneNum) {
        for (const auto& location : locations) cueMixer.Stop(location.tracked.identity);
        locations.clear();
        locationsScene = play->sceneNum;
        for (const auto& location : CueLocations) {
            if (location.scene == play->sceneNum)
                locations.push_back({NewTracked({location.cue, location.range}), &location});
        }
    }
    const Vec3f& origin = GET_PLAYER(play)->actor.world.pos;
    for (auto& [actor, tracked] : actors) {
        const auto& pos = actor->world.pos;
        const float distance = Distance(pos, origin);
        const float xz = DistanceXZ(pos, origin);
        float range = tracked.policy.range, height = tracked.policy.height;
        if (actor->id == ACTOR_EN_WOOD02 && actor->params <= WOOD_TREE_KAKARIKO_ADULT && play->sceneNum == SCENE_HYRULE_FIELD) {
            range = 3000;
            height = 1000;
        }
        ++tracked.frame;
        if (!Available(actor, play) || std::abs(pos.y - origin.y) > height ||
            (actor->room >= 0 && actor->room != play->roomCtx.curRoom.num)) {
            cueMixer.Stop(tracked.identity);
            continue;
        }
        bool pulsed = actor->id == ACTOR_EN_G_SWITCH;
        bool trigger = pulsed && (tracked.frame & 31) == 0;
        if (actor->id == ACTOR_EN_EX_RUPPY || actor->id == ACTOR_ITEM_ETCETERA || actor->id == ACTOR_ITEM_OCARINA) {
            if (play->sceneNum != SCENE_ZORAS_DOMAIN && play->sceneNum != SCENE_HYRULE_FIELD && play->sceneNum != SCENE_LAKE_HYLIA) {
                cueMixer.Stop(tracked.identity);
                continue;
            }
            const unsigned mask = xz < 10 ? 0 : xz < 20 ? 1 : xz < 40 ? 3 : xz < 70 ? 7 : xz < 110 ? 15 : xz < 200 ? 31 : 63;
            pulsed = true;
            trigger = (tracked.frame & mask) == 0;
        }
        UpdateSound(tracked, pos, distance, range, 1, pulsed, trigger);
    }
    for (auto& climb : climbs) {
        Vec3f position = climb.position;
        const float waterHeight = GET_PLAYER(play)->actor.yDistToWater + origin.y;
        if (std::isfinite(waterHeight) && position.y < waterHeight) position.y = waterHeight;
        if (std::abs(position.y - origin.y) >= 80) { cueMixer.Stop(climb.tracked.identity); continue; }
        UpdateSound(climb.tracked, position, Distance(position, origin), climb.tracked.policy.range, 1);
    }
    for (auto& location : locations) {
        Vec3f position;
        if (!LocationPosition(play, *location.location, position)) { cueMixer.Stop(location.tracked.identity); continue; }
        UpdateSound(location.tracked, position, Distance(position, origin), location.tracked.policy.range, 1);
    }
    for (auto& exit : exits) {
        const float distance = Distance(exit.position, origin);
        const float xz = DistanceXZ(exit.position, origin);
        float range, gain;
        if (ExitRange(play, exit, xz, std::abs(exit.position.y - origin.y), distance, range, gain))
            UpdateSound(exit.tracked, exit.position, distance, range, gain);
        else cueMixer.Stop(exit.tracked.identity);
    }
}
void ShutdownCues() { running = false; ResetScene(); }
void SetSceneExits(const int16_t* data, size_t count) { exitList = data; exitCount = count; collision = nullptr; }
CueMixer& GetCueMixer() { return cueMixer; }
float CueMasterGain() { return masterGain; }
}
