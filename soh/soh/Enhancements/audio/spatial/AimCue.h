#pragma once

struct PlayState;
namespace SpatialAudio {
class CueMixer;
void InitAimCue(CueMixer& mixer);
void UpdateAimCue(PlayState* play, CueMixer& mixer);
void SuspendAimCue(CueMixer& mixer);
void ResetAimCue();
}
