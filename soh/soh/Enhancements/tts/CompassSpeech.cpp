#include "CompassSpeech.h"
#include "tts.h"
#include "soh/Enhancements/audio/spatial/WorldCompass.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include <libultraship/libultraship.h>

extern "C" {
#include "global.h"
}

void UpdateCompassSpeech(PlayState* play) {
    static SpatialAudio::CompassHeading heading;
    static int scene = -1, directions = 4;
    static bool mirrored = false;
    static uint32_t frame = UINT32_MAX;
    if (!play || !GameInteractor::IsSaveLoaded() || !SpatialAudio::HasMapCompass(play->sceneNum) ||
        !CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || !CVarGetInteger(CVAR_SETTING("A11yTTSCompass"), 1) ||
        NativeOptions_IsOpen() || play->pauseCtx.state || !GET_PLAYER(play) || Play_InCsMode(play) ||
        !std::isfinite(play->view.eye.y) || !std::isfinite(play->view.lookAt.y) ||
        (GET_PLAYER(play)->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_TALKING |
             PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_DEAD)) || Message_GetState(&play->msgCtx) != TEXT_STATE_NONE) {
        heading.Reset();
        scene = -1;
        frame = UINT32_MAX;
        return;
    }
    const bool reflect = CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) != 0;
    const int sectors = CVarGetInteger(CVAR_SETTING("A11yTTSCompassDirections"), 4) == 8 ? 8 : 4;
    if (scene != play->sceneNum || reflect != mirrored || sectors != directions) {
        heading.Reset();
        frame = UINT32_MAX;
    }
    scene = play->sceneNum;
    mirrored = reflect;
    directions = sectors;
    if (frame == play->gameplayFrames) return;
    frame = play->gameplayFrames;
    const int changed = heading.Update(play->view.lookAt.x - play->view.eye.x,
                                       play->view.lookAt.z - play->view.eye.z, mirrored, directions);
    if (changed < 0) return;
    constexpr const char* keys[]{"compass_north", "compass_northeast", "compass_east", "compass_southeast",
                                 "compass_south", "compass_southwest", "compass_west", "compass_northwest"};
    TTSSpeakLocalized(keys[changed * (8 / directions)], nullptr, false);
}
