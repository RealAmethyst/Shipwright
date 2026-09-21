#include "WallProbes.h"
#include "WorldCompass.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

#include "global.h"

static void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
namespace {
enum class Terrain { Flat, Drop, Ledge, Slope, Steep, Void };
Terrain terrain;
CollisionPoly floorPoly{}, wallPoly{};
float barrierDistance, barrierHeight;
int queries, wallFlags, floorType;
bool spiked, dynamicSurface;
float Floor(float x, float z) {
    switch (terrain) {
        case Terrain::Drop: return z > 40 ? -80 : 0;
        case Terrain::Ledge: return z > barrierDistance ? barrierHeight : 0;
        case Terrain::Slope: return std::max(z, 0.0f) * 0.3f;
        case Terrain::Steep: return std::max(z, 0.0f) * 0.8f;
        case Terrain::Void: return BGCHECK_Y_MIN;
        default: return 0;
    }
}
void Reset() {
    terrain = Terrain::Flat;
    barrierDistance = 100;
    barrierHeight = 1000;
    queries = wallFlags = floorType = 0;
    spiked = dynamicSurface = false;
    floorPoly = {};
    floorPoly.normal.y = 32767;
    wallPoly = {};
    wallPoly.normal.z = -32767;
    wallPoly.dist = 100;
}
}

// Controlled geometry at the verified native query boundary. These fixtures exercise
// the production scanner, including the full player structure and dynamic surface IDs.
extern "C" {
f32 Math_SinS(s16 angle) { return std::sin(angle * 3.14159265358979323846f / 32768); }
f32 Math_CosS(s16 angle) { return std::cos(angle * 3.14159265358979323846f / 32768); }
s16 Math_Atan2S(f32 x, f32 y) { return static_cast<s16>(std::atan2(y, x) * 32768 / 3.14159265358979323846f); }
void Player_GetSlopeDirection(CollisionPoly* poly, Vec3f* normal, s16* yaw) {
    *normal = {COLPOLY_GET_NORMAL(poly->normal.x), COLPOLY_GET_NORMAL(poly->normal.y), COLPOLY_GET_NORMAL(poly->normal.z)};
    *yaw = Math_Atan2S(normal->z, normal->x);
}
f32 Math3D_UDistPlaneToPos(f32 nx, f32 ny, f32 nz, f32 distance, Vec3f* pos) {
    return std::abs(nx * pos->x + ny * pos->y + nz * pos->z + distance) / std::sqrt(nx * nx + ny * ny + nz * nz);
}
s32 BgCheck_PosInStaticBoundingBox(CollisionContext*, Vec3f* pos) {
    return std::abs(pos->x) < 2000 && std::abs(pos->z) < 2000 && pos->y >= -1000 && pos->y <= 2000;
}
f32 BgCheck_EntityRaycastFloor3(CollisionContext*, CollisionPoly** poly, s32* id, Vec3f* pos) {
    Check(++queries < 10000, "probe exceeded bounded collision work");
    const float y = Floor(pos->x, pos->z);
    *id = BGCHECK_SCENE;
    *poly = y <= pos->y && y > BGCHECK_Y_MIN ? &floorPoly : nullptr;
    return *poly ? y : BGCHECK_Y_MIN;
}
f32 BgCheck_EntityRaycastFloor1(CollisionContext* context, CollisionPoly** poly, Vec3f* pos) {
    s32 id;
    return BgCheck_EntityRaycastFloor3(context, poly, &id, pos);
}
s32 BgCheck_EntitySphVsWall3(CollisionContext*, Vec3f* result, Vec3f* next, Vec3f* previous, f32 radius,
                            CollisionPoly** poly, s32* id, Actor*, f32 height) {
    Check(++queries < 10000, "probe exceeded bounded wall work");
    *result = *next;
    const bool hit = next->z + radius >= barrierDistance && previous->z <= barrierDistance + radius &&
                     next->y + height < barrierHeight;
    *poly = hit ? &wallPoly : nullptr;
    *id = dynamicSurface ? 7 : BGCHECK_SCENE;
    return hit;
}
s32 BgCheck_EntityCheckCeiling(CollisionContext*, f32*, Vec3f*, f32, CollisionPoly**, s32*, Actor*) { return false; }
s32 BgCheck_AnyCheckCeiling(CollisionContext*, f32*, Vec3f*, f32) { return false; }
s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f* from, Vec3f* to, Vec3f* result, CollisionPoly** poly,
                           s32 wall, s32 floor, s32 ceiling, s32 oneFace, s32* id) {
    Check(wall && !floor && !ceiling && oneFace, "ledge wall-line filter changed");
    const bool hit = from->y < barrierHeight && to->z >= barrierDistance && from->z <= barrierDistance;
    *result = *to;
    *poly = hit ? &wallPoly : nullptr;
    *id = dynamicSurface ? 7 : BGCHECK_SCENE;
    return hit;
}
u32 SurfaceType_GetSlope(CollisionContext*, CollisionPoly*, s32) { return 0; }
u32 func_80041D4C(CollisionContext*, CollisionPoly*, s32) { return floorType; }
s32 func_80041DB8(CollisionContext*, CollisionPoly* poly, s32 id) {
    Check(poly == &wallPoly && id == (dynamicSurface ? 7 : BGCHECK_SCENE), "wall used the wrong collision owner");
    return wallFlags;
}
s32 func_80041E18(CollisionContext*, CollisionPoly*, s32) { return wallFlags & 2; }
u32 SurfaceType_IsWallDamage(CollisionContext*, CollisionPoly* poly, s32 id) {
    Check(poly == &wallPoly && id == (dynamicSurface ? 7 : BGCHECK_SCENE), "spikes used the wrong collision owner");
    return spiked;
}
}

int main() {
    try {
        using namespace SpatialAudio;
        Check(MapDirection(0, -1, false) == 0 && MapDirection(1, 0, false) == 1 &&
              MapDirection(0, 1, false) == 2 && MapDirection(-1, 0, false) == 3, "cardinal axes disagreed with map");
        Check(MapDirection(1, -1, false, 8) == 1 && MapDirection(-1, -1, false, 8) == 7, "diagonal axes reversed");
        Check(MapDirection(1, 0, true) == 3 && MapDirection(0, -1, true) == 0, "mirrored map disagreed");
        Check(MapDirection(0, 0, false) == -1 && MapDirection(NAN, 1, false) == -1 &&
              MapDirection(0, INFINITY, false) == -1 && MapDirection(1, 1, false, 6) == -1, "invalid direction accepted");
        for (int angle = -720; angle <= 720; ++angle) {
            const float radians = angle * 3.14159265358979323846f / 180;
            for (int count : {4, 8}) {
                const int sector = MapDirection(std::sin(radians), -std::cos(radians), false, count);
                Check(sector >= 0 && sector < count, "bearing wrapped out of range");
            }
        }
        Check(HasMapCompass(SCENE_KAKARIKO_VILLAGE) && HasMapCompass(SCENE_FOREST_TEMPLE) &&
              !HasMapCompass(SCENE_LINKS_HOUSE) && !HasMapCompass(SCENE_DEKU_TREE_BOSS) && !HasMapCompass(-1),
              "map support used an unverified room");
        CompassHeading heading;
        Check(heading.Update(0, -1, false, 4) == -1, "first heading should be silent");
        for (int i = 0; i < 3; ++i) Check(heading.Update(1, 0, false, 4) == -1, "turn announced before settling");
        Check(heading.Update(1, 0, false, 4) == 1, "settled east heading missing");
        Check(heading.Update(1, 0, false, 4) == -1, "stationary heading repeated");
        for (int i = 0; i < 30; ++i)
            Check(heading.Update(1, i & 1 ? -1.01f : -0.99f, false, 4) == -1, "sector boundary flooded speech");
        heading.Reset();
        Check(heading.Update(-1, 0, false, 4) == -1, "resume replayed a stale heading");

        auto play = std::make_unique<PlayState>();
        Player player{};
        PlayerAgeProperties age{};
        CollisionHeader collision{};
        age.wallCheckRadius = 18;
        age.unk_0C = 100;
        player.ageProperties = &age;
        player.actor.floorPoly = &floorPoly;
        player.actor.floorBgId = BGCHECK_SCENE;
        player.actor.yDistToWater = BGCHECK_Y_MIN;
        play->actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        play->colCtx.colHeader = &collision;
        auto scan = [&] {
            Player before = player;
            auto hits = ScanWalls(play.get());
            Check(std::memcmp(&before, &player, sizeof(player)) == 0, "probe changed Link's state");
            return hits;
        };
        Reset();
        auto hits = scan();
        Check(hits[0].found && !hits[1].found && !hits[2].found, "three Link-relative probes misidentified the wall");
        Check(hits[0].position.z > 75 && hits[0].position.z < 105, "wall source not at the detected wall");
        Check(MapDirection(hits[0].position.x, hits[0].position.z, false) == 2, "south wall did not select south");
        player.actor.world.rot.y = 0x4000;
        hits = scan();
        Check(!hits[0].found && !hits[1].found && hits[2].found, "turning Link did not rotate probes");
        player.actor.world.rot.y = 0;
        dynamicSurface = true;
        Check(scan()[0].found, "dynamic wall ignored");
        spiked = true;
        Check(!scan()[0].found, "spikes mislabeled as ordinary wall");
        Reset();
        wallFlags = 8;
        Check(!scan()[0].found, "climbable wall mislabeled as blocking wall");
        Reset();
        terrain = Terrain::Ledge;
        barrierHeight = 60;
        Check(!scan()[0].found, "reachable ledge mislabeled as blocking wall");
        Reset();
        terrain = Terrain::Drop;
        Check(!scan()[0].found, "wall reported past an intervening drop");
        Reset();
        terrain = Terrain::Slope;
        Check(!scan()[0].found, "traversable incline mislabeled as blocking wall");
        Reset();
        terrain = Terrain::Steep;
        Check(scan()[0].found, "steep blocking incline missing");
        Reset();
        floorType = 2;
        Check(!scan()[0].found, "unassigned lava output replaced by wall");
        Reset();
        terrain = Terrain::Void;
        Check(!scan()[0].found, "missing floor accepted");
        Reset();
        player.stateFlags1 = PLAYER_STATE1_IN_WATER;
        player.actor.yDistToWater = 20;
        Check(scan()[0].found, "wall continuing above water missing");
        player.stateFlags1 = PLAYER_STATE1_FIRST_PERSON;
        Check(!scan()[0].found && !scan()[1].found, "first-person wall cues not suppressed");
        player.stateFlags1 = PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_PARALLEL;
        player.actor.world.rot.y = 0x2000;
        player.actor.shape.rot.y = 0;
        wallFlags = 8;
        barrierDistance = 10;
        scan();
        player.stateFlags1 = 0;
        player.actor.yDistToWater = BGCHECK_Y_MIN;
        Reset();
        barrierDistance = 10000;
        for (int angle = 0; angle < 65536; angle += 1024) {
            queries = 0;
            player.actor.world.rot.y = static_cast<s16>(angle);
            hits = scan();
            Check(std::none_of(hits.begin(), hits.end(), [](auto hit) { return hit.found; }), "out-of-range wall found");
            Check(queries < 1200, "diagonal probe did excessive work");
        }
        player.actor.world.pos.x = std::numeric_limits<float>::quiet_NaN();
        Check(!scan()[0].found, "nonfinite player position accepted");
        std::cout << "Map bearings, compass settling, probe rotation/range, terrain classification, dynamic owners and read-only player checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
