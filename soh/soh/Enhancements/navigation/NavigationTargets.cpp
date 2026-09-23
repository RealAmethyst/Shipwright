#include "NavigationTargets.h"
#include "WalkingQuery.h"
#include "ApproachBounds.h"
#include "Destination.h"
#include "soh/Enhancements/tts/SignText.h"
#include "soh/Enhancements/speechsynthesizer/SpeechSynthesizer.h"
#include "soh/SaveManager.h"
#include "soh/Enhancements/audio/spatial/CueActors.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include <algorithm>
#include <map>
#include <set>
extern "C" {
#include "global.h"
#include "message_data_static.h"
#include "overlays/actors/ovl_En_Kanban/z_en_kanban.h"
#include "overlays/actors/ovl_En_Wood02/z_en_wood02.h"
}
extern "C" MessageTableEntry* sNesMessageEntryTablePtr;
extern "C" MessageTableEntry* sGerMessageEntryTablePtr;
extern "C" MessageTableEntry* sFraMessageEntryTablePtr;
std::string remap(uint8_t character);

namespace Navigation {
namespace {
std::map<Actor*, uint64_t> actors;
uint64_t identity = uint64_t{1} << 61;
constexpr const char* categories[]{"people", "items", "chests", "doors", "exits", "switches", "signs",
                                  "destructibles", "climbing", "platforms", "landmarks"};

std::string HouseName(int scene) {
    const int id = HouseSign(scene);
    if (!id || gSaveContext.language == LANGUAGE_JPN) return "";
    auto* table = gSaveContext.language == LANGUAGE_FRA ? sFraMessageEntryTablePtr :
                  gSaveContext.language == LANGUAGE_GER ? sGerMessageEntryTablePtr : sNesMessageEntryTablePtr;
    if (!table) return "";
    // Mido's sign has a subtitle. Its English line boundary is verified; don't
    // assume translated line wrapping places that boundary at the same point.
    if (id == 0x033c && gSaveContext.language != LANGUAGE_ENG) return "";
    static std::map<std::pair<MessageTableEntry*, int>, const MessageTableEntry*> captions;
    const auto key = std::pair{table, id};
    if (!captions.contains(key)) {
        const MessageTableEntry* match = nullptr;
        for (const auto* entry = table; entry->textId != 0xffff; ++entry) {
            if (entry->textId == id) { match = entry; break; }
        }
        captions[key] = match;
    }
    const auto* entry = captions[key];
    if (!entry || !entry->segment || entry->msgSize > 4096) return "";
    const auto name = SpeechText::SavedLatinName(gSaveContext.playerName,
                                                gSaveContext.ship.filenameLanguage == NAME_LANGUAGE_PAL);
    const auto text = SpeechText::SignCaption({entry->segment, entry->msgSize}, name, id == 0x033c, remap);
    return SpeechSynthesizer::PrepareText(text.c_str());
}

std::string DestinationName(int scene) {
    if (auto name = HouseName(scene); !name.empty()) return name;
    // Same native title artwork in scene_table.h; no invented exterior captions.
    if (scene == SCENE_POTION_SHOP_KAKARIKO || scene == SCENE_POTION_SHOP_GRANNY) scene = SCENE_POTION_SHOP_MARKET;
    if (scene == SCENE_MARKET_ENTRANCE_DAY || scene == SCENE_MARKET_ENTRANCE_NIGHT ||
        scene == SCENE_MARKET_ENTRANCE_RUINS) scene = SCENE_MARKET_DAY;
    if (scene == SCENE_TEMPLE_OF_TIME_EXTERIOR_DAY || scene == SCENE_TEMPLE_OF_TIME_EXTERIOR_NIGHT ||
        scene == SCENE_TEMPLE_OF_TIME_EXTERIOR_RUINS) scene = SCENE_TEMPLE_OF_TIME;
    const char* language = gSaveContext.language == LANGUAGE_FRA ? "fra" :
                           gSaveContext.language == LANGUAGE_GER ? "ger" : "eng";
    return NativeOptions::ResourceText(std::string("scenes_") + language, std::to_string(scene));
}

std::string ItemName(Actor* actor) {
    int item = ITEM_NONE;
    // Kaleidoscope's ITEM_HEART_CONTAINER is the quest page's piece counter,
    // not a whole container. Use the port's actual pickup caption instead.
    if (actor->id == ACTOR_ITEM_B_HEART) return NativeOptions::Text("actor_param_actor_en_item00_7");
    else if (actor->id == ACTOR_EN_SI) item = ITEM_SKULL_TOKEN;
    else if (actor->id == ACTOR_BG_TOKI_SWD) item = ITEM_SWORD_MASTER;
    else if (actor->id == ACTOR_EN_ITEM00) {
        switch (actor->params) {
            case ITEM00_RUPEE_GREEN: item = ITEM_RUPEE_GREEN; break;
            case ITEM00_RUPEE_BLUE: item = ITEM_RUPEE_BLUE; break;
            case ITEM00_RUPEE_RED: item = ITEM_RUPEE_RED; break;
            case ITEM00_RUPEE_PURPLE: item = ITEM_RUPEE_PURPLE; break;
            case ITEM00_RUPEE_ORANGE: item = ITEM_RUPEE_GOLD; break;
            case ITEM00_HEART: item = ITEM_HEART; break;
            case ITEM00_HEART_PIECE: item = ITEM_HEART_PIECE; break;
            case ITEM00_HEART_CONTAINER: return NativeOptions::Text("actor_param_actor_en_item00_7");
            case ITEM00_ARROWS_SINGLE: return NativeOptions::Text("actor_param_actor_en_item00_5");
            case ITEM00_ARROWS_SMALL: item = ITEM_ARROWS_SMALL; break;
            case ITEM00_ARROWS_MEDIUM: item = ITEM_ARROWS_MEDIUM; break;
            case ITEM00_ARROWS_LARGE: item = ITEM_ARROWS_LARGE; break;
            case ITEM00_SEEDS: item = ITEM_SEEDS; break;
            case ITEM00_SMALL_KEY: item = ITEM_KEY_SMALL; break;
            case ITEM00_STICK: item = ITEM_STICK; break;
            case ITEM00_NUTS: item = ITEM_NUT; break;
            case ITEM00_BOMBS_A: case ITEM00_BOMBS_B: case ITEM00_BOMBS_SPECIAL: item = ITEM_BOMB; break;
            case ITEM00_BOMBCHU: item = ITEM_BOMBCHU; break;
            case ITEM00_MAGIC_SMALL: item = ITEM_MAGIC_SMALL; break;
            case ITEM00_MAGIC_LARGE: item = ITEM_MAGIC_LARGE; break;
            case ITEM00_SHIELD_DEKU: item = ITEM_SHIELD_DEKU; break;
            case ITEM00_SHIELD_HYLIAN: item = ITEM_SHIELD_HYLIAN; break;
            case ITEM00_TUNIC_GORON: item = ITEM_TUNIC_GORON; break;
            case ITEM00_TUNIC_ZORA: item = ITEM_TUNIC_ZORA; break;
        }
    }
    return item == ITEM_NONE ? "" : NativeOptions::OriginalItemText(item);
}

bool ExtraKind(Actor* actor, Category& category, std::string& key) {
    switch (actor->id) {
        case ACTOR_OBJ_SWITCH: {
            constexpr const char* kinds[]{"floor_switch", "rusted_switch", "eye_switch", "crystal_switch", "crystal_switch"};
            const int type = actor->params & 7;
            if (type >= std::size(kinds)) return false;
            category = Category::Switches; key = kinds[type]; return true;
        }
        case ACTOR_BG_BDAN_SWITCH: case ACTOR_OBJ_LIGHTSWITCH:
            category = Category::Switches; key = "switch"; return true;
        case ACTOR_EN_KANBAN:
            if (actor->params == ENKANBAN_PIECE) return false;
            category = Category::Signs; key = "sign"; return true;
        case ACTOR_EN_A_OBJ:
            if (actor->params != A_OBJ_SIGNPOST_OBLONG && actor->params != A_OBJ_SIGNPOST_ARROW) return false;
            category = Category::Signs; key = "sign"; return true;
        case ACTOR_EN_ISHI: case ACTOR_OBJ_BOMBIWA:
            category = Category::Destructibles; key = "rock"; return true;
        case ACTOR_BG_BREAKWALL:
            if (((actor->params >> 13) & 3) >= 2) return false;
            category = Category::Destructibles; key = "breakable_wall"; return true;
        case ACTOR_BG_BOMBWALL: case ACTOR_BG_MIZU_BWALL:
            category = Category::Destructibles; key = "breakable_wall"; return true;
        case ACTOR_OBJ_COMB:
            category = Category::Destructibles; key = "beehive"; return true;
        case ACTOR_OBJ_OSHIHIKI:
            category = Category::Landmarks; key = "push_block"; return true;
        case ACTOR_OBJ_SYOKUDAI:
            category = Category::Landmarks; key = "torch"; return true;
        case ACTOR_BG_TOKI_SWD:
            category = Category::Landmarks; key = "sword_pedestal"; return true;
        case ACTOR_OBJ_ELEVATOR: case ACTOR_OBJ_LIFT: case ACTOR_BG_HIDAN_FSLIFT:
        case ACTOR_BG_JYA_LIFT: case ACTOR_BG_JYA_1FLIFT:
            category = Category::Platforms; key = "platform"; return true;
        default:
            if (actor->category == ACTORCAT_NPC && (actor->flags & ACTOR_FLAG_FRIENDLY)) {
                category = Category::People; key = "person"; return true;
            }
            return false;
    }
}
}

std::string Text(const std::string& key) { return NativeOptions::ResourceText("navigation_eng", key); }
std::string Format(const std::string& key, const std::string& value) {
    auto text = Text(key);
    if (auto at = text.find("$0"); at != std::string::npos) text.replace(at, 2, value);
    return text;
}
std::string CategoryName(Category category) {
    const auto index = static_cast<size_t>(category);
    return index < std::size(categories) ? Text(std::string("category_") + categories[index]) : "";
}
bool CanApproach(const Target& target, Point point, const WalkingQuery& query) {
    const bool pickup = target.loosePickup;
    if (pickup && !PickupApproach({point.x - target.position.x, point.y - target.position.y, point.z - target.position.z}))
        return false;
    if (!query.Approach(point, target.position, target.radius, target.actor, pickup ? 50 : 30)) return false;
    if (target.actor && (target.actor->id == ACTOR_EN_BOX || target.actor->id == ACTOR_EN_DOOR)) {
        Vec3f position{point.x, point.y, point.z}, local;
        Actor_WorldToActorCoords(target.actor, &local, &position);
        // Position tests from EnBox_WaitOpen and EnDoor_Idle. Facing remains the player's action.
        if (target.actor->id == ACTOR_EN_BOX)
            return ChestApproach({local.x, local.y, local.z});
        return DoorApproach({local.x, local.y, local.z});
    }
    return true;
}
void InitTargets() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorInit>([](void* raw) {
        auto* actor = static_cast<Actor*>(raw);
        Category category; std::string key;
        if (ExtraKind(actor, category, key)) actors[actor] = ++identity;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnActorDestroy>([](void* raw) {
        actors.erase(static_cast<Actor*>(raw));
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(ClearTargets);
}
void ClearTargets() { actors.clear(); }

std::vector<Target> CollectTargets(PlayState* play) {
    std::vector<Target> result;
    if (!play || !GET_PLAYER(play)) return result;
    std::set<Actor*> included;
    for (const auto& source : SpatialAudio::NavigationTargets(play)) {
        // These two rupee types explode on contact; type 4 is gallery scenery.
        if (source.actor && source.actor->id == ACTOR_EN_EX_RUPPY &&
            source.actor->params != 0 && source.actor->params != 3) continue;
        Target target{source.identity, source.actor};
        target.position = {source.x, source.y, source.z};
        std::string key;
        using SpatialAudio::Cue;
        switch (source.cue) {
            case Cue::Person: target.category = Category::People; key = "person"; break;
            case Cue::Item: target.category = Category::Items; key = "item"; break;
            case Cue::Door: target.category = Category::Doors; key = "door"; break;
            case Cue::Transition: target.category = Category::Exits; key = "exit"; break;
            case Cue::Destructible: target.category = Category::Destructibles; key = "destructible"; break;
            case Cue::Crawlspace: target.category = Category::Climbing; key = "crawlspace"; break;
            case Cue::Ladder: target.category = Category::Climbing; key = "ladder"; break;
            case Cue::Elevator: target.category = Category::Platforms; key = "platform"; break;
            case Cue::Pathfinder: target.category = Category::Landmarks; key = "route_marker"; break;
            default: continue;
        }
        if (source.actor) {
            included.insert(source.actor);
            switch (source.actor->id) {
                case ACTOR_EN_BOX: target.category = Category::Chests; key = "chest"; break;
                case ACTOR_OBJ_TSUBO: key = "pot"; break;
                case ACTOR_OBJ_KIBAKO: case ACTOR_OBJ_KIBAKO2: key = "crate"; break;
                case ACTOR_EN_KUSA: key = "grass"; break;
                case ACTOR_EN_WOOD02: key = source.actor->params >= WOOD_BUSH_GREEN_SMALL ? "bush" : "tree"; break;
                case ACTOR_EN_GS: target.category = Category::Signs; key = "gossip_stone"; break;
            }
            target.name = ItemName(source.actor);
            target.radius = std::max(45.0f, std::min(150.0f, source.actor->colChkInfo.cylRadius + 30.0f));
            if (source.actor->id == ACTOR_EN_ITEM00) { target.radius = 60; target.loosePickup = true; }
        }
        if (source.destination >= 0) {
            target.name = DestinationName(source.destination);
        }
        if (target.name.empty()) target.name = Text(key);
        if (!target.name.empty() && Finite(target.position)) result.push_back(std::move(target));
    }
    for (const auto& [actor, id] : actors) {
        if (included.contains(actor) || actor->init || !actor->update || !actor->draw || actor->parent ||
            actor->category == ACTORCAT_ENEMY || actor->category == ACTORCAT_BOSS ||
            (actor->room >= 0 && actor->room != play->roomCtx.curRoom.num)) continue;
        if ((actor->id == ACTOR_BG_BOMBWALL || actor->id == ACTOR_BG_BREAKWALL) &&
            Flags_GetSwitch(play, actor->params & 0x3f)) continue;
        if (actor->id == ACTOR_BG_MIZU_BWALL && Flags_GetSwitch(play, (actor->params >> 8) & 0x3f)) continue;
        Target target{id, actor}; std::string key;
        if (!ExtraKind(actor, target.category, key)) continue;
        target.position = {actor->world.pos.x, actor->world.pos.y, actor->world.pos.z};
        target.name = ItemName(actor);
        if (target.name.empty()) target.name = Text(key);
        target.radius = std::max(45.0f, std::min(150.0f, actor->colChkInfo.cylRadius + 30.0f));
        if (!target.name.empty() && Finite(target.position)) result.push_back(std::move(target));
    }
    const auto& p = GET_PLAYER(play)->actor.world.pos;
    const Point origin{p.x, p.y, p.z};
    std::stable_sort(result.begin(), result.end(), [&](const Target& a, const Target& b) {
        const auto da = Distance(a.position, origin), db = Distance(b.position, origin);
        return da == db ? a.identity < b.identity : da < db;
    });
    return result;
}
}
