#include "CueMixer.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace SpatialAudio;
static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Output { std::vector<float> pcm; size_t calls = 0; };
static void Capture(void* raw, const SpatialAudioSource& source, const float* pcm, int frames) {
    auto& output = *static_cast<Output*>(raw);
    Check(source.identity && source.x == -1 && source.y == 0 && source.z == 0, "cue position changed");
    for (int i = 0; i < frames; ++i) Check(std::isfinite(pcm[i]), "invalid cue PCM");
    output.pcm.insert(output.pcm.end(), pcm, pcm + frames);
    ++output.calls;
}
static Output Render(CueMixer& mixer, const std::vector<int>& slices, float volume) {
    mixer.StopAll();
    Check(mixer.Play(1, Cue::Item), "item did not start");
    mixer.Update(1, {1, -1, 0, 0}, volume);
    Output output;
    for (auto frames : slices) mixer.Render(frames, 0.4f, &output, Capture);
    return output;
}
static std::vector<float> ReadPcm(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    input.seekg(12);
    uint32_t tag, size;
    while (input.read(reinterpret_cast<char*>(&tag), 4) && input.read(reinterpret_cast<char*>(&size), 4)) {
        if (tag == 0x61746164) {
            std::vector<int16_t> samples(size / 2);
            Check(bool(input.read(reinterpret_cast<char*>(samples.data()), size)), "unreadable reference PCM");
            std::vector<float> result;
            for (auto sample : samples) result.push_back(sample / 32768.0f);
            return result;
        }
        input.seekg(size + (size & 1), std::ios::cur);
    }
    throw std::runtime_error("reference WAV has no PCM");
}
static Output RenderLoop(CueMixer& mixer, Cue cue, size_t frames, const std::vector<int>& blocks) {
    mixer.StopAll();
    Output output;
    size_t block = 0;
    while (output.pcm.size() < frames) {
        Check(mixer.KeepPlaying(7, cue), "cue loop did not start/continue");
        mixer.Update(7, {7, -1, 0, 0}, 1);
        const auto count = std::min(frames - output.pcm.size(), size_t(blocks[block++ % blocks.size()]));
        mixer.Render(static_cast<int>(count), 1, &output, Capture);
    }
    Check(mixer.ActiveVoices() == 1, "cue loop lost or duplicated its voice");
    return output;
}
static void CheckLoops(CueMixer& mixer, const std::string& directory) {
    for (size_t id = 0; id < CueNames.size(); ++id) {
        const Cue cue = static_cast<Cue>(id);
        const auto pcm = ReadPcm(directory + "/" + CueNames[id] + ".wav");
        const bool wall = cue >= Cue::WallNorth && cue <= Cue::WallWest;
        // Reference behavior from Time Stranger: 60 ms gaps, 2 ms wall overlap,
        // and a 400 ms destructible excerpt with a one-sample retrigger gap.
        const size_t length = cue == Cue::Destructible ? 12800 : pcm.size();
        Check(length <= pcm.size(), "destructible fixture shorter than its reference excerpt");
        const size_t gap = cue == Cue::Destructible ? 1 : 1920;
        const size_t period = wall ? length - 64 : length + gap;
        const auto loop = RenderLoop(mixer, cue, period * 3 + length, {176, 192, 208});
        const auto otherBlocks = RenderLoop(mixer, cue, loop.pcm.size(), {256, 17, 128});
        Check(loop.pcm == otherBlocks.pcm, "loop cadence depends on audio block boundaries");
        const size_t start = wall ? 64 : 0;
        for (size_t frame = start; frame + period < loop.pcm.size(); ++frame)
            Check(loop.pcm[frame] == loop.pcm[frame + period], "loop truncated, delayed or restarted by updates");
        if (wall) {
            Check(std::abs(loop.pcm[length - 1] - pcm[63]) < 0.000001f, "wall seam did not blend into its head");
            Check(loop.pcm[length] == pcm[64], "wall repeated or skipped its overlap");
            Check(loop.pcm[length - 65] == pcm[length - 65], "wall has an unwanted fade or gap");
        } else {
            for (size_t frame = length; frame < period; ++frame)
                Check(loop.pcm[frame] == 0, "wrong silence between cue repetitions");
            for (size_t frame = 64; frame + 64 < length; ++frame)
                Check(loop.pcm[frame] == pcm[frame], "cue was cut short or altered before its boundary fade");
            Check(std::abs(loop.pcm[32] - pcm[32] * 0.5f) < 0.000001f, "cue onset is not the reference 2 ms fade");
            Check(std::abs(loop.pcm[length - 32] - pcm[length - 32] * 0.5f) < 0.000001f,
                  "cue ending is not the reference 2 ms fade");
        }
        mixer.Stop(7);
        Output stopped;
        mixer.Render(192, 1, &stopped, Capture);
        Check(stopped.calls == 0, "stopped loop survived during its gap or tail");
        std::cout << CueNames[id] << ": " << period / 32.0 << " ms repeat, reference waveform verified\n";
    }
}
static void CheckNavigationLoop(CueMixer& mixer) {
    for (float seconds : {0.15f, 0.825f, 1.5f}) {
        const size_t period = static_cast<size_t>(seconds * 32000);
        auto render = [&](int block) {
            mixer.StopAll(); Output output;
            while (output.pcm.size() < period * 3) {
                Check(mixer.KeepPlayingInterval(77, Cue::Pathfinder, seconds), "navigation voice failed");
                mixer.Update(77, {77, -1, 0, 0}, 1);
                mixer.Render(static_cast<int>(std::min(size_t(block), period * 3 - output.pcm.size())), 1, &output, Capture);
            }
            return output.pcm;
        };
        const auto a = render(192), b = render(113);
        Check(a == b, "navigation interval depends on audio block size");
        Check(std::any_of(a.begin(), a.begin() + period, [](float v) { return std::abs(v) > 0.0001f; }), "navigation clip is silent");
        for (size_t i = 0; i + period < a.size(); ++i) Check(a[i] == a[i + period], "navigation loop interval is wrong");
    }
    mixer.StopAll();
    Check(!mixer.KeepPlayingInterval(77, Cue::Pathfinder, NAN) && mixer.ActiveVoices() == 0, "invalid interval retained voice");
    for (uint64_t i = 1; i <= CueMixer::MaxVoices; ++i) Check(mixer.Play(i, Cue::Door), "ordinary pool lost capacity");
    Check(mixer.KeepPlayingInterval(999, Cue::Pathfinder, 0.15f) && mixer.ActiveVoices() == CueMixer::MaxVoices + 1,
          "busy ordinary cues displaced selected guidance");
    Check(!mixer.KeepPlayingInterval(1000, Cue::Pathfinder, 0.15f), "second route replaced an unrelated owner");
    mixer.StopAll();
}

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "Supply bundled cue directory");
        CueMixer mixer;
        std::string error;
        for (size_t i = 0; i < CueNames.size(); ++i)
            Check(mixer.Load(static_cast<Cue>(i), std::string(argv[1]) + "/" + CueNames[i] + ".wav", error), error.c_str());
        const auto regular = Render(mixer, std::vector<int>(48, 16), 1);
        const auto irregular = Render(mixer, {176, 192, 192, 208}, 1);
        Check(regular.pcm == irregular.pcm, "cue waveform depends on slice size");
        const auto quieter = Render(mixer, {176, 192, 192, 208}, 0.1f);
        for (size_t i = 0; i < quieter.pcm.size(); ++i)
            Check(std::abs(quieter.pcm[i] - irregular.pcm[i] * 0.1f) < 0.000001f, "cue volume ignored");
        mixer.Update(1, {1, -1, 0, 0}, 0);
        Output stopped;
        mixer.Render(192, 1, &stopped, Capture);
        Check(stopped.calls == 0 && mixer.ActiveVoices() == 0, "disabled cue still playing");
        for (uint64_t i = 1; i <= CueMixer::MaxVoices; ++i) Check(mixer.Play(i, Cue::Door), "voice capacity too small");
        Check(!mixer.Play(999, Cue::Door), "voice pool exceeded its limit");
        mixer.Stop(32);
        Check(mixer.Play(999, Cue::Transition), "released voice was not reusable");
        mixer.StopAll();
        Check(mixer.ActiveVoices() == 0, "scene cleanup retained cue voices");
        for (size_t cue = 0; cue < CueNames.size(); ++cue) {
            Check(mixer.Play(1, static_cast<Cue>(cue)), "bundled recording did not start");
            mixer.Update(1, {1, -1, 0, 0}, 1);
            Output complete;
            for (int i = 0; i < 32000 * 60 / 192 + 1 && mixer.ActiveVoices(); ++i)
                mixer.Render(192, 1, &complete, Capture);
            Check(mixer.ActiveVoices() == 0 && complete.calls > 0, "finished recording retained a voice");
            Check(std::any_of(complete.pcm.begin(), complete.pcm.end(), [](float sample) { return std::abs(sample) > 0.0001f; }),
                  "bundled recording is silent");
        }
        Check(!mixer.Load(Cue::Item, std::string(argv[1]) + "/manifest.json", error), "non-WAV accepted");
        Check(!mixer.Play(0, Cue::Item), "zero identity accepted");
        Check(!mixer.KeepPlaying(0, Cue::Item), "zero repeating identity accepted");
        Check(mixer.KeepPlaying(21, Cue::WallNorth), "wall loop did not start");
        mixer.Update(21, {21, -1, 0, 0}, 0.1f);
        Output loop;
        for (int i = 0; i < 32000 * 7 / 192; ++i) {
            Check(mixer.KeepPlaying(21, Cue::WallNorth), "wall loop update failed");
            mixer.Render(192, 1, &loop, Capture);
        }
        Check(mixer.ActiveVoices() == 1 && loop.calls > 1000, "wall loop stopped at end of recording");
        const auto nonzero = std::count_if(loop.pcm.begin(), loop.pcm.end(), [](float sample) { return std::abs(sample) > 0.0001f; });
        Check(nonzero > 32000, "wall updates restarted the recording every frame");
        Check(mixer.KeepPlaying(21, Cue::WallEast) && mixer.ActiveVoices() == 1, "changed wall direction leaked a voice");
        mixer.StopAll();
        Check(mixer.ActiveVoices() == 0, "wall loop survived scene cleanup");
        CheckLoops(mixer, argv[1]);
        CheckNavigationLoop(mixer);
        Check(mixer.KeepPlaying(8, Cue::Person), "person loop did not resume");
        mixer.Update(8, {8, -1, 0, 0}, 1);
        Output active;
        mixer.Render(192, 1, &active, Capture);
        Check(mixer.KeepPlaying(8, Cue::Door) && mixer.ActiveVoices() == 1, "cue change leaked a voice");
        mixer.Update(8, {8, -1, 0, 0}, 0);
        Check(mixer.ActiveVoices() == 0, "disabled repeated cue still active");
        std::cout << "Bundled WAV, block continuity, gain, disabling, capacity, actor/scene cleanup and completion checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
