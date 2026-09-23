#pragma once
#include <algorithm>
#include <cmath>

namespace Navigation {
// Live boot registers, consumed by Player's auto-jump and Actor's movement pass.
struct JumpMotion {
    float speed = 0, gravity = 0, scale = 0, acceleration = 0;
    float threshold = 0, fastVelocity = 0, baseVelocity = 0, speedVelocity = 0;
    float Velocity() const { return speed > threshold ? fastVelocity : baseVelocity + speedVelocity * speed; }
    bool Valid() const {
        return speed > 3 && speed <= 12 && gravity < 0 && gravity >= -4 && scale > 0 && scale <= 3 &&
               acceleration >= 0.01f && acceleration <= 10 && Velocity() > 0 && Velocity() <= 20;
    }
    float Runup() const {
        float distance = 0;
        for (float v = 0; v < speed; v = std::min(speed, v + acceleration)) distance += v * scale;
        return distance + speed * scale * 3;
    }
};
}
