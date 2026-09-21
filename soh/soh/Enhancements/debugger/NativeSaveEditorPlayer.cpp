#include "NativeSaveEditor.h"
#include "soh/cvar_prefixes.h"
#include <libultraship/bridge.h>

extern "C" {
#include "global.h"
}

namespace NativeSaveEditor {
namespace {
Guard PlayerGuard(Player* player) {
    auto scene = CurrentSceneGuard();
    return [scene, player] { return scene() && GET_PLAYER(gPlayState) == player; };
}

template<class V> N::PagePtr VectorPage(const char* key, V& vector, Guard guard) {
    auto* pointer = &vector;
    return N::MakePage(std::string("save/player/") + key, Text(key), [pointer, guard] {
        if (!guard()) return std::vector<N::Row>{};
        return std::vector<N::Row>{Scalar("x", "X", pointer->x, "", guard),
                                  Scalar("y", "Y", pointer->y, "", guard),
                                  Scalar("z", "Z", pointer->z, "", guard)};
    });
}

N::PagePtr CurrentItemsPage(bool dpad) {
    return N::MakePage(dpad ? "save/dpad_items" : "save/current_items",
                      Text(dpad ? "dpad_items" : "current_items"), [dpad] {
        const char* keys[] = {"b_button", "c_left", "c_down", "c_right",
                              "dpad_up", "dpad_down", "dpad_left", "dpad_right"};
        std::vector<N::Row> rows;
        for (int i = dpad ? 4 : 0; i < (dpad ? 8 : 4); ++i) {
            auto row = Scalar(keys[i], Text(keys[i]), gSaveContext.equips.buttonItems[i],
                              ItemName(gSaveContext.equips.buttonItems[i]));
            row.image = ItemImage(gSaveContext.equips.buttonItems[i]);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

N::PagePtr CurrentEquipmentPage() {
    return N::MakePage("save/current_equipment", Text("current_equipment"), [] {
        std::vector<N::Row> rows;
        if (!gPlayState || !GET_PLAYER(gPlayState)) return rows;
        auto* player = GET_PLAYER(gPlayState);
        auto guard = PlayerGuard(player);
        std::map<int, std::string> swords;
        for (int item : {ITEM_NONE, ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER, ITEM_SWORD_BGS})
            swords[item] = ItemName(item);
        swords[ITEM_FISHING_POLE] = Text("fishing_pole");
        rows.push_back(N::Choice("sword", Text("sword"), player->currentSwordItemId, swords, [player, guard](int item) {
            if (!guard()) return;
            int equipment;
            switch (item) {
                case ITEM_NONE: equipment = EQUIP_VALUE_SWORD_NONE; break;
                case ITEM_SWORD_KOKIRI: equipment = EQUIP_VALUE_SWORD_KOKIRI; break;
                case ITEM_SWORD_MASTER:
                case ITEM_FISHING_POLE: equipment = EQUIP_VALUE_SWORD_MASTER; break;
                case ITEM_SWORD_BGS: equipment = EQUIP_VALUE_SWORD_BIGGORON; break;
                default: return;
            }
            player->currentSwordItemId = item;
            gSaveContext.equips.buttonItems[0] = item;
            if (item == ITEM_SWORD_BGS) {
                if (gSaveContext.swordHealth < 8) gSaveContext.swordHealth = 8;
                if (!gSaveContext.bgsFlag) gSaveContext.equips.buttonItems[0] = ITEM_SWORD_KNIFE;
            }
            Inventory_ChangeEquipment(EQUIP_TYPE_SWORD, equipment);
        }));
        const auto addEquipment = [&](const char* key, int category, auto& field,
                                      std::map<int, std::pair<int, int>> values) {
            std::map<int, std::string> names;
            for (const auto& [value, entry] : values) names[value] = ItemName(entry.first);
            auto* pointer = &field;
            auto row = N::Choice(key, Text(key), field, names, [pointer, guard, values, category](int value) {
                if (!guard() || !values.contains(value)) return;
                *pointer = value;
                Inventory_ChangeEquipment(category, values.at(value).second);
            });
            if (values.contains(field)) row.image = ItemImage(values.at(field).first);
            rows.push_back(std::move(row));
        };
        addEquipment("shield", EQUIP_TYPE_SHIELD, player->currentShield,
            {{PLAYER_SHIELD_NONE, {ITEM_NONE, EQUIP_VALUE_SHIELD_NONE}},
             {PLAYER_SHIELD_DEKU, {ITEM_SHIELD_DEKU, EQUIP_VALUE_SHIELD_DEKU}},
             {PLAYER_SHIELD_HYLIAN, {ITEM_SHIELD_HYLIAN, EQUIP_VALUE_SHIELD_HYLIAN}},
             {PLAYER_SHIELD_MIRROR, {ITEM_SHIELD_MIRROR, EQUIP_VALUE_SHIELD_MIRROR}}});
        addEquipment("tunic", EQUIP_TYPE_TUNIC, player->currentTunic,
            {{PLAYER_TUNIC_KOKIRI, {ITEM_TUNIC_KOKIRI, EQUIP_VALUE_TUNIC_KOKIRI}},
             {PLAYER_TUNIC_GORON, {ITEM_TUNIC_GORON, EQUIP_VALUE_TUNIC_GORON}},
             {PLAYER_TUNIC_ZORA, {ITEM_TUNIC_ZORA, EQUIP_VALUE_TUNIC_ZORA}}});
        addEquipment("boots", EQUIP_TYPE_BOOTS, player->currentBoots,
            {{PLAYER_BOOTS_KOKIRI, {ITEM_BOOTS_KOKIRI, EQUIP_VALUE_BOOTS_KOKIRI}},
             {PLAYER_BOOTS_IRON, {ITEM_BOOTS_IRON, EQUIP_VALUE_BOOTS_IRON}},
             {PLAYER_BOOTS_HOVER, {ITEM_BOOTS_HOVER, EQUIP_VALUE_BOOTS_HOVER}}});
        return rows;
    });
}
}

N::PagePtr PlayerPage() {
    return N::MakePage("save/player", Text("player"), [] {
        std::vector<N::Row> rows;
        if (!gPlayState || !GET_PLAYER(gPlayState)) return rows;
        auto* player = GET_PLAYER(gPlayState);
        auto guard = PlayerGuard(player);
        rows.push_back(N::Link("position", Text("position"), [player, guard] {
            return guard() ? VectorPage("position", player->actor.world.pos, guard) : nullptr;
        }));
        rows.push_back(N::Link("rotation", Text("rotation"), [player, guard] {
            return guard() ? VectorPage("rotation", player->actor.world.rot, guard) : nullptr;
        }, Text("rotation_help")));
        rows.push_back(N::Link("model_rotation", Text("model_rotation"), [player, guard] {
            return guard() ? VectorPage("model_rotation", player->actor.shape.rot, guard) : nullptr;
        }, Text("model_rotation_help")));
        rows.push_back(Scalar("linear_velocity", Text("linear_velocity"), player->linearVelocity, Text("linear_velocity_help"), guard));
        rows.push_back(Scalar("y_velocity", Text("y_velocity"), player->actor.velocity.y, Text("y_velocity_help"), guard));
        rows.push_back(Scalar("wall_height", Text("wall_height"), player->yDistToLedge, Text("wall_height_help"), guard));
        rows.push_back(Scalar("invincibility", Text("invincibility"), player->invincibilityTimer, Text("invincibility_help"), guard));
        rows.push_back(Scalar("gravity", Text("gravity"), player->actor.gravity, Text("gravity_help"), guard));
        rows.push_back(N::Choice("load_age", Text("load_age"), gPlayState->linkAgeOnLoad,
            {{0, N::Text("adult")}, {1, N::Text("child")}}, [guard](int value) {
                if (guard()) gPlayState->linkAgeOnLoad = value;
            }, Text("load_age_help")));
        rows.push_back(N::Link("equipment", Text("current_equipment"), CurrentEquipmentPage));
        rows.push_back(N::Link("items", Text("current_items"), [] { return CurrentItemsPage(false); }));
        if (CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0))
            rows.push_back(N::Link("dpad", Text("dpad_items"), [] { return CurrentItemsPage(true); }));
        rows.push_back(N::Link("state", Text("player_state"), PlayerFlagsPage));
        auto sword = N::Action("sword_state", Text("sword"), [] { N::ReadCurrentDescription(); });
        sword.value = std::to_string(player->meleeWeaponState);
        rows.push_back(std::move(sword));
        return rows;
    });
}
}
