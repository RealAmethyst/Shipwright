#pragma once
#include "WalkingQuery.h"

#include "global.h"
namespace Navigation {
inline bool PreferSceneGraph(Point from, Point to) { return DistanceXZ(from, to) > WalkingQuery::MaxWalkDistance * 2; }
// Candidate connections come from the loaded native floor triangles. Native
// collision still validates every edge, including dynamic geometry and actors.
class SceneGraph {
  public:
    void Build(PlayState* play, float radius);
    void Clear();
    bool Ready() const { return header != nullptr; }
    void Expand(PlayState* play, const WalkingQuery& query, Point from, Point target,
                float goalRadius, std::vector<Connection>& out) const;
    size_t Anchors() const { return anchorCount; }
  private:
    enum class Probe { None, Boundary, Ledge, Ladder };
    struct Anchor { Point position, outward; Probe probe = Probe::None; };
    const CollisionHeader* header = nullptr;
    std::vector<std::vector<Anchor>> floors;
    size_t anchorCount = 0;
};
}
