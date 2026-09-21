#pragma once
#include "CueMixer.h"
#include <cstddef>

struct PlayState;
namespace SpatialAudio {
void InitCues();
void UpdateCues(PlayState* play);
void ShutdownCues();
void SetSceneExits(const int16_t* exits, size_t count);
CueMixer& GetCueMixer();
float CueMasterGain();
}
