#pragma once
#include "RouteSearch.h"
#include "JumpMotion.h"
#include <array>
struct PlayState;
struct Actor;
namespace Navigation {
enum class QueryFailure { Floor, Hazard, Water, Obstacle, Bounds, Wall, Ceiling, JumpState, Edge,
                          JumpFloor, Runway, EarlyArc, LateArc, Landing, DepartureFloor, RisingFloor, LandingSupport,
                          Fall, Count };
struct QueryStats {
    std::array<size_t, static_cast<size_t>(QueryFailure::Count)> rejected{};
    size_t walks = 0, bodies = 0, traversals = 0;
    Point wallFrom{}, wallTo{}, wallNormal{};
    Point departure{}, departureFloor{};
};
// Read-only native collision adapter. It never runs actor/player movement code.
class WalkingQuery {
  public:
    explicit WalkingQuery(PlayState* play, JumpMotion motion = {}, QueryStats* diagnostics = nullptr, bool ledgeGrabs = true);
    bool Walk(Point from, Point requested, Point& result) const;
    bool Support(Point from, Point& result) const;
    bool Segment(Point from, Point to) const;
    bool Jump(Point from, Point toward, JumpLink& result) const;
    bool Traverse(Point from, Point toward, JumpLink& result) const;
    bool Climb(Point from, Point toward, JumpLink& result) const;
    bool Ladder(Point from, Point toward, JumpLink& result) const;
    bool Revalidate(const JumpLink& link, JumpLink& result) const;
    static constexpr float MaxWalkDistance = 800;
    bool Approach(Point from, Point target, float radius, const Actor* targetActor = nullptr,
                  float heightTolerance = 30) const;
    float Radius() const { return radius; }
  private:
    PlayState* play;
    float radius = 0, height = 0;
    float objectRadius = 0, objectHeight = 0, objectShift = 0;
    JumpMotion motion;
    QueryStats* diagnostics;
    bool ledgeGrabs;
    bool Reject(QueryFailure reason) const;
    bool Floor(Point& point, float previousY) const;
    bool ClearBody(Point from, Point to, float wallRadius = 0, Point* floorCorrection = nullptr) const;
    bool Arc(Point takeoff, Point direction, float phase, Point& landing) const;
};
void ClearObstacles();
}
