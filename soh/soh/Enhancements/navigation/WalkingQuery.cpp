#include "WalkingQuery.h"
#include "Navigation.h"
#include <algorithm>
#include "global.h"

namespace Navigation {
namespace {
struct Obstacle { Point position; float radius, height; bool sphere; };
std::vector<Obstacle> obstacles;
}
void ClearObstacles() { obstacles.clear(); }
WalkingQuery::WalkingQuery(PlayState* context, JumpMotion jump, QueryStats* stats, bool grabs) :
    play(context), motion(jump), diagnostics(stats), ledgeGrabs(grabs) {
    if (play && GET_PLAYER(play) && GET_PLAYER(play)->ageProperties) {
        radius = GET_PLAYER(play)->ageProperties->wallCheckRadius;
        height = GET_PLAYER(play)->ageProperties->ceilingCheckHeight;
        const auto& cylinder = GET_PLAYER(play)->cylinder.dim;
        objectRadius = cylinder.radius > 0 ? cylinder.radius : radius;
        objectHeight = cylinder.height > 0 ? cylinder.height : height;
        objectShift = cylinder.height > 0 ? cylinder.yShift : 0;
    }
}
bool WalkingQuery::Reject(QueryFailure reason) const {
    if (diagnostics) ++diagnostics->rejected[static_cast<size_t>(reason)];
    return false;
}
bool WalkingQuery::Floor(Point& point, float previousY) const {
    Vec3f ray{point.x, previousY + 18, point.z};
    CollisionPoly* floor = nullptr;
    s32 owner = BGCHECK_SCENE;
    const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &owner, &ray);
    if (!floor || !std::isfinite(y) || y <= BGCHECK_Y_MIN || y - previousY > 18 || previousY - y > 11 ||
        COLPOLY_GET_NORMAL(floor->normal.y) < 0.5f) return Reject(QueryFailure::Floor);
    const auto type = func_80041D4C(&play->colCtx, floor, owner);
    const auto property = func_80041EA4(&play->colCtx, floor, owner);
    // Native damage/void consumers: lava, damaging floor, respawn and void-out.
    if (type == 2 || type == 3 || type == 9 || property == 5 || property == 12 ||
        SurfaceType_IsWallDamage(&play->colCtx, floor, owner) ||
        SurfaceType_GetSlope(&play->colCtx, floor, owner) == 1) return Reject(QueryFailure::Hazard);
    float waterY = y;
    WaterBox* water = nullptr;
    if (WaterBox_GetSurface1(play, &play->colCtx, point.x, point.z, &waterY, &water) && waterY - y > 20) return Reject(QueryFailure::Water);
    point.y = y;
    return true;
}
bool WalkingQuery::Walk(Point from, Point requested, Point& result) const {
    if (diagnostics) ++diagnostics->walks;
    if (!play || radius <= 0 || height <= 0 || !Finite(from) || !Finite(requested)) return false;
    const float length = DistanceXZ(from, requested);
    if (length > MaxWalkDistance || !std::isfinite(length)) return false;
    const int steps = std::max(1, static_cast<int>(std::ceil(length / 8)));
    Point previous = from;
    for (int i = 1; i <= steps; ++i) {
        const float t = i / static_cast<float>(steps);
        Point next{from.x + (requested.x - from.x) * t, previous.y, from.z + (requested.z - from.z) * t};
        if (!Floor(next, previous.y)) return false;
        if (!ClearBody(previous, next)) return false;
        previous = next;
    }
    result = previous;
    return true;
}
bool WalkingQuery::Support(Point from, Point& result) const {
    result = from;
    // Link already occupies the origin. Contact there must not forbid a
    // checked walking edge that takes him away from the wall or object.
    return play && radius > 0 && height > 0 && Finite(from) && Floor(result, from.y);
}
bool WalkingQuery::ClearBody(Point previous, Point next, float wallRadius, Point* floorCorrection) const {
    if (diagnostics) ++diagnostics->bodies;
    Vec3f p{previous.x, previous.y, previous.z}, q{next.x, next.y, next.z}, corrected;
    const float sweptHeight = height + next.y - previous.y - 10;
    CollisionPoly* poly = nullptr;
    s32 owner = BGCHECK_SCENE;
    if (!BgCheck_PosInStaticBoundingBox(&play->colCtx, &q)) return Reject(QueryFailure::Bounds);
    BgCheck_EntitySphVsWall3(&play->colCtx, &corrected, &q, &p, wallRadius > 0 ? wallRadius : radius,
                           &poly, &owner, nullptr, 26);
    // On a fast descent the native "wall" query also sweeps floors. It places
    // the actor just below contact so the following floor query can land it.
    // Accept that exact floor correction in airborne simulation only; walls
    // and rising contacts still reject the arc.
    if (floorCorrection && poly && COLPOLY_GET_NORMAL(poly->normal.y) > 0.5f && next.y < previous.y) {
        next = {corrected.x, corrected.y, corrected.z};
    } else if ((poly && (floorCorrection || COLPOLY_GET_NORMAL(poly->normal.y) > 0.5f)) ||
               !std::isfinite(corrected.x) || !std::isfinite(corrected.z) ||
               std::hypot(corrected.x - q.x, corrected.z - q.z) > 0.25f) {
        if (diagnostics) {
            diagnostics->wallFrom = previous; diagnostics->wallTo = next;
            diagnostics->wallNormal = poly ? Point{COLPOLY_GET_NORMAL(poly->normal.x), COLPOLY_GET_NORMAL(poly->normal.y),
                                                  COLPOLY_GET_NORMAL(poly->normal.z)} : Point{};
        }
        return Reject(QueryFailure::Wall);
    }
    for (const auto& obstacle : obstacles) {
        const float bottom = next.y + objectShift;
        if (bottom + objectHeight <= obstacle.position.y || bottom >= obstacle.position.y + obstacle.height) continue;
        float width = obstacle.radius;
        if (obstacle.sphere) {
            const float centerY = obstacle.position.y + width;
            const float dy = centerY - std::clamp(centerY, bottom, bottom + objectHeight);
            width = std::sqrt(std::max(0.0f, width * width - dy * dy));
        }
        if (DistanceXZ(next, obstacle.position) < objectRadius + width) return Reject(QueryFailure::Obstacle);
    }
    float ceiling;
    // Actor_UpdateBgCheckInfo checks the swept head height from previous Y + 10.
    q = {next.x, previous.y + 10, next.z};
    if (BgCheck_EntityCheckCeiling(&play->colCtx, &ceiling, &q, sweptHeight,
                                 &poly, &owner, nullptr)) return Reject(QueryFailure::Ceiling);
    if (floorCorrection) *floorCorrection = next;
    return true;
}
bool WalkingQuery::Arc(Point takeoff, Point direction, float phase, Point& landing) const {
    const float advance = motion.speed * motion.scale;
    // Grounded Actor_UpdateBgCheckInfo sets vertical speed to -4. Gravity is
    // applied before the edge-crossing move, then Player starts the auto-jump.
    Point position{takeoff.x + direction.x * advance * phase,
                   takeoff.y + (-4 + motion.gravity) * motion.scale,
                   takeoff.z + direction.z * advance * phase};
    if (!ClearBody(takeoff, position, 0, &position)) return false;
    Vec3f ray{position.x, takeoff.y + 50, position.z};
    CollisionPoly* poly = nullptr;
    s32 owner = BGCHECK_SCENE;
    float ground = BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &owner, &ray);
    if (poly && position.y - ground <= 20) {
        if (diagnostics) { diagnostics->departure = position; diagnostics->departureFloor = {takeoff.y, ground, COLPOLY_GET_NORMAL(poly->normal.y)}; }
        return Reject(QueryFailure::DepartureFloor);
    }
    float velocity = motion.Velocity();
    for (int frame = 0; frame < 80; ++frame) {
        velocity = std::max(-20.0f, velocity + motion.gravity);
        Point next{position.x + direction.x * advance, position.y + velocity * motion.scale,
                   position.z + direction.z * advance};
        // The floor consumer rays from previous Y + 50, after wall/ceiling checks.
        if (!ClearBody(position, next, 0, &next)) return false;
        ray = {next.x, position.y + 50, next.z};
        poly = nullptr; owner = BGCHECK_SCENE;
        ground = BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &owner, &ray);
        if (poly && ground >= next.y) {
            // A rising arc hitting a raised floor is a collision, not a landing.
            if (velocity > 0 || ground > position.y || takeoff.y - ground >= 400) return Reject(QueryFailure::RisingFloor);
            next.y = ground;
            Point supported = next;
            if (!Floor(supported, ground) || !ClearBody(next, next)) return Reject(QueryFailure::LandingSupport);
            // Reserve a full stride beyond contact; don't aim at a platform lip.
            Point ahead{next.x + direction.x * radius, next.y, next.z + direction.z * radius};
            if (!Walk(next, ahead, supported)) return Reject(QueryFailure::LandingSupport);
            landing = supported;
            return true;
        }
        float waterY = next.y;
        WaterBox* water = nullptr;
        if (WaterBox_GetSurface1(play, &play->colCtx, next.x, next.z, &waterY, &water) && waterY >= next.y)
            return Reject(QueryFailure::Water);
        if (takeoff.y - next.y >= 400) return Reject(QueryFailure::Fall);
        position = next;
    }
    return false;
}
bool WalkingQuery::Jump(Point from, Point toward, JumpLink& result) const {
    if (!play || radius <= 0 || height <= 0 || !motion.Valid() || !Finite(from) || !Finite(toward)) return Reject(QueryFailure::JumpState);
    const float length = DistanceXZ(from, toward);
    if (length < 1 || length > 40) return false;
    const Point direction{(toward.x - from.x) / length, 0, (toward.z - from.z) / length};
    Point supported;
    if (!Walk(from, from, supported)) return false;
    // Locate the first edge. A blocked wall or a shallow step cannot trigger
    // func_8083AA10's automatic jump and must not become a jump connection.
    Point takeoff = supported;
    Point unsupported{};
    bool edge = false;
    for (float distance = 2; distance <= length + 2; distance += 2) {
        Point next{from.x + direction.x * distance, takeoff.y, from.z + direction.z * distance};
        if (!Walk(takeoff, next, supported)) { unsupported = next; edge = true; break; }
        takeoff = supported;
    }
    if (!edge) return Reject(QueryFailure::Edge);
    // func_8083AA10 needs more than 20 units of air below Link. A blocked
    // body on continuing floor cannot trigger its automatic jump. Reject that
    // case before refining an edge and simulating both departure phases.
    Vec3f edgeRay{unsupported.x, takeoff.y + 18, unsupported.z};
    CollisionPoly* edgeFloor = nullptr;
    s32 edgeOwner = BGCHECK_SCENE;
    const float edgeY = BgCheck_EntityRaycastFloor3(&play->colCtx, &edgeFloor, &edgeOwner, &edgeRay);
    if (edgeFloor && takeoff.y - edgeY <= 20) return Reject(QueryFailure::Edge);
    // Refine the last supported foot position to less than 0.008 units. Native
    // departure can occur anywhere within a movement update, including almost
    // immediately beyond the edge; a half-stride launch would overstate reach.
    for (int i = 0; i < 8; ++i) {
        Point middle{(takeoff.x + unsupported.x) * 0.5f, takeoff.y, (takeoff.z + unsupported.z) * 0.5f};
        if (Walk(takeoff, middle, supported)) takeoff = supported;
        else unsupported = middle;
    }
    Vec3f ray{takeoff.x, takeoff.y + 18, takeoff.z};
    CollisionPoly* poly = nullptr;
    s32 owner = BGCHECK_SCENE;
    BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &owner, &ray);
    if (!poly) return false;
    const auto property = func_80041EA4(&play->colCtx, poly, owner);
    const auto type = func_80041D4C(&play->colCtx, poly, owner);
    if (property == 6 || property == 7 || property == 8 || property == 9 || property == 11 || type == 5 ||
        SurfaceType_GetConveyorSpeed(&play->colCtx, poly, owner)) return Reject(QueryFailure::JumpFloor);
    Point runup{takeoff.x - direction.x * motion.Runup(), takeoff.y,
                takeoff.z - direction.z * motion.Runup()};
    if (!Walk(takeoff, runup, supported) || std::abs(supported.y - takeoff.y) > 2) return Reject(QueryFailure::Runway);
    runup = supported;
    if (!Segment(from, runup) || !Segment(runup, takeoff)) return Reject(QueryFailure::Runway);
    Point early, late, landing;
    // Cover native update phase at the edge, not just one ideal launch point.
    if (!Arc(takeoff, direction, 0.01f, early)) return Reject(QueryFailure::EarlyArc);
    if (!Arc(takeoff, direction, 1.01f, late)) return Reject(QueryFailure::LateArc);
    if (!Segment(early, late)) return Reject(QueryFailure::Landing);
    landing = late;
    result = {runup, takeoff, landing, Traversal::Jump, from, toward};
    return true;
}
bool WalkingQuery::Segment(Point from, Point to) const {
    Point actual;
    return Walk(from, to, actual) && std::abs(actual.y - to.y) <= 4;
}
bool WalkingQuery::Traverse(Point from, Point toward, JumpLink& result) const {
    if (diagnostics) ++diagnostics->traversals;
    return Jump(from, toward, result) || Climb(from, toward, result) || Ladder(from, toward, result);
}
bool WalkingQuery::Revalidate(const JumpLink& link, JumpLink& result) const {
    // Preserve the original probe. The native ledge query projects its approach
    // along the wall normal; on sloping ground that changes both position and
    // height. Reconstructing a probe from the landing can hit a different face.
    const bool valid = IsLadder(link.kind) ? Ladder(link.probeFrom, link.probeToward, result) :
        link.kind == Traversal::Ledge ? Climb(link.probeFrom, link.probeToward, result) :
                                      Jump(link.probeFrom, link.probeToward, result);
    return valid && result.kind == link.kind && Segment(link.runup, result.runup) &&
           Segment(result.landing, link.landing);
}
bool WalkingQuery::Ladder(Point from, Point toward, JumpLink& result) const {
    if (!play || !motion.Valid() || !GET_PLAYER(play) || !GET_PLAYER(play)->ageProperties ||
        !Finite(from) || !Finite(toward)) return false;
    const auto& age = *GET_PLAYER(play)->ageProperties;
    const float length = DistanceXZ(from, toward);
    if (length < 1 || length > 40 || age.unk_3C <= 6 || age.unk_0C <= 0) return false;
    const Point direction{(toward.x - from.x) / length, 0, (toward.z - from.z) / length};
    Vec3f a{from.x, from.y + 18, from.z};
    Vec3f b{a.x + direction.x * (length + radius + 10), a.y, a.z + direction.z * (length + radius + 10)}, hit;
    CollisionPoly* wall = nullptr;
    s32 owner = BGCHECK_SCENE;
    if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &wall, true, false, false, true, &owner) ||
        !wall || owner != BGCHECK_SCENE || std::abs(wall->normal.y) >= 600) return false;
    const auto flags = func_80041DB8(&play->colCtx, wall, owner);
    if (!(flags & (2 | 4 | 8))) return false;
    const bool vines = flags & 8;
    const Point normal{COLPOLY_GET_NORMAL(wall->normal.x), 0, COLPOLY_GET_NORMAL(wall->normal.z)};
    if (-(direction.x * normal.x + direction.z * normal.z) < 0.382684f) return false;
    const auto bounds = [&](CollisionPoly* poly, Point& low, Point& high) {
        low = {INFINITY, INFINITY, INFINITY}; high = {-INFINITY, -INFINITY, -INFINITY};
        const auto* header = play->colCtx.colHeader;
        for (int i : {COLPOLY_VTX_INDEX(poly->flags_vIA), COLPOLY_VTX_INDEX(poly->flags_vIB), int(poly->vIC)}) {
            if (!header || i >= header->numVertices) return false;
            const auto& v = header->vtxList[i];
            low.x = std::min(low.x, float(v.x)); low.y = std::min(low.y, float(v.y)); low.z = std::min(low.z, float(v.z));
            high.x = std::max(high.x, float(v.x)); high.y = std::max(high.y, float(v.y)); high.z = std::max(high.z, float(v.z));
        }
        return true;
    };
    Point low, high;
    if (!bounds(wall, low, high)) return false;
    // Ladders snap to their polygon centre. Climbable walls retain the player's
    // lateral position (func_8083EC18's wallFlags & 8 branch).
    const Point center{vines ? hit.x : (low.x + high.x) * 0.5f, from.y,
                       vines ? hit.z : (low.z + high.z) * 0.5f};
    Point approach{center.x + normal.x * (radius + 1), from.y, center.z + normal.z * (radius + 1)}, supported;
    if (DistanceXZ(from, approach) > 40 || !Walk(from, approach, supported)) return false;
    approach = supported;
    // Player_ActionHandler_5/func_8083EC18 require forward contact, alignment
    // within eight units of the native ladder centre, and yDistToLedge >= 79.
    Vec3f ray{center.x - normal.x * 10, approach.y + age.unk_0C, center.z - normal.z * 10};
    CollisionPoly* floor = nullptr;
    s32 floorOwner = BGCHECK_SCENE;
    float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorOwner, &ray);
    float ledge = y - approach.y, ceiling;
    a = {approach.x, approach.y, approach.z};
    CollisionPoly* obstruction = nullptr;
    s32 obstructionOwner = BGCHECK_SCENE;
    if (ledge < 18 || BgCheck_EntityCheckCeiling(&play->colCtx, &ceiling, &a, ledge + 20,
                                               &obstruction, &obstructionOwner, nullptr)) ledge = 399.96002f;
    if (ledge < 79) return false;
    const bool ascending = vines || (flags & 2);
    ray = {center.x - normal.x * (radius + 10), ascending ? high.y + 18 : approach.y + 18,
           center.z - normal.z * (radius + 10)};
    y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorOwner, &ray);
    Point landing{ray.x, y, ray.z};
    if (!floor || !Finite(landing) || !Floor(landing, y) || !ClearBody(landing, landing) ||
        (ascending ? landing.y - approach.y < 79 : approach.y - landing.y < 18)) return false;
    // Descent entry flags live on the back of a ladder. Find its actual front
    // before checking the vertical travel; never connect through a plain wall.
    Point faceCenter = center, faceNormal = normal;
    if (!ascending) {
        a = {center.x - normal.x * (radius * 2 + 10), approach.y - 26, center.z - normal.z * (radius * 2 + 10)};
        b = {center.x + normal.x * (radius + 10), a.y, center.z + normal.z * (radius + 10)};
        if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &wall, true, false, false, true, &owner) ||
            !wall || owner != BGCHECK_SCENE || !(func_80041DB8(&play->colCtx, wall, owner) & 2) ||
            std::abs(wall->normal.y) >= 600 || !bounds(wall, low, high)) return false;
        faceNormal = {COLPOLY_GET_NORMAL(wall->normal.x), 0, COLPOLY_GET_NORMAL(wall->normal.z)};
        faceCenter = {(low.x + high.x) * 0.5f, 0, (low.z + high.z) * 0.5f};
        if (faceNormal.x * normal.x + faceNormal.z * normal.z > -0.99f || DistanceXZ(faceCenter, center) > 10) return false;
    }
    const float bottom = std::min(approach.y, landing.y), top = std::max(approach.y, landing.y);
    if (bottom + 26 < low.y || top > high.y + 18 || top - bottom > 10000) return false;
    // Player_Action_8084BF1C uses radius six and age.unk_3C wall offset while
    // climbing. Validate every part of that corridor with native collision.
    const float end = std::min(top, high.y - 26);
    if (end < bottom) return false;
    const int samples = std::max(1, int(std::ceil((end - bottom) / 8)));
    Point previous{faceCenter.x + faceNormal.x * age.unk_3C, bottom, faceCenter.z + faceNormal.z * age.unk_3C};
    for (int i = 0; i <= samples; ++i) {
        Point next{previous.x, bottom + (end - bottom) * i / samples, previous.z};
        a = {next.x + faceNormal.x * 20, next.y + 26, next.z + faceNormal.z * 20};
        b = {next.x - faceNormal.x * 50, a.y, next.z - faceNormal.z * 50};
        if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &wall, true, false, false, true, &owner) ||
            !wall || owner != BGCHECK_SCENE || !(func_80041DB8(&play->colCtx, wall, owner) & (vines ? 8 : 2)) ||
            COLPOLY_GET_NORMAL(wall->normal.x) * faceNormal.x + COLPOLY_GET_NORMAL(wall->normal.z) * faceNormal.z < 0.99f ||
            std::abs((hit.x - faceCenter.x) * faceNormal.x + (hit.z - faceCenter.z) * faceNormal.z) > 1 ||
            !ClearBody(previous, next, 6)) return false;
        previous = next;
    }
    result = {approach, approach, landing, ascending ? Traversal::LadderUp : Traversal::LadderDown, from, toward};
    return true;
}
bool WalkingQuery::Climb(Point from, Point toward, JumpLink& result) const {
    if (!play || !motion.Valid() || !GET_PLAYER(play) || !GET_PLAYER(play)->ageProperties ||
        !Finite(from) || !Finite(toward)) return false;
    const auto& age = *GET_PLAYER(play)->ageProperties;
    const float length = DistanceXZ(from, toward);
    if (length < 1 || length > 40 || age.unk_14 <= age.unk_1C || age.unk_1C < 18) return false;
    const Point direction{(toward.x - from.x) / length, 0, (toward.z - from.z) / length};
    Vec3f a{from.x, from.y + 18, from.z};
    Vec3f b{from.x + direction.x * (length + radius + 10), a.y,
            from.z + direction.z * (length + radius + 10)}, hit;
    CollisionPoly* wall = nullptr;
    s32 owner = BGCHECK_SCENE;
    if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &wall, true, false, false, true, &owner) ||
        !wall || std::abs(wall->normal.y) >= 600) return false;
    const float nx = COLPOLY_GET_NORMAL(wall->normal.x), nz = COLPOLY_GET_NORMAL(wall->normal.z);
    // Player_ProcessSceneCollision and Player_ActionHandler_12: ordinary ledges
    // auto-climb after six forward updates. No-climb and button-only walls do not.
    const auto flags = func_80041DB8(&play->colCtx, wall, owner);
    if ((flags & 1) || (owner != BGCHECK_SCENE && (flags & 0x40)) ||
        -(direction.x * nx + direction.z * nz) < 0.382684f) return false;
    Point approach{hit.x + nx * (radius + 1), from.y, hit.z + nz * (radius + 1)}, supported;
    if (!Walk(from, approach, supported)) return false;
    approach = supported;
    Vec3f ray{hit.x - nx * 10, approach.y + age.unk_0C, hit.z - nz * 10};
    CollisionPoly* floor = nullptr;
    s32 floorOwner = BGCHECK_SCENE;
    const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &floorOwner, &ray);
    const float rise = y - approach.y;
    // Player_Action_80845668 also jumps up to the age-specific probe height;
    // Player_Action_8084411C grabs the edge, and continued forward input climbs.
    if (!floor || rise < age.unk_1C || rise >= age.unk_0C || std::abs(floor->normal.y) <= 28000) return false;
    if (rise >= age.unk_14 && !ledgeGrabs) return false;
    float ceiling;
    a = {approach.x, approach.y, approach.z};
    CollisionPoly* poly = nullptr;
    s32 bgId = BGCHECK_SCENE;
    if (BgCheck_EntityCheckCeiling(&play->colCtx, &ceiling, &a, rise + 20, &poly, &bgId, nullptr)) return false;
    a.y = b.y = y + 5;
    b.x = approach.x - nx * (radius + 10); b.z = approach.z - nz * (radius + 10);
    if (BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &poly, true, false, false, true, &bgId)) return false;
    Point landing{ray.x - nx * radius, y, ray.z - nz * radius};
    if (!Floor(landing, y) || !ClearBody(landing, landing)) return false;
    // Check clearance above the whole ledge crossing, including dynamic objects.
    Point raised{approach.x, y, approach.z};
    const int samples = std::max(1, static_cast<int>(std::ceil(DistanceXZ(raised, landing) / 8)));
    for (int i = 1; i <= samples; ++i) {
        Point next{raised.x + (landing.x - raised.x) * i / samples, y + (landing.y - y) * i / samples,
                   raised.z + (landing.z - raised.z) * i / samples};
        if (!ClearBody(next, next)) return false;
    }
    result = {approach, approach, landing, Traversal::Ledge, from, toward};
    return true;
}
bool WalkingQuery::Approach(Point from, Point target, float goalRadius, const Actor* targetActor, float heightTolerance) const {
    if (!Finite(from) || !Finite(target) || Distance(from, target) > goalRadius || std::abs(from.y - target.y) > heightTolerance)
        return false;
    Vec3f a{from.x, from.y + 26, from.z}, b{target.x, target.y + 26, target.z}, hit;
    CollisionPoly* poly = nullptr;
    s32 owner = BGCHECK_SCENE;
    if (!BgCheck_EntityLineTest1(&play->colCtx, &a, &b, &hit, &poly, true, true, true, true, &owner)) return true;
    // A climbable surface or exit can lie exactly on its collision boundary.
    // Contact at the named endpoint is different from an obstruction before it.
    if (std::hypot(std::hypot(hit.x - b.x, hit.z - b.z), hit.y - b.y) <= 1) return true;
    const auto* object = owner != BGCHECK_SCENE ? DynaPoly_GetActor(&play->colCtx, owner) : nullptr;
    return targetActor && object && &object->actor == targetActor;
}
}

extern "C" void Navigation_CaptureCollision(PlayState* play) {
    using namespace Navigation;
    obstacles.clear();
    if (!play || !GET_PLAYER(play)) return;
    const auto& player = GET_PLAYER(play)->cylinder.base;
    const auto& context = play->colChkCtx;
    // Copy immediately after the engine's OC pass, while its collider owners are alive.
    // Shape, compatibility and element checks follow CollisionCheck_OC and SetOCvsOC.
    for (int i = 0; i < context.colOCCount && i < COLLISION_CHECK_OC_MAX; ++i) {
        const auto* collider = context.colOC[i];
        if (!collider || !collider->actor || collider->actor == &GET_PLAYER(play)->actor ||
            !(collider->ocFlags1 & OC1_ON) || (collider->ocFlags1 & OC1_NO_PUSH) ||
            !(player.ocFlags1 & collider->ocFlags2 & OC1_TYPE_ALL) ||
            !(player.ocFlags2 & collider->ocFlags1 & OC1_TYPE_ALL) ||
            ((player.ocFlags2 & OC2_UNK1) && (collider->ocFlags2 & OC2_UNK2)) ||
            ((collider->ocFlags2 & OC2_UNK1) && (player.ocFlags2 & OC2_UNK2))) continue;
        if (collider->shape == COLSHAPE_CYLINDER) {
            const auto* cylinder = reinterpret_cast<const ColliderCylinder*>(collider);
            const auto& dim = cylinder->dim;
            if ((cylinder->info.ocElemFlags & OCELEM_ON) && dim.radius > 0 && dim.height > 0)
                obstacles.push_back({{float(dim.pos.x), float(dim.pos.y + dim.yShift), float(dim.pos.z)},
                                     float(dim.radius), float(dim.height), false});
        } else if (collider->shape == COLSHAPE_JNTSPH) {
            const auto* spheres = reinterpret_cast<const ColliderJntSph*>(collider);
            if (!spheres->elements || spheres->count < 0 || spheres->count > 256) continue;
            for (int j = 0; j < spheres->count; ++j) {
                const auto& element = spheres->elements[j];
                const auto& sphere = element.dim.worldSphere;
                if ((element.info.ocElemFlags & OCELEM_ON) && sphere.radius > 0)
                    obstacles.push_back({{float(sphere.center.x), float(sphere.center.y - sphere.radius),
                                          float(sphere.center.z)}, float(sphere.radius), float(sphere.radius * 2), true});
            }
        }
    }
}
