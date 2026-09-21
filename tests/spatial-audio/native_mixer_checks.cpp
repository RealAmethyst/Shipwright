#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>

extern "C" {
#include "soh/mixer.h"
}

static bool active = false;
static bool accept = true;
static std::array<float, 256> captured{};
static int captureCount = 0;
static bool badMetadata = false;

extern "C" int SpatialAudio_PositionalActive() { return active; }
extern "C" void SpatialAudio_MixCues(int16_t*, int16_t*, int16_t*, int16_t*, int, float, float) {}
extern "C" int SpatialAudio_Capture(const SpatialAudioSource* source, const float* pcm, int frames) {
    if (source->identity != 42 || frames > captured.size()) {
        badMetadata = true;
        return false;
    }
    std::copy_n(pcm, frames, captured.begin());
    ++captureCount;
    return accept;
}

static void Check(bool valid, const char* error) { if (!valid) throw std::runtime_error(error); }
using Samples = std::array<int16_t, 192>;
struct Result { Samples left, right, wetLeft, wetRight; };

static Result Mix(bool spatial, bool source, bool accepted, const Samples& input) {
    active = spatial;
    accept = accepted;
    captured.fill(0);
    captureCount = 0;
    SpatialAudioSource position{42, 1, 0, 0};
    aSetSpatialSourceImpl(source ? &position : nullptr);
    aClearBufferImpl(0x940, 0x680);
    aLoadBufferImpl(input.data(), 0x3C0, input.size() * 2);
    aEnvSetup1Impl(64, 16, 180, 110);
    aEnvSetup2Impl(18000, 29000);
    aEnvMixerImpl(0x3C0, input.size(), false, false, false, false, false, 0x94AEC8E2, 0);
    Result result;
    aSaveBufferImpl(0x940, result.left.data(), input.size() * 2);
    aSaveBufferImpl(0xAE0, result.right.data(), input.size() * 2);
    aSaveBufferImpl(0xC80, result.wetLeft.data(), input.size() * 2);
    aSaveBufferImpl(0xE20, result.wetRight.data(), input.size() * 2);
    return result;
}

int main() {
    try {
        Samples input;
        for (size_t i = 0; i < input.size(); ++i) input[i] = static_cast<int16_t>(std::sin(i * 0.17) * 9000);
        const auto original = Mix(false, true, true, input);
        Check(captureCount == 0, "stereo/mono entered HRTF capture");
        const auto music = Mix(true, false, true, input);
        Check(captureCount == 0 && original.left == music.left && original.right == music.right &&
              original.wetLeft == music.wetLeft && original.wetRight == music.wetRight,
              "nonpositional/music processing changed");
        const auto spatial = Mix(true, true, true, input);
        Check(!badMetadata, "wrong source metadata");
        Check(captureCount == 1, "world source was not captured exactly once");
        Check(spatial.wetLeft == original.wetLeft && spatial.wetRight == original.wetRight,
              "spatialization altered the native reverb send");
        Check(std::all_of(spatial.left.begin(), spatial.left.end(), [](int16_t v) { return v == 0; }) &&
              std::all_of(spatial.right.begin(), spatial.right.end(), [](int16_t v) { return v == 0; }),
              "positional dry sound was duplicated in the original mix");
        for (size_t i = 0; i < input.size(); ++i) {
            const float l = (18000 + (i / 8) * 180) / 65536.0f;
            const float r = (29000 + (i / 8) * 110) / 65536.0f;
            const float expected = input[i] * std::sqrt(l * l + r * r) / 32768.0f;
            Check(std::abs(expected - captured[i]) < 0.000001f, "gain ramp or source waveform changed");
        }
        const auto fallback = Mix(true, true, false, input);
        Check(original.left == fallback.left && original.right == fallback.right &&
              original.wetLeft == fallback.wetLeft && original.wetRight == fallback.wetRight,
              "rejected capture lost or altered native sound");
        std::cout << "Production mixer checks passed: native bypass, music, wet sends, dry separation and gain ramps.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
