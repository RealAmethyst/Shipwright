#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <queue>
#include <tuple>
#include <vector>

namespace Navigation {
struct Point { float x = 0, y = 0, z = 0; };
inline bool Finite(Point p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
inline float Distance(Point a, Point b) { return std::hypot(std::hypot(a.x - b.x, a.z - b.z), a.y - b.y); }
inline float DistanceXZ(Point a, Point b) { return std::hypot(a.x - b.x, a.z - b.z); }

// Time Stranger's cadence, expressed in the host game's native distance units.
inline float BeepInterval(float distance) {
    return std::isfinite(distance) && distance >= 0 ? 0.15f + std::min(distance / 200.0f, 1.0f) * 1.35f : 0;
}

enum class SearchState { Idle, Searching, Found, Unreachable, Limit, InvalidStart };
enum class Traversal { Jump, Ledge, LadderUp, LadderDown };
inline bool IsLadder(Traversal kind) { return kind == Traversal::LadderUp || kind == Traversal::LadderDown; }
struct JumpLink {
    Point runup, takeoff, landing;
    Traversal kind = Traversal::Jump;
    Point probeFrom{}, probeToward{};
};
struct Connection { Point position; std::optional<JumpLink> traversal; bool checkWalk = false; };
class RouteSearch {
  public:
    using Walk = std::function<bool(Point, Point, Point&)>;
    using Goal = std::function<bool(Point)>;
    using Jump = std::function<bool(Point, Point, JumpLink&)>;
    using Expand = std::function<void(Point, std::vector<Connection>&)>;
    using Support = std::function<bool(Point, Point&)>;
    void Begin(Point start, Point destination, float radius, float spacing, Walk walk, Goal goal, Jump jump = {}, Expand expand = {}, Support support = {});
    void Step(size_t expansions);
    void Reset();
    SearchState State() const { return state; }
    const std::vector<Point>& Path() const { return path; }
    const std::map<size_t, JumpLink>& Jumps() const { return jumps; }
    size_t Visited() const { return nodes.size(); }
    Point Closest() const { return closest; }
    static constexpr size_t MaxNodes = 131072;

  private:
    using Key = std::tuple<int, int, int>;
    struct Node { Point position; float cost; size_t parent; bool closed = false; std::optional<JumpLink> jump; };
    struct Candidate {
        float score, cost;
        size_t node, parent = 0;
        Point position;
        std::optional<JumpLink> traversal;
        bool checkWalk = false;
        bool operator<(const Candidate& other) const {
            return score != other.score ? score > other.score : node > other.node;
        }
    };
    SearchState state = SearchState::Idle;
    Point origin{}, target{}, closest{};
    float radius = 0, spacing = 16;
    Walk walk;
    Goal goal;
    Jump jump;
    Expand expand;
    Support support;
    std::vector<Node> nodes;
    std::map<Key, size_t> indices;
    std::priority_queue<Candidate> open;
    std::vector<Point> path;
    std::map<size_t, JumpLink> jumps;
    Key GridKey(Point p) const;
    float Heuristic(Point p) const;
    void Finish(size_t node);
    void Connect(size_t parent, const Connection& connection);
};
}
