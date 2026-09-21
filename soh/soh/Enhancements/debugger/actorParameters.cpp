#include "actorParameters.h"
#include "soh/ActorDB.h"

extern "C" {
#include "z64actor.h"
}

namespace {
namespace N = NativeOptions;
std::map<int, std::string> Choices(uint16_t id) {
    if (id == ACTOR_EN_TITE) return {{-2, N::Text("actor_param_actor_en_tite_0")}, {-1, N::Text("actor_param_actor_en_tite_1")}};
    if (id == ACTOR_EN_AM) return {{0, N::Text("actor_param_actor_en_am_0")}, {1, N::Text("actor_param_actor_en_am_1")}};
    if (id == ACTOR_BG_ICE_TURARA) return {{0, N::Text("actor_param_actor_bg_ice_turara_0")}, {1, N::Text("actor_param_actor_bg_ice_turara_1")}, {2, N::Text("actor_param_actor_bg_ice_turara_2")}};
    if (id == ACTOR_BG_BREAKWALL) return {{0, N::Text("actor_param_actor_bg_breakwall_0")}, {1, N::Text("actor_param_actor_bg_breakwall_1")}, {2, N::Text("actor_param_actor_bg_breakwall_2")}, {3, N::Text("actor_param_actor_bg_breakwall_3")}};
    if (id == ACTOR_EN_TEST) return {{0, N::Text("actor_param_actor_en_test_0")}, {1, N::Text("actor_param_actor_en_test_1")}, {2, N::Text("actor_param_actor_en_test_2")}, {3, N::Text("actor_param_actor_en_test_3")}, {4, N::Text("actor_param_actor_en_test_4")}, {5, N::Text("actor_param_actor_en_test_5")}};
    if (id == ACTOR_EN_TANA) return {{0, N::Text("actor_param_actor_en_tana_0")}, {1, N::Text("actor_param_actor_en_tana_1")}, {2, N::Text("actor_param_actor_en_tana_2")}};
    if (id == ACTOR_EN_XC) return {{0, N::Text("actor_param_actor_en_xc_0")}, {1, N::Text("actor_param_actor_en_xc_1")}, {2, N::Text("actor_param_actor_en_xc_2")}, {3, N::Text("actor_param_actor_en_xc_3")}, {4, N::Text("actor_param_actor_en_xc_4")}, {5, N::Text("actor_param_actor_en_xc_5")}, {6, N::Text("actor_param_actor_en_xc_6")}, {7, N::Text("actor_param_actor_en_xc_7")}, {8, N::Text("actor_param_actor_en_xc_8")}, {9, N::Text("actor_param_actor_en_xc_9")}};
    if (id == ACTOR_SHOT_SUN) return {{64, N::Text("actor_param_actor_shot_sun_0")}, {65, N::Text("actor_param_actor_shot_sun_1")}, {66, N::Text("actor_param_actor_shot_sun_2")}};
    if (id == ACTOR_EN_HONOTRAP) return {{0, N::Text("actor_param_actor_en_honotrap_0")}, {1, N::Text("actor_param_actor_en_honotrap_1")}, {2, N::Text("actor_param_actor_en_honotrap_2")}};
    if (id == ACTOR_EN_ITEM00) return {{0, N::Text("actor_param_actor_en_item00_0")}, {1, N::Text("actor_param_actor_en_item00_1")}, {2, N::Text("actor_param_actor_en_item00_2")}, {3, N::Text("actor_param_actor_en_item00_3")}, {4, N::Text("actor_param_actor_en_item00_4")}, {5, N::Text("actor_param_actor_en_item00_5")}, {6, N::Text("actor_param_actor_en_item00_6")}, {7, N::Text("actor_param_actor_en_item00_7")}, {8, N::Text("actor_param_actor_en_item00_8")}, {9, N::Text("actor_param_actor_en_item00_9")}, {10, N::Text("actor_param_actor_en_item00_10")}, {11, N::Text("actor_param_actor_en_item00_11")}, {12, N::Text("actor_param_actor_en_item00_12")}, {13, N::Text("actor_param_actor_en_item00_13")}, {14, N::Text("actor_param_actor_en_item00_14")}, {15, N::Text("actor_param_actor_en_item00_15")}, {16, N::Text("actor_param_actor_en_item00_16")}, {17, N::Text("actor_param_actor_en_item00_17")}, {18, N::Text("actor_param_actor_en_item00_18")}, {19, N::Text("actor_param_actor_en_item00_19")}, {20, N::Text("actor_param_actor_en_item00_20")}, {21, N::Text("actor_param_actor_en_item00_21")}, {22, N::Text("actor_param_actor_en_item00_22")}, {23, N::Text("actor_param_actor_en_item00_23")}, {24, N::Text("actor_param_actor_en_item00_24")}, {25, N::Text("actor_param_actor_en_item00_25")}, {26, N::Text("actor_param_actor_en_item00_26")}};
    if (id == ACTOR_OBJ_COMB) return {{0, N::Text("actor_param_actor_obj_comb_0")}, {1, N::Text("actor_param_actor_obj_comb_1")}, {2, N::Text("actor_param_actor_obj_comb_2")}, {3, N::Text("actor_param_actor_obj_comb_3")}, {4, N::Text("actor_param_actor_obj_comb_4")}, {5, N::Text("actor_param_actor_obj_comb_5")}, {6, N::Text("actor_param_actor_obj_comb_6")}, {7, N::Text("actor_param_actor_obj_comb_7")}, {8, N::Text("actor_param_actor_obj_comb_8")}, {9, N::Text("actor_param_actor_obj_comb_9")}, {10, N::Text("actor_param_actor_obj_comb_10")}, {11, N::Text("actor_param_actor_obj_comb_11")}, {12, N::Text("actor_param_actor_obj_comb_12")}, {13, N::Text("actor_param_actor_obj_comb_13")}, {14, N::Text("actor_param_actor_obj_comb_14")}, {15, N::Text("actor_param_actor_obj_comb_15")}, {16, N::Text("actor_param_actor_obj_comb_16")}, {17, N::Text("actor_param_actor_obj_comb_17")}, {18, N::Text("actor_param_actor_obj_comb_18")}, {19, N::Text("actor_param_actor_obj_comb_19")}, {20, N::Text("actor_param_actor_obj_comb_20")}, {21, N::Text("actor_param_actor_obj_comb_21")}, {22, N::Text("actor_param_actor_obj_comb_22")}, {23, N::Text("actor_param_actor_obj_comb_23")}, {24, N::Text("actor_param_actor_obj_comb_24")}, {25, N::Text("actor_param_actor_obj_comb_25")}, {26, N::Text("actor_param_actor_obj_comb_26")}};
    if (id == ACTOR_EN_GIRLA) return {{0, N::Text("actor_param_actor_en_girla_0")}, {1, N::Text("actor_param_actor_en_girla_1")}, {2, N::Text("actor_param_actor_en_girla_2")}, {3, N::Text("actor_param_actor_en_girla_3")}, {4, N::Text("actor_param_actor_en_girla_4")}, {5, N::Text("actor_param_actor_en_girla_5")}, {6, N::Text("actor_param_actor_en_girla_6")}, {7, N::Text("actor_param_actor_en_girla_7")}, {8, N::Text("actor_param_actor_en_girla_8")}, {9, N::Text("actor_param_actor_en_girla_9")}, {10, N::Text("actor_param_actor_en_girla_10")}, {11, N::Text("actor_param_actor_en_girla_11")}, {12, N::Text("actor_param_actor_en_girla_12")}, {13, N::Text("actor_param_actor_en_girla_13")}, {14, N::Text("actor_param_actor_en_girla_14")}, {15, N::Text("actor_param_actor_en_girla_15")}, {16, N::Text("actor_param_actor_en_girla_16")}, {17, N::Text("actor_param_actor_en_girla_17")}, {18, N::Text("actor_param_actor_en_girla_18")}, {19, N::Text("actor_param_actor_en_girla_19")}, {20, N::Text("actor_param_actor_en_girla_20")}, {21, N::Text("actor_param_actor_en_girla_21")}, {22, N::Text("actor_param_actor_en_girla_22")}, {23, N::Text("actor_param_actor_en_girla_23")}, {24, N::Text("actor_param_actor_en_girla_24")}, {25, N::Text("actor_param_actor_en_girla_25")}, {26, N::Text("actor_param_actor_en_girla_26")}, {27, N::Text("actor_param_actor_en_girla_27")}, {28, N::Text("actor_param_actor_en_girla_28")}, {29, N::Text("actor_param_actor_en_girla_29")}, {30, N::Text("actor_param_actor_en_girla_30")}, {31, N::Text("actor_param_actor_en_girla_31")}, {32, N::Text("actor_param_actor_en_girla_32")}, {33, N::Text("actor_param_actor_en_girla_33")}, {34, N::Text("actor_param_actor_en_girla_34")}, {35, N::Text("actor_param_actor_en_girla_35")}, {36, N::Text("actor_param_actor_en_girla_36")}, {37, N::Text("actor_param_actor_en_girla_37")}, {38, N::Text("actor_param_actor_en_girla_38")}, {39, N::Text("actor_param_actor_en_girla_39")}, {40, N::Text("actor_param_actor_en_girla_40")}, {41, N::Text("actor_param_actor_en_girla_41")}, {42, N::Text("actor_param_actor_en_girla_42")}, {43, N::Text("actor_param_actor_en_girla_43")}, {44, N::Text("actor_param_actor_en_girla_44")}, {45, N::Text("actor_param_actor_en_girla_45")}, {46, N::Text("actor_param_actor_en_girla_46")}, {47, N::Text("actor_param_actor_en_girla_47")}, {48, N::Text("actor_param_actor_en_girla_48")}, {49, N::Text("actor_param_actor_en_girla_49")}, {50, N::Text("actor_param_actor_en_girla_50")}};
    if (id == ACTOR_EN_FIRE_ROCK) return {{0, N::Text("actor_param_actor_en_fire_rock_0")}, {1, N::Text("actor_param_actor_en_fire_rock_1")}, {2, N::Text("actor_param_actor_en_fire_rock_2")}, {3, N::Text("actor_param_actor_en_fire_rock_3")}, {5, N::Text("actor_param_actor_en_fire_rock_4")}, {6, N::Text("actor_param_actor_en_fire_rock_5")}};
    if (id == ACTOR_EN_EX_ITEM) return {{0, N::Text("actor_param_actor_en_ex_item_0")}, {1, N::Text("actor_param_actor_en_ex_item_1")}, {2, N::Text("actor_param_actor_en_ex_item_2")}, {3, N::Text("actor_param_actor_en_ex_item_3")}, {4, N::Text("actor_param_actor_en_ex_item_4")}, {5, N::Text("actor_param_actor_en_ex_item_5")}, {6, N::Text("actor_param_actor_en_ex_item_6")}, {7, N::Text("actor_param_actor_en_ex_item_7")}, {8, N::Text("actor_param_actor_en_ex_item_8")}, {9, N::Text("actor_param_actor_en_ex_item_9")}, {10, N::Text("actor_param_actor_en_ex_item_10")}, {11, N::Text("actor_param_actor_en_ex_item_11")}, {12, N::Text("actor_param_actor_en_ex_item_12")}, {13, N::Text("actor_param_actor_en_ex_item_13")}, {14, N::Text("actor_param_actor_en_ex_item_14")}, {15, N::Text("actor_param_actor_en_ex_item_15")}, {16, N::Text("actor_param_actor_en_ex_item_16")}, {17, N::Text("actor_param_actor_en_ex_item_17")}, {18, N::Text("actor_param_actor_en_ex_item_18")}, {19, N::Text("actor_param_actor_en_ex_item_19")}};
    if (id == ACTOR_EN_ELF) return {{0, N::Text("actor_param_actor_en_elf_0")}, {1, N::Text("actor_param_actor_en_elf_1")}, {2, N::Text("actor_param_actor_en_elf_2")}, {3, N::Text("actor_param_actor_en_elf_3")}, {4, N::Text("actor_param_actor_en_elf_4")}, {5, N::Text("actor_param_actor_en_elf_5")}, {6, N::Text("actor_param_actor_en_elf_6")}, {7, N::Text("actor_param_actor_en_elf_7")}};
    if (id == ACTOR_EN_CLEAR_TAG) return {{0, N::Text("actor_param_actor_en_clear_tag_0")}, {1, N::Text("actor_param_actor_en_clear_tag_1")}, {100, N::Text("actor_param_actor_en_clear_tag_2")}};
    if (id == ACTOR_EN_BOMBF) return {{-1, N::Text("actor_param_actor_en_bombf_0")}, {0, N::Text("actor_param_actor_en_bombf_1")}, {1, N::Text("actor_param_actor_en_bombf_2")}};
    if (id == ACTOR_EN_BOM) return {{0, N::Text("actor_param_actor_en_bom_0")}, {1, N::Text("actor_param_actor_en_bom_1")}};
    if (id == ACTOR_DOOR_WARP1) return {{-2, N::Text("actor_param_actor_door_warp1_0")}, {-1, N::Text("actor_param_actor_door_warp1_1")}, {0, N::Text("actor_param_actor_door_warp1_2")}, {1, N::Text("actor_param_actor_door_warp1_3")}, {2, N::Text("actor_param_actor_door_warp1_4")}, {3, N::Text("actor_param_actor_door_warp1_5")}, {4, N::Text("actor_param_actor_door_warp1_6")}, {5, N::Text("actor_param_actor_door_warp1_7")}, {6, N::Text("actor_param_actor_door_warp1_8")}, {7, N::Text("actor_param_actor_door_warp1_9")}, {8, N::Text("actor_param_actor_door_warp1_10")}, {9, N::Text("actor_param_actor_door_warp1_11")}, {10, N::Text("actor_param_actor_door_warp1_12")}};
    if (id == ACTOR_EN_DY_EXTRA) return {{0, N::Text("actor_param_actor_en_dy_extra_0")}, {1, N::Text("actor_param_actor_en_dy_extra_1")}};
    if (id == ACTOR_EN_WF) return {{0, N::Text("actor_param_actor_en_wf_0")}, {1, N::Text("actor_param_actor_en_wf_1")}};
    if (id == ACTOR_EN_BOX) return {{0, N::Text("actor_param_actor_en_box_0")}, {1, N::Text("actor_param_actor_en_box_1")}, {2, N::Text("actor_param_actor_en_box_2")}, {3, N::Text("actor_param_actor_en_box_3")}, {4, N::Text("actor_param_actor_en_box_4")}, {5, N::Text("actor_param_actor_en_box_5")}, {6, N::Text("actor_param_actor_en_box_6")}, {7, N::Text("actor_param_actor_en_box_7")}, {8, N::Text("actor_param_actor_en_box_8")}, {9, N::Text("actor_param_actor_en_box_9")}, {10, N::Text("actor_param_actor_en_box_10")}, {11, N::Text("actor_param_actor_en_box_11")}};
    if (id == ACTOR_EN_DOOR) return {{0, N::Text("actor_param_actor_en_door_0")}, {1, N::Text("actor_param_actor_en_door_1")}, {2, N::Text("actor_param_actor_en_door_2")}, {3, N::Text("actor_param_actor_en_door_3")}, {4, N::Text("actor_param_actor_en_door_4")}, {5, N::Text("actor_param_actor_en_door_5")}, {6, N::Text("actor_param_actor_en_door_6")}, {7, N::Text("actor_param_actor_en_door_7")}};
    if (id == ACTOR_EN_KUSA) return {{0, N::Text("actor_param_actor_en_kusa_0")}, {1, N::Text("actor_param_actor_en_kusa_1")}, {2, N::Text("actor_param_actor_en_kusa_2")}};
    if (id == ActorDB::Instance->RetrieveId("En_Partner")) return {{0, N::Text("actor_param_partner_0")}, {1, N::Text("actor_param_partner_1")}, {2, N::Text("actor_param_partner_2")}, {3, N::Text("actor_param_partner_3")}};
    return {};
}
}

std::vector<NativeOptions::Row> ActorParameterRows(uint16_t id, uint16_t params, std::function<void(uint16_t)> set) {
    std::vector<N::Row> rows;
    auto integer = [&](const char* key, uint16_t mask, int shift, int maximum) {
        rows.push_back(N::Integer(key, N::Text(key), (params & mask) >> shift, 0, maximum, 1,
            [=](int value) { set((params & ~mask) | ((value << shift) & mask)); }));
    };
    auto flag = [&](const char* key, uint16_t mask) {
        rows.push_back(N::Toggle(key, N::Text(key), (params & mask) != 0,
            [=](bool value) { set(value ? params | mask : params & ~mask); }));
    };
    auto choice = [&](const char* key, uint16_t mask = 0xFFFF, int shift = 0) {
        rows.push_back(N::Choice(key, N::Text(key), mask == 0xFFFF ? static_cast<int16_t>(params) : (params & mask) >> shift,
            Choices(id), [=](int value) { set((params & ~mask) | ((static_cast<uint16_t>(value) << shift) & mask)); }));
    };
    switch (id) {
        case ACTOR_EN_DEKUNUTS:
            rows.push_back(N::Toggle("actor_flower", N::Text("actor_flower"), params == 10,
                [=](bool value) { set(value ? 10 : 0x100); }));
            if (params != 10) {
                const int shots = params >> 8;
                rows.push_back(N::Integer("actor_shots", N::Text("actor_shots"), shots == 0 || shots == 255 ? 1 : shots,
                    1, 254, 1, [=](int value) { set(value << 8); }));
            }
            break;
        case ACTOR_EN_REEBA:
            rows.push_back(N::Toggle("actor_big", N::Text("actor_big"), params != 0,
                [=](bool value) { set(value); }));
            break;
        case ACTOR_EN_TK:
            rows.push_back(N::Toggle("actor_turn", N::Text("actor_turn"), static_cast<int16_t>(params) >= 0,
                [=](bool value) { set(value ? 0 : 0xFFFF); }));
            break;
        case ACTOR_EN_ITEM00:
            flag("actor_auto_collect", 0x8000);
            integer("actor_collectible_flag", 0x3F00, 8, 0x3F);
            choice("actor_item", 0xFF);
            break;
        case ACTOR_OBJ_COMB:
            choice("actor_item_drop", 0xFF);
            if ((params & 0xFF) == 6) integer("actor_poh_flag", 0x3F00, 8, 0x3F);
            break;
        case ACTOR_EN_GM:
            integer("actor_switch_flag", 0x3F00, 8, 0x3F);
            break;
        case ACTOR_EN_SKB:
            integer("actor_size", 0xFF, 0, 255);
            break;
        case ACTOR_EN_WF:
            choice("actor_type", 0xFF);
            integer("actor_switch_flag", 0xFF00, 8, 255);
            break;
        case ACTOR_EN_BOX:
            integer("actor_treasure_flag", 0x1F, 0, 0x1F);
            integer("actor_item_id", 0xFE0, 5, 0x7F);
            choice("actor_type", 0xF000, 12);
            break;
        case ACTOR_EN_DOOR:
            integer("actor_transition_index", 0xFC00, 10, 0x3F);
            choice("actor_type", 0x380, 7);
            flag("actor_double_door", 0x40);
            if (((params >> 7) & 7) == 1) integer("actor_switch_flag", 0x3F, 0, 0x3F);
            if (((params >> 7) & 7) == 5) integer("actor_door_text", 0x3F, 0, 0x3F);
            break;
        case ACTOR_EN_PO_DESERT:
            integer("actor_path", 0xFF00, 8, 255);
            break;
        case ACTOR_EN_KANBAN:
            rows.push_back(N::Toggle("actor_piece", N::Text("actor_piece"), params == 0xFFDD,
                [=](bool value) { set(value ? 0xFFDD : 0); }));
            rows.push_back(N::Toggle("actor_fishing_sign", N::Text("actor_fishing_sign"), params == 0x300,
                [=](bool value) { set(value ? 0x300 : 0); }));
            if (params != 0xFFDD && params != 0x300) integer("actor_text_id", 0xFF, 0, 255);
            break;
        case ACTOR_EN_KUSA:
            choice("actor_type", 3);
            flag("actor_bugs", 0x10);
            if ((params & 3) == 2) integer("actor_drop_params", 0xF00, 8, 0xD);
            break;
        default:
            if (!Choices(id).empty())
                choice(id == ACTOR_EN_DY_EXTRA ? "actor_color" :
                       id == ActorDB::Instance->RetrieveId("En_Partner") ? "actor_controller_port" : "actor_type");
            break;
    }
    return rows;
}
