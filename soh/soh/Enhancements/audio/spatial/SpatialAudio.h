#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpatialAudioSource {
    uint64_t identity;
    float x;
    float y;
    float z;
} SpatialAudioSource;

void SpatialAudio_Init(void);
void SpatialAudio_Shutdown(void);
int SpatialAudio_SetMode(int mode);
int SpatialAudio_GetMode(void);
int SpatialAudio_HrtfAvailable(void);
int SpatialAudio_HrtfActive(void);
int SpatialAudio_PositionalActive(void);
void SpatialAudio_BeginGameFrame(void);
void SpatialAudio_RecordProjection(const void* matrix, const void* world, const void* projected);
void SpatialAudio_PublishListener(void);
void SpatialAudio_SetSfxSource(int channel, const void* entry, const float* x, const float* y, const float* z,
                               int newSound);
void SpatialAudio_RemoveSfxSource(const void* entry);
SpatialAudioSource SpatialAudio_GetChannelSource(int channel);
SpatialAudioSource SpatialAudio_WorldSource(uint64_t identity, float x, float y, float z);
void SpatialAudio_RefreshSource(SpatialAudioSource* source);
int SpatialAudio_DistanceSquared(const void* entry, const float* x, const float* y, const float* z, float* distance);
void SpatialAudio_BeginSlice(int frames);
int SpatialAudio_Capture(const SpatialAudioSource* source, const float* mono, int frames);
void SpatialAudio_EndSlice(int16_t* stereo, int frames);
void SpatialAudio_BeginBatch(void);
const int16_t* SpatialAudio_SpeakerBuffer(int frames);
void SpatialAudio_MixCues(int16_t* left, int16_t* right, int16_t* wetLeft, int16_t* wetRight,
                          int frames, float gameGain, float reverb);
float Audio_AccessibilityReverb(void);

#ifdef __cplusplus
}
#endif
