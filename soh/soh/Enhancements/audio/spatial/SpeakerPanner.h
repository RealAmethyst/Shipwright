#pragma once

#include "BinauralMixer.h"
#include <array>
#include <vector>

namespace SpatialAudio {

// FL, FR, C, LFE, SL, SR, BL, BR, TFL, TFR, TBL, TBR.
// Music remains in FL/FR. The speaker system owns bass management.
class SpeakerPanner {
  public:
    static constexpr size_t Channels = 12;
    using Gains = std::array<float, Channels>;
    SpeakerPanner();
    bool Compute(Direction direction, Gains& gains) const;

  private:
    struct Triangle {
        std::array<size_t, 3> indices;
        std::array<Direction, 3> inverse;
        float locality;
    };
    std::vector<Triangle> triangles;
};

} // namespace SpatialAudio
