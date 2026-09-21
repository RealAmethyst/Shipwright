#include "WallProbes.h"
#include <algorithm>
#include <cmath>

#include "global.h"
extern "C" void Player_GetSlopeDirection(CollisionPoly*, Vec3f*, s16*);

namespace SpatialAudio {
namespace {
constexpr float Range = 500.0f, ProbeSpeed = 5.5f;
constexpr int MaxSteps = 512;
bool Finite(const Vec3f& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
float Distance(const Vec3f& a, const Vec3f& b) { return std::hypot(a.x - b.x, a.z - b.z); }

// PR 5435's three floor-following probes, with only the approved wall output.
// All positions, rotations and climbing trials belong to this temporary probe.
class Probe {
    PlayState* play;
    const Player* player;
    Vec3f pos, previous, velocity{}, expected{};
    s16 yaw;
    float speed = ProbeSpeed, radius, height;
    CollisionPoly* floor = nullptr;
    CollisionPoly* wall = nullptr;
    s32 floorId = BGCHECK_SCENE, wallId = BGCHECK_SCENE;
    int steps = 0;

    bool Bounded(const Vec3f& point) const {
        return Finite(point) && Distance(point, player->actor.world.pos) <= Range &&
               std::abs(point.y - player->actor.world.pos.y) <= Range;
    }
    void SetVelocity() {
        velocity = {Math_SinS(yaw) * speed, 25.0f, Math_CosS(yaw) * speed};
        expected = velocity;
        if (!floor || SurfaceType_GetSlope(&play->colCtx, floor, floorId) != 1) return;
        Vec3f normal;
        s16 slopeYaw;
        Player_GetSlopeDirection(floor, &normal, &slopeYaw);
        const auto difference = static_cast<s16>(slopeYaw - yaw);
        if (std::abs(static_cast<int>(difference)) > 16000) {
            const float push = (1.0f - normal.y) * 40.0f;
            velocity.x += push * Math_SinS(slopeYaw);
            velocity.z += push * Math_CosS(slopeYaw);
        }
    }
    bool PushedAway() const { return Distance(velocity, expected) >= speed; }

    // 0: horizontal, 1: follow ground, 2: ascend while climbing.
    bool Move(int mode = 1) {
        if (++steps > MaxSteps) return false;
        if (mode == 2) pos.y += speed;
        else {
            if (!Finite(velocity) || std::hypot(velocity.x, velocity.z) < 0.001f) return false;
            pos.x += velocity.x;
            pos.z += velocity.z;
            if (mode == 1) {
                pos.y += velocity.y;
                pos.y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorId, &pos);
                if (!floor || pos.y <= BGCHECK_Y_MIN) return false;
            }
        }
        return Bounded(pos) && BgCheck_PosInStaticBoundingBox(&play->colCtx, &pos);
    }
    bool CheckWall(Vec3f next, Vec3f before) {
        Vec3f corrected;
        wall = nullptr;
        wallId = BGCHECK_SCENE;
        BgCheck_EntitySphVsWall3(&play->colCtx, &corrected, &next, &before, radius,
                               &wall, &wallId, nullptr, height);
        return wall != nullptr;
    }
    bool ClimbableWall() const {
        const int flags = func_80041DB8(&play->colCtx, wall, wallId);
        return (flags & 8) || (flags & 2);
    }
    bool Lava(Vec3f point) const {
        CollisionPoly* poly = nullptr;
        s32 id = BGCHECK_SCENE;
        point.y += 20;
        const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &id, &point);
        if (!poly || !std::isfinite(y) || y <= BGCHECK_Y_MIN) return false;
        const auto type = func_80041D4C(&play->colCtx, poly, id);
        return type == 2 || type == 3;
    }
    WallHit Hit() const { return {Bounded(pos), pos}; }

    float WallHeight() const {
        if (!wall || std::abs(static_cast<int>(wall->normal.y)) >= 600) return 399.96002f;
        const float nx = COLPOLY_GET_NORMAL(wall->normal.x);
        const float ny = COLPOLY_GET_NORMAL(wall->normal.y);
        const float nz = COLPOLY_GET_NORMAL(wall->normal.z);
        Vec3f base = pos;
        const float distance = Math3D_UDistPlaneToPos(nx, ny, nz, wall->dist, &base) + 10;
        Vec3f ray{pos.x - distance * nx, pos.y + player->ageProperties->unk_0C, pos.z - distance * nz};
        CollisionPoly* poly = nullptr;
        const float top = BgCheck_EntityRaycastFloor1(&play->colCtx, &poly, &ray);
        const float rise = top - pos.y;
        float ceiling;
        s32 id = BGCHECK_SCENE;
        if (!poly || !std::isfinite(rise) || rise < 18 ||
            BgCheck_EntityCheckCeiling(&play->colCtx, &ceiling, &base, rise + 20, &poly, &id, nullptr))
            return 399.96002f;

        // Player_PosVsWallLineTest's actual point construction, facing this probe.
        const float reach = player->ageProperties->wallCheckRadius + 10;
        Vec3f from{pos.x, top + 5, pos.z};
        Vec3f to{from.x + reach * Math_SinS(yaw), from.y, from.z + reach * Math_CosS(yaw)};
        Vec3f intersection;
        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &intersection, &poly, true, false, false, true, &id) && poly) {
            const auto difference = static_cast<s16>(Math_Atan2S(wall->normal.z, wall->normal.x) -
                                                     Math_Atan2S(poly->normal.z, poly->normal.x));
            if (std::abs(static_cast<int>(difference)) < 0x4000 && !func_80041E18(&play->colCtx, poly, id))
                return 399.96002f;
        }
        return rise;
    }
    bool ClimbStep() {
        SetVelocity();
        return Move() && !PushedAway() && !CheckWall(pos, previous);
    }
    bool CanClimb(float rise) const {
        Probe test = *this;
        test.pos.y += rise;
        test.speed = 1;
        bool supported = false;
        for (int i = 0; i < 100; ++i) {
            test.SetVelocity();
            if (!test.Move()) return false;
            if (test.pos.y >= pos.y + rise - 10) { supported = true; break; }
            test.pos.y = pos.y + rise;
        }
        if (!supported) return false;
        test.speed = ProbeSpeed;
        test.previous = test.pos;
        test.yaw = static_cast<s16>(yaw + 0x4000);
        const bool left = test.ClimbStep();
        const float leftY = test.pos.y;
        test.pos = test.previous;
        test.yaw = static_cast<s16>(yaw - 0x4000);
        const bool right = test.ClimbStep();
        const float difference = std::abs(leftY - test.pos.y);
        return left && right && (difference < 10 || difference > rise - 5);
    }
    bool PerpendicularWall(s16 originalYaw) {
        yaw = static_cast<s16>(player->actor.shape.rot.y + 0x8000);
        SetVelocity();
        for (int i = 0; i < 4; ++i) if (!Move(0)) return false;
        yaw = static_cast<s16>(originalYaw + 0x8000);
        SetVelocity();
        for (int i = 0; i < 3; ++i) if (!Move(0)) return false;
        previous = pos;
        yaw = originalYaw;
        SetVelocity();
        for (int i = 0; i < 4; ++i) if (!Move(0)) return false;
        return CheckWall(pos, previous) && Distance(pos, player->actor.world.pos) <= 200;
    }
    bool VinePlatform(float baseHeight) {
        const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorId, &pos);
        return floor && std::isfinite(y) && y - baseHeight > 100;
    }

    WallHit Swimming() {
        const Vec3f& origin = player->actor.world.pos;
        const float depth = player->actor.yDistToWater;
        if (!std::isfinite(depth)) return {};
        for (int i = 0; i < MaxSteps; ++i) {
            previous = pos;
            SetVelocity();
            pos.y = player->actor.prevPos.y;
            if (!Move() || pos.y > origin.y) return {}; // Leaving the water: incline cue.
            pos.y = std::max(pos.y, origin.y);
            if (!CheckWall(pos, previous)) continue;
            pos.y += depth;
            previous.y += depth;
            if (CheckWall(pos, previous)) return Hit();

            pos.y = origin.y - 10;
            for (int step = 0; pos.y < origin.y + depth; ++step) {
                if (step >= MaxSteps) return {};
                pos.y = origin.y + 40;
                if (!Move()) return {};
            }
            const float rise = std::abs(pos.y - (origin.y + depth - (player->ageProperties->unk_92 == 0 ? 45.5f : 30.0f)));
            const Vec3f top = pos;
            previous = pos;
            pos.y += 20;
            if (!Move()) return {};
            const bool forward = std::abs(pos.y - top.y) < 1;
            const s16 originalYaw = yaw;
            yaw = static_cast<s16>(originalYaw + 0x4000);
            const bool left = ClimbStep();
            const float leftY = pos.y;
            yaw = static_cast<s16>(originalYaw - 0x4000);
            pos = previous;
            const bool right = ClimbStep();
            const float difference = std::abs(leftY - pos.y);
            pos = top;
            if (left && right && (forward || rise < 44) && rise < 48 && (difference < 2 || difference > rise - 5))
                return {}; // Reachable shore ledge has its own unassigned cue.
            return Hit();
        }
        return {};
    }

    WallHit Climbing(bool forward) {
        Vec3f origin = player->actor.world.pos;
        const float base = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorId, &origin);
        if (!floor || !std::isfinite(base) || base <= BGCHECK_Y_MIN) return {};
        for (int i = 0; i < MaxSteps; ++i) {
            previous = pos;
            const s16 originalYaw = yaw;
            SetVelocity();
            if (!Move(forward ? 2 : 0)) return {};
            if (CheckWall(pos, previous)) {
                if (forward && !ClimbableWall()) return {}; // End of the climbable surface.
                if (!forward && VinePlatform(base)) return {};
                continue;
            }
            if (!forward) {
                const Vec3f side = pos;
                previous = pos;
                yaw = player->actor.shape.rot.y;
                SetVelocity();
                for (int trial = 0; trial < 4 && !wall; ++trial) {
                    if (!Move(0)) return {};
                    CheckWall(pos, previous);
                }
                if (wall) {
                    if (!ClimbableWall()) return PerpendicularWall(originalYaw) ? Hit() : WallHit{};
                    if (VinePlatform(base)) return {};
                    yaw = originalYaw;
                    continue;
                }
                const Vec3f front = pos;
                pos = side;
                yaw = static_cast<s16>(player->actor.shape.rot.y + 0x8000);
                SetVelocity();
                for (int trial = 0; trial < 4 && !wall; ++trial) {
                    if (!Move(0)) return {};
                    CheckWall(pos, previous);
                }
                if (wall) {
                    if (!ClimbableWall() || VinePlatform(base)) return {};
                    yaw = originalYaw;
                    continue;
                }
                const Vec3f back = pos;
                pos = side;
                yaw = originalYaw;
                if (CheckWall(back, front)) {
                    if (!ClimbableWall() || VinePlatform(base)) return {};
                    continue;
                }
                return PerpendicularWall(originalYaw) ? Hit() : WallHit{};
            }
            if (!Move(2)) return {};
            const float rise = std::abs(origin.y - pos.y);
            float ceiling;
            if (rise < 100 && BgCheck_AnyCheckCeiling(&play->colCtx, &ceiling, &origin, rise + 30)) {
                pos.y = ceiling;
                return Hit();
            }
            return {};
        }
        return {};
    }

  public:
    Probe(PlayState* play, s16 relativeYaw) : play(play), player(GET_PLAYER(play)), pos(player->actor.world.pos),
        previous(pos), yaw(static_cast<s16>(((player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) ?
             player->actor.shape.rot.y : player->actor.world.rot.y) + relativeYaw)) {
        const bool crawling = (player->stateFlags2 & PLAYER_STATE2_CRAWLING) != 0;
        radius = crawling ? 10.0f : player->ageProperties->wallCheckRadius;
        height = crawling ? 15.0f : 26.0f;
        floor = player->actor.floorPoly;
        floorId = player->actor.floorBgId;
    }

    WallHit Scan(bool forward) {
        if (!Bounded(pos) || !std::isfinite(radius) || radius <= 0) return {};
        if (player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) return Climbing(forward);
        if ((player->stateFlags3 & PLAYER_STATE3_MIDAIR) || (player->stateFlags2 & PLAYER_STATE2_HOPPING)) {
            pos.y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorId, &pos);
            if (!floor || !Bounded(pos) || pos.y <= BGCHECK_Y_MIN) return {};
        }
        if (player->stateFlags1 & PLAYER_STATE1_IN_WATER) return Swimming();
        if (Lava(player->actor.world.pos)) return {}; // The PR reports leaving lava separately.
        for (int i = 0; i < MaxSteps; ++i) {
            previous = pos;
            SetVelocity();
            if (!Move()) return {};
            if (PushedAway()) return Hit();
            const float rise = pos.y - previous.y;
            if (rise <= -20) return {}; // Ledge, not a wall.
            if (player->actor.yDistToWater < 0 && pos.y - player->actor.prevPos.y < player->actor.yDistToWater)
                return {}; // Water has its own unassigned cue.
            if (rise > 1.2f && rise < 20) {
                const Vec3f bottom = pos;
                if (!Move()) return {};
                if (std::abs(pos.y - bottom.y) > 3.5f) return Hit();
                return {}; // Traversable incline.
            }
            if ((rise < -1.2f && rise > -20) || Lava(pos)) return {};
            if (!CheckWall(pos, previous)) continue;
            if (SurfaceType_IsWallDamage(&play->colCtx, wall, wallId) || ClimbableWall()) return {};
            const float riseToTop = WallHeight();
            if (riseToTop <= player->ageProperties->unk_0C && CanClimb(riseToTop)) return {};
            return Hit(); // Includes tall walls skipped by the PR's unreachable final branch.
        }
        return {};
    }
};
}

std::array<WallHit, 3> ScanWalls(PlayState* play) {
    std::array<WallHit, 3> hits{};
    if (!play || !play->colCtx.colHeader) return hits;
    const auto* player = GET_PLAYER(play);
    if (!player || !player->ageProperties || play->csCtx.state != CS_STATE_IDLE ||
        (player->stateFlags1 & (PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_FIRST_PERSON))) return hits;
    const std::array<s16, 3> offsets{0, 0x4000, -0x4000};
    for (size_t i = 0; i < hits.size(); ++i) hits[i] = Probe(play, offsets[i]).Scan(i == 0);
    return hits;
}
}
