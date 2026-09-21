#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/speechsynthesizer/SpeechSynthesizer.h"

#include "tts.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "SpeechText.h"
#include "MenuSpeech.h"
#include <algorithm>
#include <vector>
#include <set>
#include <ship/resource/File.h>
#include <ship/resource/type/Json.h>
#include <libultraship/classes.h>
#include <nlohmann/json.hpp>

#include "soh/ShipInit.hpp"
#include "soh/ResourceManagerHelpers.h"
#include "message_data_static.h"
extern "C" {
#include "overlays/gamestates/ovl_file_choose/file_choose.h"
}
#include "soh/Enhancements/boss-rush/BossRush.h"
#include "soh/Enhancements/FileSelectEnhancements.h"

extern "C" {
extern MapData* gMapData;
extern SaveContext gSaveContext;
extern PlayState* gPlayState;
}

typedef enum {
    /* 0x00 */ TEXT_BANK_SCENES,
    /* 0x01 */ TEXT_BANK_MISC,
    /* 0x02 */ TEXT_BANK_KALEIDO,
    /* 0x03 */ TEXT_BANK_FILECHOOSE,
} TextBank;

nlohmann::json sceneMap = nullptr;
nlohmann::json miscMap = nullptr;
nlohmann::json kaleidoMap = nullptr;
nlohmann::json fileChooseMap = nullptr;

// MARK: - Helpers

std::string GetParameritizedText(std::string key, TextBank bank, const char* arg) {
    const nlohmann::json* banks[] = { &sceneMap, &miscMap, &kaleidoMap, &fileChooseMap };
    if (bank < TEXT_BANK_SCENES || bank > TEXT_BANK_FILECHOOSE || !banks[bank]->is_object()) {
        return "";
    }
    const auto entry = banks[bank]->find(key);
    if (entry == banks[bank]->end() || !entry->is_string())
        return "";
    auto value = entry->get<std::string>();
    const auto index = value.find("$0");
    if (index != std::string::npos) {
        if (arg == nullptr) {
            return "";
        }
        value.replace(index, 2, arg);
    }
    return value;
}

static std::string WithPosition(const std::string& text, int position, int total) {
    const auto entry = miscMap.find("position");
    if (entry == miscMap.end() || !entry->is_string())
        return "";
    return SpeechText::WithPosition(text, entry->get<std::string>(), position, total);
}

const char* GetLanguageCode() {
    switch (CVarGetInteger(CVAR_SETTING("Languages"), 0)) {
        case LANGUAGE_FRA:
            return "fr-FR";
        case LANGUAGE_GER:
            return "de-DE";
    }

    return "en-US";
}

void TTSSpeakLocalized(const char* key, const char* argument, bool interrupt) {
    if (CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) && SpeechSynthesizer::Instance != nullptr) {
        const auto text = GetParameritizedText(key, TEXT_BANK_MISC, argument);
        SpeechSynthesizer::Instance->Speak(text.c_str(), GetLanguageCode(), interrupt);
    }
}

// MARK: - Boss Title Cards

std::string NameForSceneId(int16_t sceneId) {
    auto key = std::to_string(sceneId);
    auto name = GetParameritizedText(key, TEXT_BANK_SCENES, nullptr);
    return name;
}

static std::string titleCardText;

void RegisterOnSceneInitHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;

        titleCardText = NameForSceneId(sceneNum);
    });
}

void RegisterOnPresentTitleCardHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPresentTitleCard>([]() {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;

        SpeechSynthesizer::Instance->Speak(titleCardText.c_str(), GetLanguageCode());
    });
}

// MARK: - Interface Updates

void RegisterOnInterfaceUpdateHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnInterfaceUpdate>([]() {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;

        static uint32_t prevTimer = 0;
        static char ttsAnnounceBuf[32];

        uint32_t timer = 0;
        if (gSaveContext.timerState != TIMER_STATE_OFF) {
            timer = gSaveContext.timerSeconds;
        } else if (gSaveContext.subTimerState != SUBTIMER_STATE_OFF) {
            timer = gSaveContext.subTimerSeconds;
        }

        if (timer > 0 && timer % (timer < 60 ? 10 : 30) == 0 && timer != prevTimer) {
            uint32_t minutes = timer / 60;
            uint32_t seconds = timer % 60;
            char* announceBuf = ttsAnnounceBuf;
            char arg[8]; // at least big enough where no s8 string will overflow
            if (minutes > 0) {
                snprintf(arg, sizeof(arg), "%d", minutes);
                auto translation =
                    GetParameritizedText((minutes > 1) ? "minutes_plural" : "minutes_singular", TEXT_BANK_MISC, arg);
                announceBuf += snprintf(announceBuf, sizeof(ttsAnnounceBuf), "%s ", translation.c_str());
            }
            if (seconds > 0) {
                snprintf(arg, sizeof(arg), "%d", seconds);
                auto translation =
                    GetParameritizedText((seconds > 1) ? "seconds_plural" : "seconds_singular", TEXT_BANK_MISC, arg);
                announceBuf += snprintf(announceBuf, sizeof(ttsAnnounceBuf), "%s", translation.c_str());
            }
            assert(announceBuf < ttsAnnounceBuf + sizeof(ttsAnnounceBuf));
            SpeechSynthesizer::Instance->Speak(ttsAnnounceBuf, GetLanguageCode());
        }

        prevTimer = timer;

        if (!GameInteractor::IsSaveLoaded(true))
            return;

        static int16_t lostHealth = 0;
        static int16_t prevHealth = 0;

        if (gSaveContext.health - prevHealth < 0) {
            lostHealth += prevHealth - gSaveContext.health;
        }

        if (gPlayState->state.frames % 7 == 0) {
            if (lostHealth >= 16) {
                Audio_PlaySoundGeneral(NA_SE_SY_CANCEL, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                                       &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                lostHealth -= 16;
            }
        }

        prevHealth = gSaveContext.health;
    });
}

static std::string PauseItemWithPosition(const std::string& text, PauseContext* pause, bool dungeon) {
    std::vector<int> points;
    const int page = pause->pageIndex;
    const int cursorPage = page == PAUSE_MAP && !dungeon ? PAUSE_WORLD_MAP : page;
    const auto add = [&](int point, bool available) {
        if (available)
            points.push_back(point);
    };
    const int cursorSetting = CVarGetInteger(CVAR_ENHANCEMENT("PauseAnyCursor"), 0);
    const bool anyCursor =
        cursorSetting == PAUSE_ANY_CURSOR_ALWAYS_ON || (cursorSetting == PAUSE_ANY_CURSOR_RANDO_ONLY && IS_RANDO);
    switch (page) {
        case PAUSE_ITEM:
            for (int i = 0; i < 24; ++i) {
                add(i, anyCursor || gSaveContext.inventory.items[i] != ITEM_NONE);
            }
            break;
        case PAUSE_QUEST:
            // KaleidoScope_UpdateQuestStatusPoint accepts all 25 points, including empty ones.
            for (int i = 0; i < 25; ++i)
                add(i, true);
            break;
        case PAUSE_EQUIP:
            for (int i = 0; i < 16; ++i) {
                if (i % 4 == 0) {
                    add(i, i == 0 ? CUR_UPG_VALUE(UPG_BULLET_BAG) || CUR_UPG_VALUE(UPG_QUIVER)
                                  : CUR_UPG_VALUE(i / 4) != 0);
                } else {
                    add(i, anyCursor || (gSaveContext.inventory.equipment & gBitFlags[i - 1]));
                }
            }
            break;
        case PAUSE_MAP:
            if (dungeon) {
                for (int i = 0; i < 3; ++i)
                    add(i, CHECK_DUNGEON_ITEM(i, gSaveContext.mapIndex));
                for (int i = 0; i < 8; ++i) {
                    add(i + 3, (gSaveContext.sceneFlags[gSaveContext.mapIndex].floors & gBitFlags[i]) ||
                                   (CHECK_DUNGEON_ITEM(DUNGEON_MAP, gSaveContext.mapIndex) &&
                                    gMapData->floorID[gPlayState->interfaceCtx.unk_25A][i] != 0));
                }
            } else {
                for (int i = 0; i < 12; ++i)
                    add(i, pause->worldMapPoints[i] != 0);
            }
            break;
    }
    const auto selected = std::find(points.begin(), points.end(), pause->cursorPoint[cursorPage]);
    if (selected == points.end())
        return "";
    return WithPosition(text, static_cast<int>(std::distance(points.begin(), selected)) + 1,
                        static_cast<int>(points.size()));
}

static bool resumePauseSpeech = false;

void TTSResumePauseMenu() {
    resumePauseSpeech = true;
}

void RegisterOnKaleidoscopeUpdateHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnKaleidoscopeUpdate>([](int16_t inDungeonScene) {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;

        static int16_t prevCursorIndex = 0;
        static uint16_t prevCursorSpecialPos = 0;
        static uint16_t prevCursorPoint[5] = { 0 };
        static int16_t prevPromptChoice = -1;
        static int16_t prevSubState = -1;
        static int16_t prevState = -1;
        static int16_t spokenPage = -1;
        static bool firstPauseItemQueues = false;
        static bool pauseOpened = false;
        static bool pauseHintsPending = false;

        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        Input* input = &gPlayState->state.input[0];
        if (resumePauseSpeech) {
            spokenPage = -1;
            prevCursorIndex = -1;
            resumePauseSpeech = false;
        }
        if (pauseCtx->state == 0) {
            spokenPage = -1;
            prevCursorIndex = -1;
            prevPromptChoice = -1;
            firstPauseItemQueues = false;
            pauseOpened = false;
            pauseHintsPending = false;
        }
        if (pauseCtx->state == 7 && prevState != 7) {
            prevPromptChoice = -1;
            spokenPage = -1;
        }

        // Save game prompt
        if (pauseCtx->state == 7) {
            if (pauseCtx->unk_1EC == 1) {
                // prompt
                if (prevPromptChoice != pauseCtx->promptChoice) {
                    auto prompt =
                        GetParameritizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                    prompt = WithPosition(prompt, pauseCtx->promptChoice + 1, 2);
                    if (prevPromptChoice == -1) {
                        auto translation = GetParameritizedText("save_prompt", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                        SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode(), false);
                    } else {
                        SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode());
                    }

                    prevPromptChoice = pauseCtx->promptChoice;
                }
            } else if (pauseCtx->unk_1EC == 4 && prevSubState != 4) {
                // Saved
                auto translation = GetParameritizedText("game_saved", TEXT_BANK_KALEIDO, nullptr);
                SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
            }
            prevSubState = pauseCtx->unk_1EC;
            prevState = pauseCtx->state;
            return;
        }

        // Game over + prompts
        if (pauseCtx->state >= 0xC && pauseCtx->state <= 0x10) {
            // Reset prompt tracker after state change
            if (prevState != pauseCtx->state) {
                prevPromptChoice = -1;
            }

            switch (pauseCtx->state) {
                // Game over in full alpha
                case 0xC: {
                    // Fire once on state change
                    if (prevState != pauseCtx->state) {
                        auto translation = GetParameritizedText("game_over", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                    }
                    break;
                }
                // Prompt for save
                case 0xE: {
                    if (prevPromptChoice != pauseCtx->promptChoice) {
                        auto prompt =
                            GetParameritizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                        prompt = WithPosition(prompt, pauseCtx->promptChoice + 1, 2);
                        if (prevPromptChoice == -1) {
                            auto translation = GetParameritizedText("save_prompt", TEXT_BANK_KALEIDO, nullptr);
                            SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                            SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode(), false);
                        } else {
                            SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode());
                        }

                        prevPromptChoice = pauseCtx->promptChoice;
                    }
                    break;
                }
                // Game saved
                case 0xF: {
                    // Fire once on state change
                    if (prevState != pauseCtx->state) {
                        auto translation = GetParameritizedText("game_saved", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                    }
                    break;
                }
                // Prompt to continue playing
                case 0x10: {
                    if (prevPromptChoice != pauseCtx->promptChoice) {
                        auto prompt =
                            GetParameritizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                        prompt = WithPosition(prompt, pauseCtx->promptChoice + 1, 2);
                        if (prevPromptChoice == -1) {
                            auto translation = GetParameritizedText("continue_game", TEXT_BANK_KALEIDO, nullptr);
                            SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                            SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode(), false);
                        } else {
                            SpeechSynthesizer::Instance->Speak(prompt.c_str(), GetLanguageCode());
                        }

                        prevPromptChoice = pauseCtx->promptChoice;
                    }
                    break;
                }
            }

            prevState = pauseCtx->state;
            return;
        }

        // Announce the actual settled page, retaining the queue until its item is readable.
        if (pauseCtx->state == 6 && pauseCtx->optionsTab) {
            if (spokenPage != 4) NativeOptions::SpeakPauseTab();
            spokenPage = 4;
            prevCursorIndex = -1;
            prevState = pauseCtx->state;
            return;
        }
        if (pauseCtx->state == 6 && !pauseOpened) {
            pauseOpened = true;
            pauseHintsPending = true;
        }
        if (pauseCtx->state == 6 && pauseCtx->unk_1E4 == 0 && spokenPage != pauseCtx->pageIndex) {
            const char* keys[] = { "item_menu", "map_menu", "quest_menu", "equip_menu" };
            if (pauseCtx->pageIndex <= PAUSE_EQUIP) {
                const auto title = GetParameritizedText(keys[pauseCtx->pageIndex], TEXT_BANK_KALEIDO, nullptr);
                SpeechSynthesizer::Instance->Speak(title.c_str(), GetLanguageCode());
                firstPauseItemQueues = !title.empty();
            }
            spokenPage = pauseCtx->pageIndex;
            prevCursorIndex = -1;
        }

        prevState = pauseCtx->state;

        if (pauseCtx->state != 6) {
            // Reset cursor index and values so it is announced when pause is reopened
            prevCursorIndex = -1;
            prevPromptChoice = -1;
            prevSubState = -1;
            return;
        }

        const bool dpadNavigates =
            CVarGetInteger(CVAR_SETTING("DPadOnPause"), 0) && !CHECK_BTN_ALL(input->cur.button, BTN_CUP);
        if (!dpadNavigates && (pauseCtx->debugState != 1) && (pauseCtx->debugState != 2)) {
            char arg[8];
            if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
                // Normalize hearts to fractional count similar to z_lifemeter
                int curHeartFraction = gSaveContext.health % 16;
                int fullHearts = gSaveContext.health / 16;
                float fraction = ceilf((float)curHeartFraction / 5) * 0.25;
                float health = (float)fullHearts + fraction;
                snprintf(arg, sizeof(arg), "%g", health);
                auto translation = GetParameritizedText("health", TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT) && gSaveContext.magicCapacity != 0) {
                // Normalize magic to percentage
                float magicLevel = ((float)gSaveContext.magic / gSaveContext.magicCapacity) * 100;
                snprintf(arg, sizeof(arg), "%.0f%%", magicLevel);
                auto translation = GetParameritizedText("magic", TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN)) {
                if (gPlayState->sceneNum >= SCENE_FOREST_TEMPLE && gPlayState->sceneNum <= SCENE_INSIDE_GANONS_CASTLE) {
                    snprintf(arg, sizeof(arg), "%d",
                             std::max(gSaveContext.inventory.dungeonKeys[gPlayState->sceneNum], (s8)0));
                    auto translation = GetParameritizedText("keys", TEXT_BANK_KALEIDO, arg);
                    SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                } else {
                    snprintf(arg, sizeof(arg), "%d", gSaveContext.rupees);
                    auto translation = GetParameritizedText("rupees", TEXT_BANK_KALEIDO, arg);
                    SpeechSynthesizer::Instance->Speak(translation.c_str(), GetLanguageCode());
                }
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DRIGHT)) {
                // TODO: announce timer?
            }
        }

        if (pauseCtx->unk_1E4 != 0)
            return;

        uint16_t cursorIndex =
            (pauseCtx->pageIndex == PAUSE_MAP && !inDungeonScene) ? PAUSE_WORLD_MAP : pauseCtx->pageIndex;
        if (prevCursorIndex == cursorIndex && prevCursorSpecialPos == pauseCtx->cursorSpecialPos &&
            prevCursorPoint[cursorIndex] == pauseCtx->cursorPoint[cursorIndex]) {
            return;
        }

        prevCursorSpecialPos = pauseCtx->cursorSpecialPos;

        if (pauseCtx->cursorSpecialPos > 0) {
            return;
        }

        std::string buttonNames[] = {
            "input_button_c_left", "input_button_c_down", "input_button_c_right", "input_d_pad_up",
            "input_d_pad_down",    "input_d_pad_left",    "input_d_pad_right",
        };
        int8_t assignedTo = -1;

        const auto speakPauseItem = [&](const std::string& item) {
            const auto text = PauseItemWithPosition(item, pauseCtx, inDungeonScene != 0);
            if (!text.empty()) {
                SpeechSynthesizer::Instance->Speak(text.c_str(), GetLanguageCode(), !firstPauseItemQueues);
                firstPauseItemQueues = false;
                if (pauseHintsPending) {
                    const auto hints = GetParameritizedText("input_button_l", TEXT_BANK_MISC, nullptr) + ", " +
                                       GetParameritizedText("input_button_r", TEXT_BANK_MISC, nullptr);
                    SpeechSynthesizer::Instance->Speak(hints.c_str(), GetLanguageCode(), false);
                    pauseHintsPending = false;
                }
            }
        };

        switch (pauseCtx->pageIndex) {
            case PAUSE_ITEM: {
                char arg[8]; // at least big enough where no s8 string will overflow
                switch (pauseCtx->cursorItem[PAUSE_ITEM]) {
                    case ITEM_STICK:
                    case ITEM_NUT:
                    case ITEM_BOMB:
                    case ITEM_BOMBCHU:
                    case ITEM_SLINGSHOT:
                    case ITEM_BOW:
                    case ITEM_BEAN:
                        snprintf(arg, sizeof(arg), "%d", AMMO(pauseCtx->cursorItem[PAUSE_ITEM]));
                        break;
                    default:
                        arg[0] = '\0';
                }

                if (pauseCtx->cursorItem[PAUSE_ITEM] == PAUSE_ITEM_NONE ||
                    pauseCtx->cursorItem[PAUSE_ITEM] == ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_ITEM]);
                std::string itemTranslation = GetParameritizedText(key, TEXT_BANK_KALEIDO, arg);

                // Check if item is assigned to a button
                for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); i++) {
                    if (gSaveContext.equips.buttonItems[i + 1] == pauseCtx->cursorItem[PAUSE_ITEM]) {
                        assignedTo = i;
                        break;
                    }
                }

                if (assignedTo != -1) {
                    auto button = GetParameritizedText(buttonNames[assignedTo], TEXT_BANK_MISC, nullptr);
                    auto translation = GetParameritizedText("assigned_to", TEXT_BANK_KALEIDO, button.c_str());
                    speakPauseItem((itemTranslation + " - " + translation));
                } else {
                    speakPauseItem(itemTranslation);
                }
                break;
            }
            case PAUSE_MAP:
                if (inDungeonScene) {
                    // Dungeon map items
                    if (pauseCtx->cursorItem[PAUSE_MAP] != PAUSE_ITEM_NONE) {
                        std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_MAP]);
                        auto translation = GetParameritizedText(key, TEXT_BANK_KALEIDO, nullptr);
                        speakPauseItem(translation);
                    } else {
                        // Dungeon map floor numbers
                        char arg[8];
                        int cursorPoint = pauseCtx->cursorPoint[PAUSE_MAP];

                        // Cursor is on a dungeon floor position
                        if (cursorPoint >= 3 && cursorPoint < 11) {
                            int floorID =
                                gMapData->floorID[gPlayState->interfaceCtx.unk_25A][pauseCtx->dungeonMapSlot - 3];
                            // Normalize so F1 == 0, and negative numbers are basement levels
                            int normalizedFloor = (floorID * -1) + 8;
                            if (normalizedFloor >= 0) {
                                snprintf(arg, sizeof(arg), "%d", normalizedFloor + 1);
                                auto translation = GetParameritizedText("floor", TEXT_BANK_KALEIDO, arg);
                                speakPauseItem(translation);
                            } else {
                                snprintf(arg, sizeof(arg), "%d", normalizedFloor * -1);
                                auto translation = GetParameritizedText("basement", TEXT_BANK_KALEIDO, arg);
                                speakPauseItem(translation);
                            }
                        }
                    }
                } else {
                    std::string key = std::to_string(0x0100 + pauseCtx->cursorPoint[PAUSE_WORLD_MAP]);
                    auto translation = GetParameritizedText(key, TEXT_BANK_KALEIDO, nullptr);
                    speakPauseItem(translation);
                }
                break;
            case PAUSE_QUEST: {
                char arg[8]; // at least big enough where no s8 string will overflow
                switch (pauseCtx->cursorItem[PAUSE_QUEST]) {
                    case ITEM_SKULL_TOKEN:
                        snprintf(arg, sizeof(arg), "%d", gSaveContext.inventory.gsTokens);
                        break;
                    case ITEM_HEART_CONTAINER:
                        snprintf(arg, sizeof(arg), "%d", (gSaveContext.inventory.questItems & 0xF0000000) >> 0x1C);
                        break;
                    default:
                        arg[0] = '\0';
                }

                if (pauseCtx->cursorItem[PAUSE_QUEST] == PAUSE_ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_QUEST]);
                auto translation = GetParameritizedText(key, TEXT_BANK_KALEIDO, arg);
                speakPauseItem(translation);
                break;
            }
            case PAUSE_EQUIP: {
                if (pauseCtx->namedItem == PAUSE_ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_EQUIP]);
                auto itemTranslation = GetParameritizedText(key, TEXT_BANK_KALEIDO, nullptr);
                uint8_t checkEquipItem = pauseCtx->namedItem;

                // BGS from kaleido reports as ITEM_HEART_PIECE_2 (122)
                // remap BGS and broken knife to be the BGS item for the current equip check
                if (checkEquipItem == ITEM_HEART_PIECE_2 || checkEquipItem == ITEM_SWORD_KNIFE) {
                    checkEquipItem = ITEM_SWORD_BGS;
                }

                // Check if equipment item is currently equipped or assigned to a button
                if (checkEquipItem >= ITEM_SWORD_KOKIRI && checkEquipItem <= ITEM_BOOTS_HOVER) {
                    uint8_t checkEquipType = (checkEquipItem - ITEM_SWORD_KOKIRI) / 3;
                    uint8_t checkEquipValue = ((checkEquipItem - ITEM_SWORD_KOKIRI) % 3) + 1;

                    if (CUR_EQUIP_VALUE(checkEquipType) == checkEquipValue) {
                        itemTranslation = GetParameritizedText("equipped", TEXT_BANK_KALEIDO, itemTranslation.c_str());
                    }

                    for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); i++) {
                        if (gSaveContext.equips.buttonItems[i + 1] == checkEquipItem) {
                            assignedTo = i;
                            break;
                        }
                    }
                }

                if (assignedTo != -1) {
                    auto button = GetParameritizedText(buttonNames[assignedTo], TEXT_BANK_MISC, nullptr);
                    auto translation = GetParameritizedText("assigned_to", TEXT_BANK_KALEIDO, button.c_str());
                    speakPauseItem((itemTranslation + " - " + translation));
                } else {
                    speakPauseItem(itemTranslation);
                }
                break;
            }
            default:
                break;
        }

        prevCursorIndex = cursorIndex;
        memcpy(prevCursorPoint, pauseCtx->cursorPoint, sizeof(prevCursorPoint));
    });
}

static int fileSelectedSetting = FS_SETTING_AUDIO;
static int previousFileView = -1;
static std::set<int> openedFileViews;
static SpeechText::MenuSpeech fileSpeech;

void TTSResumeFileSelect() {
    fileSpeech.Resume();
}

static void SpeakFileText(const std::string& text, bool interrupt) {
    SpeechSynthesizer::Instance->Speak(text.c_str(), GetLanguageCode(), interrupt);
}

static std::string FileText(const char* key) {
    return GetParameritizedText(key, TEXT_BANK_FILECHOOSE, nullptr);
}

static std::string KeyboardText(FileChooseContext* file) {
    if (file->kbdY == 5) {
        return file->kbdX == FS_KBD_BTN_BACKSPACE ? FileText("backspace")
               : file->kbdX == FS_KBD_BTN_END     ? FileText("end")
                                                  : "";
    }
    if (file->charIndex < 0 || file->charIndex >= 65 || gSaveContext.language == LANGUAGE_JPN) {
        return "";
    }
    int code = D_808123F0[file->charIndex];
    if (ResourceMgr_GetGameRegion(0) != GAME_REGION_PAL) {
        // z_file_nameset_data.c defines the actual NTSC font indices.
        code = gKeyboardCharactersAlphanumeric[file->charIndex];
        if (code >= 0xAB && code <= 0xDE) {
            code = code - 0xAB + 10;
        } else if (code == 0xDF) {
            code = 62;
        } else if (code == 0xE4) {
            code = 63;
        } else if (code == 0xEA) {
            code = 64;
        }
    }
    if (code < 10) {
        return std::to_string(code);
    }
    if (code < 36) {
        const std::string letter(1, static_cast<char>('A' + code - 10));
        return GetParameritizedText("capital_letter", TEXT_BANK_FILECHOOSE, letter.c_str());
    }
    if (code < 62) {
        return std::string(1, static_cast<char>('a' + code - 36));
    }
    return code == 62 ? FileText("space") : code == 63 ? FileText("hyphen") : code == 64 ? FileText("period") : "";
}

static void SpeakFileSelect(FileChooseContext* file) {
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || gSaveContext.language == LANGUAGE_JPN) {
        return;
    }
    const int confirmView = 100;
    int view = file->configMode;
    if (file->menuMode == FS_MENU_MODE_SELECT) {
        if (file->selectMode != SM_CONFIRM_FILE) {
            return;
        }
        view = confirmView;
    } else if (file->menuMode != FS_MENU_MODE_CONFIG) {
        return;
    }

    std::string text, title;
    int position = 0, total = 0;
    bool controls = file->controlsAlpha > 0 && (view <= CM_NAME_ENTRY_TO_MAIN || view >= CM_UNUSED_DELAY);
    const char* mainKeys[] = { "file1", "file2", "file3", "copy", "erase", "options" };
    const char* fileKeys[] = { "file1", "file2", "file3", "quit" };
    switch (view) {
        case CM_MAIN_MENU:
            if (file->buttonIndex < 0 || file->buttonIndex > FS_BTN_MAIN_OPTIONS)
                return;
            title = FileText("title_select_file");
            text = FileText(mainKeys[file->buttonIndex]);
            position = file->buttonIndex + 1;
            total = FS_BTN_MAIN_OPTIONS + 1;
            break;
        case CM_SELECT_COPY_SOURCE:
        case CM_SELECT_COPY_DEST:
        case CM_ERASE_SELECT:
            if (file->buttonIndex < 0 || file->buttonIndex > FS_BTN_COPY_QUIT)
                return;
            title = FileText(view == CM_ERASE_SELECT         ? "title_erase_file"
                             : view == CM_SELECT_COPY_SOURCE ? "title_copy_from"
                                                             : "title_copy_to");
            text = FileText(fileKeys[file->buttonIndex]);
            position = file->buttonIndex + 1;
            total = FS_BTN_COPY_QUIT + 1;
            if (view == CM_SELECT_COPY_DEST) {
                if (file->buttonIndex == file->selectedFileIndex)
                    return;
                position -= file->buttonIndex > file->selectedFileIndex;
                --total;
            }
            break;
        case CM_COPY_CONFIRM:
        case CM_ERASE_CONFIRM:
        case confirmView: {
            const int choice = view == confirmView ? file->confirmButtonIndex : file->buttonIndex;
            if (choice < FS_BTN_CONFIRM_YES || choice > FS_BTN_CONFIRM_QUIT)
                return;
            title = FileText(view == confirmView ? "title_open_file" : "title_confirm");
            text = FileText(choice == FS_BTN_CONFIRM_YES ? "confirm" : "quit");
            position = choice + 1;
            total = FS_BTN_CONFIRM_QUIT + 1;
            break;
        }
        case CM_OPTIONS_MENU: {
            const int version = ResourceMgr_GameHasMasterQuest() && ResourceMgr_GameHasOriginal();
            const bool languageRow = ResourceMgr_GetGameRegion(version) == GAME_REGION_PAL &&
                                     ResourceMgr_GetGamePlatform(version) == GAME_PLATFORM_N64;
            title = FileText("options");
            if (fileSelectedSetting == FS_SETTING_AUDIO) {
                const char* keys[] = { "audio_stereo", "audio_mono", "audio_headset", "audio_surround" };
                if (gSaveContext.audioSetting > FS_AUDIO_SURROUND)
                    return;
                text = FileText(keys[gSaveContext.audioSetting]);
            } else if (fileSelectedSetting == FS_SETTING_TARGET) {
                text = FileText(gSaveContext.zTargetSetting == FS_TARGET_SWITCH ? "target_switch" : "target_hold");
            } else if (languageRow && gSaveContext.language <= LANGUAGE_FRA) {
                const char* keys[] = { "language_english", "language_german", "language_french" };
                text = FileText(keys[gSaveContext.language]);
            }
            position = fileSelectedSetting + 1;
            total = languageRow ? 3 : 2;
            controls = false;
            break;
        }
        case CM_QUEST_MENU: {
            title = FileText("title_quest");
            const int quest = file->questType[file->buttonIndex];
            const char* keys[] = { "quest_sel_vanilla", "quest_sel_mq", "quest_sel_randomizer", "quest_sel_boss_rush" };
            for (int i = ResourceMgr_GameHasOriginal() ? QUEST_NORMAL : QUEST_MASTER; i <= QUEST_BOSSRUSH; ++i) {
                if (i == QUEST_MASTER && !ResourceMgr_GameHasMasterQuest())
                    continue;
                ++total;
                if (i == quest)
                    position = total;
            }
            if (position == 0)
                return;
            text = FileText(keys[quest]);
            break;
        }
        case CM_BOSS_RUSH_MENU: {
            const int option = file->bossRushIndex;
            if (option < 0 || option >= BR_OPTIONS_MAX)
                return;
            title = FileText("title_boss_rush");
            text = BossRush_GetSettingName(option, gSaveContext.language);
            text += ", ";
            text += BossRush_GetSettingChoiceName(option, gSaveContext.ship.quest.data.bossRush.options[option],
                                                  gSaveContext.language);
            position = option + 1;
            total = BR_OPTIONS_MAX;
            break;
        }
        case CM_RANDOMIZER_SETTINGS_MENU:
            if (file->randomizerIndex < RSM_START_RANDOMIZER || file->randomizerIndex > RSM_OPEN_RANDOMIZER_SETTINGS)
                return;
            title = FileText("title_randomizer");
            text = SohFileSelect_GetSettingText(file->randomizerIndex, gSaveContext.language);
            position = file->randomizerIndex - RSM_START_RANDOMIZER + 1;
            total = RSM_OPEN_RANDOMIZER_SETTINGS - RSM_START_RANDOMIZER + 1;
            break;
        case CM_NAME_ENTRY:
            title = FileText("title_name");
            text = KeyboardText(file);
            position = file->kbdY == 5 ? 66 + file->kbdX - FS_KBD_BTN_BACKSPACE : file->charIndex + 1;
            total = 67;
            break;
        default:
            return;
    }

    if (view != previousFileView) {
        // Returning from a child retains its parent's opening announcement.
        if (view == CM_MAIN_MENU) {
            const bool mainWasOpen = openedFileViews.count(CM_MAIN_MENU) != 0;
            openedFileViews.clear();
            if (mainWasOpen)
                openedFileViews.insert(CM_MAIN_MENU);
        } else if (view == CM_QUEST_MENU) {
            openedFileViews.erase(CM_NAME_ENTRY);
            openedFileViews.erase(CM_BOSS_RUSH_MENU);
            openedFileViews.erase(CM_RANDOMIZER_SETTINGS_MENU);
        } else if (view == CM_SELECT_COPY_SOURCE) {
            openedFileViews.erase(CM_SELECT_COPY_DEST);
            openedFileViews.erase(CM_COPY_CONFIRM);
        } else if (view == CM_SELECT_COPY_DEST) {
            openedFileViews.erase(CM_COPY_CONFIRM);
        } else if (view == CM_ERASE_SELECT) {
            openedFileViews.erase(CM_ERASE_CONFIRM);
        }
        const bool opening = openedFileViews.insert(view).second;
        fileSpeech.Enter(title, opening, controls ? FileText("controls") : "", SpeakFileText);
        previousFileView = view;
    }
    fileSpeech.Item(WithPosition(text, position, total), SpeakFileText);
}

void RegisterOnUpdateMainMenuSelection() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnFileSelectUpdate>(
        [](void* file) { SpeakFileSelect(static_cast<FileChooseContext*>(file)); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnFileSelectClose>([]() {
        openedFileViews.clear();
        previousFileView = -1;
        fileSpeech = {};
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileAudioSelection>(
        [](uint8_t) { fileSelectedSetting = FS_SETTING_AUDIO; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileTargetSelection>(
        [](uint8_t) { fileSelectedSetting = FS_SETTING_TARGET; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileLanguageSelection>(
        [](uint8_t) { fileSelectedSetting = FS_SETTING_LANGUAGE; });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBootLogo>(
        [](uint8_t logo) { TTSSpeakLocalized(logo == 0 ? "logo_libultraship" : "logo_nintendo"); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnTitleScreen>([](bool masterQuest, const char* prompt) {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;
        if (prompt == nullptr) {
            TTSSpeakLocalized(masterQuest ? "title_ocarina_mq" : "title_ocarina");
        } else {
            SpeechSynthesizer::Instance->Speak(prompt, GetLanguageCode(), false);
        }
    });
}

static uint8_t ttsHasMessage;
static uint8_t ttsHasNewMessage;
static int8_t ttsCurrentHighlightedChoice = -1;
static std::vector<std::string> ttsDialogChoices;
static bool ttsChoiceQueues = false;

std::string remap(uint8_t character) {
    switch (character) {
        case 0x80:
            return "À";
        case 0x81:
            return "î";
        case 0x82:
            return "Â";
        case 0x83:
            return "Ä";
        case 0x84:
            return "Ç";
        case 0x85:
            return "È";
        case 0x86:
            return "É";
        case 0x87:
            return "Ê";
        case 0x88:
            return "Ë";
        case 0x89:
            return "Ï";
        case 0x8A:
            return "Ô";
        case 0x8B:
            return "Ö";
        case 0x8C:
            return "Ù";
        case 0x8D:
            return "Û";
        case 0x8E:
            return "Ü";
        case 0x8F:
            return "ß";
        case 0x90:
            return "à";
        case 0x91:
            return "á";
        case 0x92:
            return "â";
        case 0x93:
            return "ä";
        case 0x94:
            return "ç";
        case 0x95:
            return "è";
        case 0x96:
            return "é";
        case 0x97:
            return "ê";
        case 0x98:
            return "ë";
        case 0x99:
            return "ï";
        case 0x9A:
            return "ô";
        case 0x9B:
            return "ö";
        case 0x9C:
            return "ù";
        case 0x9D:
            return "û";
        case 0x9E:
            return "ü";
        case 0x9F:
            return GetParameritizedText("input_button_a", TEXT_BANK_MISC, nullptr);
        case 0xA0:
            return GetParameritizedText("input_button_b", TEXT_BANK_MISC, nullptr);
        case 0xA1:
            return GetParameritizedText("input_button_c", TEXT_BANK_MISC, nullptr);
        case 0xA2:
            return GetParameritizedText("input_button_l", TEXT_BANK_MISC, nullptr);
        case 0xA3:
            return GetParameritizedText("input_button_r", TEXT_BANK_MISC, nullptr);
        case 0xA4:
            return GetParameritizedText("input_button_z", TEXT_BANK_MISC, nullptr);
        case 0xA5:
            return GetParameritizedText("input_button_c_up", TEXT_BANK_MISC, nullptr);
        case 0xA6:
            return GetParameritizedText("input_button_c_down", TEXT_BANK_MISC, nullptr);
        case 0xA7:
            return GetParameritizedText("input_button_c_left", TEXT_BANK_MISC, nullptr);
        case 0xA8:
            return GetParameritizedText("input_button_c_right", TEXT_BANK_MISC, nullptr);
        case 0xAA:
            return GetParameritizedText("input_analog_stick", TEXT_BANK_MISC, nullptr);
        case 0xAB:
            return GetParameritizedText("input_d_pad", TEXT_BANK_MISC, nullptr);
        default:
            return "";
    }
}

std::string Message_TTS_Decode(uint8_t* sourceBuf, uint16_t startOfset, uint16_t size) {
    std::string output;
    uint32_t destWriteIndex = 0;
    uint8_t isListingChoices = 0;

    for (uint16_t i = 0; i < size; i++) {
        uint8_t cchar = sourceBuf[i + startOfset];

        if (cchar < ' ') {
            switch (cchar) {
                case MESSAGE_NEWLINE:
                    output += (isListingChoices) ? '\n' : ' ';
                    break;
                case MESSAGE_THREE_CHOICE:
                case MESSAGE_TWO_CHOICE:
                    output += '\n';
                    isListingChoices = 1;
                    break;
                case MESSAGE_COLOR:
                case MESSAGE_SHIFT:
                case MESSAGE_TEXT_SPEED:
                case MESSAGE_BOX_BREAK_DELAYED:
                case MESSAGE_FADE:
                case MESSAGE_ITEM_ICON:
                    i++;
                    break;
                case MESSAGE_FADE2:
                case MESSAGE_SFX:
                case MESSAGE_TEXTID:
                    i += 2;
                    break;
                default:
                    break;
            }
        } else {
            if (cchar < 0x80) {
                output += cchar;
            } else {
                output += remap(cchar);
            }
        }
    }

    return output;
}

void RegisterOnDialogMessageHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDialogMessage>([]() {
        if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1))
            return;

        if (gSaveContext.language == LANGUAGE_JPN)
            return;
        MessageContext* msgCtx = &gPlayState->msgCtx;

        if (msgCtx->msgMode == MSGMODE_TEXT_NEXT_MSG || msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN ||
            (msgCtx->msgMode == MSGMODE_TEXT_CONTINUING && msgCtx->stateTimer == 1)) {
            ttsHasNewMessage = 1;
        } else if (msgCtx->msgMode == MSGMODE_TEXT_DISPLAYING || msgCtx->msgMode == MSGMODE_TEXT_AWAIT_NEXT ||
                   msgCtx->msgMode == MSGMODE_TEXT_DONE || msgCtx->msgMode == MSGMODE_TEXT_DELAYED_BREAK ||
                   msgCtx->msgMode == MSGMODE_OCARINA_STARTING || msgCtx->msgMode == MSGMODE_OCARINA_PLAYING ||
                   msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED_TEXT ||
                   msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED_TEXT ||
                   msgCtx->msgMode == MSGMODE_SONG_PLAYED_ACT_BEGIN || msgCtx->msgMode == MSGMODE_SONG_PLAYED_ACT ||
                   msgCtx->msgMode == MSGMODE_SONG_PLAYBACK_STARTING || msgCtx->msgMode == MSGMODE_SONG_PLAYBACK ||
                   msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_STARTING ||
                   msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_SELECT_INSTRUMENT ||
                   msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION) {
            if (ttsHasNewMessage) {
                ttsHasMessage = 1;
                ttsHasNewMessage = 0;
                ttsCurrentHighlightedChoice = -1;
                ttsDialogChoices.clear();
                ttsChoiceQueues = false;

                const auto decoded = Message_TTS_Decode(msgCtx->msgBufDecoded, 0, msgCtx->decodedTextLen);
                auto dialog = SpeechText::SplitDialog(decoded, msgCtx->choiceNum);
                SpeechSynthesizer::Instance->Speak(dialog.body.c_str(), GetLanguageCode());
                ttsDialogChoices = std::move(dialog.choices);
                ttsChoiceQueues = !dialog.body.empty();
            }
            // The native renderer sets the initial choice (including Better Owl) at TEXT_DONE.
            if (msgCtx->msgMode == MSGMODE_TEXT_DONE && msgCtx->choiceIndex < ttsDialogChoices.size() &&
                msgCtx->choiceIndex != ttsCurrentHighlightedChoice) {
                const auto choice = WithPosition(ttsDialogChoices[msgCtx->choiceIndex], msgCtx->choiceIndex + 1,
                                                 static_cast<int>(ttsDialogChoices.size()));
                SpeechSynthesizer::Instance->Speak(choice.c_str(), GetLanguageCode(), !ttsChoiceQueues);
                ttsChoiceQueues = false;
                ttsCurrentHighlightedChoice = msgCtx->choiceIndex;
            }
        } else if (ttsHasMessage) {
            ttsHasMessage = 0;
            ttsHasNewMessage = 0;
            ttsDialogChoices.clear();
            ttsChoiceQueues = false;

            if (msgCtx->decodedTextLen < 3 || (msgCtx->msgBufDecoded[msgCtx->decodedTextLen - 2] != MESSAGE_FADE &&
                                               msgCtx->msgBufDecoded[msgCtx->decodedTextLen - 3] != MESSAGE_FADE2)) {
                SpeechSynthesizer::Instance->Stop(); // except for faded out messages
            }
        }
    });
}

// MARK: - Main Registration

void InitTTSBank() {
    std::string languageSuffix = "_eng.json";
    switch (CVarGetInteger(CVAR_SETTING("Languages"), 0)) {
        case LANGUAGE_FRA:
            languageSuffix = "_fra.json";
            break;
        case LANGUAGE_GER:
            languageSuffix = "_ger.json";
            break;
    }

    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(Ship::ResourceType::Json);
    initData->ResourceVersion = 0;

    auto loadBank = [&](const char* name) {
        const auto resource =
            std::dynamic_pointer_cast<Ship::Json>(Ship::Context::GetInstance()->GetResourceManager()->LoadResource(
                std::string("accessibility/texts/") + name + languageSuffix, true, initData));
        if (resource == nullptr || !resource->Data.is_object()) {
            SPDLOG_ERROR("[Speech] Missing or invalid text bank: {}{}", name, languageSuffix);
            return nlohmann::json::object();
        }
        return resource->Data;
    };
    sceneMap = loadBank("scenes");
    miscMap = loadBank("misc");
    kaleidoMap = loadBank("kaleidoscope");
    fileChooseMap = loadBank("filechoose");
}

void RegisterOnSetGameLanguageHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSetGameLanguage>([]() { InitTTSBank(); });
}

void RegisterOnSetDoAction() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSetDoAction>([](uint16_t action) {
        if (CVarGetInteger(CVAR_SETTING("A11yTTS"), 1)) {
            const char* key;
            switch (action) {
                case DO_ACTION_CHECK:
                    key = "action_check";
                    break;
                case DO_ACTION_ENTER:
                    key = "action_enter";
                    break;
                case DO_ACTION_OPEN:
                    key = "action_open";
                    break;
                case DO_ACTION_CLIMB:
                    key = "action_climb";
                    break;
                case DO_ACTION_SPEAK:
                    key = "action_speak";
                    break;
                case DO_ACTION_GRAB:
                    key = "action_grab";
                    break;
                case DO_ACTION_DOWN: {
                    Player* player = GET_PLAYER(gPlayState);
                    if (player == NULL || !(player->stateFlags1 & PLAYER_STATE1_ON_HORSE))
                        return;
                    key = "action_down";
                } break;
                default:
                    return;
            }
            TTSSpeakLocalized(key);
        }
    });
}

static void RegisterTTSModHooks() {
    RegisterOnSetGameLanguageHook();
    RegisterOnDialogMessageHook();
    RegisterOnSceneInitHook();
    RegisterOnPresentTitleCardHook();
    RegisterOnInterfaceUpdateHook();
    RegisterOnKaleidoscopeUpdateHook();
    RegisterOnUpdateMainMenuSelection();
    RegisterOnSetDoAction();
}

static void RegisterTTS() {
    InitTTSBank();
    RegisterTTSModHooks();
}

static RegisterShipInitFunc initFunc(RegisterTTS);

static void UpdateSpeechEnabled() {
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) && SpeechSynthesizer::Instance != nullptr) {
        SpeechSynthesizer::Instance->Stop();
    }
}

static RegisterShipInitFunc speechEnabledInit(UpdateSpeechEnabled, { CVAR_SETTING("A11yTTS") });
