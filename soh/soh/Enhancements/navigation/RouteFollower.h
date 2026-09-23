#pragma once
#include "TraversalProgress.h"
#include "WalkingQuery.h"

namespace Navigation {
enum class FollowState { Guiding, Waiting, Replan };
struct FollowResult { FollowState state; Point cue{}; const char* reason = ""; };
class RouteFollower {
  public:
    void Reset() { route.clear(); jumps.clear(); waypoint = 0; bypassed = 0; progress.Reset(); }
    void Set(const RouteSearch& search) { route = search.Path(); jumps = search.Jumps(); waypoint = 0; bypassed = 0; progress.Reset(); }
    bool HasRoute() const { return !route.empty(); }
    const JumpLink* Planned(Point position, float radius) const {
        if (auto link = jumps.find(waypoint); link != jumps.end() && bypassed != waypoint) return &link->second;
        if (auto link = jumps.find(waypoint + 1); link != jumps.end() &&
            DistanceXZ(position, link->second.runup) <= radius * 2) return &link->second;
        return nullptr;
    }
    bool Rejoin(Point position, const WalkingQuery& query) {
        std::vector<std::pair<float, size_t>> candidates;
        for (size_t i = 0; i < route.size(); ++i) {
            const float distance = Distance(position, route[i]);
            if (distance <= 800) candidates.emplace_back(distance, i);
        }
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first < b.first : a.second > b.second;
        });
        for (size_t i = 0; i < candidates.size() && i < 16; ++i) {
            const size_t at = candidates[i].second;
            if (!query.Segment(position, route[at])) continue;
            waypoint = at;
            // A fresh walking connection can rejoin beyond a traversal that
            // Link has already passed on his own. It cannot cross a gap/wall.
            bypassed = at;
            progress.Reset();
            return true;
        }
        return false;
    }
    FollowResult Update(Point position, bool grounded, bool ledge, bool ladder, const WalkingQuery& query) {
        if (route.empty()) return {FollowState::Replan, {}, "empty route"};
        if (const auto next = jumps.find(waypoint + 1); next != jumps.end()) {
            const bool animation = TraversalAnimation(next->second.kind, ledge, ladder);
            const bool earlyJump = !grounded && next->second.kind == Traversal::Jump &&
                                   DistanceXZ(position, next->second.takeoff) <= query.Radius() * 3;
            if ((animation && DistanceXZ(position, next->second.runup) <= query.Radius() * 2) || earlyJump) ++waypoint;
        }
        if (const auto jump = jumps.find(waypoint); jump != jumps.end() && bypassed != waypoint) {
            const auto& link = jump->second;
            const auto phase = progress.Update(link.kind, grounded, ledge, ladder);
            if (phase == TraversalPhase::Landed) {
                progress.Reset(); bypassed = waypoint;
                if (!Rejoin(position, query)) return {FollowState::Replan, {}, "landing disconnected"};
            } else {
                if (phase == TraversalPhase::Approach) {
                    const float length = DistanceXZ(link.runup, link.takeoff);
                    const float cross = std::abs((position.x - link.runup.x) * (link.takeoff.z - link.runup.z) -
                                                (position.z - link.runup.z) * (link.takeoff.x - link.runup.x));
                    const bool offCourse = link.kind != Traversal::Jump ? Distance(position, link.runup) > query.Radius() * 2 :
                        length <= 0 || cross > length * query.Radius() || !query.Segment(position, link.takeoff);
                    if (offCourse) {
                        if (query.Segment(position, link.runup)) return {FollowState::Guiding, link.runup};
                        return {FollowState::Replan, {}, "traversal approach disconnected"};
                    }
                }
                return {FollowState::Guiding, link.landing};
            }
        }
        if (!grounded || ledge || ladder) return {FollowState::Waiting};
        size_t last = std::min(route.size() - 1, waypoint + 12);
        if (const auto link = jumps.upper_bound(waypoint); link != jumps.end()) last = std::min(last, link->first - 1);
        // Check the farthest useful point first; do not repeat every shorter
        // collision segment on every update while Link walks along a clear path.
        bool connected = false;
        for (size_t i = last; i > waypoint; --i) {
            if (Distance(position, route[i]) <= 180 && query.Segment(position, route[i])) {
                waypoint = i; connected = true; break;
            }
        }
        if (!connected && !query.Segment(position, route[waypoint]) && !Rejoin(position, query))
            return {FollowState::Replan, {}, "walking corridor disconnected"};
        if (Distance(position, route[waypoint]) < 10 && waypoint + 1 < route.size()) {
            if (const auto jump = jumps.find(waypoint + 1); jump != jumps.end()) {
                const auto& link = jump->second;
                JumpLink fresh;
                if (!query.Revalidate(link, fresh)) return {FollowState::Replan, {}, "traversal changed"};
            } else if (!query.Segment(position, route[waypoint + 1])) {
                if (query.Segment(route[waypoint], route[waypoint + 1]))
                    return {FollowState::Guiding, route[waypoint]}; // Finish rounding this corner.
                return {FollowState::Replan, {}, "next corridor changed"};
            }
            ++waypoint;
        }
        return {FollowState::Guiding, route[waypoint]};
    }
  private:
    std::vector<Point> route;
    std::map<size_t, JumpLink> jumps;
    size_t waypoint = 0;
    size_t bypassed = 0;
    TraversalProgress progress;
};
}
