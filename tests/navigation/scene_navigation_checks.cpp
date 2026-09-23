#include "WalkingQuery.h"
#include "SceneGraph.h"
#include "RouteFollower.h"
#include "SurfaceGroups.h"
#include "ApproachBounds.h"
#include "soh/Enhancements/audio/spatial/WallProbes.h"
#include "global.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <chrono>

using namespace Navigation;
extern "C" {
GameInfo* gGameInfo;
SaveContext gSaveContext;
uintptr_t gSegments[NUM_SEGMENTS];
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
void osSyncPrintfUnused(const char*, ...) {}
void* THA_AllocEndAlign(TwoHeadArena*, size_t size, size_t) { return std::calloc(1, size); }
void* GameState_Alloc(GameState*, size_t size, char*, s32) { return std::calloc(1, size); }
void LogUtils_HungupThread(const char*, s32) { std::abort(); }
// Rendering, resource loading and actor updates are forbidden in this offline check.
void Actor_SetObjectDependency(PlayState*, Actor*) { std::abort(); }
void DynaPolyActor_UnsetAllInteractFlags(DynaPolyActor*) { std::abort(); }
void Collider_DrawPoly(GraphicsContext*, Vec3f*, Vec3f*, Vec3f*, u8, u8, u8) { std::abort(); }
CollisionHeader* ResourceMgr_LoadColByName(const char*) { std::abort(); }
int ResourceMgr_OTRSigCheck(char*) { std::abort(); }
void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) { std::abort(); }
f32 Rand_ZeroOne() { std::abort(); }
void guMtxF2L(float[4][4], Mtx*) { std::abort(); }
void* Graph_Alloc(GraphicsContext*, size_t) { std::abort(); }
void FrameInterpolation_RecordSkinMatrixMtxFToMtx(MtxF*, Mtx*) { std::abort(); }
void SpatialAudio_RecordProjection(const void*, const void*, const void*) {}
// These scene checks do not exercise sliding terrain; do not simulate its physics.
void Player_GetSlopeDirection(CollisionPoly*, Vec3f*, s16*) { std::abort(); }
}

struct CollisionFixture {
    CollisionHeader header{};
    std::vector<Vec3s> vertices, cameraPositions;
    std::vector<CollisionPoly> polygons;
    std::vector<SurfaceType> surfaces;
    std::vector<CamData> cameras;
    std::vector<WaterBox> waters;
    explicit CollisionFixture(const char* path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("missing local collision fixture");
        const auto read = [&]<typename T>() {
            T result{};
            if (!file.read(reinterpret_cast<char*>(&result), sizeof result))
                throw std::runtime_error("truncated collision fixture");
            return result;
        };
        file.seekg(64);
        header.minBounds = read.operator()<Vec3s>(); header.maxBounds = read.operator()<Vec3s>();
        vertices.resize(read.operator()<uint32_t>());
        for (auto& v : vertices) v = read.operator()<Vec3s>();
        polygons.resize(read.operator()<uint32_t>());
        for (auto& p : polygons) p = read.operator()<CollisionPoly>();
        surfaces.resize(read.operator()<uint32_t>());
        for (auto& s : surfaces) { s.data[1] = read.operator()<uint32_t>(); s.data[0] = read.operator()<uint32_t>(); }
        cameras.resize(read.operator()<uint32_t>());
        std::vector<int32_t> cameraIndices;
        for (auto& c : cameras) {
            c.cameraSType = read.operator()<uint16_t>(); c.numCameras = read.operator()<int16_t>();
            cameraIndices.push_back(read.operator()<int32_t>());
        }
        cameraPositions.resize(read.operator()<uint32_t>());
        for (auto& p : cameraPositions) p = read.operator()<Vec3s>();
        for (size_t i = 0; i < cameras.size(); ++i)
            cameras[i].camPosData = cameraPositions.empty() ? nullptr : &cameraPositions.at(cameraIndices[i]);
        waters.resize(read.operator()<uint32_t>());
        for (auto& w : waters) {
            w.xMin = read.operator()<int16_t>(); w.ySurface = read.operator()<int16_t>();
            w.zMin = read.operator()<int16_t>(); w.xLength = read.operator()<int16_t>();
            w.zLength = read.operator()<int16_t>(); w.properties = read.operator()<uint32_t>();
        }
        header.numVertices = vertices.size(); header.vtxList = vertices.data();
        header.numPolygons = polygons.size(); header.polyList = polygons.data();
        header.surfaceTypeList = surfaces.data(); header.cameraDataList = cameras.data();
        header.cameraDataListLen = cameras.size(); header.numWaterBoxes = waters.size(); header.waterBoxes = waters.data();
    }
};

int main(int argc, char** argv) {
    try {
        if (argc != 8 && argc != 9) throw std::runtime_error("usage: scene_navigation_checks collision startX startY startZ goalX goalY goalZ [--jump|--ledge|--ladder|--roof|--survey|--walls|--blocked]");
        const std::string mode = argc == 9 ? argv[8] : "";
        if (!mode.empty() && mode != "--jump" && mode != "--ledge" && mode != "--ladder" && mode != "--roof" &&
            mode != "--survey" && mode != "--walk-survey" && mode != "--walls" && mode != "--blocked") throw std::runtime_error("unknown mode");
        CollisionFixture fixture(argv[1]);
        auto play = std::make_unique<PlayState>();
        Player player{}; PlayerAgeProperties age{}; GameInfo info{}; gGameInfo = &info;
        age.wallCheckRadius = 14; age.ceilingCheckHeight = 40; player.ageProperties = &age;
        age.unk_0C = 71; age.unk_14 = 47; age.unk_1C = 27;
        age.unk_3C = 12; age.unk_40 = 55;
        play->actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        play->sceneNum = SCENE_KOKIRI_FOREST; play->roomCtx.curRoom.num = 0;
        if (const char* scene = std::getenv("NAV_SCENE_ID")) play->sceneNum = std::stoi(scene);
        BgCheck_Allocate(&play->colCtx, play.get(), &fixture.header);
        const auto exits = GroupSurfaces(&fixture.header, [&](CollisionPoly* poly) {
            return poly->normal.y > 0 ? int(SurfaceType_GetSceneExitIndex(&play->colCtx, poly, BGCHECK_SCENE)) : 0;
        });
        if (play->sceneNum == SCENE_KOKIRI_FOREST && exits.size() != 9)
            throw std::runtime_error("Kokiri entrance triangles were not grouped into nine thresholds");
        const Point start{std::stof(argv[2]), std::stof(argv[3]), std::stof(argv[4])};
        const Point goal{std::stof(argv[5]), std::stof(argv[6]), std::stof(argv[7])};
        if (mode == "--walls") {
            player.actor.world.pos = player.actor.prevPos = {start.x, start.y, start.z};
            player.actor.world.rot.y = Math_Atan2S(goal.z - start.z, goal.x - start.x);
            player.actor.yDistToWater = BGCHECK_Y_MIN;
            Vec3f ray{start.x, start.y + 1, start.z};
            s32 floorId = BGCHECK_SCENE;
            player.actor.floorHeight = BgCheck_EntityRaycastFloor3(&play->colCtx, &player.actor.floorPoly,
                                                                  &floorId, &ray);
            player.actor.floorBgId = floorId;
            if (!player.actor.floorPoly || std::abs(player.actor.floorHeight - start.y) > 1)
                throw std::runtime_error("wall scan did not start on the expected floor");
            const Player before = player;
            const auto hits = SpatialAudio::ScanWalls(play.get());
            if (std::memcmp(&before, &player, sizeof player)) throw std::runtime_error("wall scan changed player state");
            if (hits[0].found) throw std::runtime_error("accessible porch ladder or floor drop sounded as a wall");
            std::cout << "Native scene wall scan leaves this ladder/drop silent.\n";
            return 0;
        }
        for (auto p : {start, goal}) {
            Vec3f ray{p.x, p.y + 50, p.z}; CollisionPoly* floor = nullptr; s32 owner = BGCHECK_SCENE;
            const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &owner, &ray);
            std::cout << "Floor " << p.x << ',' << p.z << ": " << y << ", normal " << (floor ? floor->normal.y : 0) << '\n';
        }
        QueryStats stats;
        // sBootData[PLAYER_BOOTS_KOKIRI_CHILD], consumed by Player_SetBootData.
        const JumpMotion childMotion{5.5f, -1, 1.5f, 2, 5.4f, 7.5f, 1.25f, 0.4f};
        WalkingQuery query(play.get(), childMotion, &stats);
        SceneGraph graph;
        const auto graphBegan = std::chrono::steady_clock::now();
        graph.Build(play.get(), query.Radius());
        const double graphMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - graphBegan).count();
        std::cout << "Graph ms " << graphMs << ", anchors " << graph.Anchors() << '\n';
        if (mode == "--walk-survey") {
            std::vector<Point> floors;
            for (const auto& poly : fixture.polygons) {
                if (poly.normal.y <= COLPOLY_SNORMAL(0.5f) || COLPOLY_VIA_FLAG_TEST(poly.flags_vIA, 2)) continue;
                Point p{};
                for (int i : {COLPOLY_VTX_INDEX(poly.flags_vIA), COLPOLY_VTX_INDEX(poly.flags_vIB), int(poly.vIC)}) {
                    const auto& v = fixture.vertices.at(i); p.x += v.x / 3.0f; p.y += v.y / 3.0f; p.z += v.z / 3.0f;
                }
                Point supported;
                if (query.Support(p, supported) && query.Segment(supported, supported)) floors.push_back(supported);
            }
            size_t checked = 0;
            for (size_t a = 0; a < floors.size() && checked < 8; a += 7) for (size_t b = a + 1; b < floors.size(); b += 5) {
                const Point from = floors[a], to = floors[b];
                if (Distance(from, to) < 150 || Distance(from, to) > 750 || !query.Segment(from, to)) continue;
                RouteSearch route;
                route.Begin(from, to, 30, 14,
                    [&](Point p, Point q, Point& out) { return query.Walk(p, q, out); },
                    [&](Point p) { return query.Approach(p, to, 30); }, {},
                    [&](Point p, std::vector<Connection>& out) { graph.Expand(play.get(), query, p, to, 30, out); },
                    [&](Point p, Point& out) { return query.Support(p, out); });
                while (route.State() == SearchState::Searching) route.Step(128);
                if (route.State() != SearchState::Found) throw std::runtime_error("native supported map corridor was not routed");
                RouteFollower follower; follower.Set(route);
                if (!follower.Rejoin(from, query)) throw std::runtime_error("other-map route could not be joined");
                Point p = from;
                for (int sample = 1; sample <= 100; ++sample) {
                    Point wanted{from.x + (to.x - from.x) * sample / 100, p.y, from.z + (to.z - from.z) * sample / 100};
                    if (!query.Walk(p, wanted, p)) throw std::runtime_error("verified native corridor lost movement support");
                    if (follower.Update(p, true, false, false, query).state == FollowState::Replan)
                        throw std::runtime_error("other-map movement discarded its route");
                }
                ++checked;
                break;
            }
            if (checked != 8) throw std::runtime_error("scene fixture lacked eight supported test corridors");
            std::cout << "Eight native corridors and movement replays passed in scene " << play->sceneNum << '\n';
            return 0;
        }
        if (mode == "--survey") {
            bool reachable = true;
            for (const auto& exit : exits) {
                RouteSearch route;
                route.Begin(start, exit.position, 45, 14,
                    [&](Point a, Point b, Point& p) { return query.Walk(a, b, p); },
                    [&](Point p) { return query.Approach(p, exit.position, 45); },
                    [&](Point a, Point b, JumpLink& link) { return query.Traverse(a, b, link); },
                    [&](Point p, std::vector<Connection>& out) { graph.Expand(play.get(), query, p, exit.position, 45, out); },
                    [&](Point p, Point& out) { return query.Support(p, out); });
                while (route.State() == SearchState::Searching) route.Step(128);
                reachable &= route.State() == SearchState::Found;
                std::cout << "Threshold " << exit.tag << " at " << exit.position.x << ',' << exit.position.y << ',' << exit.position.z
                          << ": state " << int(route.State()) << ", " << route.Visited() << " nodes\n";
            }
            return reachable ? 0 : 1;
        }
        if (mode == "--jump") {
            JumpLink link;
            const bool jumped = query.Jump(start, goal, link);
            std::cout << "Jump " << jumped << '\n';
            for (size_t i = 0; i < stats.rejected.size(); ++i) std::cout << "Rejected " << i << ": " << stats.rejected[i] << '\n';
            for (auto p : {stats.departure, stats.departureFloor}) std::cout << "Departure " << p.x << ',' << p.y << ',' << p.z << '\n';
            for (auto p : {stats.wallFrom, stats.wallTo, stats.wallNormal}) std::cout << "Wall " << p.x << ',' << p.y << ',' << p.z << '\n';
            return jumped ? 0 : 1;
        }
        RouteSearch search;
        const Player before = player;
        const bool pickup = mode == "--roof" || mode == "--ledge";
        const float goalRadius = pickup ? 60 : 45;
        const auto began = std::chrono::steady_clock::now();
        RouteSearch::Expand neighbours;
        if (!std::getenv("NAV_DETAILED") && (std::getenv("NAV_SURFACES") || PreferSceneGraph(start, goal))) neighbours = [&](Point p, std::vector<Connection>& out) {
            graph.Expand(play.get(), query, p, goal, goalRadius, out);
        };
        search.Begin(start, goal, goalRadius, 14,
            [&](Point a, Point b, Point& p) { return query.Walk(a, b, p); },
            [&](Point p) {
                return (!pickup || PickupApproach({p.x - goal.x, p.y - goal.y, p.z - goal.z})) &&
                       query.Approach(p, goal, goalRadius, nullptr, pickup ? 50 : 30);
            },
            [&](Point a, Point b, JumpLink& p) { return query.Traverse(a, b, p); },
            std::move(neighbours),
            [&](Point p, Point& out) { return query.Support(p, out); });
        size_t slices = 0;
        while (search.State() == SearchState::Searching) {
            ++slices;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(6);
            do { search.Step(1); } while (search.State() == SearchState::Searching && std::chrono::steady_clock::now() < deadline);
        }
        const double elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count();
        std::cout << "Search ms " << elapsed << ", walks " << stats.walks << ", bodies " << stats.bodies
                  << ", traversals " << stats.traversals << ", update slices " << slices << '\n';
        std::cout << "State " << int(search.State()) << ", " << search.Visited() << " nodes, " << search.Jumps().size() << " jumps\n";
        std::cout << "Closest " << search.Closest().x << ',' << search.Closest().y << ',' << search.Closest().z << '\n';
        if (std::memcmp(&before, &player, sizeof player)) throw std::runtime_error("route query changed player state");
        if (search.State() == SearchState::Found) {
            RouteFollower follower;
            follower.Set(search);
            if (!follower.Rejoin(start, query)) throw std::runtime_error("cannot join newly found route");
            const auto follow = [&](Point p, bool grounded, bool ledge, bool ladder) {
                const auto result = follower.Update(p, grounded, ledge, ladder, query);
                if (result.state == FollowState::Replan) {
                    std::cerr << "Follower at " << p.x << ',' << p.y << ',' << p.z << ": " << result.reason << '\n';
                    throw std::runtime_error("guidance discarded a valid route during movement");
                }
            };
            follow(start, true, false, false);
            for (size_t i = 1; i < search.Path().size(); ++i) {
                if (const auto jump = search.Jumps().find(i); jump != search.Jumps().end()) {
                    const auto& link = jump->second;
                    follow(link.runup, true, false, false);
                    follow(link.takeoff, false, link.kind == Traversal::Ledge, IsLadder(link.kind));
                    follow(link.landing, false, link.kind == Traversal::Ledge, IsLadder(link.kind));
                    follow(link.landing, true, false, false);
                } else {
                    const Point a = search.Path()[i - 1], b = search.Path()[i];
                    if (!query.Segment(a, b)) throw std::runtime_error("returned walking edge failed native validation");
                    const int samples = std::max(1, int(std::ceil(DistanceXZ(a, b) / 8)));
                    Point previous = a;
                    for (int sample = 1; sample <= samples; ++sample) {
                        const float t = float(sample) / samples;
                        Point requested{a.x + (b.x - a.x) * t, previous.y, a.z + (b.z - a.z) * t}, actual;
                        if (!query.Walk(previous, requested, actual)) throw std::runtime_error("native movement replay lost support");
                        follow(actual, true, false, false);
                        previous = actual;
                    }
                }
            }
            std::cout << "Guidance movement replay passed.\n";
        }
        // Every link must survive the same revalidation used during guidance.
        for (const auto& [index, link] : search.Jumps()) {
            JumpLink checked;
            const float length = DistanceXZ(link.takeoff, link.landing);
            Point toward{link.takeoff.x + (link.landing.x - link.takeoff.x) * 14 / length, link.takeoff.y,
                         link.takeoff.z + (link.landing.z - link.takeoff.z) * 14 / length};
            if (!query.Revalidate(link, checked)) throw std::runtime_error("route cannot be revalidated");
            if (!IsLadder(link.kind)) continue;
            std::vector<std::pair<size_t, uint16_t>> changed;
            for (size_t i = 0; i < fixture.polygons.size(); ++i)
                if (func_80041DB8(&play->colCtx, &fixture.polygons[i], BGCHECK_SCENE) & (2 | 8)) {
                    changed.emplace_back(i, fixture.polygons[i].type);
                    fixture.polygons[i].type = 0;
                }
            if (query.Ladder(link.takeoff, toward, checked)) throw std::runtime_error("plain wall became a ladder");
            for (const auto& [i, type] : changed) fixture.polygons[i].type = type;
        }
        if (mode == "--ladder" || mode == "--roof") {
            bool ladder = false;
            for (const auto& [index, link] : search.Jumps()) if (IsLadder(link.kind)) {
                ladder = true;
                if (index == 0 || Distance(search.Path()[index - 1], link.runup) > 0.01f)
                    throw std::runtime_error("ladder entry metadata was lost");
                if (query.Segment(link.runup, link.landing)) throw std::runtime_error("ladder test used walkable ground");
                JumpLink checked;
                if (!query.Revalidate(link, checked)) throw std::runtime_error("ladder route cannot be revalidated");
            }
            if (!ladder) throw std::runtime_error("porch route lost its native ladder connection");
        }
        if (mode == "--ledge" || mode == "--roof") {
            bool ledge = false;
            for (const auto& [index, link] : search.Jumps()) if (link.kind == Traversal::Ledge) {
                ledge = true;
                if (query.Segment(link.runup, link.landing)) throw std::runtime_error("raised platform test did not require a climb");
                if (index == 0 || Distance(search.Path()[index - 1], link.runup) > 0.01f)
                    throw std::runtime_error("ledge approach metadata was lost");
                JumpLink disabled;
                WalkingQuery noGrabs(play.get(), childMotion, nullptr, false);
                Point toward{link.runup.x + (link.landing.x - link.runup.x) * 14 / DistanceXZ(link.runup, link.landing),
                             link.runup.y, link.runup.z + (link.landing.z - link.runup.z) * 14 / DistanceXZ(link.runup, link.landing)};
                if (mode == "--ledge" && link.landing.y - link.runup.y >= age.unk_14 && noGrabs.Climb(link.runup, toward, disabled))
                    throw std::runtime_error("disabled ledge grabs were used");
            }
            if (!ledge) throw std::runtime_error("raised platform route lost its native ledge connection");
        }
        for (size_t i = 0; i < stats.rejected.size(); ++i) std::cout << "Rejected " << i << ": " << stats.rejected[i] << '\n';
        for (auto p : {stats.wallFrom, stats.wallTo, stats.wallNormal}) std::cout << "Wall " << p.x << ',' << p.y << ',' << p.z << '\n';
        for (auto p : search.Path()) std::cout << p.x << ',' << p.y << ',' << p.z << '\n';
        for (const auto& [index, link] : search.Jumps()) std::cout << "Traversal " << int(link.kind) << " at " << index << '\n';
        return search.State() == (mode == "--blocked" ? SearchState::Unreachable : SearchState::Found) ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
