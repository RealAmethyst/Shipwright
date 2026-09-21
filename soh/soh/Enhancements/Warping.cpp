#include <libultraship/bridge.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "functions.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/util.h"

extern "C" {
#include "z64.h"
#include "global.h"
#include "soh/Enhancements/enhancementTypes.h"
void Sram_InitDebugSave(void);
void Select_LoadGame(SelectContext* selectContext, s32 entranceIndex);
}

#define CVAR_BOOTSEQUENCE_NAME CVAR_SETTING("BootSequence")
#define CVAR_BOOTSEQUENCE_DEFAULT BOOTSEQUENCE_DEFAULT
#define CVAR_BOOTSEQUENCE_VALUE CVarGetInteger(CVAR_BOOTSEQUENCE_NAME, CVAR_BOOTSEQUENCE_DEFAULT)

typedef struct WarpPoint {
    s32 entranceId;
    s8 roomNum;
    Vec3f pos;
    s16 rotY;
    bool bootToPoint;
} WarpPoint;
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3f, x, y, z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WarpPoint, entranceId, roomNum, pos, rotY, bootToPoint)
std::map<std::string, WarpPoint> warpPoints;

void LoadConfig() {
    auto allConfig = Ship::Context::GetInstance()->GetConfig()->GetNestedJson();
    if (allConfig.find("WarpPoints") == allConfig.end() || !allConfig["WarpPoints"].is_object()) {
        allConfig["WarpPoints"] = nlohmann::json::object();
    }
    warpPoints = allConfig["WarpPoints"];
}

void SaveConfig() {
    auto allConfig = Ship::Context::GetInstance()->GetConfig()->GetNestedJson();
    allConfig["WarpPoints"] = warpPoints;
    Ship::Context::GetInstance()->GetConfig()->SetBlock("WarpPoints", warpPoints);
    Ship::Context::GetInstance()->GetConfig()->Save();
}

void Warp(WarpPoint& warpPoint) {
    if (gPlayState == NULL) {
        // If gPlayState is NULL, it means the the user opted into BootToWarpPoint and the game is starting up.
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        gSaveContext.fileNum = 0xFE; // temporary file so that this will respect debug save file option
        Sram_InitDebugSave();
        gSaveContext.magicFillTarget = gSaveContext.magic;
        gSaveContext.magic = 0;
        gSaveContext.magicCapacity = 0;
        gSaveContext.magicLevel = gSaveContext.magic;
        gSaveContext.fileNum = 0xFF;
        gSaveContext.sceneSetupIndex = 0;
        gSaveContext.cutsceneIndex = 0;
        gSaveContext.linkAge = 0;
        gSaveContext.nightFlag = 0;
        gSaveContext.skyboxTime = gSaveContext.dayTime = 0x8000;

        // Copied from Select_LoadGame
        for (int buttonIndex = 0; buttonIndex < ARRAY_COUNT(gSaveContext.buttonStatus); buttonIndex++) {
            gSaveContext.buttonStatus[buttonIndex] = BTN_ENABLED;
        }
        gSaveContext.forceRisingButtonAlphas = gSaveContext.unk_13E8 = gSaveContext.unk_13EA = gSaveContext.unk_13EC =
            0;
        Audio_QueueSeqCmd(SEQ_PLAYER_BGM_MAIN << 24 | NA_BGM_STOP);
        gSaveContext.entranceIndex = warpPoint.entranceId;

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = 0xFF;
        gSaveContext.showTitleCard = true;
        gWeatherMode = 0;
        gGameState->running = false;
        SET_NEXT_GAMESTATE(gGameState, Play_Init, PlayState);
        GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);
    } else {
        gPlayState->nextEntranceIndex = warpPoint.entranceId;
        gPlayState->transitionTrigger = TRANS_TRIGGER_START;
        gPlayState->transitionType = TRANS_TYPE_INSTANT;
    }
    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = warpPoint.entranceId;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex = warpPoint.roomNum;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = warpPoint.pos;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = warpPoint.rotY;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0xDFF;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;
    gSaveContext.respawnFlag = 1;
    static HOOK_ID hookId = 0;
    hookId = REGISTER_VB_SHOULD(VB_INFLICT_VOID_DAMAGE, {
        *should = false;
        GameInteractor::Instance->UnregisterGameHookForID<GameInteractor::OnVanillaBehavior>(hookId);
    });
}

static std::string warpNameInput = "";

NativeOptions::PagePtr WarpPointsPage() {
    namespace N = NativeOptions;
    return N::MakePage("advanced/warp", N::Text("warp_points"), [] {
        std::vector<N::Row> rows;
        rows.push_back(N::String("name", N::Text("warp_name"), warpNameInput,
                                [](std::string value) { warpNameInput = std::move(value); }));
        auto add = N::Action("add", N::Text("warp_add"), [] {
            const auto save = [] {
                if (!gPlayState || !GET_PLAYER(gPlayState) || warpNameInput.empty()) return;
                auto* player = GET_PLAYER(gPlayState);
                warpPoints[warpNameInput] = WarpPoint{gSaveContext.entranceIndex, gPlayState->roomCtx.curRoom.num,
                                                     player->actor.world.pos, player->actor.shape.rot.y, false};
                SaveConfig();
                warpNameInput.clear();
            };
            if (warpPoints.contains(warpNameInput))
                N::Confirm(N::Text("warp_add"), warpNameInput, N::Text("replace"), save);
            else save();
        });
        add.enabled = gPlayState && GET_PLAYER(gPlayState) && !warpNameInput.empty();
        rows.push_back(std::move(add));
        for (const auto& [name, point] : warpPoints) {
            auto row = N::Link("point/" + name, name, [name] {
                return N::MakePage("warp/" + name, name, [name] {
                    std::vector<N::Row> controls;
                    const auto found = warpPoints.find(name);
                    if (found == warpPoints.end()) return controls;
                    auto warp = N::Action("warp", N::Text("warp"), [name] {
                        const auto current = warpPoints.find(name);
                        if (gPlayState && current != warpPoints.end()) {
                            Warp(current->second);
                            N::GetModel().Close();
                        }
                    });
                    warp.enabled = gPlayState != nullptr;
                    controls.push_back(std::move(warp));
                    controls.push_back(N::Toggle("boot", N::Text("warp_boot"), found->second.bootToPoint, [name](bool value) {
                        if (value) {
                            for (auto& entry : warpPoints) entry.second.bootToPoint = false;
                        }
                        if (auto current = warpPoints.find(name); current != warpPoints.end())
                            current->second.bootToPoint = value;
                        SaveConfig();
                    }));
                    controls.push_back(N::Action("delete", N::Text("delete"), [name] {
                        N::Confirm(N::Text("delete"), name, N::Text("delete"), [name] {
                            warpPoints.erase(name);
                            SaveConfig();
                            N::GetModel().Back();
                        });
                    }));
                    return controls;
                });
            });
            if (point.bootToPoint) row.value = N::Text("warp_boot_marker");
            rows.push_back(std::move(row));
        }
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
        return rows;
    });
}

void RegisterWarping() {
    static bool loadedConfig = false;
    if (!loadedConfig) {
        LoadConfig();
        loadedConfig = true;
    }

    COND_HOOK(OnZTitleUpdate, CVAR_BOOTSEQUENCE_VALUE == BOOTSEQUENCE_DEBUGWARPSCREEN, [](void* gameState) {
        TitleContext* titleContext = (TitleContext*)gameState;

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = 0xFF;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        titleContext->state.running = false;
        SET_NEXT_GAMESTATE(&titleContext->state, Select_Init, SelectContext);
    });

    COND_HOOK(OnZTitleUpdate, CVAR_BOOTSEQUENCE_VALUE == BOOTSEQUENCE_WARPPOINT, [](void* gameState) {
        for (auto& wp : warpPoints) {
            if (wp.second.bootToPoint) {
                Warp(wp.second);
                return;
            }
        }

        // Fallback to Debug Warp Screen if no warp point is set to boot to
        TitleContext* titleContext = (TitleContext*)gameState;

        gSaveContext.seqId = (u8)NA_BGM_DISABLED;
        gSaveContext.natureAmbienceId = 0xFF;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        titleContext->state.running = false;
        SET_NEXT_GAMESTATE(&titleContext->state, Select_Init, SelectContext);
    });
}

static RegisterShipInitFunc initFunc(RegisterWarping, { CVAR_BOOTSEQUENCE_NAME });
