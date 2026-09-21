#include "WorldCompass.h"
extern "C" {
#include "z64scene.h"
}

namespace SpatialAudio {
bool HasMapCompass(int16_t scene) {
    // Map_Init and Minimap_Draw establish these axes for these scenes only.
    switch (scene) {
        case SCENE_DEKU_TREE: case SCENE_DODONGOS_CAVERN: case SCENE_JABU_JABU:
        case SCENE_FOREST_TEMPLE: case SCENE_FIRE_TEMPLE: case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE: case SCENE_SHADOW_TEMPLE: case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_HYRULE_FIELD: case SCENE_KAKARIKO_VILLAGE: case SCENE_GRAVEYARD:
        case SCENE_ZORAS_RIVER: case SCENE_KOKIRI_FOREST: case SCENE_SACRED_FOREST_MEADOW:
        case SCENE_LAKE_HYLIA: case SCENE_ZORAS_DOMAIN: case SCENE_ZORAS_FOUNTAIN:
        case SCENE_GERUDO_VALLEY: case SCENE_LOST_WOODS: case SCENE_DESERT_COLOSSUS:
        case SCENE_GERUDOS_FORTRESS: case SCENE_HAUNTED_WASTELAND: case SCENE_HYRULE_CASTLE:
        case SCENE_DEATH_MOUNTAIN_TRAIL: case SCENE_DEATH_MOUNTAIN_CRATER: case SCENE_GORON_CITY:
        case SCENE_LON_LON_RANCH: case SCENE_OUTSIDE_GANONS_CASTLE:
            return true;
        default: return false;
    }
}
}
