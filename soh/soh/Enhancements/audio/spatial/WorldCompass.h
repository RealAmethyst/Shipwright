#pragma once

#include <cmath>
#include <cstdint>

namespace SpatialAudio {
// The native map places -Z upward and +X rightward. Mirroring reflects map X.
inline int MapDirection(float dx, float dz, bool mirrored, int directions = 4) {
    if ((directions != 4 && directions != 8) || !std::isfinite(dx) || !std::isfinite(dz) ||
        std::hypot(dx, dz) < 0.001f) return -1;
    if (!std::isfinite(std::hypot(dx, dz))) return -1;
    const float angle = std::atan2(mirrored ? -dx : dx, -dz);
    const int sector = static_cast<int>(std::floor(angle * directions / (2.0f * 3.14159265358979323846f) + 0.5f));
    return (sector + directions) % directions;
}

bool HasMapCompass(int16_t scene);
struct CompassHeading {
    int previous = -1, candidate = -1, stable = 0;
    float lastX = 0, lastZ = 0;

    void Reset() { *this = {}; }
    // Coalesce a camera turn, including crossings back and forth at a boundary.
    int Update(float dx, float dz, bool mirrored, int directions) {
        const int next = MapDirection(dx, dz, mirrored, directions);
        if (next < 0) { Reset(); return -1; }
        const float length = std::hypot(dx, dz);
        dx /= length;
        dz /= length;
        if (previous < 0) {
            previous = candidate = next;
            lastX = dx;
            lastZ = dz;
            return -1;
        }
        const bool turning = dx * lastX + dz * lastZ < 0.99985f; // One degree per sample.
        lastX = dx;
        lastZ = dz;
        if (next != candidate || turning) { candidate = next; stable = 0; }
        if (next == previous) { stable = 0; return -1; }
        if (turning || ++stable < 3) return -1;
        previous = next;
        stable = 0;
        return next;
    }
};
}
