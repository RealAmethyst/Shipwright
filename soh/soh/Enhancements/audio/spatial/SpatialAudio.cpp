#include "SpatialAudio.h"
#include "BinauralMixer.h"
#include "SpeakerPanner.h"
#include "CueActors.h"
#include "soh/Enhancements/tts/CompassSpeech.h"
#include "soh/Enhancements/navigation/Navigation.h"

#include <libultraship/libultraship.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <set>

extern "C" {
#include "global.h"
extern PlayState* gPlayState;
}

namespace {
struct Projection {
    Vec3f world;
    Vec3f projected;
};
struct ProjectionSlot {
    const float* position = nullptr;
    uint64_t frame = 0;
    Projection value{};
};
struct Source {
    const float* position = nullptr;
    Projection projection{};
    SpatialAudioSource snapshot{};
};

std::unique_ptr<SpatialAudio::BinauralMixer> mixer;
std::array<ProjectionSlot, 4096> projections{};
uint64_t projectionFrame = 1;
bool reportedProjectionCapacity = false;
std::unordered_map<const void*, Source> sources;
std::array<SpatialAudioSource, 16> channels{};
std::atomic<int> requestedMode{0};
std::atomic<bool> ready{false};
MtxF listener{};
Vec3f distanceOrigin{};
bool listenerValid = false;
bool mirrored = false;
uint64_t nextIdentity = 0;
int mixingMode = -1;
bool sliceOpen = false;
bool reportedCaptureFailure = false;
std::set<std::pair<uint16_t, std::string>> reportedSources;
SpatialAudio::SpeakerPanner speakerPanner;
using SpeakerFrame = SpatialAudio::SpeakerPanner::Gains;
std::array<SpeakerFrame, SpatialAudio::BinauralMixer::MaxSliceFrames> speakerSlice{};
std::array<std::array<int16_t, SpatialAudio::SpeakerPanner::Channels>, 1680> speakerBatch{};
size_t batchFrames = 0;
int sliceFrames = 0;
uint64_t sliceNumber = 0;
struct SpeakerVoice {
    uint64_t identity = 0, lastSlice = 0;
    SpeakerFrame gains{};
};
std::array<SpeakerVoice, 256> speakerVoices{};

bool Finite(const Vec3f& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

ProjectionSlot* ProjectionFor(const float* position) {
    const auto key = reinterpret_cast<uintptr_t>(position);
    const size_t first = ((key >> 4) ^ (key >> 17)) % projections.size();
    for (size_t i = 0; i < projections.size(); ++i) {
        auto& slot = projections[(first + i) % projections.size()];
        if (slot.frame != projectionFrame || slot.position == position) return &slot;
    }
    return nullptr;
}

SpatialAudioSource Direction(uint64_t identity, const Vec3f& world) {
    if (!listenerValid || !Finite(world)) return {};
    const float x = listener.xx * world.x + listener.xy * world.y + listener.xz * world.z + listener.xw;
    const float y = listener.yx * world.x + listener.yy * world.y + listener.yz * world.z + listener.yw;
    const float z = listener.zx * world.x + listener.zy * world.y + listener.zz * world.z + listener.zw;
    const float length = std::sqrt(x * x + y * y + z * z);
    if (!std::isfinite(length) || length < 0.00001f) return {};
    return {identity, (mirrored ? -x : x) / length, y / length, z / length};
}

bool FindProjection(const void* entry, const float* x, const float* y, const float* z, Projection& result,
                    const char** reason = nullptr) {
    const auto fail = [&](const char* message) { if (reason) *reason = message; return false; };
    if (!listenerValid) return fail("listener unavailable");
    if (!x || !y || !z) return fail("position component missing");
    if (x == &gSfxDefaultPos.x) return fail("nonpositional source");
    auto* current = ProjectionFor(x);
    if (current && current->frame == projectionFrame) result = current->value;
    else {
        auto previous = sources.find(entry);
        if (previous == sources.end()) return fail("world projection not recorded");
        if (previous->second.position != x) return fail("position storage changed");
        result = previous->second.projection;
    }
    // A reused stack address or changed position is not proof of a world source.
    if (!Finite(result.world)) return fail("nonfinite world position");
    if (result.projected.x != *x || result.projected.y != *y || result.projected.z != *z)
        return fail("projected position changed after capture");
    return true;
}
} // namespace

extern "C" void SpatialAudio_Init() {
    sources.reserve(128);
    SpatialAudio::InitCues();
    auto next = std::make_unique<SpatialAudio::BinauralMixer>();
    const auto path = Ship::Context::GetPathRelativeToAppDirectory("accessibility/audio/cipic_124.sofa");
    std::ifstream input(path, std::ios::binary);
    std::vector<uint8_t> sofa((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    std::string error;
    if (!next->Initialize(32000, sofa.data(), sofa.size(), error)) {
        SPDLOG_ERROR("Spatial audio initialization: {}; HRTF file: {}", error, path);
        return;
    }
    mixer = std::move(next);
    ready = true;
    SPDLOG_INFO("Steam Audio 4.8.1 initialized: CIPIC 124, 32000 Hz, 128-frame HRTF blocks");
}

extern "C" void SpatialAudio_Shutdown() {
    SpatialAudio::ShutdownCues();
    ready = false;
    mixer.reset();
    projections.fill({});
    sources.clear();
    channels.fill({});
    reportedSources.clear();
    listenerValid = sliceOpen = false;
    mixingMode = -1;
}

extern "C" int SpatialAudio_HrtfAvailable() { return ready; }
extern "C" int SpatialAudio_HrtfActive() { return ready && requestedMode == 2; }
extern "C" int SpatialAudio_PositionalActive() { return SpatialAudio_HrtfActive() || requestedMode == 3; }
extern "C" int SpatialAudio_GetMode() { return requestedMode; }
extern "C" int SpatialAudio_SetMode(int mode) {
    if (mode < 0 || mode > 3 || (mode == 2 && !ready)) return requestedMode;
    auto audio = Ship::Context::GetInstance()->GetAudio();
    if (!audio || !audio->TrySetAudioChannels(mode == 3 ? audioSpatial714 : audioStereo)) {
        SPDLOG_ERROR("Sound mode {} unavailable; retaining mode {}", mode, requestedMode.load());
        return requestedMode;
    }
    requestedMode = mode;
    return mode;
}

extern "C" void SpatialAudio_BeginGameFrame() {
    if (++projectionFrame == 0) { projections.fill({}); projectionFrame = 1; }
}

extern "C" void SpatialAudio_RecordProjection(const void* matrix, const void* world, const void* projected) {
    if (!gPlayState || matrix != &gPlayState->viewProjectionMtxF || !world || !projected) return;
    const Projection value{*static_cast<const Vec3f*>(world), *static_cast<const Vec3f*>(projected)};
    if (!Finite(value.world) || !Finite(value.projected)) return;
    const auto* position = &static_cast<const Vec3f*>(projected)->x;
    if (auto* slot = ProjectionFor(position)) *slot = {position, projectionFrame, value};
    else if (!reportedProjectionCapacity) {
        SPDLOG_WARN("Spatial audio projection capacity exceeded; unverifiable sources retain native mixing");
        reportedProjectionCapacity = true;
    }
}

extern "C" void SpatialAudio_PublishListener() {
    UpdateCompassSpeech(gPlayState);
    listenerValid = gPlayState && GET_PLAYER(gPlayState);
    if (!listenerValid) {
        sources.clear();
        channels.fill({});
        SpatialAudio::UpdateCues(nullptr);
        Navigation_PublishAudio(nullptr);
        return;
    }
    Matrix_MtxToMtxF(&gPlayState->view.viewing, &listener);
    distanceOrigin = GET_PLAYER(gPlayState)->actor.world.pos;
    listenerValid = Finite(distanceOrigin);
    mirrored = CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0) != 0;
    for (auto& [key, source] : sources)
        source.snapshot = Direction(source.snapshot.identity, source.projection.world);
    SpatialAudio::UpdateCues(gPlayState);
    Navigation_PublishAudio(gPlayState);
}

extern "C" SpatialAudioSource SpatialAudio_WorldSource(uint64_t identity, float x, float y, float z) {
    return Direction(identity, {x, y, z});
}

extern "C" void SpatialAudio_SetSfxSource(int channel, const void* entry, const float* x, const float* y,
                                          const float* z, int newSound) {
    if (channel < 0 || channel >= channels.size()) return;
    Projection projection;
    const char* reason = nullptr;
    if (!FindProjection(entry, x, y, z, projection, &reason)) {
        channels[channel] = {};
        auto existing = sources.find(entry);
        if (existing != sources.end()) {
            existing->second.snapshot = {existing->second.snapshot.identity, 0, 0, 0};
            channels[channel] = existing->second.snapshot;
        }
        if (entry && listenerValid && x != &gSfxDefaultPos.x && SpatialAudio_PositionalActive()) {
            const auto sound = static_cast<const SoundBankEntry*>(entry)->sfxId;
            if (reportedSources.emplace(sound, reason).second)
                SPDLOG_WARN("Spatial SFX 0x{:04x}: {}; retaining native direct mix", sound, reason);
        }
        return;
    }
    auto& source = sources[entry];
    if (newSound || !source.snapshot.identity) source.snapshot.identity = ++nextIdentity;
    source.position = x;
    source.projection = projection;
    source.snapshot = Direction(source.snapshot.identity, projection.world);
    channels[channel] = source.snapshot;
}

extern "C" void SpatialAudio_RemoveSfxSource(const void* entry) {
    auto found = sources.find(entry);
    if (found == sources.end()) return;
    for (auto& channel : channels)
        if (channel.identity == found->second.snapshot.identity) channel = {};
    sources.erase(found);
}

extern "C" SpatialAudioSource SpatialAudio_GetChannelSource(int channel) {
    return channel >= 0 && channel < channels.size() ? channels[channel] : SpatialAudioSource{};
}

extern "C" void SpatialAudio_RefreshSource(SpatialAudioSource* source) {
    if (!source || !source->identity) return;
    for (const auto& channel : channels)
        if (channel.identity == source->identity) {
            *source = channel;
            return;
        }
}

extern "C" int SpatialAudio_DistanceSquared(const void* entry, const float* x, const float* y, const float* z,
                                             float* distance) {
    if (!SpatialAudio_PositionalActive() || !distance) return false;
    Projection projection;
    if (!FindProjection(entry, x, y, z, projection)) return false;
    const auto& p = projection.world;
    *distance = (p.x - distanceOrigin.x) * (p.x - distanceOrigin.x) +
                (p.y - distanceOrigin.y) * (p.y - distanceOrigin.y) +
                (p.z - distanceOrigin.z) * (p.z - distanceOrigin.z);
    return std::isfinite(*distance);
}

extern "C" void SpatialAudio_BeginSlice(int frames) {
    sliceOpen = false;
    const int mode = requestedMode;
    if (mode != mixingMode) {
        if (mixer) mixer->Reset();
        speakerVoices.fill({});
        mixingMode = mode;
    }
    sliceFrames = frames;
    ++sliceNumber;
    if (mode == 2 && mixer) sliceOpen = mixer->BeginSlice(frames);
    if (mode == 3 && frames > 0 && frames <= speakerSlice.size() && batchFrames + frames <= speakerBatch.size()) {
        speakerSlice.fill({});
        sliceOpen = true;
    }
}

extern "C" int SpatialAudio_Capture(const SpatialAudioSource* source, const float* mono, int frames) {
    if (!sliceOpen || !source || !source->identity) return false;
    bool captured = false;
    if (mixingMode == 2) captured = mixer->AddSource(source->identity, {source->x, source->y, source->z}, mono, frames);
    if (mixingMode == 3 && mono && frames == sliceFrames) {
        SpeakerFrame gains;
        if (!speakerPanner.Compute({source->x, source->y, source->z}, gains)) return false;
        for (int i = 0; i < frames; ++i) if (!std::isfinite(mono[i])) return false;
        SpeakerVoice* voice = nullptr;
        for (auto& candidate : speakerVoices)
            if (candidate.identity == source->identity) { voice = &candidate; break; }
        if (!voice) {
            for (auto& candidate : speakerVoices)
                if (!candidate.identity || candidate.lastSlice + 1 < sliceNumber) { voice = &candidate; break; }
            if (voice) *voice = {source->identity, sliceNumber, gains};
        }
        if (voice) {
            for (int i = 0; i < frames; ++i) {
                const float t = static_cast<float>(i + 1) / frames;
                for (size_t c = 0; c < gains.size(); ++c)
                    speakerSlice[i][c] += mono[i] * (voice->gains[c] + t * (gains[c] - voice->gains[c]));
            }
            voice->gains = gains;
            voice->lastSlice = sliceNumber;
            captured = true;
        }
    }
    if (!captured && !reportedCaptureFailure) {
        SPDLOG_ERROR("Spatial audio source rejected: invalid block, direction or source capacity; retaining native mix");
        reportedCaptureFailure = true;
    }
    return captured;
}

extern "C" void SpatialAudio_EndSlice(int16_t* stereo, int frames) {
    if (!sliceOpen || frames != sliceFrames) return;
    if (mixingMode == 3) {
        for (int f = 0; f < frames; ++f) {
            speakerSlice[f][0] += stereo[f * 2] / 32768.0f;
            speakerSlice[f][1] += stereo[f * 2 + 1] / 32768.0f;
            for (size_t c = 0; c < speakerSlice[f].size(); ++c)
                speakerBatch[batchFrames + f][c] = static_cast<int16_t>(std::clamp(
                    std::lround(speakerSlice[f][c] * 32768.0f), -32768L, 32767L));
        }
        batchFrames += frames;
        sliceOpen = false;
        return;
    }
    std::array<SpatialAudio::BinauralMixer::StereoFrame, SpatialAudio::BinauralMixer::MaxSliceFrames> bed{}, output{};
    for (int frame = 0; frame < frames; ++frame)
        bed[frame] = {stereo[frame * 2] / 32768.0f, stereo[frame * 2 + 1] / 32768.0f};
    if (mixer->EndSlice(bed.data(), output.data(), frames)) {
        for (int frame = 0; frame < frames; ++frame)
            for (int channel = 0; channel < 2; ++channel)
                stereo[frame * 2 + channel] = static_cast<int16_t>(std::clamp(
                    std::lround(output[frame][channel] * 32768.0f), -32768L, 32767L));
    }
    sliceOpen = false;
}

extern "C" void SpatialAudio_BeginBatch() { batchFrames = 0; }
extern "C" const int16_t* SpatialAudio_SpeakerBuffer(int frames) {
    return requestedMode == 3 && frames == batchFrames ? speakerBatch[0].data() : nullptr;
}

extern "C" void SpatialAudio_MixCues(int16_t* left, int16_t* right, int16_t* wetLeft, int16_t* wetRight,
                                     int frames, float gameGain, float reverb) {
    struct Output { int16_t* left; int16_t* right; int16_t* wetLeft; int16_t* wetRight; float reverb; };
    Output output{left, right, wetLeft, wetRight, std::clamp(reverb, 0.0f, 1.0f)};
    SpatialAudio::GetCueMixer().Render(frames, gameGain * SpatialAudio::CueMasterGain(), &output,
        [](void* raw, const SpatialAudioSource& source, const float* mono, int count) {
            auto& out = *static_cast<Output*>(raw);
            const bool spatial = SpatialAudio_Capture(&source, mono, count);
            const float nativeX = mirrored ? -source.x : source.x;
            const int pan = std::clamp(static_cast<int>((nativeX + 1.0f) * 63.5f), 0, 127);
            const float l = requestedMode == 1 ? 0.707f : gStereoPanVolume[pan];
            const float r = requestedMode == 1 ? 0.707f : gStereoPanVolume[127 - pan];
            const auto add = [](int16_t& buffer, float sample) {
                buffer = static_cast<int16_t>(std::clamp(std::lround(buffer + sample * 32768.0f), -32768L, 32767L));
            };
            for (int i = 0; i < count; ++i) {
                if (!spatial) { add(out.left[i], mono[i] * l); add(out.right[i], mono[i] * r); }
                if (out.wetLeft && out.wetRight) {
                    add(out.wetLeft[i], mono[i] * l * out.reverb);
                    add(out.wetRight[i], mono[i] * r * out.reverb);
                }
            }
        });
}
