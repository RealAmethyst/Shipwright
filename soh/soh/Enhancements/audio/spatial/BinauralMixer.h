#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace SpatialAudio {

struct Direction {
    float x = 0;
    float y = 0;
    float z = -1;
};

// Streams the game's variable synthesis slices through fixed-size HRTF blocks.
// The unprocessed stereo bed follows the same buffering delay as the sources.
class BinauralMixer {
  public:
    static constexpr size_t BlockFrames = 128;
    static constexpr size_t MaxSliceFrames = 256;
    static constexpr size_t MaxSources = 256;
    using StereoFrame = std::array<float, 2>;

    BinauralMixer();
    ~BinauralMixer();
    BinauralMixer(const BinauralMixer&) = delete;
    BinauralMixer& operator=(const BinauralMixer&) = delete;

    bool Initialize(int sampleRate, const uint8_t* sofa, size_t sofaBytes, std::string& error);
    void Reset();
    bool BeginSlice(size_t frames);
    bool AddSource(uint64_t identity, Direction direction, const float* mono, size_t frames);
    bool EndSlice(const StereoFrame* bed, StereoFrame* output, size_t frames);
    size_t ActiveSources() const;

  private:
    struct State;
    std::unique_ptr<State> state;
};

} // namespace SpatialAudio
