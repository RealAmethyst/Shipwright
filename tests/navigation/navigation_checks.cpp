#include "RouteSearch.h"
#include "WalkingQuery.h"
#include "Navigation.h"
#include "InputCapture.h"
#include "StepEstimate.h"
#include "SurfaceGroups.h"
#include "TraversalProgress.h"
#include "RouteFollower.h"
#include "ApproachBounds.h"
#include "Destination.h"
#include "soh/Enhancements/tts/SignText.h"
#include "global.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>

using namespace Navigation;
static void Check(bool pass, const char* message) { if (!pass) throw std::runtime_error(message); }
namespace {
CollisionPoly floorPoly{}, wallPoly{};
DynaPolyActor dynamicActor{};
bool barrier = false, ceiling = false, water = false, pit = false;
int surfaceType = 0, floorProperty = 0, floorOwner = BGCHECK_SCENE;
int lineOwner = BGCHECK_SCENE;
float floorHeight = 0;
float pitEnd = 10000, farHeight = 0;
float gapCeiling = 10000;
int landingType = 0;
void Reset() {
    ClearObstacles();
    barrier = ceiling = water = pit = false;
    surfaceType = floorProperty = 0;
    floorOwner = lineOwner = BGCHECK_SCENE;
    floorHeight = 0;
    pitEnd = 10000; farHeight = 0;
    gapCeiling = 10000; landingType = 0;
    floorPoly.normal.y = 32767;
}
}
extern "C" {
s32 BgCheck_PosInStaticBoundingBox(CollisionContext*, Vec3f* p) { return std::abs(p->x) <= 500 && std::abs(p->z) <= 500; }
f32 BgCheck_EntityRaycastFloor3(CollisionContext*, CollisionPoly** poly, s32* owner, Vec3f* point) {
    const float y = pit && point->x > 50 ? (point->x < pitEnd ? BGCHECK_Y_MIN : farHeight) : floorHeight;
    floorPoly.type = point->x >= pitEnd ? 1 : 0;
    *poly = y > BGCHECK_Y_MIN && y <= point->y ? &floorPoly : nullptr;
    *owner = floorOwner;
    return *poly ? y : BGCHECK_Y_MIN;
}
s32 BgCheck_EntitySphVsWall3(CollisionContext*, Vec3f* result, Vec3f* next, Vec3f*, f32 radius,
                            CollisionPoly** poly, s32* owner, Actor*, f32 height) {
    Check(height == 26, "native wall-check height changed");
    *result = *next;
    const bool hit = barrier && std::abs(next->x - 80) < radius && std::abs(next->z) < 50 + radius;
    if (hit) result->x = next->x < 80 ? 80 - radius : 80 + radius;
    *poly = hit ? &wallPoly : nullptr; *owner = BGCHECK_SCENE;
    return hit;
}
s32 BgCheck_EntityCheckCeiling(CollisionContext*, f32*, Vec3f* p, f32 height, CollisionPoly**, s32*, Actor*) {
    return ceiling || (p->x > 50 && p->x < pitEnd && p->y + height > gapCeiling);
}
u32 func_80041D4C(CollisionContext*, CollisionPoly* p, s32 owner) {
    Check(p == &floorPoly && owner == floorOwner, "floor type read from wrong collision owner");
    return p->type == 1 ? landingType : surfaceType;
}
u32 func_80041EA4(CollisionContext*, CollisionPoly*, s32 owner) {
    Check(owner == floorOwner, "floor property read from wrong owner"); return floorProperty;
}
u32 SurfaceType_IsWallDamage(CollisionContext*, CollisionPoly*, s32 owner) {
    Check(owner == floorOwner, "damage read from wrong owner"); return 0;
}
u32 SurfaceType_GetSlope(CollisionContext*, CollisionPoly*, s32) { return 0; }
u32 SurfaceType_GetConveyorSpeed(CollisionContext*, CollisionPoly*, s32) { return 0; }
s32 func_80041DB8(CollisionContext*, CollisionPoly*, s32) { return 0; }
s32 WaterBox_GetSurface1(PlayState*, CollisionContext*, f32, f32, f32* y, WaterBox**) { *y = 100; return water; }
s32 BgCheck_EntityLineTest1(CollisionContext*, Vec3f* a, Vec3f* b, Vec3f* hit, CollisionPoly** poly,
                           s32 wall, s32 floor, s32 roof, s32 oneFace, s32* owner) {
    Check(wall && floor && roof && oneFace, "approach must check all geometry types");
    *owner = lineOwner; *poly = nullptr;
    if (!barrier || a->x == b->x) return false;
    const float t = (80 - a->x) / (b->x - a->x);
    if (t < 0 || t > 1 || std::abs(a->z + t * (b->z - a->z)) >= 50) return false;
    *hit = {80, a->y + t * (b->y - a->y), a->z + t * (b->z - a->z)};
    *poly = &wallPoly; return true;
}
DynaPolyActor* DynaPoly_GetActor(CollisionContext*, s32 owner) { return owner == 7 ? &dynamicActor : nullptr; }
}

static void CheckInstalledSigns(const char* path) {
    std::ifstream stream(path, std::ios::binary);
    Check(bool(stream), "cannot read installed message resource");
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    size_t offset = 64;
    const auto read = [&](size_t count) {
        Check(offset + count <= bytes.size(), "truncated installed message resource");
        uint32_t value = 0;
        for (size_t i = 0; i < count; ++i) value |= uint32_t(bytes[offset++]) << (i * 8);
        return value;
    };
    const auto count = read(4);
    Check(count > 0 && count <= 65536, "invalid installed message count");
    const std::map<int, std::string> expected{
        {HouseSign(SCENE_LINKS_HOUSE), "Link's House"}, {HouseSign(SCENE_MIDOS_HOUSE), "House of the Great Mido"},
        {HouseSign(SCENE_KNOW_IT_ALL_BROS_HOUSE), "House of the Know-it-All Brothers"},
        {HouseSign(SCENE_TWINS_HOUSE), "House of Twins"}, {HouseSign(SCENE_SARIAS_HOUSE), "Saria's House"}};
    const std::map<int, std::string> names{
        {0x033c, "Mido"}, {0x033f, "Saria"}, {0x2041, "Malon"}, {0x702c, "Talon"},
        {0x2014, "Ingo"}, {0x301a, "Darunia"}, {0x605f, "Nabooru"}, {0x402f, "Ruto"},
        {0x7060, "Zelda"}, {0x400a, "King Zora"}};
    size_t found = 0;
    size_t foundNames = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const auto id = read(2); read(1); read(1);
        const auto length = read(4);
        Check(length <= bytes.size() - offset, "invalid installed message length");
        if (expected.contains(id)) {
            const std::string_view raw(reinterpret_cast<const char*>(bytes.data() + offset), length);
            Check(SpeechText::SignCaption(raw, "Link", id == 0x33c, [](uint8_t) { return ""; }) == expected.at(id),
                  "house label disagrees with actual installed sign caption");
            ++found;
        }
        if (names.contains(id)) {
            const std::string_view raw(reinterpret_cast<const char*>(bytes.data() + offset), length);
            Check(SpeechText::HighlightedName(raw) == names.at(id),
                  "NPC name disagrees with actual installed game message");
            ++foundNames;
        }
        offset += length;
    }
    Check(found == expected.size(), "missing or duplicated installed house sign");
    Check(foundNames == names.size(), "missing or duplicated installed NPC name message");
    std::cout << "Five native house captions and ten NPC names verified in installed game text\n";
}

int main(int argc, char** argv) {
    try {
        if (argc == 2) { CheckInstalledSigns(argv[1]); return 0; }
        TraversalProgress movement;
        Check(movement.Update(Traversal::LadderDown, true, false, false) == TraversalPhase::Approach,
              "ladder completed before entry");
        Check(movement.Update(Traversal::LadderDown, true, false, true) == TraversalPhase::Moving,
              "grounded ladder-entry animation ended guidance");
        for (int frame = 0; frame < 90; ++frame)
            Check(movement.Update(Traversal::LadderDown, false, false, true) == TraversalPhase::Moving,
                  "stationary ladder lost traversal state");
        Check(movement.Update(Traversal::LadderDown, true, false, true) == TraversalPhase::Moving &&
              movement.Update(Traversal::LadderDown, true, false, false) == TraversalPhase::Landed,
              "ladder completion ignored native animation lifetime");
        movement.Reset();
        Check(movement.Update(Traversal::LadderUp, false, false, true) == TraversalPhase::Moving &&
              movement.Update(Traversal::LadderUp, true, true, false) == TraversalPhase::Moving &&
              movement.Update(Traversal::LadderUp, true, false, false) == TraversalPhase::Landed,
              "climbable-wall exit animation lost guidance before reaching the top");
        movement.Reset();
        Check(movement.Update(Traversal::Ledge, true, true, false) == TraversalPhase::Moving &&
              movement.Update(Traversal::Ledge, true, false, false) == TraversalPhase::Landed,
              "grounded ledge animation was treated as ordinary walking");
        movement.Reset();
        Check(movement.Update(Traversal::Jump, false, false, false) == TraversalPhase::Moving &&
              movement.Update(Traversal::Jump, true, false, false) == TraversalPhase::Landed,
              "running jump completion changed");
        InputCapture capture;
        Check(!capture.Consume(false, true), "closed menu captured ordinary gameplay");
        capture.Open();
        Check(!capture.Ready(true) && capture.Consume(true, true), "opening press leaked or selected a target");
        Check(capture.Ready(false) && capture.Ready(true), "menu did not arm after neutral input");
        capture.Close();
        for (int frame = 0; frame < 10; ++frame)
            Check(capture.Consume(false, true) && !capture.CanToggle() && capture.BlocksGame(false), "held closing input leaked");
        Check(capture.Consume(false, false) && !capture.BlocksGame(false), "neutral release frame was not consumed");
        Check(!capture.Consume(false, true) && capture.CanToggle(), "fresh input did not return to gameplay");
        Check(ChestApproach({0,0,-40}) && !ChestApproach({0,0,40}) && !ChestApproach({25,0,-30}) &&
              !ChestApproach({0,15,-30}), "chest route ended outside its opening side");
        Check(DoorApproach({0,0,-40}) && DoorApproach({0,0,40}) && !DoorApproach({25,0,0}) &&
              !DoorApproach({0,25,0}), "door route ignored native approach bounds");
        Check(PickupApproach({30,50,0}) && PickupApproach({0,-50,30}) &&
              !PickupApproach({31,0,0}) && !PickupApproach({0,51,0}) && !PickupApproach({25,0,25}),
              "pickup route ended outside native collection bounds");
        auto play = std::make_unique<PlayState>();
        Player player{}; PlayerAgeProperties age{};
        age.wallCheckRadius = 14; age.ceilingCheckHeight = 40;
        player.ageProperties = &age;
        play->actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
        Reset();
        WalkingQuery query(play.get()); Point actual;
        const Player untouched = player;
        Check(query.Walk({0,0,0}, {40,0,0}, actual) && actual.x == 40, "flat walking rejected");
        ceiling = true; Check(!query.Walk({0,0,0}, {40,0,0}, actual), "low ceiling accepted"); ceiling = false;
        water = true; Check(!query.Walk({0,0,0}, {40,0,0}, actual), "deep water accepted as a walking route"); water = false;
        pit = true; Check(!query.Walk({0,0,0}, {80,0,0}, actual), "route crossed unsupported ground"); pit = false;
        for (int type : {2, 3, 9}) { surfaceType = type; Check(!query.Walk({0,0,0}, {8,0,0}, actual), "damaging floor accepted"); }
        surfaceType = 0;
        for (int property : {5, 12}) { floorProperty = property; Check(!query.Walk({0,0,0}, {8,0,0}, actual), "void floor accepted"); }
        floorProperty = 0; floorOwner = 7;
        Check(query.Walk({0,0,0}, {8,0,0}, actual), "dynamic floor lost its owner");
        floorOwner = BGCHECK_SCENE;
        floorHeight = 30; Check(!query.Walk({0,0,0}, {8,0,0}, actual), "unclimbed ledge accepted");
        floorHeight = -20; Check(!query.Walk({0,0,0}, {8,0,0}, actual), "drop accepted"); floorHeight = 0;
        barrier = true;
        Check(!query.Segment({0,0,0}, {160,0,0}), "direct wall crossing accepted");
        Check(!query.Approach({50,0,0}, {100,0,0}, 60), "arrived through a wall");
        Check(query.Approach({50,0,0}, {80,0,0}, 45), "surface endpoint contact rejected");
        lineOwner = 7;
        Check(query.Approach({50,0,0}, {100,0,0}, 60, &dynamicActor.actor), "target door's own collision rejected");
        Check(!query.Approach({50,0,0}, {100,0,0}, 60, &player.actor), "unrelated dynamic obstruction ignored");
        lineOwner = BGCHECK_SCENE;

        RouteSearch search;
        Point supportedContact;
        Check(query.Support({70, 0, 0}, supportedContact), "wall contact rejected the occupied start floor");
        Check(!query.Segment({66, 0, 0}, {74, 0, 0}), "contact fix allowed walking into a wall");
        Check(query.Segment({66.1f, 0, 0}, {66.1f, 0, 8}), "harmless native wall contact blocked parallel walking");
        search.Begin({70, 0, 0}, {20, 0, 0}, 10, 14,
            [&](Point a, Point b, Point& out) { return query.Walk(a, b, out); },
            [&](Point p) { return query.Approach(p, {20, 0, 0}, 10); }, {}, {},
            [&](Point p, Point& out) { return query.Support(p, out); });
        while (search.State() == SearchState::Searching) search.Step(16);
        Check(search.State() == SearchState::Found, "cannot leave wall contact while retaining native clearance");
        RouteFollower follower;
        follower.Set(search);
        Check(follower.Rejoin({42, 0, 0}, query), "moving while a search finishes prevented joining its route");
        Check(follower.Update({42, 0, 0}, false, false, false, query).state == FollowState::Waiting && follower.HasRoute(),
              "brief loss of grounding discarded the route");
        Check(follower.Update({42, 0, 0}, true, false, false, query).state == FollowState::Guiding,
              "grounding did not resume guidance");
        auto begin = [&](Point goal) {
            search.Begin({0,0,0}, goal, 20, 14,
                [&](Point a, Point b, Point& out) { return query.Walk(a,b,out); },
                [&, goal](Point p) { return query.Approach(p, goal, 20); });
        };
        begin({160,0,0});
        search.Step(1);
        Check(search.State() == SearchState::Searching, "search did not yield");
        size_t updates = 0;
        while (search.State() == SearchState::Searching && ++updates < 5000) search.Step(16);
        Check(search.State() == SearchState::Found, "no route around finite wall");
        bool detour = false;
        const auto path = search.Path();
        for (size_t i = 1; i < path.size(); ++i) {
            Check(query.Segment(path[i-1], path[i]), "returned path contains an unwalkable segment");
            detour |= std::abs(path[i].z) >= 64;
        }
        Check(detour, "route failed to clear Link's radius at the wall corner");
        begin({160,100,0});
        while (search.State() == SearchState::Searching) search.Step(128);
        Check(search.State() == SearchState::Unreachable && search.Path().empty(), "wrong floor accepted");
        search.Reset(); Check(search.State() == SearchState::Idle && search.Path().empty(), "cancel retained a route");
        search.Begin({NAN,0,0}, {10,0,0}, 20, 14, {}, {});
        Check(search.State() == SearchState::Unreachable, "invalid search accepted");
        search.Begin({0, 0, 0}, {40, 0, 0}, 0, 14,
            [](Point from, Point to, Point& out) {
                out = to;
                return !(from.x == 0 && from.z == 0 && to.x == 20);
            }, [](Point p) { return p.x == 40; }, {},
            [](Point p, std::vector<Connection>& out) {
                if (p.x == 0 && p.z == 0) out = {{{20, 0, 0}, {}, true}, {{0, 0, 40}, {}, true}};
                else if (p.x == 0) out = {{{20, 0, 0}, {}, true}};
                else if (p.x == 20) out = {{{40, 0, 0}, {}, true}};
            });
        while (search.State() == SearchState::Searching) search.Step(1);
        Check(search.State() == SearchState::Found && search.Path().size() == 4,
              "a rejected lazy edge hid a valid alternate approach");
        const Point distant{8000,0,3000};
        search.Begin({0,0,0}, distant, 45, 14,
            [](Point, Point requested, Point& out) { out = requested; return true; },
            [distant](Point p) { return Distance(p, distant) <= 45; });
        while (search.State() == SearchState::Searching) search.Step(128);
        Check(search.State() == SearchState::Found && search.Visited() < 6000, "large open area caused excessive search work");

        Reset();
        ColliderCylinder obstacle{};
        Actor obstacleActor{};
        obstacle.base.actor = &obstacleActor;
        obstacle.base.shape = COLSHAPE_CYLINDER;
        obstacle.base.ocFlags1 = OC1_ON | OC1_TYPE_ALL;
        obstacle.base.ocFlags2 = OC2_TYPE_1;
        obstacle.info.ocElemFlags = OCELEM_ON;
        obstacle.dim = {20, 60, 0, {50, 0, 0}};
        player.cylinder.base.ocFlags1 = OC1_ON | OC1_TYPE_ALL;
        player.cylinder.base.ocFlags2 = OC2_TYPE_PLAYER;
        play->colChkCtx.colOCCount = 1; play->colChkCtx.colOC[0] = &obstacle.base;
        Navigation_CaptureCollision(play.get());
        Check(!query.Walk({0,0,0}, {50,0,0}, actual), "solid actor ignored");
        player.cylinder.dim = {12, 40, 0, {0, 0, 0}};
        WalkingQuery nativeBody(play.get());
        Check(nativeBody.Walk({18, 0, -5}, {18, 0, 5}, actual), "native object-contact radius replaced by wall radius");
        Check(!nativeBody.Walk({19, 0, 0}, {19, 0, 0}, actual), "overlapping a solid object was allowed");
        obstacle.base.ocFlags1 |= OC1_NO_PUSH;
        Navigation_CaptureCollision(play.get());
        Check(query.Walk({0,0,0}, {50,0,0}, actual), "nonblocking trigger treated as solid");
        ClearObstacles();

        Reset(); pit = true; pitEnd = 110;
        const JumpMotion child{5.5f, -1, 1.5f, 2, 5.4f, 7.5f, 1.25f, 0.4f};
        const JumpMotion adult{6, -1, 1.5f, 2, 5.9f, 7.5f, 1.25f, 0.2f};
        Check(child.Valid() && adult.Valid() && child.Velocity() == 7.5f, "native boot jump parameters rejected");
        WalkingQuery jumping(play.get(), child);
        JumpLink jump;
        Check(!query.Walk({42,0,0}, {56,0,0}, actual), "gap became walkable");
        Check(jumping.Jump({42,0,0}, {56,0,0}, jump), "native automatic jump across a short gap rejected");
        Check(jump.runup.x < 42 && jump.takeoff.x <= 50 && jump.landing.x > pitEnd &&
              jumping.Segment({42,0,0}, jump.runup), "jump lacks a reachable runway or solid landing");
        Check(WalkingQuery(play.get(), adult).Jump({42,0,0}, {56,0,0}, jump), "adult jump rejected");
        auto iron = adult; iron.speed = 3;
        Check(!WalkingQuery(play.get(), iron).Jump({42,0,0}, {56,0,0}, jump), "speed at native threshold accepted");
        pitEnd = 240;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "unreachable wide jump accepted");
        pitEnd = 167;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "jump depended on a favorable edge-crossing phase");
        pitEnd = 110; farHeight = -410;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "damaging landing drop accepted");
        farHeight = 0;
        gapCeiling = 55;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "arc passed through a low ceiling over the gap");
        gapCeiling = 10000;
        landingType = 2;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "safe takeoff routed onto lava landing");
        landingType = 0;
        ceiling = true;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "jump through ceiling accepted");
        ceiling = false; barrier = true;
        Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "jump through wall accepted");
        barrier = false;
        for (int property : {6, 7, 8, 9, 11}) {
            floorProperty = property;
            Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "native no-jump floor accepted");
        }
        floorProperty = 0;
        for (int type : {2, 3, 9}) {
            surfaceType = type;
            Check(!jumping.Jump({42,0,0}, {56,0,0}, jump), "hazardous jump accepted");
        }
        surfaceType = 0;
        search.Begin({0,0,0}, {200,0,0}, 20, 14,
            [&](Point a, Point b, Point& out) { return jumping.Walk(a,b,out); },
            [&](Point at) { return jumping.Approach(at, {200,0,0}, 20); },
            [&](Point a, Point b, JumpLink& out) { return jumping.Jump(a,b,out); });
        while (search.State() == SearchState::Searching) search.Step(16);
        Check(search.State() == SearchState::Found && !search.Jumps().empty(), "search failed to use automatic jump");
        for (size_t i = 1; i < search.Path().size(); ++i) {
            if (search.Jumps().contains(i)) {
                const auto& link = search.Jumps().at(i);
                Check(Distance(link.runup, search.Path()[i-1]) == 0 && Distance(link.landing, search.Path()[i]) == 0,
                      "route lost jump runway or landing metadata");
            } else Check(jumping.Segment(search.Path()[i-1], search.Path()[i]), "jump search added an invalid walking edge");
        }
        Check(Building(SCENE_LINKS_HOUSE) && Building(SCENE_MIDOS_HOUSE) && Building(SCENE_BAZAAR) &&
              !Building(SCENE_KOKIRI_FOREST) && !Building(SCENE_HYRULE_FIELD), "house/map cue classification is wrong");
        const uint8_t ntscName[]{0xb6,0xcd,0xd2,0xcf,0xdf,0xdf,0xdf,0xdf};
        const auto name = SpeechText::SavedLatinName(ntscName, false);
        Check(name == "Link", "saved NTSC name decoded incorrectly");
        const std::string sign = std::string("\x08\x06\x1e\x0f") + "'s House\x09\x02";
        const auto glyph = [](uint8_t c) { return c == 0x96 ? "é" : ""; };
        Check(SpeechText::SignCaption(sign, name, false, glyph) == "Link's House", "house sign lost saved-name substitution");
        Check(SpeechText::SignCaption("\x08\x05\x41" "Chez Saria\x05\x40\x09\x02", "", false, glyph) == "Chez Saria",
              "caption leaked colour bytes");
        Check(SpeechText::SignCaption("\x08" "Caf\x96\x09\x02", "", false, glyph) == "Café", "caption lost accented glyph");
        Check(SpeechText::SignCaption("House\x01" "Subtitle\x09\x02", "", true, glyph) == "House", "subtitle entered house name");
        Check(SpeechText::SignCaption("Bad\x05", "", false, glyph).empty() &&
              SpeechText::SignCaption("Bad\x1e\x00\x02", "", false, glyph).empty(), "invalid raw caption did not fail closed");
        Check(SpeechText::HighlightedName("House of the Great \x05" "AMido\x05@\x02") == "Mido" &&
              SpeechText::HighlightedName("My name is \x05" "AMalon\x05@!\x02") == "Malon" &&
              SpeechText::HighlightedName("\x05" "AMido\x05@ and \x05" "ASaria\x05@\x02").empty() &&
              SpeechText::HighlightedName("\x05" "AMido\x01\x05@\x02").empty(),
              "NPC name parser accepted ambiguous or uncontrolled text");
        // Only this test's explicit collider setup may change the player.
        Player after = player; after.cylinder = untouched.cylinder;
        Check(std::memcmp(&untouched, &after, sizeof(player)) == 0, "route queries changed Link's state");
        Check(std::abs(BeepInterval(0) - 0.15f) < 1e-6f && std::abs(BeepInterval(200) - 1.5f) < 1e-6f &&
              BeepInterval(100) < BeepInterval(150) && BeepInterval(1000) == BeepInterval(200) &&
              BeepInterval(NAN) == 0, "distance cadence disagrees with Time Stranger");
        // Default boot rows and normal gait consumers in z_player_lib/z_player.
        // Compare actual distance per native footfall, not raw coordinate counts.
        const WalkingStride childStride{5.5f, 3.7f, 0.5f, 0.4f, 0.8f, 0.4f, 1.5f};
        const WalkingStride adultStride{6, 3.7f, 0.55f, 0.27f, 0.6f, 0.35f, 1.5f};
        Check(std::abs(childStride.Length() - 41.53646f) < 0.001f &&
              std::abs(adultStride.Length() - 43.39152f) < 0.001f, "step distance disagrees with native footfalls");
        for (auto stride : {childStride, adultStride}) {
            Check(stride.Estimate(0) == 0 && stride.Estimate(35) == 0 && stride.Estimate(50) == 1 &&
                  stride.Estimate(100) == 2 && stride.Estimate(420) == 10, "menu step estimates use the wrong scale");
            Check(stride.Estimate(NAN) == -1 && stride.Estimate(INFINITY) == -1 && stride.Estimate(-1) == -1 &&
                  stride.Estimate(std::numeric_limits<float>::max()) == -1, "invalid distance did not fail closed");
            const float original = stride.Length();
            stride.updateScale = 1;
            Check(std::abs(stride.Length() - original) < 0.001f, "step unit changes with normal update rate");
            stride.speed = 0;
            Check(stride.Estimate(35) == -1, "uninitialized movement data produced a distance");
        }
        Vec3s vertices[]{{0, 0, 0}, {60, 0, 0}, {60, 0, 60}, {0, 0, 60},
                        {200, 0, 0}, {260, 0, 0}, {260, 0, 60}, {90, 0, 30}};
        CollisionPoly surfaces[5]{};
        const int indices[][3]{{0, 1, 2}, {0, 2, 3}, {4, 5, 6}, {1, 2, 7}, {4, 6, 7}};
        for (int i = 0; i < 5; ++i) {
            surfaces[i].type = i == 3 ? 2 : 1;
            surfaces[i].flags_vIA = indices[i][0]; surfaces[i].flags_vIB = indices[i][1]; surfaces[i].vIC = indices[i][2];
        }
        surfaces[4].flags_vIA |= 0x4000;
        CollisionHeader geometry{};
        geometry.vtxList = vertices; geometry.numVertices = std::size(vertices);
        geometry.polyList = surfaces; geometry.numPolygons = std::size(surfaces);
        const auto groups = GroupSurfaces(&geometry, [](CollisionPoly* p) { return p->type; });
        Check(groups.size() == 3, "connected entrance triangles duplicated, distinct exits merged, or excluded surface exposed");
        Check(std::count_if(groups.begin(), groups.end(), [](const SurfaceGroup& group) { return group.polygons.size() == 2; }) == 1,
              "entrance grouping lost its connected component");
        std::cout << "Navigation collision and route checks passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
