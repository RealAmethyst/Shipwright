#pragma once

#include "OptionsLayout.h"
#include <array>
#include <span>
#include <libultraship/libultra/gbi.h>

namespace NativeOptions {

using TextureLookup = std::function<const void*(uint16_t)>;

// Match the game's two graphics pools. BeginFrame is called only when that pool
// is reset, so interpolation and a paused graphics capture retain their lists.
class DisplayLists {
  public:
    void BeginFrame(uint32_t poolIndex);
    std::span<const Gfx> Build(const std::vector<DrawCommand>& commands, const TextureLookup& glyph,
                               const TextureLookup& panel);

  private:
    struct Frame {
        std::vector<std::vector<Gfx>> lists;
        size_t used = 0;
    };
    std::array<Frame, 2> frames;
    size_t current = 0;
};

} // namespace NativeOptions
