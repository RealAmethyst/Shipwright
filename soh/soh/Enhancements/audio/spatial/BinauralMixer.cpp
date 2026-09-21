#include "BinauralMixer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#ifdef SOH_STEAM_AUDIO
#include <phonon.h>

namespace SpatialAudio {
namespace {
thread_local std::string initializationMessage;
void IPLCALL InitializationLog(IPLLogLevel level, const char* message) {
    if (level == IPL_LOGLEVEL_ERROR && message) initializationMessage = message;
}
}

struct BinauralMixer::State {
    struct Voice {
        IPLBinauralEffect effect = nullptr;
        uint64_t identity = 0;
        Direction direction;
        std::array<float, MaxSliceFrames> slice{};
        std::array<float, BlockFrames> pending{};
        bool inSlice = false;
        bool inBlock = false;
        bool tail = false;
    };

    IPLContext context = nullptr;
    IPLHRTF hrtf = nullptr;
    std::array<Voice, MaxSources> voices{};
    std::array<StereoFrame, BlockFrames> bed{};
    std::array<StereoFrame, BlockFrames * 2> output{};
    std::array<float, BlockFrames> left{};
    std::array<float, BlockFrames> right{};
    size_t filled = 0;
    size_t read = 0;
    size_t queued = BlockFrames;
    size_t sliceFrames = 0;
    bool ready = false;
    bool sliceOpen = false;

    ~State() {
        for (auto& voice : voices)
            if (voice.effect) iplBinauralEffectRelease(&voice.effect);
        if (hrtf) iplHRTFRelease(&hrtf);
        if (context) iplContextRelease(&context);
    }

    void Render() {
        float* channels[] = {left.data(), right.data()};
        IPLAudioBuffer result{2, static_cast<int>(BlockFrames), channels};
        for (auto& voice : voices) {
            if (!voice.identity) continue;
            IPLAudioEffectState remaining = IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;
            if (voice.inBlock) {
                float* mono[] = {voice.pending.data()};
                IPLAudioBuffer input{1, static_cast<int>(BlockFrames), mono};
                IPLBinauralEffectParams parameters{};
                parameters.direction = {voice.direction.x, voice.direction.y, voice.direction.z};
                parameters.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
                parameters.spatialBlend = 1.0f;
                parameters.hrtf = hrtf;
                remaining = iplBinauralEffectApply(voice.effect, &parameters, &input, &result);
            } else if (voice.tail) {
                remaining = iplBinauralEffectGetTail(voice.effect, &result);
            } else {
                if (!voice.inSlice) voice.identity = 0;
                continue;
            }
            for (size_t frame = 0; frame < BlockFrames; ++frame) {
                bed[frame][0] += left[frame];
                bed[frame][1] += right[frame];
            }
            voice.tail = remaining == IPL_AUDIOEFFECTSTATE_TAILREMAINING;
            voice.inBlock = false;
            voice.pending.fill(0);
        }
        for (const auto& frame : bed) {
            output[(read + queued) % output.size()] = frame;
            ++queued;
        }
        filled = 0;
    }
};

BinauralMixer::BinauralMixer() : state(std::make_unique<State>()) {}
BinauralMixer::~BinauralMixer() = default;

bool BinauralMixer::Initialize(int sampleRate, const uint8_t* sofa, size_t sofaBytes, std::string& error) {
    if (sampleRate <= 0 || !sofa || !sofaBytes || sofaBytes > std::numeric_limits<int>::max()) {
        error = "Steam Audio: invalid sample rate or HRTF data";
        return false;
    }
    auto next = std::make_unique<State>();
    IPLContextSettings contextSettings{};
    initializationMessage.clear();
    contextSettings.version = STEAMAUDIO_VERSION;
    contextSettings.logCallback = InitializationLog;
    contextSettings.simdLevel = IPL_SIMDLEVEL_SSE2;
    auto status = iplContextCreate(&contextSettings, &next->context);
    if (status != IPL_STATUS_SUCCESS) {
        error = "Steam Audio: context creation failed (" + std::to_string(status) + ")";
        return false;
    }
    IPLAudioSettings audioSettings{sampleRate, static_cast<int>(BlockFrames)};
    IPLHRTFSettings hrtfSettings{};
    // The built-in HRTF has no 32 kHz variant. The SDK's SOFA reader resamples
    // the same CIPIC 124 measurements once, without resampling the game's mix.
    hrtfSettings.type = IPL_HRTFTYPE_SOFA;
    hrtfSettings.sofaData = sofa;
    hrtfSettings.sofaDataSize = static_cast<int>(sofaBytes);
    hrtfSettings.volume = 1;
    hrtfSettings.normType = IPL_HRTFNORMTYPE_RMS;
    status = iplHRTFCreate(next->context, &audioSettings, &hrtfSettings, &next->hrtf);
    if (status != IPL_STATUS_SUCCESS) {
        error = "Steam Audio: HRTF creation failed (" + std::to_string(status) + "): " + initializationMessage;
        return false;
    }
    IPLBinauralEffectSettings settings{next->hrtf};
    for (auto& voice : next->voices) {
        status = iplBinauralEffectCreate(next->context, &audioSettings, &settings, &voice.effect);
        if (status != IPL_STATUS_SUCCESS) {
            error = "Steam Audio: voice creation failed (" + std::to_string(status) + ")";
            return false;
        }
    }
    next->ready = true;
    state = std::move(next);
    error.clear();
    return true;
}

void BinauralMixer::Reset() {
    for (auto& voice : state->voices) {
        if (voice.effect) iplBinauralEffectReset(voice.effect);
        voice.identity = 0;
        voice.slice.fill(0);
        voice.pending.fill(0);
        voice.inSlice = voice.inBlock = voice.tail = false;
    }
    state->bed.fill({0, 0});
    state->output.fill({0, 0});
    state->filled = state->read = state->sliceFrames = 0;
    state->queued = BlockFrames;
    state->sliceOpen = false;
}

bool BinauralMixer::BeginSlice(size_t frames) {
    if (!state->ready || state->sliceOpen || frames == 0 || frames > MaxSliceFrames) return false;
    for (auto& voice : state->voices) {
        voice.inSlice = false;
        voice.slice.fill(0);
    }
    state->sliceFrames = frames;
    state->sliceOpen = true;
    return true;
}

bool BinauralMixer::AddSource(uint64_t identity, Direction direction, const float* mono, size_t frames) {
    if (!state->sliceOpen || !identity || !mono || frames != state->sliceFrames) return false;
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y + direction.z * direction.z);
    if (!std::isfinite(length) || length < 0.00001f) return false;
    for (size_t frame = 0; frame < frames; ++frame)
        if (!std::isfinite(mono[frame])) return false;
    State::Voice* selected = nullptr;
    for (auto& voice : state->voices)
        if (voice.identity == identity) {
            selected = &voice;
            break;
        }
    if (!selected) {
        for (auto& voice : state->voices)
            if (!voice.identity) {
                selected = &voice;
                iplBinauralEffectReset(voice.effect);
                voice.identity = identity;
                break;
            }
    }
    if (!selected) return false;
    selected->direction = {direction.x / length, direction.y / length, direction.z / length};
    selected->inSlice = true;
    for (size_t frame = 0; frame < frames; ++frame) selected->slice[frame] += mono[frame];
    return true;
}

bool BinauralMixer::EndSlice(const StereoFrame* bed, StereoFrame* output, size_t frames) {
    if (!state->sliceOpen || !bed || !output || frames != state->sliceFrames) return false;
    for (size_t frame = 0; frame < frames; ++frame) {
        state->bed[state->filled] = bed[frame];
        for (auto& voice : state->voices) {
            if (!voice.inSlice) continue;
            voice.pending[state->filled] = voice.slice[frame];
            voice.inBlock = true;
        }
        if (++state->filled == BlockFrames) state->Render();
        output[frame] = state->output[state->read];
        state->read = (state->read + 1) % state->output.size();
        --state->queued;
    }
    state->sliceOpen = false;
    return true;
}

size_t BinauralMixer::ActiveSources() const {
    return std::count_if(state->voices.begin(), state->voices.end(), [](const auto& voice) { return voice.identity != 0; });
}

} // namespace SpatialAudio
#else
namespace SpatialAudio {
struct BinauralMixer::State {};
BinauralMixer::BinauralMixer() : state(std::make_unique<State>()) {}
BinauralMixer::~BinauralMixer() = default;
bool BinauralMixer::Initialize(int, const uint8_t*, size_t, std::string& error) {
    error = "Steam Audio is unavailable in this platform build";
    return false;
}
void BinauralMixer::Reset() {}
bool BinauralMixer::BeginSlice(size_t) { return false; }
bool BinauralMixer::AddSource(uint64_t, Direction, const float*, size_t) { return false; }
bool BinauralMixer::EndSlice(const StereoFrame*, StereoFrame*, size_t) { return false; }
size_t BinauralMixer::ActiveSources() const { return 0; }
} // namespace SpatialAudio
#endif
