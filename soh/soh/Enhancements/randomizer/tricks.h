#pragma once

#ifndef TRICKS_H
#define TRICKS_H

#include "randomizerTypes.h"

#include <map>
#include <set>
#include <string>

namespace Rando {
class Tricks {
  public:
    enum class Tag {
        NOVICE,
        INTERMEDIATE,
        ADVANCED,
        EXPERT,
        EXTREME,
        EXPERIMENTAL,
        GLITCH,
    };

    static const std::string& GetAreaName(RandomizerArea area);
    static bool CheckTags(const std::map<Tag, bool>& showTag, const std::set<Tag>& rtTags);
    static const std::string GetTagName(Tag tag);
};
} // namespace Rando

#endif // TRICKS_H
