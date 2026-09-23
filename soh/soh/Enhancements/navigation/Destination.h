#pragma once
#include "global.h"

namespace Navigation {
// Scene identities from scene_table.h and entrance_table.h. Buildings use the
// door recording even when their threshold is a collision exit, not EnDoor.
inline bool Building(int scene) {
    switch (scene) {
        case SCENE_KNOW_IT_ALL_BROS_HOUSE: case SCENE_TWINS_HOUSE: case SCENE_MIDOS_HOUSE:
        case SCENE_SARIAS_HOUSE: case SCENE_KAKARIKO_CENTER_GUEST_HOUSE: case SCENE_BACK_ALLEY_HOUSE:
        case SCENE_BAZAAR: case SCENE_KOKIRI_SHOP: case SCENE_GORON_SHOP: case SCENE_ZORA_SHOP:
        case SCENE_POTION_SHOP_KAKARIKO: case SCENE_POTION_SHOP_MARKET: case SCENE_BOMBCHU_SHOP:
        case SCENE_HAPPY_MASK_SHOP: case SCENE_LINKS_HOUSE: case SCENE_DOG_LADY_HOUSE:
        case SCENE_STABLE: case SCENE_IMPAS_HOUSE: case SCENE_LAKESIDE_LABORATORY:
        case SCENE_GRAVEKEEPERS_HUT: case SCENE_SHOOTING_GALLERY: case SCENE_BOMBCHU_BOWLING_ALLEY:
        case SCENE_LON_LON_BUILDINGS: case SCENE_MARKET_GUARD_HOUSE: case SCENE_POTION_SHOP_GRANNY:
        case SCENE_HOUSE_OF_SKULLTULA: case SCENE_TREASURE_BOX_SHOP: return true;
        default: return false;
    }
}
inline int HouseSign(int scene) {
    switch (scene) {
        case SCENE_LINKS_HOUSE: return 0x031f;
        case SCENE_MIDOS_HOUSE: return 0x033c;
        case SCENE_KNOW_IT_ALL_BROS_HOUSE: return 0x033d;
        case SCENE_TWINS_HOUSE: return 0x033e;
        case SCENE_SARIAS_HOUSE: return 0x033f;
        default: return 0;
    }
}
}
