#include "BinauralMixer.h"
#include "SpeakerPanner.h"
#include <chrono>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace SpatialAudio;
using Stereo = BinauralMixer::StereoFrame;

static void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static std::vector<Stereo> Render(BinauralMixer& mixer, Direction direction, const std::vector<size_t>& sizes,
                                  bool source, bool music) {
    mixer.Reset();
    std::vector<Stereo> result;
    size_t position = 0;
    for (size_t frames : sizes) {
        std::vector<float> mono(frames);
        std::vector<Stereo> bed(frames), output(frames);
        for (size_t i = 0; i < frames; ++i) {
            mono[i] = position + i == 0 ? 0.25f : 0;
            if (music) bed[i] = {std::sin(static_cast<float>(position + i) * 0.03f) * 0.1f,
                                std::cos(static_cast<float>(position + i) * 0.05f) * 0.1f};
        }
        Check(mixer.BeginSlice(frames), "begin slice failed");
        if (source && position == 0) Check(mixer.AddSource(1, direction, mono.data(), frames), "source failed");
        Check(mixer.EndSlice(bed.data(), output.data(), frames), "end slice failed");
        result.insert(result.end(), output.begin(), output.end());
        position += frames;
    }
    return result;
}

static float Energy(const std::vector<Stereo>& samples, size_t channel) {
    float sum = 0;
    for (auto sample : samples) {
        Check(std::isfinite(sample[channel]), "nonfinite HRTF output");
        sum += sample[channel] * sample[channel];
    }
    return sum;
}

int main(int argc, char** argv) {
    try {
        SpatialAudio::SpeakerPanner speakers;
        SpatialAudio::SpeakerPanner::Gains gains;
        Check(speakers.Compute({0, 0, -1}, gains) && gains[2] > 0.999f, "front center not isolated");
        Check(speakers.Compute({-1, 0, 0}, gains) && gains[4] > 0.999f, "left speaker not isolated");
        Check(speakers.Compute({0, 1, 0}, gains), "directly overhead rejected");
        float height = 0;
        for (size_t c = 8; c < 12; ++c) height += gains[c] * gains[c];
        Check(height > 0.999f, "overhead sound leaked to floor speakers");
        for (int elevation = -90; elevation <= 90; ++elevation) {
            SpatialAudio::SpeakerPanner::Gains previous{};
            for (int azimuth = -180; azimuth < 180; ++azimuth) {
                const float e = elevation * 0.01745329252f, a = azimuth * 0.01745329252f;
                Check(speakers.Compute({std::sin(a) * std::cos(e), std::sin(e), -std::cos(a) * std::cos(e)}, gains),
                      "speaker layout has a directional gap");
                float energy = 0;
                for (float gain : gains) { Check(std::isfinite(gain) && gain >= 0, "invalid speaker gain"); energy += gain * gain; }
                Check(std::abs(energy - 1) < 0.00001f && gains[3] == 0, "speaker energy or LFE routing changed");
                if (elevation >= 0 && azimuth > -180) {
                    float change = 0;
                    for (size_t c = 0; c < gains.size(); ++c) change += std::pow(gains[c] - previous[c], 2);
                    Check(change < 0.01f, "moving speaker source jumps between triangles");
                }
                previous = gains;
            }
        }
        BinauralMixer mixer;
        std::string error;
        Check(argc == 2, "Supply the verified CIPIC 124 SOFA path");
        std::ifstream file(argv[1], std::ios::binary);
        std::vector<uint8_t> sofa((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        const bool initialized = mixer.Initialize(32000, sofa.data(), sofa.size(), error);
        Check(initialized, error.c_str());
        const std::vector<size_t> irregular{176, 192, 192, 176, 176, 176, 192, 176, 192, 208, 192, 192};
        const size_t total = 2240;
        Check(total == [&] {size_t n = 0; for (auto x : irregular) n += x; return n;}(), "test length mismatch");
        const std::vector<size_t> regular(total / 16, 16);
        const auto left = Render(mixer, {-1, 0, 0}, irregular, true, false);
        const auto right = Render(mixer, {1, 0, 0}, irregular, true, false);
        Check(Energy(left, 0) > Energy(left, 1), "left source not louder in left ear");
        Check(Energy(right, 1) > Energy(right, 0), "right source not louder in right ear");
        const auto front = Render(mixer, {0, 0, -1}, irregular, true, false);
        const auto rear = Render(mixer, {0, 0, 1}, irregular, true, false);
        const auto above = Render(mixer, {0, 1, 0}, irregular, true, false);
        const auto below = Render(mixer, {0, -1, 0}, irregular, true, false);
        Check(front != rear, "front and rear HRTF responses are identical");
        Check(above != below, "above and below HRTF responses are identical");
        const auto split = Render(mixer, {-1, 0, 0}, regular, true, false);
        for (size_t i = 0; i < left.size(); ++i)
            for (size_t c = 0; c < 2; ++c)
                Check(std::abs(left[i][c] - split[i][c]) < 0.000001f, "slice boundaries changed waveform");
        const auto bed = Render(mixer, {0, 0, -1}, irregular, false, true);
        for (size_t i = 0; i < bed.size(); ++i) {
            if (i < BinauralMixer::BlockFrames) Check(bed[i] == Stereo{0, 0}, "incorrect initial delay");
            else {
                const size_t p = i - BinauralMixer::BlockFrames;
                Check(bed[i] == Stereo{std::sin(static_cast<float>(p) * 0.03f) * 0.1f,
                                      std::cos(static_cast<float>(p) * 0.05f) * 0.1f}, "music bed changed");
            }
        }
        Check(mixer.ActiveSources() == 0, "music created a positional source");
        mixer.Reset();
        Check(!mixer.BeginSlice(0) && !mixer.BeginSlice(257), "invalid slice accepted");
        Check(mixer.BeginSlice(128), "valid slice rejected");
        std::array<float, 128> samples{};
        Check(!mixer.AddSource(1, {0, 0, 0}, samples.data(), 128), "invalid direction accepted");
        samples[0] = std::numeric_limits<float>::quiet_NaN();
        Check(!mixer.AddSource(1, {0, 0, -1}, samples.data(), 128), "NaN source accepted");
        Check(!mixer.BeginSlice(128), "nested slice accepted");
        mixer.Reset();
        Check(mixer.BeginSlice(128), "capacity slice rejected");
        samples.fill(0.001f);
        for (uint64_t id = 1; id <= BinauralMixer::MaxSources; ++id)
            Check(mixer.AddSource(id, {0, 0, -1}, samples.data(), 128), "preallocated source capacity missing");
        Check(!mixer.AddSource(9999, {0, 0, -1}, samples.data(), 128), "source capacity overflow accepted");
        std::array<Stereo, 128> empty{}, output{};
        Check(mixer.EndSlice(empty.data(), output.data(), 128), "capacity rendering failed");
        for (int i = 0; i < 32; ++i) {
            Check(mixer.BeginSlice(128), "tail slice rejected");
            Check(mixer.EndSlice(empty.data(), output.data(), 128), "tail rendering failed");
        }
        Check(mixer.ActiveSources() == 0, "finished HRTF tails retained source identities");
        mixer.Reset();
        const auto start = std::chrono::steady_clock::now();
        for (int block = 0; block < 250; ++block) {
            Check(mixer.BeginSlice(128), "stress slice rejected");
            for (uint64_t id = 1; id <= 64; ++id)
                Check(mixer.AddSource(id, {std::sin(id * 0.3f), 0, std::cos(id * 0.3f)}, samples.data(), 128), "stress source rejected");
            Check(mixer.EndSlice(empty.data(), output.data(), 128), "stress render failed");
        }
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        std::cout << "Steam Audio direction, height, streaming, music, capacity and tail checks passed.\n";
        std::cout << "64 continuous sources: one second of audio rendered in " << milliseconds << " milliseconds.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
