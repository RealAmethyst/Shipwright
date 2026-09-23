#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace Navigation {
// Player_SetBootData's live gait registers, evaluated at normal full-stick speed.
struct WalkingStride {
    float speed, threshold, walkBase, walkScale, runBlend, runScale, updateScale;

    float Length() const {
        for (float value : {speed, threshold, walkBase, walkScale, runBlend, runScale, updateScale})
            if (!std::isfinite(value) || value < 0) return 0;
        if (speed <= 0 || updateScale <= 0) return 0;
        const float running = speed - threshold;
        // func_80841EE4 blends walking/running; func_8084029C advances a
        // 29-phase cycle with two footfalls, clamping each update to 7.25.
        const float phaseRate = running < 0 || runBlend * running < 1 ?
            walkBase + walkScale * speed : 1.2f + runScale * running;
        const float phaseAdvance = std::min(phaseRate * updateScale, 7.25f);
        if (!std::isfinite(phaseAdvance) || phaseAdvance <= 0) return 0;
        return speed * updateScale * 29 / (2 * phaseAdvance);
    }

    // Zero means less than one step; -1 means the data cannot be verified.
    int Estimate(float distance) const {
        const float stride = Length();
        if (!std::isfinite(distance) || distance < 0 || !std::isfinite(stride) || stride <= 0) return -1;
        const double steps = double(distance) / stride;
        if (steps >= std::numeric_limits<int>::max() - 0.5) return -1;
        return steps < 1 ? 0 : static_cast<int>(std::lround(steps));
    }
};
}
