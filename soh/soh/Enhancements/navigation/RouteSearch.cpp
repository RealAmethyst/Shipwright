#include "RouteSearch.h"
#include <algorithm>
#include <limits>

namespace Navigation {
void RouteSearch::Reset() {
    state = SearchState::Idle;
    nodes.clear(); indices.clear(); open = {}; path.clear();
    walk = {}; goal = {}; jump = {}; expand = {}; support = {}; jumps.clear();
}
RouteSearch::Key RouteSearch::GridKey(Point p) const {
    const float cell = expand ? 0.25f : spacing;
    return {static_cast<int>(std::lround((p.x - origin.x) / cell)),
            static_cast<int>(std::lround(p.y / 4)),
            static_cast<int>(std::lround((p.z - origin.z) / cell))};
}
float RouteSearch::Heuristic(Point p) const { return std::max(0.0f, Distance(p, target) - radius); }
void RouteSearch::Begin(Point start, Point destination, float goalRadius, float gridSpacing, Walk test, Goal reached, Jump leap, Expand neighbours, Support floorSupport) {
    Reset();
    if (!Finite(start) || !Finite(destination) || !std::isfinite(goalRadius) || goalRadius < 0 ||
        goalRadius > 10000 || !std::isfinite(gridSpacing) || gridSpacing < 4 || gridSpacing > 1000 || !test || !reached ||
        std::abs(start.x) > 100000 || std::abs(start.y) > 100000 || std::abs(start.z) > 100000 ||
        std::abs(destination.x) > 100000 || std::abs(destination.y) > 100000 || std::abs(destination.z) > 100000 ||
        Distance(start, destination) > 100000) {
        state = SearchState::Unreachable;
        return;
    }
    walk = std::move(test); goal = std::move(reached); jump = std::move(leap); expand = std::move(neighbours);
    support = std::move(floorSupport);
    origin = closest = start; target = destination; radius = goalRadius; spacing = gridSpacing;
    Point supported;
    if (!(support ? support(start, supported) : walk(start, start, supported))) { state = SearchState::InvalidStart; return; }
    origin = supported;
    nodes.push_back({origin, 0, 0});
    indices[GridKey(origin)] = 0;
    open.push({Heuristic(origin), 0, 0, 0, origin});
    state = SearchState::Searching;
}
void RouteSearch::Finish(size_t node) {
    std::vector<size_t> chain;
    for (;;) {
        chain.push_back(node);
        if (node == 0) break;
        node = nodes[node].parent;
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        const auto& entry = nodes[*it];
        if (entry.jump) {
            path.push_back(entry.jump->runup);
            jumps[path.size()] = *entry.jump;
        }
        path.push_back(entry.position);
    }
    state = SearchState::Found;
}
void RouteSearch::Step(size_t expansions) {
    if (state != SearchState::Searching) return;
    while (expansions-- && !open.empty()) {
        const auto current = open.top(); open.pop();
        if (nodes[current.node].closed) continue;
        Point supported = current.position;
        if (current.checkWalk) {
            if (!walk(nodes[current.parent].position, current.position, supported)) {
                JumpLink link;
                if (!expand && jump && DistanceXZ(nodes[current.parent].position, current.position) <= 40 &&
                    jump(nodes[current.parent].position, current.position, link))
                    Connect(current.parent, {link.landing, link});
                if (state != SearchState::Searching) return;
                continue;
            }
        }
        nodes[current.node].position = supported;
        nodes[current.node].parent = current.parent;
        nodes[current.node].jump = current.traversal;
        nodes[current.node].cost = current.cost;
        nodes[current.node].closed = true;
        // Copy before adding nodes: vector growth invalidates references.
        const auto node = nodes[current.node];
        if (Heuristic(node.position) < Heuristic(closest)) closest = node.position;
        if (Distance(node.position, target) <= radius && goal(node.position)) { Finish(current.node); return; }
        std::vector<Connection> connections;
        if (expand) expand(node.position, connections);
        else for (int x = -1; x <= 1; ++x) for (int z = -1; z <= 1; ++z) {
            if (!x && !z) continue;
            const Point requested{node.position.x + x * spacing, node.position.y, node.position.z + z * spacing};
            Point next;
            std::optional<JumpLink> link;
            if (!(support ? support(requested, next) : walk(node.position, requested, next))) {
                JumpLink result;
                if (!jump || !jump(node.position, requested, result)) continue;
                link = result; next = result.landing;
            }
            connections.push_back({next, link, !link && bool(support)});
        }
        for (const auto& connection : connections) {
            Connect(current.node, connection);
            if (state != SearchState::Searching) return;
        }
    }
    if (open.empty()) state = SearchState::Unreachable;
}
void RouteSearch::Connect(size_t parent, const Connection& connection) {
    const Point next = connection.position;
    const auto& link = connection.traversal;
    if (!Finite(next)) return;
    const auto node = nodes[parent];
    const float edgeCost = link ? Distance(node.position, link->runup) + Distance(link->runup, next) : Distance(node.position, next);
    const float cost = node.cost + edgeCost;
    const auto key = GridKey(next);
    auto found = indices.find(key);
    size_t index;
    if (found == indices.end()) {
        if (nodes.size() >= MaxNodes) { state = SearchState::Limit; path.clear(); return; }
        index = nodes.size();
        nodes.push_back({next, std::numeric_limits<float>::infinity(), parent});
        indices.emplace(key, index);
    } else {
        index = found->second;
        if (nodes[index].closed) return;
    }
    // Weighted A* avoids flooding a large open field with equivalent alternatives.
    // Every edge still passes the same native collision checks.
    // Keep alternate incoming edges until one validates. A blocked
    // optimistic edge must not hide a valid approach from another side.
    if (open.size() >= MaxNodes * 4) { state = SearchState::Limit; path.clear(); return; }
    open.push({cost + Heuristic(next) * 4.0f, cost, index, parent, next, link, connection.checkWalk});
}
}
