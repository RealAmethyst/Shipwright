#pragma once
#include "CueMixer.h"
#include <cstddef>

struct PlayState;
struct Actor;
namespace SpatialAudio {
struct CueTarget {
    uint64_t identity;
    Actor* actor;
    Cue cue;
    float x, y, z;
    int destination = -1;
};
std::vector<CueTarget> NavigationTargets(PlayState* play);
void InitCues();
void UpdateCues(PlayState* play);
void ShutdownCues();
void SetSceneExits(const int16_t* exits, size_t count);
CueMixer& GetCueMixer();
float CueMasterGain();
}
