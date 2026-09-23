#include "AimCue.h"
#include "CueLocations.h"
#include "CueMixer.h"
#include "SpatialAudio.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/navigation/NavigationTargets.h"
#include "soh/Enhancements/navigation/StepEstimate.h"
#include "soh/Enhancements/speechsynthesizer/SpeechSynthesizer.h"
#include <libultraship/libultraship.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <vector>

extern "C" {
#include "global.h"
#include "overlays/actors/ovl_Bg_Bdan_Switch/z_bg_bdan_switch.h"
#include "overlays/actors/ovl_Bg_Po_Event/z_bg_po_event.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Kakasi2/z_en_kakasi2.h"
#include "overlays/actors/ovl_En_Wonder_Item/z_en_wonder_item.h"
#include "overlays/actors/ovl_Obj_Switch/z_obj_switch.h"
#include "overlays/actors/ovl_Shot_Sun/z_shot_sun.h"
void EnBox_WaitOpen(EnBox*, PlayState*);
void BgPoEvent_PaintingPresent(BgPoEvent*, PlayState*);
}

namespace SpatialAudio {
namespace {
constexpr int Bow = 1, Sling = 2, Shoot = Bow | Sling, Hook = 4, Boom = 8, Cup = 16;
constexpr int AllWeapons = Bow | Sling | Hook | Boom;
constexpr uint64_t AimIdentity = uint64_t{1} << 59;
constexpr float Pi = 3.14159265358979323846f;

struct Tool { int mask = 0; float range = 1000; };
struct Target {
    uint64_t identity = 0;
    Actor* actor = nullptr;
    Vec3f position{};
    float error = std::numeric_limits<float>::infinity();
    float distance = 0;
    int16_t yawDelta = 0;
};

std::set<Actor*> aimActors;
CueMixer* activeMixer = nullptr;
uint64_t selectedTarget = 0;
bool manualTarget = false;
bool cycleReady = false;

void Replace(std::string& text, const char* key, const std::string& value) {
    const auto position = text.find(key);
    if (position != std::string::npos) text.replace(position, 2, value);
}

void Announce(const Target* target, size_t index, size_t count, const Vec3f& origin) {
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || !SpeechSynthesizer::Instance) return;
    std::string text;
    if (!target) text = Navigation::Text("aim_empty");
    else {
        text = Navigation::Text("aim_target");
        Replace(text, "$0", std::to_string(index + 1));
        Replace(text, "$1", std::to_string(count));
        text += ", " + Navigation::Text(target->yawDelta < -0x5b0 ? "aim_left" :
                                        target->yawDelta > 0x5b0 ? "aim_right" : "aim_ahead");
        if (gGameInfo) {
            const Navigation::WalkingStride stride{R_RUN_SPEED_LIMIT / 100.0f, REG(48) / 100.0f,
                REG(35) / 1000.0f, REG(36) / 1000.0f, REG(37) / 1000.0f, REG(38) / 1000.0f,
                R_UPDATE_RATE * 0.5f};
            const int steps = stride.Estimate(target->distance);
            if (steps >= 0) text += ", " + (steps == 0 ? Navigation::Text("distance_near") :
                steps == 1 ? Navigation::Text("distance_one") :
                Navigation::Format("distance", std::to_string(steps)));
        }
        if (target->position.y - origin.y > 40) text += ", " + Navigation::Text("above");
        else if (origin.y - target->position.y > 40) text += ", " + Navigation::Text("below");
    }
    if (!text.empty()) SpeechSynthesizer::Instance->Speak(text.c_str(), "en-US", true);
}

bool Supported(int16_t id) {
    switch (id) {
        case ACTOR_OBJ_SWITCH: case ACTOR_SHOT_SUN: case ACTOR_OBJ_HSBLOCK: case ACTOR_BG_MIZU_MOVEBG:
        case ACTOR_EN_KAKASI2: case ACTOR_EN_BOX: case ACTOR_OBJ_KIBAKO2: case ACTOR_EN_HATA:
        case ACTOR_EN_WONDER_ITEM: case ACTOR_BG_BDAN_SWITCH: case ACTOR_BOSS_GANONDROF:
        case ACTOR_BG_PO_EVENT: case ACTOR_EN_PO_SISTERS: case ACTOR_BG_SPOT06_OBJECTS:
        case ACTOR_BG_MENKURI_EYE: case ACTOR_EN_FIREFLY: case ACTOR_EN_SI: case ACTOR_EN_SW:
        case ACTOR_EN_FZ: case ACTOR_EN_G_SWITCH:
            return true;
        default: return false;
    }
}

Tool HeldTool(const Player* player) {
    if (!(player->stateFlags1 & PLAYER_STATE1_FIRST_PERSON)) return {};
    if (player->unk_6AD != 2) return {Cup, 1000};
    switch (player->heldItemAction) {
        case PLAYER_IA_BOW: case PLAYER_IA_BOW_FIRE: case PLAYER_IA_BOW_ICE:
        case PLAYER_IA_BOW_LIGHT: case PLAYER_IA_BOW_0C: case PLAYER_IA_BOW_0D:
        case PLAYER_IA_BOW_0E: return {Bow, 1000};
        case PLAYER_IA_SLINGSHOT: return {Sling, 1000};
        case PLAYER_IA_HOOKSHOT: return {Hook, 380};
        case PLAYER_IA_LONGSHOT: return {Hook, 770};
        case PLAYER_IA_BOOMERANG: return {Boom, 380};
        case PLAYER_IA_NONE: return {Cup, 1000};
        default: return {};
    }
}

int ActorMask(Actor* actor, PlayState* play) {
    switch (actor->id) {
        case ACTOR_OBJ_SWITCH: {
            const int type = actor->params & 7;
            if (type == OBJSWITCH_TYPE_EYE) {
                const auto* eye = reinterpret_cast<ObjSwitch*>(actor);
                return ((actor->params >> 4) & 7) != 0 || eye->eyeTexIndex == 0 ? Shoot : 0;
            }
            return type == OBJSWITCH_TYPE_CRYSTAL || type == OBJSWITCH_TYPE_CRYSTAL_TARGETABLE ? AllWeapons : 0;
        }
        case ACTOR_SHOT_SUN:
            return (actor->params & 0xff) != 0x40 && (actor->params & 0xff) != 0x41 &&
                   actor->xzDistToPlayer <= 120 && gSaveContext.dayTime >= 0x4555 &&
                   gSaveContext.dayTime < 0x5000 ? Bow : 0;
        case ACTOR_OBJ_HSBLOCK: case ACTOR_OBJ_KIBAKO2: case ACTOR_EN_HATA:
        case ACTOR_BG_SPOT06_OBJECTS: return Hook;
        case ACTOR_BG_MIZU_MOVEBG: return (static_cast<uint16_t>(actor->params) >> 12) == 7 ? Hook : 0;
        case ACTOR_EN_KAKASI2: {
            const int flag = reinterpret_cast<EnKakasi2*>(actor)->switchFlag;
            return flag >= 0 && Flags_GetSwitch(play, flag) ? Hook : 0;
        }
        case ACTOR_EN_BOX:
            return reinterpret_cast<EnBox*>(actor)->actionFunc == EnBox_WaitOpen &&
                   !Flags_GetTreasure(play, actor->params & 0x1f) ? Hook : 0;
        case ACTOR_EN_WONDER_ITEM: {
            const auto* wonder = reinterpret_cast<EnWonderItem*>(actor);
            if (wonder->wonderMode == WONDERITEM_BOMB_SOLDIER) return Sling;
            if (wonder->wonderMode != WONDERITEM_INTERACT_SWITCH) return 0;
            switch (actor->world.rot.z & 0xff) {
                case 1: return Bow;
                case 4: return Sling;
                case 5: return Boom;
                case 6: return Hook;
                default: return 0;
            }
        }
        case ACTOR_BG_BDAN_SWITCH: {
            const int type = actor->params & 0xff;
            return type == YELLOW_TALL_1 || type == YELLOW_TALL_2 ? AllWeapons : 0;
        }
        case ACTOR_BOSS_GANONDROF: return actor->params == 1 ? Shoot | Hook : 0;
        case ACTOR_BG_PO_EVENT:
            return reinterpret_cast<BgPoEvent*>(actor)->actionFunc == BgPoEvent_PaintingPresent ? Bow : 0;
        case ACTOR_EN_PO_SISTERS: return actor->category != ACTORCAT_PROP ? AllWeapons : 0;
        case ACTOR_BG_MENKURI_EYE:
            return std::abs(static_cast<int16_t>(actor->yawTowardsPlayer - actor->shape.rot.y)) < 0x2000 ? Bow : 0;
        case ACTOR_EN_FIREFLY: return Shoot;
        case ACTOR_EN_SI: return Boom | Hook;
        case ACTOR_EN_SW: return AllWeapons | Cup;
        case ACTOR_EN_FZ: return AllWeapons;
        case ACTOR_EN_G_SWITCH: return Shoot | Cup;
        default: return 0;
    }
}

Vec3f AimPoint(Actor* actor) {
    if (actor->id == ACTOR_SHOT_SUN) {
        const auto& hitbox = reinterpret_cast<ShotSun*>(actor)->hitboxPos;
        Vec3f position{static_cast<float>(hitbox.x), static_cast<float>(hitbox.y), static_cast<float>(hitbox.z)};
        position.y += 55;
        return position;
    }
    Vec3f position = actor->world.pos;
    if (actor->id == ACTOR_BG_MIZU_MOVEBG) {
        position.x += Math_SinS(actor->shape.rot.y) * 50;
        position.z += Math_CosS(actor->shape.rot.y) * 50;
    }
    position.y += 25;
    return position;
}

void Stop(CueMixer& mixer) {
    mixer.Stop(AimIdentity);
    selectedTarget = 0;
    manualTarget = false;
    cycleReady = false;
}
}

void InitAimCue(CueMixer& mixer) {
    activeMixer = &mixer;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* raw) {
        auto* actor = static_cast<Actor*>(raw);
        if (Supported(actor->id)) aimActors.insert(actor);
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorDestroy>([](void* raw) {
        auto* actor = static_cast<Actor*>(raw);
        aimActors.erase(actor);
        if (selectedTarget == reinterpret_cast<uintptr_t>(actor) && activeMixer) Stop(*activeMixer);
    });
}

void ResetAimCue() {
    aimActors.clear();
    if (activeMixer) Stop(*activeMixer);
}

void SuspendAimCue(CueMixer& mixer) { Stop(mixer); }

void UpdateAimCue(PlayState* play, CueMixer& mixer) {
    auto* player = GET_PLAYER(play);
    const Tool tool = HeldTool(player);
    const float gain = CVarGetInteger(CVAR_SETTING("A11yAudio.aim.Enabled"), 1) ?
        std::clamp(CVarGetInteger(CVAR_SETTING("A11yAudio.aim.Volume"), 50), 0, 100) / 100.0f : 0;
    if (!tool.mask) { Stop(mixer); return; }

    const bool cycleDown = (play->state.input[0].cur.button & BTN_AIM_CYCLE) != 0;
    const bool cycle = cycleDown && cycleReady;
    if (!cycleDown) cycleReady = true;

    const Vec3f& origin = player->actor.world.pos;
    const Vec3f& eye = player->actor.focus.pos;
    const float aimPitch = -static_cast<float>(player->actor.focus.rot.x) * (Pi / 2) / 14000;
    Target best, previous;
    std::vector<Target> candidates;
    const auto consider = [&](uint64_t identity, Actor* actor, Vec3f position, float range) {
        const float x = position.x - origin.x, y = position.y - origin.y, z = position.z - origin.z;
        const Vec3f& rangePoint = actor ? actor->world.pos : position;
        const float rangeX = rangePoint.x - origin.x, rangeY = rangePoint.y - origin.y;
        const float rangeZ = rangePoint.z - origin.z;
        const float distance = std::sqrt(rangeX * rangeX + rangeY * rangeY + rangeZ * rangeZ);
        if (!std::isfinite(distance) || distance >= range) return;
        const int16_t yawDelta = static_cast<int16_t>(Math_Atan2S(z, x) - player->yaw);
        const float yaw = std::abs(yawDelta) * (2 * Pi / 65536);
        if (yaw > Pi / 2) return;
        const float horizontal = std::sqrt(x * x + z * z);
        const float pitch = std::atan2(position.y - eye.y, horizontal);
        const float error = std::hypot(yaw, pitch - aimPitch);
        if (!std::isfinite(error)) return;
        Target target{identity, actor, position, error, distance, yawDelta};
        candidates.push_back(target);
        if (identity == selectedTarget) previous = target;
        if (!best.identity || error < best.error || (error == best.error && distance < best.distance)) best = target;
    };

    for (Actor* actor : aimActors) {
        if (actor->init || !actor->update || (actor->room >= 0 && actor->room != play->roomCtx.curRoom.num)) continue;
        if (!(ActorMask(actor, play) & tool.mask)) continue;
        if (tool.mask != Cup && !(player->stateFlags1 &
            (PLAYER_STATE1_USING_BOOMERANG | PLAYER_STATE1_ITEM_IN_HAND))) continue;
        consider(reinterpret_cast<uintptr_t>(actor), actor, AimPoint(actor), tool.range);
    }
    for (size_t i = 0; i < std::size(CueLocations); ++i) {
        const auto& location = CueLocations[i];
        if (!(location.aimMask & tool.mask) || location.scene != play->sceneNum ||
            location.room != play->roomCtx.curRoom.num) continue;
        Vec3f position = location.position;
        position.y += 25;
        consider((uint64_t{1} << 62) + i, nullptr, position, std::min(tool.range, location.range));
    }
    std::sort(candidates.begin(), candidates.end(), [](const Target& a, const Target& b) {
        if (a.yawDelta != b.yawDelta) return a.yawDelta < b.yawDelta;
        if (a.distance != b.distance) return a.distance < b.distance;
        return a.identity < b.identity;
    });
    if (!previous.identity) manualTarget = false;
    Target target = manualTarget ? previous :
        previous.identity && previous.error <= best.error + 0.05f ? previous : best;
    if (cycle) {
        if (candidates.empty()) Announce(nullptr, 0, 0, origin);
        else {
            const auto current = std::find_if(candidates.begin(), candidates.end(), [target](const Target& candidate) {
                return candidate.identity == target.identity;
            });
            const size_t next = current == candidates.end() ? 0 :
                (static_cast<size_t>(current - candidates.begin()) + 1) % candidates.size();
            target = candidates[next];
            manualTarget = true;
            Announce(&target, next, candidates.size(), origin);
        }
        cycleReady = false;
    }
    if (!target.identity) {
        mixer.Stop(AimIdentity);
        selectedTarget = 0;
        manualTarget = false;
        return;
    }
    if (target.identity != selectedTarget) mixer.Stop(AimIdentity);
    selectedTarget = target.identity;
    if (gain <= 0) { mixer.Stop(AimIdentity); return; }
    const float seconds = std::clamp(0.16f + target.error * 2.75f, 0.16f, 1.5f);
    const auto source = SpatialAudio_WorldSource(AimIdentity, target.position.x, target.position.y, target.position.z);
    if (!source.identity || !mixer.KeepPlayingPulse(AimIdentity, Cue::Pathfinder, seconds)) {
        Stop(mixer);
        return;
    }
    mixer.Update(AimIdentity, source, gain);
}
}
