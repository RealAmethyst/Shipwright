#pragma once
#include "RouteSearch.h"
namespace Navigation {
// Native EnBox_WaitOpen and EnDoor_Idle position tests, in actor-local space.
inline bool ChestApproach(Point local) {
    return Finite(local) && local.z > -50 && local.z < 0 && std::abs(local.y) < 10 && std::abs(local.x) < 20;
}
inline bool DoorApproach(Point local) {
    return Finite(local) && std::abs(local.z) < 50 && std::abs(local.y) < 20 && std::abs(local.x) < 20;
}
// EnItem00_Update uses these bounds before giving a loose pickup to Link.
inline bool PickupApproach(Point delta) {
    return Finite(delta) && std::hypot(delta.x, delta.z) <= 30 && std::abs(delta.y) <= 50;
}
}
