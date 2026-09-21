#include "AudioEditor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "sequence.h"

#include <map>
#include <set>
#include <string>
#include <libultraship/libultraship.h>
#include <functions.h>
#include "../randomizer/3drando/random.hpp"
#include "soh/OTRGlobals.h"
#include "soh/cvar_prefixes.h"
#include <ship/utils/StringHelper.h>
#include "soh/SohGui/SohMenu.h"
#include "soh/SohGui/SohGui.hpp"
#include "AudioCollection.h"
#include "soh/Enhancements/enhancementTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64save.h"
extern SaveContext gSaveContext;
}

Vec3f pos = { 0.0f, 0.0f, 0.0f };
f32 freqScale = 1.0f;
s8 reverbAdd = 0;

using namespace UIWidgets;

static WidgetInfo lowHpAlarm;
static WidgetInfo naviCall;
static WidgetInfo enemyProx;
static WidgetInfo leeverProx;
static WidgetInfo leadingMusic;
static WidgetInfo displaySeqName;
static WidgetInfo ovlDuration;
static WidgetInfo voicePitch;
static WidgetInfo randomAudioGenModes;
static WidgetInfo lowerOctaves;

namespace SohGui {
extern std::shared_ptr<SohMenu> mSohMenu;
}

// Authentic sequence counts
// used to ensure we have enough to shuffle
#define SEQ_COUNT_BGM_WORLD 30
#define SEQ_COUNT_BGM_BATTLE 6
#define SEQ_COUNT_FANFARE 15
#define SEQ_COUNT_OCARINA 12
#define SEQ_COUNT_NOSHUFFLE 6
#define SEQ_COUNT_BGM_EVENT 17
#define SEQ_COUNT_INSTRUMENT 6
#define SEQ_COUNT_SFX 57
#define SEQ_COUNT_VOICE 108

size_t AuthenticCountBySequenceType(SeqType type) {
    switch (type) {
        case SEQ_NOSHUFFLE:
            return SEQ_COUNT_NOSHUFFLE;
        case SEQ_BGM_WORLD:
            return SEQ_COUNT_BGM_WORLD;
        case SEQ_BGM_EVENT:
            return SEQ_COUNT_BGM_EVENT;
        case SEQ_BGM_BATTLE:
            return SEQ_COUNT_BGM_BATTLE;
        case SEQ_OCARINA:
            return SEQ_COUNT_OCARINA;
        case SEQ_FANFARE:
            return SEQ_COUNT_FANFARE;
        case SEQ_SFX:
            return SEQ_COUNT_SFX;
        case SEQ_INSTRUMENT:
            return SEQ_COUNT_INSTRUMENT;
        case SEQ_VOICE:
            return SEQ_COUNT_VOICE;
        default:
            return 0;
    }
}

static const std::map<int32_t, const char*> audioRandomizerModes = {
    { RANDOMIZE_OFF, "Manual" },
    { RANDOMIZE_ON_NEW_SCENE, "On New Scene" },
    { RANDOMIZE_ON_RANDO_GEN_ONLY, "On Rando Gen Only" },
    { RANDOMIZE_ON_FILE_LOAD, "On File Load" },
    { RANDOMIZE_ON_FILE_LOAD_SEEDED, "On File Load (Seeded)" },
};

// Grabs the current BGM sequence ID and replays it
// which will lookup the proper override, or reset back to vanilla
void ReplayCurrentBGM() {
    u16 curSeqId = func_800FA0B4(SEQ_PLAYER_BGM_MAIN);
    // TODO: replace with Audio_StartSeq when the macro is shared
    // The fade time and audio player flags will always be 0 in the case of replaying the BGM, so they are not set here
    Audio_QueueSeqCmd(0x00000000 | curSeqId);
}

// Attempt to update the BGM if it matches the current sequence that is being played
// The seqKey that is passed in should be the vanilla ID, not the override ID
void UpdateCurrentBGM(u16 seqKey, SeqType seqType) {
    if (seqType != SEQ_BGM_WORLD) {
        return;
    }

    u16 curSeqId = func_800FA0B4(SEQ_PLAYER_BGM_MAIN);
    if (curSeqId == seqKey) {
        ReplayCurrentBGM();
    }
}

void RandomizeGroup(SeqType type, bool manual = true) {
    std::vector<u16> values;

    uint64_t localRngState = 0;
    uint64_t* shuffleState = nullptr;

    if (!manual) {
        int randomizeMode = CVarGetInteger(CVAR_AUDIO("RandomizeAudioGenModes"), 0);
        if (randomizeMode == RANDOMIZE_ON_FILE_LOAD_SEEDED || randomizeMode == RANDOMIZE_ON_RANDO_GEN_ONLY) {

            uint32_t finalSeed = type + (IS_RANDO ? Rando::Context::GetInstance()->GetSeed()
                                                  : static_cast<uint32_t>(gSaveContext.ship.stats.fileCreatedAt));
            ShipUtils::RandInit(finalSeed, &localRngState);
            shuffleState = &localRngState;
        }
        // For RANDOMIZE_ON_NEW_SCENE, shuffleState remains nullptr, which uses the global RNG
    }

    // An empty IncludedSequences set means that the AudioEditor window has never been drawn
    if (AudioCollection::Instance->GetIncludedSequences().empty()) {
        AudioCollection::Instance->InitializeShufflePool();
    }

    // use a while loop to add duplicates if we don't have enough included sequences
    while (values.size() < AuthenticCountBySequenceType(type)) {
        for (const auto& seqData : AudioCollection::Instance->GetIncludedSequences()) {
            if (seqData->category & type && seqData->canBeUsedAsReplacement) {
                values.push_back(seqData->sequenceId);
            }
        }

        // if we didn't find any, return early without shuffling to prevent an infinite loop
        if (!values.size())
            return;
    }
    ShipUtils::Shuffle(values, shuffleState);
    for (const auto& [seqId, seqData] : AudioCollection::Instance->GetAllSequences()) {
        const std::string cvarKey = AudioCollection::Instance->GetCvarKey(seqData.sfxKey);
        const std::string cvarLockKey = AudioCollection::Instance->GetCvarLockKey(seqData.sfxKey);
        // don't randomize locked entries
        if ((seqData.category & type) && CVarGetInteger(cvarLockKey.c_str(), 0) == 0) {
            // Only save authentic sequence CVars
            if ((((seqData.category & SEQ_BGM_CUSTOM) || seqData.category == SEQ_FANFARE) &&
                 seqData.sequenceId >= MAX_AUTHENTIC_SEQID) ||
                seqData.canBeReplaced == false) {
                continue;
            }
            const int randomValue = values.back();
            CVarSetInteger(cvarKey.c_str(), randomValue);
            values.pop_back();
        }
    }
}

void ResetGroup(const std::map<u16, SequenceInfo>& map, SeqType type) {
    for (const auto& [defaultValue, seqData] : map) {
        if (seqData.category == type) {
            // Only save authentic sequence CVars
            if (seqData.category == SEQ_FANFARE && defaultValue >= MAX_AUTHENTIC_SEQID) {
                continue;
            }
            const std::string cvarKey = AudioCollection::Instance->GetCvarKey(seqData.sfxKey);
            const std::string cvarLockKey = AudioCollection::Instance->GetCvarLockKey(seqData.sfxKey);
            if (CVarGetInteger(cvarLockKey.c_str(), 0) == 0) {
                CVarClear(cvarKey.c_str());
            }
        }
    }
}

void LockGroup(const std::map<u16, SequenceInfo>& map, SeqType type) {
    for (const auto& [defaultValue, seqData] : map) {
        if (seqData.category == type) {
            // Only save authentic sequence CVars
            if (seqData.category == SEQ_FANFARE && defaultValue >= MAX_AUTHENTIC_SEQID) {
                continue;
            }
            const std::string cvarKey = AudioCollection::Instance->GetCvarKey(seqData.sfxKey);
            const std::string cvarLockKey = AudioCollection::Instance->GetCvarLockKey(seqData.sfxKey);
            CVarSetInteger(cvarLockKey.c_str(), 1);
        }
    }
}

void UnlockGroup(const std::map<u16, SequenceInfo>& map, SeqType type) {
    for (const auto& [defaultValue, seqData] : map) {
        if (seqData.category == type) {
            // Only save authentic sequence CVars
            if (seqData.category == SEQ_FANFARE && defaultValue >= MAX_AUTHENTIC_SEQID) {
                continue;
            }
            const std::string cvarKey = AudioCollection::Instance->GetCvarKey(seqData.sfxKey);
            const std::string cvarLockKey = AudioCollection::Instance->GetCvarLockKey(seqData.sfxKey);
            CVarSetInteger(cvarLockKey.c_str(), 0);
        }
    }
}

extern "C" u16 AudioEditor_GetReplacementSeq(u16 seqId) {
    return AudioCollection::Instance->GetReplacementSequence(seqId);
}

std::string GetSequenceTypeName(SeqType type) {
    switch (type) {
        case SEQ_NOSHUFFLE:
            return "No Shuffle";
        case SEQ_BGM_WORLD:
            return "World";
        case SEQ_BGM_EVENT:
            return "Event";
        case SEQ_BGM_BATTLE:
            return "Battle";
        case SEQ_OCARINA:
            return "Ocarina";
        case SEQ_FANFARE:
            return "Fanfare";
        case SEQ_BGM_ERROR:
            return "Error";
        case SEQ_SFX:
            return "SFX";
        case SEQ_VOICE:
            return "Voice";
        case SEQ_INSTRUMENT:
            return "Instrument";
        case SEQ_BGM_CUSTOM:
            return "Custom";
        default:
            return "No Sequence Type";
    }
}

void AudioEditorRegisterOnSceneInitHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (gSaveContext.gameMode != GAMEMODE_END_CREDITS &&
            CVarGetInteger(CVAR_AUDIO("RandomizeAudioGenModes"), 0) == RANDOMIZE_ON_NEW_SCENE) {

            AudioEditor_AutoRandomizeAll();
        }
    });
}

void AudioEditorRegisterOnGenerationCompletionHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGenerationCompletion>([]() {
        if (CVarGetInteger(CVAR_AUDIO("RandomizeAudioGenModes"), 0) == RANDOMIZE_ON_RANDO_GEN_ONLY) {

            AudioEditor_AutoRandomizeAll();
        }
    });
}

void AudioEditorRegisterOnLoadGameHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>([](int32_t fileNum) {
        if (CVarGetInteger(CVAR_AUDIO("RandomizeAudioGenModes"), 0) == RANDOMIZE_ON_FILE_LOAD ||
            CVarGetInteger(CVAR_AUDIO("RandomizeAudioGenModes"), 0) == RANDOMIZE_ON_FILE_LOAD_SEEDED) {

            AudioEditor_AutoRandomizeAll();
        }
    });
}

void InitializeAudioEditor() {
    AudioEditorRegisterOnSceneInitHook();
    AudioEditorRegisterOnGenerationCompletionHook();
    AudioEditorRegisterOnLoadGameHook();
}

std::vector<SeqType> allTypes = {
    SEQ_BGM_WORLD, SEQ_BGM_EVENT, SEQ_BGM_BATTLE, SEQ_OCARINA, SEQ_FANFARE, SEQ_INSTRUMENT, SEQ_SFX, SEQ_VOICE,
};

void AudioEditor_RandomizeAll() {
    for (auto type : allTypes) {
        RandomizeGroup(type);
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ReplayCurrentBGM();
}

void AudioEditor_AutoRandomizeAll() {
    for (auto type : allTypes) {
        RandomizeGroup(type, false);
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ReplayCurrentBGM();
}

void AudioEditor_RandomizeGroup(SeqType group) {
    RandomizeGroup(group);

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ReplayCurrentBGM();
}

void AudioEditor_ResetAll() {
    for (auto type : allTypes) {
        ResetGroup(AudioCollection::Instance->GetAllSequences(), type);
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ReplayCurrentBGM();
}

void AudioEditor_ResetGroup(SeqType group) {
    ResetGroup(AudioCollection::Instance->GetAllSequences(), group);

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    ReplayCurrentBGM();
}

void AudioEditor_LockAll() {
    for (auto type : allTypes) {
        LockGroup(AudioCollection::Instance->GetAllSequences(), type);
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

void AudioEditor_UnlockAll() {
    for (auto type : allTypes) {
        UnlockGroup(AudioCollection::Instance->GetAllSequences(), type);
    }

    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}


namespace {
using namespace NativeOptions;

void StopAudioPreview() {
    if (CVarGetInteger(CVAR_AUDIO("Playing"), 0)) {
        func_800F5C2C();
        CVarSetInteger(CVAR_AUDIO("Playing"), 0);
    }
}

void PlayAudioPreview(uint16_t id, SeqType type) {
    if (type == SEQ_SFX || type == SEQ_VOICE) {
        Audio_PlaySoundGeneral(id, &pos, 4, &freqScale, &freqScale, &reverbAdd);
    } else if (type == SEQ_INSTRUMENT) {
        Audio_OcaSetInstrument(id - INSTRUMENT_OFFSET);
        Audio_OcaSetSongPlayback(9, 1);
    } else {
        if (CVarGetInteger(CVAR_AUDIO("Playing"), 0))
            Audio_QueuePreviewSeqCmd(id);
        else
            PreviewSequence(id);
        CVarSetInteger(CVAR_AUDIO("Playing"), id);
    }
}

Row PreviewRow(uint16_t id, SeqType type) {
    const bool playing = CVarGetInteger(CVAR_AUDIO("Playing"), 0) == id;
    return Action("preview", NativeOptions::Text(playing ? "stop_preview" : "play_preview"), [=] {
        if (playing) StopAudioPreview(); else PlayAudioPreview(id, type);
    });
}

PagePtr SequencePage(uint16_t id, SeqType type) {
    const auto sequence = AudioCollection::Instance->GetAllSequences().at(id);
    const auto key = AudioCollection::Instance->GetCvarKey(sequence.sfxKey);
    const auto lock = AudioCollection::Instance->GetCvarLockKey(sequence.sfxKey);
    return MakePage("audio/sequence/" + std::to_string(id), sequence.label, [=] {
        std::map<int, std::string> choices;
        const auto sequences = AudioCollection::Instance->GetAllSequences();
        for (const auto& [value, candidate] : sequences)
            if ((candidate.category & type) && (candidate.canBeUsedAsReplacement || candidate.sfxKey == sequence.sfxKey))
                choices.emplace(value, candidate.label);
        int current = CVarGetInteger(key.c_str(), id);
        if (!sequences.contains(current)) current = id;
        std::vector<Row> rows = {
            Choice("replacement", NativeOptions::Text("replacement"), current, choices, [=](int value) {
                CVarSetInteger(key.c_str(), value);
                SaveSettings();
                UpdateCurrentBGM(id, type);
            }),
            PreviewRow((type == SEQ_SFX || type == SEQ_VOICE || type == SEQ_INSTRUMENT) ? id : current, type),
            Action("reset", NativeOptions::Text("reset_default"), [=] {
                CVarClear(key.c_str());
                CVarClear(lock.c_str());
                SaveSettings();
                UpdateCurrentBGM(id, type);
            }),
            Action("random", NativeOptions::Text("random_sound"), [=] {
                std::vector<SequenceInfo*> candidates;
                for (auto* candidate : AudioCollection::Instance->GetIncludedSequences())
                    if ((candidate->category & type) && candidate->canBeUsedAsReplacement)
                        candidates.push_back(candidate);
                if (!candidates.empty()) {
                    CVarSetInteger(key.c_str(), candidates[ShipUtils::next32() % candidates.size()]->sequenceId);
                    CVarClear(lock.c_str());
                    SaveSettings();
                    UpdateCurrentBGM(id, type);
                }
            }),
            CVarToggle(NativeOptions::Text("lock"), lock),
        };
        return rows;
    });
}

PagePtr SequenceGroup(SeqType type, const std::string& title) {
    return MakePage("audio/group/" + std::to_string(type), title, [=] {
        std::vector<Row> rows;
        for (const auto& [id, sequence] : AudioCollection::Instance->GetAllSequences()) {
            if (!(sequence.category & type) || !sequence.canBeReplaced ||
                (((sequence.category & SEQ_BGM_CUSTOM) || sequence.category == SEQ_FANFARE) && id >= MAX_AUTHENTIC_SEQID))
                continue;
            auto row = Link(std::to_string(id), sequence.label, [=] { return SequencePage(id, type); });
            const auto replacement = AudioCollection::Instance->GetReplacementSequence(id);
            if (const auto* name = AudioCollection::Instance->GetSequenceName(replacement))
                row.value = name;
            rows.push_back(std::move(row));
        }
        rows.push_back(Action("reset-all", NativeOptions::Text("reset_all"), [=] { AudioEditor_ResetGroup(type); }));
        rows.push_back(Action("random-all", NativeOptions::Text("random_all"), [=] { AudioEditor_RandomizeGroup(type); }));
        rows.push_back(Action("lock-all", NativeOptions::Text("lock_all"), [=] {
            LockGroup(AudioCollection::Instance->GetAllSequences(), type);
            SaveSettings();
        }));
        rows.push_back(Action("unlock-all", NativeOptions::Text("unlock_all"), [=] {
            UnlockGroup(AudioCollection::Instance->GetAllSequences(), type);
            SaveSettings();
        }));
        return rows;
    });
}

PagePtr ShufflePool() {
    struct Filter {
        ImGuiTextFilter text;
        std::map<SeqType, bool> types;
        bool Matches(const SequenceInfo* sequence) {
            return types[sequence->category] && text.PassFilter(sequence->label.c_str());
        }
    };
    auto filter = std::make_shared<Filter>();
    for (auto type : allTypes) filter->types[type] = true;
    filter->types[SEQ_BGM_CUSTOM] = true;
    return MakePage("audio/pool", NativeOptions::Text("shuffle_pool"), [filter] {
        std::vector<Row> rows = {
            NativeOptions::String("filter", NativeOptions::Text("filter_expression"), filter->text.InputBuf, [filter](std::string value) {
                std::snprintf(filter->text.InputBuf, sizeof(filter->text.InputBuf), "%s", value.c_str());
                filter->text.Build();
            }, "", sizeof(filter->text.InputBuf) - 1),
            Link("types", NativeOptions::Text("sequence_types"), [filter] {
                return MakePage("audio/pool/types", NativeOptions::Text("sequence_types"), [filter] {
                    std::vector<Row> rows;
                    for (auto [type, enabled] : filter->types)
                        rows.push_back(Toggle(std::to_string(type), GetSequenceTypeName(type), enabled,
                                              [filter, type](bool value) { filter->types[type] = value; }));
                    return rows;
                });
            }),
            Action("exclude-all", NativeOptions::Text("exclude_all"), [filter] {
                for (auto* sequence : AudioCollection::Instance->GetIncludedSequences())
                    if (filter->Matches(sequence)) AudioCollection::Instance->RemoveFromShufflePool(sequence);
            }),
            Action("include-all", NativeOptions::Text("include_all"), [filter] {
                for (auto* sequence : AudioCollection::Instance->GetExcludedSequences())
                    if (filter->Matches(sequence)) AudioCollection::Instance->AddToShufflePool(sequence);
            }),
        };
        for (bool included : {true, false}) {
            auto sequences = included ? AudioCollection::Instance->GetIncludedSequences()
                                      : AudioCollection::Instance->GetExcludedSequences();
            for (auto* sequence : sequences) {
                if (!filter->Matches(sequence)) continue;
                auto row = Link(std::to_string(sequence->sequenceId), sequence->label, [sequence] {
                    return MakePage("audio/pool/" + std::to_string(sequence->sequenceId), sequence->label, [sequence] {
                        const bool isIncluded = AudioCollection::Instance->GetIncludedSequences().contains(sequence);
                        return std::vector<Row>{
                            Toggle("included", NativeOptions::Text("included"), isIncluded, [sequence](bool value) {
                                if (value) AudioCollection::Instance->AddToShufflePool(sequence);
                                else AudioCollection::Instance->RemoveFromShufflePool(sequence);
                            }),
                            PreviewRow(sequence->sequenceId, sequence->category),
                        };
                    });
                });
                row.value = NativeOptions::Text(included ? "included" : "excluded");
                row.description = GetSequenceTypeName(sequence->category);
                rows.push_back(std::move(row));
            }
        }
        return rows;
    });
}

PagePtr AudioOptions() {
    return MakePage("audio/options", NativeOptions::Text("audio_options"), [] {
        std::vector<Row> rows;
        for (auto* widget : {&lowHpAlarm, &naviCall, &enemyProx, &leeverProx, &leadingMusic, &displaySeqName,
                              &ovlDuration, &voicePitch, &randomAudioGenModes, &lowerOctaves}) {
            if (widget == &leeverProx && CVarGetInteger(CVAR_AUDIO("EnemyBGMDisable"), 0)) continue;
            AppendWidget(rows, *widget, widget->cVar);
        }
        rows.push_back(Action("reset-voice", NativeOptions::Text("reset_voice_pitch"), [] {
            CVarSetFloat(CVAR_AUDIO("LinkVoiceFreqMultiplier"), 1);
            SaveSettings();
        }));
        return rows;
    });
}

PagePtr AudioEditorPage() {
    AudioCollection::Instance->InitializeShufflePool();
    auto page = MakePage("audio/editor", NativeOptions::Text("audio_editor"), [] {
        std::vector<Row> rows = { Link("options", NativeOptions::Text("audio_options"), AudioOptions) };
        const std::pair<SeqType, const char*> groups[] = {
            {SEQ_BGM_WORLD, "background_music"}, {SEQ_FANFARE, "fanfares"}, {SEQ_BGM_EVENT, "events"},
            {SEQ_BGM_BATTLE, "battle_music"}, {SEQ_ENDING, "ending"}, {SEQ_INSTRUMENT, "instruments"},
            {SEQ_OCARINA, "ocarina"}, {SEQ_SFX, "sound_effects"}, {SEQ_VOICE, "voices"},
        };
        for (const auto& [type, key] : groups) {
            auto title = NativeOptions::Text(key);
            rows.push_back(Link(key, title, [=] { return SequenceGroup(type, title); }));
        }
        rows.push_back(Link("pool", NativeOptions::Text("shuffle_pool"), ShufflePool));
        rows.push_back(Action("random", NativeOptions::Text("random_all_groups"), AudioEditor_RandomizeAll));
        rows.push_back(Action("reset", NativeOptions::Text("reset_all_groups"), AudioEditor_ResetAll));
        rows.push_back(Action("lock", NativeOptions::Text("lock_all_groups"), AudioEditor_LockAll));
        rows.push_back(Action("unlock", NativeOptions::Text("unlock_all_groups"), AudioEditor_UnlockAll));
        return rows;
    });
    page->onClose = StopAudioPreview;
    return page;
}
} // namespace

void RegisterAudioWidgets() {
    NativeOptions::RegisterPage("Audio Editor", AudioEditorPage);
    lowHpAlarm = { .name = "Mute Low HP Alarm", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    lowHpAlarm.CVar(CVAR_AUDIO("LowHpAlarm"))
        .Options(CheckboxOptions().Color(THEME_COLOR).Tooltip("Disable the low HP beeping sound."));
    SohGui::mSohMenu->AddSearchWidget({ lowHpAlarm, "Enhancements", "Audio Editor", "Audio Options" });

    naviCall = { .name = "Disable Navi Call Audio", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    naviCall.CVar(CVAR_AUDIO("DisableNaviCallAudio"))
        .Options(CheckboxOptions().Color(THEME_COLOR).Tooltip("Disables the voice audio when Navi calls you."));
    SohGui::mSohMenu->AddSearchWidget({ naviCall, "Enhancements", "Audio Editor", "Audio Options" });

    enemyProx = { .name = "Disable Enemy Proximity Music", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    enemyProx.CVar(CVAR_AUDIO("EnemyBGMDisable"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Disables the music change when getting close to enemies. Useful for hearing "
                              "your custom music for each scene more often."));

    leeverProx = { .name = "Enable Enemy Proximity Music for Leever", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    leeverProx.CVar(CVAR_AUDIO("LeeverEnemyBGM"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Plays the battle music when getting close to a Leever, like in Majora's Mask."));

    leadingMusic = { .name = "Disable Leading Music in Lost Woods", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    leadingMusic.CVar(CVAR_AUDIO("LostWoodsConsistentVolume"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Disables the volume shifting in the Lost Woods. Useful for hearing "
                              "your custom music in the Lost Woods if you don't need the navigation assitance "
                              "the volume changing provides. If toggling this while in the Lost Woods, reload "
                              "the area for the effect to kick in."));
    SohGui::mSohMenu->AddSearchWidget({ leadingMusic, "Enhancements", "Audio Editor", "Audio Options" });

    displaySeqName = { .name = "Display Sequence Name in Notifications", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    displaySeqName.CVar(CVAR_AUDIO("SeqNameNotification"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Emits a notification with the current song name whenever it changes. "
                              "(does not apply to fanfares or enemy BGM)."));
    SohGui::mSohMenu->AddSearchWidget({ displaySeqName, "Enhancements", "Audio Editor", "Audio Options" });

    ovlDuration = { .name = "Sequence Notification Duration: %d seconds", .type = WidgetType::WIDGET_CVAR_SLIDER_INT };
    ovlDuration.CVar(CVAR_AUDIO("SeqNameNotificationDuration"))
        .Options(IntSliderOptions().Color(THEME_COLOR).Min(1).Max(20).DefaultValue(10).Size(ImVec2(300.0f, 0.0f)));
    SohGui::mSohMenu->AddSearchWidget({ ovlDuration, "Enhancements", "Audio Editor", "Audio Options" });

    voicePitch = { .name = "Link's Voice Pitch Multiplier", .type = WidgetType::WIDGET_CVAR_SLIDER_FLOAT };
    voicePitch.CVar(CVAR_AUDIO("LinkVoiceFreqMultiplier"))
        .Options(FloatSliderOptions()
                     .Color(THEME_COLOR)
                     .IsPercentage()
                     .Min(0.4f)
                     .Max(2.5f)
                     .DefaultValue(1.0f)
                     .Size(ImVec2(300.0f, 0.0f)));
    SohGui::mSohMenu->AddSearchWidget({ voicePitch, "Enhancements", "Audio Editor", "Audio Options" });

    randomAudioGenModes = { .name = "Automatically Randomize All Music and Sound Effects",
                            .type = WidgetType::WIDGET_CVAR_COMBOBOX };
    randomAudioGenModes.CVar(CVAR_AUDIO("RandomizeAudioGenModes"))
        .Options(
            ComboboxOptions()
                .DefaultIndex(RANDOMIZE_OFF)
                .ComboMap(audioRandomizerModes)
                .Tooltip(
                    "Set when the music and sound effects is automaticly randomized:\n"
                    "- Manual: Manually randomize music or sound effects by pressing the 'Randomize all Groups' "
                    "button\n"
                    "- On New Scene : Randomizes when you enter a new scene.\n"
                    "- On Rando Gen Only: Randomizes only when you generate a new randomizer.\n"
                    "- On File Load: Randomizes on File Load.\n"
                    "- On File Load (Seeded): Randomizes on file load based on the current randomizer seed/file."));
    SohGui::mSohMenu->AddSearchWidget({ randomAudioGenModes, "Enhancements", "Audio Editor", "Audio Options" });

    lowerOctaves = { .name = "Lower Octaves of Unplayable High Notes", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    lowerOctaves.CVar(CVAR_AUDIO("ExperimentalOctaveDrop"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("Some custom sequences may have notes that are too high for the game's audio "
                              "engine to play. Enabling this checkbox will cause these notes to drop a "
                              "couple of octaves so they can still harmonize with the other notes of the "
                              "sequence."));
    SohGui::mSohMenu->AddSearchWidget({ lowerOctaves, "Enhancements", "Audio Editor", "Audio Options" });
}

static RegisterMenuInitFunc menuInitFunc(RegisterAudioWidgets);
