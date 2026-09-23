#include "SceneGraph.h"
#include "global.h"
#include <map>

namespace Navigation {
namespace {
Point Mix(Point a, Point b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }
using Vertex = std::tuple<int, int, int>;
Vertex Key(Point p) { return {int(p.x), int(p.y), int(p.z)}; }
}
void SceneGraph::Clear() { header = nullptr; floors.clear(); anchorCount = 0; }
void SceneGraph::Build(PlayState* play, float radius) {
    Clear();
    if (!play || !play->colCtx.colHeader || radius <= 0) return;
    header = play->colCtx.colHeader;
    floors.resize(header->numPolygons);
    struct Edge { size_t floor; Point a, b, center; };
    std::map<std::pair<Vertex, Vertex>, std::vector<Edge>> edges;
    std::vector<Point> centers(header->numPolygons);
    const auto supportedAnchor = [&](Point point, Point outward, Probe probe) {
        Vec3f ray{point.x, point.y + 18, point.z};
        CollisionPoly* floor = nullptr;
        s32 owner = BGCHECK_SCENE;
        const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &owner, &ray);
        if (!floor || owner != BGCHECK_SCENE || std::abs(y - point.y) > 18) return;
        const auto index = floor - header->polyList;
        if (index >= 0 && size_t(index) < floors.size()) floors[index].push_back({{point.x, y, point.z}, outward, probe});
    };
    for (size_t i = 0; i < floors.size(); ++i) {
        const auto& poly = header->polyList[i];
        // StaticLookup_AddPoly's floor classification and native entity filter.
        if (poly.normal.y <= COLPOLY_SNORMAL(0.5f) || COLPOLY_VIA_FLAG_TEST(poly.flags_vIA, 2)) continue;
        const int indices[]{COLPOLY_VTX_INDEX(poly.flags_vIA), COLPOLY_VTX_INDEX(poly.flags_vIB), poly.vIC};
        Point vertices[3];
        bool valid = true;
        for (int j = 0; j < 3; ++j) {
            if (indices[j] < 0 || indices[j] >= header->numVertices) { valid = false; break; }
            const auto& v = header->vtxList[indices[j]];
            vertices[j] = {float(v.x), float(v.y), float(v.z)};
        }
        if (!valid) continue;
        Point center{(vertices[0].x + vertices[1].x + vertices[2].x) / 3,
                     (vertices[0].y + vertices[1].y + vertices[2].y) / 3,
                     (vertices[0].z + vertices[1].z + vertices[2].z) / 3};
        centers[i] = center;
        floors[i].push_back({center, {}});
        // Subdivide only large triangles into fixed candidate points. Search
        // must not manufacture endlessly different partial long-edge positions.
        std::vector<std::array<Point, 3>> pending{{vertices[0], vertices[1], vertices[2]}};
        while (!pending.empty()) {
            const auto triangle = pending.back(); pending.pop_back();
            int longest = 0;
            for (int j = 1; j < 3; ++j)
                if (DistanceXZ(triangle[j], triangle[(j + 1) % 3]) > DistanceXZ(triangle[longest], triangle[(longest + 1) % 3])) longest = j;
            if (DistanceXZ(triangle[longest], triangle[(longest + 1) % 3]) > 600) {
                const auto a = triangle[longest], b = triangle[(longest + 1) % 3], c = triangle[(longest + 2) % 3];
                const auto middle = Mix(a, b, 0.5f);
                pending.push_back({a, middle, c}); pending.push_back({middle, b, c});
            } else floors[i].push_back({Mix(Mix(triangle[0], triangle[1], 0.5f), triangle[2], 1.0f / 3), {}});
        }
        for (int j = 0; j < 3; ++j) {
            Point a = vertices[j], b = vertices[(j + 1) % 3];
            auto ka = Key(a), kb = Key(b);
            if (kb < ka) std::swap(ka, kb);
            edges[{ka, kb}].push_back({i, a, b, center});
        }
    }
    for (const auto& [key, joined] : edges) for (const auto& edge : joined) {
        const float length = DistanceXZ(edge.a, edge.b);
        if (length < 1) continue;
        if (joined.size() > 1) {
            for (const auto& other : joined) if (other.floor != edge.floor)
                floors[edge.floor].push_back({centers[other.floor], {}});
            for (float t : {0.2f, 0.5f, 0.8f}) floors[edge.floor].push_back({Mix(edge.a, edge.b, t), {}});
        } else {
            Point outward{(edge.b.z - edge.a.z) / length, 0, -(edge.b.x - edge.a.x) / length};
            const Point middle = Mix(edge.a, edge.b, 0.5f);
            if ((edge.center.x - middle.x) * outward.x + (edge.center.z - middle.z) * outward.z > 0) {
                outward.x = -outward.x; outward.z = -outward.z;
            }
            const int count = std::max(1, int(std::ceil(length / 64)));
            for (int sample = 0; sample < count; ++sample) {
                Point point = Mix(edge.a, edge.b, (sample + 0.5f) / count);
                point.x -= outward.x * (radius + 1); point.z -= outward.z * (radius + 1);
                const auto& poly = header->polyList[edge.floor];
                // Plane height at the inset foot point; the following native walk
                // resolves actual support instead of trusting this candidate.
                point.y = -(COLPOLY_GET_NORMAL(poly.normal.x) * point.x + COLPOLY_GET_NORMAL(poly.normal.z) * point.z + poly.dist) /
                           COLPOLY_GET_NORMAL(poly.normal.y);
                floors[edge.floor].push_back({point, {}});
                supportedAnchor(point, outward, Probe::Boundary);
            }
        }
    }
    // Wall/ledge and ladder approaches can lie inside a floor triangle, away
    // from its boundary. Derive them from native vertical wall geometry too.
    for (size_t i = 0; i < floors.size(); ++i) {
        auto* poly = &header->polyList[i];
        if (std::abs(poly->normal.y) >= 600 || COLPOLY_VIA_FLAG_TEST(poly->flags_vIA, 2)) continue;
        const auto flags = func_80041DB8(&play->colCtx, poly, BGCHECK_SCENE);
        if ((flags & 1) && !(flags & (2 | 4 | 8))) continue;
        Point low{INFINITY, INFINITY, INFINITY}, high{-INFINITY, -INFINITY, -INFINITY};
        std::array<Point, 3> vertices;
        int vertex = 0;
        for (int index : {COLPOLY_VTX_INDEX(poly->flags_vIA), COLPOLY_VTX_INDEX(poly->flags_vIB), int(poly->vIC)}) {
            if (index >= header->numVertices) continue;
            const auto& v = header->vtxList[index];
            vertices[vertex++] = {float(v.x), float(v.y), float(v.z)};
            low.x = std::min(low.x, float(v.x)); low.y = std::min(low.y, float(v.y)); low.z = std::min(low.z, float(v.z));
            high.x = std::max(high.x, float(v.x)); high.y = std::max(high.y, float(v.y)); high.z = std::max(high.z, float(v.z));
        }
        if (vertex != 3) continue;
        const Point normal{COLPOLY_GET_NORMAL(poly->normal.x), 0, COLPOLY_GET_NORMAL(poly->normal.z)};
        Point a = vertices[0], b = vertices[1];
        for (int j = 0; j < 3; ++j) for (int k = j + 1; k < 3; ++k)
            if (DistanceXZ(vertices[j], vertices[k]) > DistanceXZ(a, b)) { a = vertices[j]; b = vertices[k]; }
        const int count = (flags & (2 | 4)) ? 1 : std::max(1, int(std::ceil(DistanceXZ(a, b) / 32)));
        for (int sample = 0; sample < count; ++sample) for (float level : {low.y, high.y}) {
            const Point point = Mix(a, b, (sample + 0.5f) / count);
            Vec3f ray{point.x + normal.x * (radius + 1), level + 18, point.z + normal.z * (radius + 1)};
            CollisionPoly* floor = nullptr;
            s32 owner = BGCHECK_SCENE;
            const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &floor, &owner, &ray);
            if (floor && owner == BGCHECK_SCENE && y > BGCHECK_Y_MIN)
                supportedAnchor({ray.x, y, ray.z}, {-normal.x, 0, -normal.z}, (flags & (2 | 4 | 8)) ? Probe::Ladder : Probe::Ledge);
        }
    }
    for (auto& floor : floors) {
        std::sort(floor.begin(), floor.end(), [](const Anchor& a, const Anchor& b) {
            return std::tie(a.position.x, a.position.y, a.position.z, a.outward.x, a.outward.z, a.probe) <
                   std::tie(b.position.x, b.position.y, b.position.z, b.outward.x, b.outward.z, b.probe);
        });
        floor.erase(std::unique(floor.begin(), floor.end(), [](const Anchor& a, const Anchor& b) {
            return Distance(a.position, b.position) < 0.01f && Distance(a.outward, b.outward) < 0.01f && a.probe == b.probe;
        }), floor.end());
        anchorCount += floor.size();
    }
}
void SceneGraph::Expand(PlayState* play, const WalkingQuery& query, Point from, Point target,
                        float goalRadius, std::vector<Connection>& out) const {
    if (!play || header != play->colCtx.colHeader) return;
    const auto add = [&](Point candidate) {
        const float distance = DistanceXZ(from, candidate);
        if (distance < 0.5f) return;
        if (distance > 800) return;
        // Lazy A*: only collision-check candidates that reach the front of the
        // search queue. Every accepted path edge still receives the full check.
        out.push_back({candidate, {}, true});
    };
    if (std::abs(from.y - target.y) <= 18 || DistanceXZ(from, target) <= goalRadius * 2) add(target);
    if (DistanceXZ(from, target) <= goalRadius * 2 ||
        (DistanceXZ(from, target) <= 800 && std::abs(from.y - target.y) <= 18)) {
        for (int i = 0; i < 16; ++i) for (float scale : {0.5f, 0.75f, 0.95f}) {
            const float angle = i * 6.28318530718f / 16;
            add({target.x + std::sin(angle) * goalRadius * scale, target.y,
                 target.z + std::cos(angle) * goalRadius * scale});
        }
    }
    Vec3f ray{from.x, from.y + 18, from.z};
    CollisionPoly* poly = nullptr;
    s32 owner = BGCHECK_SCENE;
    const float y = BgCheck_EntityRaycastFloor3(&play->colCtx, &poly, &owner, &ray);
    if (!poly || owner != BGCHECK_SCENE || std::abs(y - from.y) > 18) return;
    const auto index = poly - header->polyList;
    if (index < 0 || size_t(index) >= floors.size()) return;
    for (const auto& anchor : floors[index]) {
        add(anchor.position);
        if ((anchor.outward.x || anchor.outward.z) && DistanceXZ(from, anchor.position) < 2 && std::abs(from.y - anchor.position.y) < 4) {
            JumpLink link;
            const Point toward{from.x + anchor.outward.x * 28, from.y, from.z + anchor.outward.z * 28};
            if (anchor.probe == Probe::Ladder) {
                if (query.Ladder(from, toward, link)) out.push_back({link.landing, link});
                continue;
            }
            if (anchor.probe == Probe::Ledge) {
                if (query.Climb(from, toward, link)) out.push_back({link.landing, link});
                continue;
            }
            Point next;
            if (query.Walk(from, toward, next)) out.push_back({next, {}});
            else {
                if (query.Traverse(from, toward, link)) out.push_back({link.landing, link});
                for (int step : {-3, -2, -1, 1, 2, 3}) {
                    const float angle = step * 0.3926990817f;
                    const float x = anchor.outward.x * std::cos(angle) - anchor.outward.z * std::sin(angle);
                    const float z = anchor.outward.x * std::sin(angle) + anchor.outward.z * std::cos(angle);
                    if (query.Jump(from, {from.x + x * 28, from.y, from.z + z * 28}, link)) out.push_back({link.landing, link});
                }
            }
        }
    }
}
}
