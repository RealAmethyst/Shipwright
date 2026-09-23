#pragma once
#include "RouteSearch.h"
#include <cstdint>
#include <string>
struct Actor;
struct PlayState;
namespace Navigation {
class WalkingQuery;
enum class Category { People, Items, Chests, Doors, Exits, Switches, Signs, Destructibles, Climbing, Platforms, Landmarks, Count };
struct Target {
    uint64_t identity = 0;
    Actor* actor = nullptr;
    Category category = Category::Items;
    Point position{};
    std::string name;
    float radius = 45;
    bool loosePickup = false;
};
std::string Text(const std::string& key);
std::string Format(const std::string& key, const std::string& value);
std::string CategoryName(Category category);
void InitTargets();
void ClearTargets();
std::vector<Target> CollectTargets(PlayState* play);
bool CanApproach(const Target& target, Point point, const WalkingQuery& query);
}
