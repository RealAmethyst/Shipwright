#pragma once

#include "SpatialAudio.h"
#include <array>
#include <string>
#include <vector>

namespace SpatialAudio {
enum class Cue : size_t {
    Item, Person, Door, Transition, Destructible, Crawlspace, Ladder, Elevator, Pathfinder,
    WallNorth, WallEast, WallSouth, WallWest, Count
};
constexpr std::array<const char*, static_cast<size_t>(Cue::Count)> CueNames{
    "item", "person", "door", "transition", "destructible", "crawlspace", "ladder", "elevator", "pathfinder",
    "wall_north", "wall_east", "wall_south", "wall_west"};

// Game updates and audio rendering are serialized by OTRAudio's frame handshake.
// Files are decoded at startup; rendering uses only this fixed voice pool.
class CueMixer {
  public:
    static constexpr size_t MaxVoices = 128, MaxFrames = 256;
    using RenderCallback = void (*)(void*, const SpatialAudioSource&, const float*, int);
    bool Load(Cue cue, const std::string& path, std::string& error);
    bool Play(uint64_t identity, Cue cue);
    bool KeepPlaying(uint64_t identity, Cue cue);
    // Selected guidance has one reserved voice in addition to the ordinary pool.
    bool KeepPlayingInterval(uint64_t identity, Cue cue, float seconds);
    bool KeepPlayingPulse(uint64_t identity, Cue cue, float seconds);
    void Update(uint64_t identity, SpatialAudioSource source, float gain);
    void Stop(uint64_t identity);
    void StopAll();
    void StopAllExcept(uint64_t identity);
    void Render(int frames, float gameGain, void* context, RenderCallback callback);
    size_t ActiveVoices() const;

  private:
    struct Voice {
        uint64_t identity = 0;
        Cue cue{};
        size_t cursor = 0;
        SpatialAudioSource source{};
        float gain = 0;
        bool repeat = false;
        size_t gap = 0;
        size_t interval = 0;
    };
    std::array<std::vector<float>, static_cast<size_t>(Cue::Count)> samples;
    std::array<Voice, MaxVoices + 1> voices{};
};
}
