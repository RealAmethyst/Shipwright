#pragma once
#include "stdint.h"

#ifdef __cplusplus

#include <libultraship/libultraship.h>
#include <imgui.h>
#include "AudioCollection.h"

void InitializeAudioEditor();

void AudioEditor_RandomizeAll();
void AudioEditor_AutoRandomizeAll();
void AudioEditor_RandomizeGroup(SeqType group);
void AudioEditor_ResetAll();
void AudioEditor_ResetGroup(SeqType group);
void AudioEditor_LockAll();
void AudioEditor_UnlockAll();

extern "C" {
#endif

u16 AudioEditor_GetReplacementSeq(u16 seqId);

#ifdef __cplusplus
}
#endif
