#pragma once

#include "CueMixer.h"
#include "z64scene.h"

namespace SpatialAudio {
enum class PointRule { Always, ForestBasementLeft, ForestBasementRight, ForestTwistedHallway };
struct CueLocation {
    int16_t scene, room;
    Cue cue;
    Vec3f position;
    float range;
    PointRule rule;
    int aimMask = 0;
};
// Scene points and ranges from PR 5435, head 257c6c0dfdf9cf9330735b43bc7512e1026d6fae.
// Exact duplicate points are merged. The separately unassigned sword pedestal is omitted.
inline constexpr CueLocation CueLocations[] = {
    {SCENE_KOKIRI_FOREST, 0, Cue::Crawlspace, {-784.0f, 120.0f, 1046.0f}, 2000, PointRule::Always},
    {SCENE_KOKIRI_FOREST, 0, Cue::Pathfinder, {2146.0f, 1.0f, -142.8f}, 1000, PointRule::Always},
    {SCENE_KOKIRI_FOREST, 2, Cue::Crawlspace, {-788.0f, 120.0f, 1392.0f}, 2000, PointRule::Always},
    {SCENE_DESERT_COLOSSUS, 0, Cue::Pathfinder, {2259.0f, 108.0f, -1550.0f}, 1000, PointRule::Always},
    {SCENE_LOST_WOODS, 1, Cue::Pathfinder, {1348.0f, 25.0f, -25.0f}, 700, PointRule::Always, 2},
    {SCENE_DEKU_TREE, 7, Cue::Crawlspace, {-1209.0f, -820.0f, 3.5f}, 2000, PointRule::Always},
    {SCENE_DEKU_TREE, 3, Cue::Crawlspace, {-901.0f, -820.0f, 0.5f}, 2000, PointRule::Always},
    {SCENE_DEKU_TREE, 3, Cue::Pathfinder, {-181.76f, -905.0f, -28.3f}, 1000, PointRule::Always},
    {SCENE_DODONGOS_CAVERN, 0, Cue::Pathfinder, {-80.0f, 310.0f, -1540.0f}, 150, PointRule::Always},
    {SCENE_DODONGOS_CAVERN, 0, Cue::Pathfinder, {80.0f, 310.0f, -1540.0f}, 150, PointRule::Always},
    {SCENE_DODONGOS_CAVERN, 0, Cue::Pathfinder, {-80.0f, 510.0f, -1540.0f}, 150, PointRule::Always},
    {SCENE_DODONGOS_CAVERN, 0, Cue::Pathfinder, {80.0f, 510.0f, -1540.0f}, 150, PointRule::Always},
    {SCENE_DODONGOS_CAVERN, 2, Cue::Pathfinder, {-1958.0f, 20.0f, -1297.0f}, 1000, PointRule::Always},
    {SCENE_JABU_JABU, 2, Cue::Pathfinder, {-260.0f, -400.0f, -3377.0f}, 200, PointRule::Always},
    {SCENE_JABU_JABU, 2, Cue::Pathfinder, {230.0f, -400.0f, -3211.0f}, 200, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {1734.0f, 0.0f, 140.514f}, 1000, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {1040.0f, 0.0f, 140.514f}, 1000, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {230.0f, 0.0f, 188.514f}, 1000, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {-426.0f, 0.0f, 130.514f}, 1000, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {-1206.0f, 0.0f, 133.514f}, 1000, PointRule::Always},
    {SCENE_CASTLE_COURTYARD_GUARDS_DAY, 0, Cue::Pathfinder, {-1571.0f, 0.0f, -834.514f}, 1000, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 0, Cue::Pathfinder, {-50.0f, -530.0f, -2300.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 0, Cue::Pathfinder, {25.0f, -530.0f, -2900.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 0, Cue::Pathfinder, {300.0f, -530.0f, -3020.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {370.0f, -500.0f, -3430.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {410.0f, -530.0f, -3770.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {675.0f, -570.0f, -3930.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {675.0f, -610.0f, -4300.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {560.0f, -600.0f, -4500.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {470.0f, -570.0f, -4775.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {300.0f, -570.0f, -4910.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 1, Cue::Pathfinder, {230.0f, -570.0f, -5300.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {300.0f, -570.0f, -5400.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {500.0f, -570.0f, -5400.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {650.0f, -570.0f, -5275.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {1200.0f, -730.0f, -5125.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {1345.0f, -730.0f, -4930.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {1560.0f, -730.0f, -4765.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {1730.0f, -730.0f, -4550.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 2, Cue::Pathfinder, {1940.0f, -730.0f, -4430.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1990.0f, -730.0f, -4185.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1800.0f, -730.0f, -3950.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1720.0f, -730.0f, -3850.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1690.0f, -730.0f, -3145.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1655.0f, -668.0f, -3035.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {1710.0f, -668.0f, -2660.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {2285.0f, -610.0f, -2650.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {2625.0f, -610.0f, -2700.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {3080.0f, -530.0f, -2700.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {3230.0f, -470.0f, -2515.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {3170.0f, -420.0f, -2300.0f}, 500, PointRule::Always},
    {SCENE_WINDMILL_AND_DAMPES_GRAVE, 3, Cue::Pathfinder, {2960.0f, -410.0f, -2000.0f}, 500, PointRule::Always},
    {SCENE_GRAVEYARD, 0, Cue::Pathfinder, {2135.0f, 124.0f, 85.0f}, 300, PointRule::Always},
    {SCENE_FOREST_TEMPLE, 17, Cue::Pathfinder, {119.0f, -779.0f, -1566.0f}, 333, PointRule::ForestBasementLeft},
    {SCENE_FOREST_TEMPLE, 17, Cue::Pathfinder, {119.0f, -779.0f, -1566.0f}, 333, PointRule::ForestBasementRight},
    {SCENE_FOREST_TEMPLE, 19, Cue::Pathfinder, {-1835.0f, 1033.0f, -3320.0f}, 1000, PointRule::ForestTwistedHallway},
    {SCENE_FOREST_TEMPLE, 19, Cue::Pathfinder, {-1760.0f, 1033.0f, -3200.0f}, 1000, PointRule::ForestTwistedHallway},
    {SCENE_FOREST_TEMPLE, 20, Cue::Pathfinder, {1877.0f, 1033.0f, -3320.0f}, 1000, PointRule::ForestTwistedHallway},
    {SCENE_FOREST_TEMPLE, 20, Cue::Pathfinder, {2000.0f, 1033.0f, -3200.0f}, 1000, PointRule::Always},
    {SCENE_FOREST_TEMPLE, 15, Cue::Pathfinder, {2070.0f, -403.0f, -3000.0f}, 1500, PointRule::Always},
    {SCENE_FOREST_TEMPLE, 15, Cue::Pathfinder, {2150.0f, -403.0f, -2560.0f}, 1500, PointRule::Always},
    {SCENE_FOREST_TEMPLE, 15, Cue::Pathfinder, {1990.0f, -403.0f, -1850.0f}, 1500, PointRule::Always},
    {SCENE_FIRE_TEMPLE, 10, Cue::Pathfinder, {-2350.0f, 2840.0f, 475.0f}, 1000, PointRule::Always},
    {SCENE_FIRE_TEMPLE, 16, Cue::Pathfinder, {475.0f, 2840.0f, -30.0f}, 1000, PointRule::Always},
    {SCENE_ICE_CAVERN, 9, Cue::Pathfinder, {860.0f, 200.0f, -2400.0f}, 750, PointRule::Always, 16},
};
}
