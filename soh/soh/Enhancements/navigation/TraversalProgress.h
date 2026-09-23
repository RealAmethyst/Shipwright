#pragma once
#include "RouteSearch.h"

namespace Navigation {
enum class TraversalPhase { Approach, Moving, Landed };
inline bool TraversalAnimation(Traversal kind, bool ledge, bool ladder) {
    return (kind == Traversal::Ledge && ledge) || (IsLadder(kind) && (ladder || ledge));
}
class TraversalProgress {
  public:
    void Reset() { entered = false; }
    TraversalPhase Update(Traversal kind, bool grounded, bool ledge, bool ladder) {
        if (!grounded || TraversalAnimation(kind, ledge, ladder)) {
            entered = true;
            return TraversalPhase::Moving;
        }
        return entered ? TraversalPhase::Landed : TraversalPhase::Approach;
    }
  private:
    bool entered = false;
};
}
