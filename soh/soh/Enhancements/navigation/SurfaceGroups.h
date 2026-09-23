#pragma once
#include "RouteSearch.h"
#include "global.h"
#include <functional>
#include <numeric>

namespace Navigation {
struct SurfaceGroup { int tag; Point position; std::vector<size_t> polygons; };
// Connected native surfaces form one entrance/ladder. Equal names or nearby
// centres are not identities: two separate doors may lead to the same scene.
inline std::vector<SurfaceGroup> GroupSurfaces(CollisionHeader* header,
                                             const std::function<int(CollisionPoly*)>& classify) {
    std::vector<SurfaceGroup> result;
    if (!header || !header->polyList || !header->vtxList) return result;
    std::vector<size_t> parents(header->numPolygons);
    std::iota(parents.begin(), parents.end(), 0);
    const auto root = [&](size_t i) { while (parents[i] != i) i = parents[i]; return i; };
    std::map<std::tuple<int, int, int>, size_t> edges;
    std::map<size_t, int> tags;
    for (size_t i = 0; i < header->numPolygons; ++i) {
        auto* poly = &header->polyList[i];
        // Same entity exclusion flag used by native floor/wall queries.
        if (COLPOLY_VIA_FLAG_TEST(poly->flags_vIA, 2)) continue;
        const int tag = classify(poly);
        if (!tag) continue;
        const int v[]{COLPOLY_VTX_INDEX(poly->flags_vIA), COLPOLY_VTX_INDEX(poly->flags_vIB), poly->vIC};
        if (std::any_of(std::begin(v), std::end(v), [&](int index) { return index >= header->numVertices; })) continue;
        tags[i] = tag;
        for (int edge = 0; edge < 3; ++edge) {
            const auto key = std::tuple{tag, std::min(v[edge], v[(edge + 1) % 3]), std::max(v[edge], v[(edge + 1) % 3])};
            if (const auto old = edges.find(key); old != edges.end()) parents[root(i)] = root(old->second);
            else edges[key] = i;
        }
    }
    std::map<size_t, std::vector<size_t>> groups;
    for (const auto& [index, tag] : tags) groups[root(index)].push_back(index);
    for (auto& [parent, members] : groups) {
        std::vector<Point> centres;
        Point centre{};
        for (auto index : members) {
            const auto& poly = header->polyList[index];
            Point p{};
            for (int vertex : {COLPOLY_VTX_INDEX(poly.flags_vIA), COLPOLY_VTX_INDEX(poly.flags_vIB), int(poly.vIC)}) {
                const auto& v = header->vtxList[vertex]; p.x += v.x / 3.0f; p.y += v.y / 3.0f; p.z += v.z / 3.0f;
            }
            centres.push_back(p); centre.x += p.x; centre.y += p.y; centre.z += p.z;
        }
        centre.x /= centres.size(); centre.y /= centres.size(); centre.z /= centres.size();
        // Keep the endpoint on a real triangle even for concave components.
        const auto closest = std::min_element(centres.begin(), centres.end(), [&](Point a, Point b) {
            return Distance(a, centre) < Distance(b, centre);
        });
        result.push_back({tags.at(parent), *closest, std::move(members)});
    }
    return result;
}
}
