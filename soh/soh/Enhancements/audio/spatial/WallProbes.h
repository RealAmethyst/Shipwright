#pragma once
#include <array>
extern "C" {
#include "z64math.h"
}
struct PlayState;
namespace SpatialAudio {
struct WallHit { bool found = false; Vec3f position{}; };
std::array<WallHit, 3> ScanWalls(PlayState* play);
}
