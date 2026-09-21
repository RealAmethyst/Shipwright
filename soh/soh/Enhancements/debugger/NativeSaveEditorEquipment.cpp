#include "NativeSaveEditor.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/util.h"

extern "C" {
#include "global.h"
}

namespace NativeSaveEditor {
namespace {
N::Row Upgrade(const char* key, int category, std::map<int, std::string> names) {
    return N::Choice(key, Text(key), CUR_UPG_VALUE(category), std::move(names),
                     [category](int value) { Inventory_ChangeUpgrade(category, value); });
}

N::Row UpgradeItems(const char* key, int category, std::vector<int> items) {
    std::map<int, std::string> names;
    for (size_t i = 0; i < items.size(); ++i) names[i] = ItemName(items[i]);
    auto row = Upgrade(key, category, names);
    const size_t current = CUR_UPG_VALUE(category);
    if (current < items.size()) row.image = ItemImage(items[current]);
    return row;
}

N::PagePtr OwnedEquipmentPage() {
    return N::MakePage("save/owned_equipment", Text("equipment"), [] {
        const int items[] = {
            ITEM_SWORD_KOKIRI, ITEM_SWORD_MASTER, ITEM_SWORD_BGS, ITEM_SWORD_BROKEN,
            ITEM_SHIELD_DEKU, ITEM_SHIELD_HYLIAN, ITEM_SHIELD_MIRROR, ITEM_NONE,
            ITEM_TUNIC_KOKIRI, ITEM_TUNIC_GORON, ITEM_TUNIC_ZORA, ITEM_NONE,
            ITEM_BOOTS_KOKIRI, ITEM_BOOTS_IRON, ITEM_BOOTS_HOVER, ITEM_NONE};
        std::vector<N::Row> rows;
        for (int bit = 0; bit < std::size(items); ++bit) {
            if (items[bit] == ITEM_NONE) continue;
            auto row = Flag(std::to_string(bit), ItemName(items[bit]), gSaveContext.inventory.equipment, uint32_t{1} << bit);
            row.image = ItemImage(items[bit]);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

N::PagePtr DungeonPage() {
    auto scene = std::make_shared<int>(SCENE_DEKU_TREE);
    if (gPlayState && gPlayState->sceneNum >= SCENE_DEKU_TREE && gPlayState->sceneNum <= SCENE_JABU_JABU_BOSS)
        *scene = gPlayState->sceneNum;
    return N::MakePage("save/dungeon", Text("dungeon_items"), [scene] {
        std::vector<N::Row> rows;
        std::map<int, std::string> scenes;
        for (int i = SCENE_DEKU_TREE; i <= SCENE_JABU_JABU_BOSS && i < std::size(gSaveContext.inventory.dungeonItems); ++i)
            scenes[i] = SohUtils::GetSceneName(i);
        rows.push_back(N::Choice("scene", Text("map"), *scene, scenes, [scene](int value) { *scene = value; }));
        const int current = *scene;
        for (int item : {ITEM_KEY_BOSS, ITEM_COMPASS, ITEM_DUNGEON_MAP}) {
            auto row = Flag(std::to_string(item), ItemName(item), gSaveContext.inventory.dungeonItems[current],
                            uint32_t{1} << (item - ITEM_KEY_BOSS));
            row.image = ItemImage(item);
            rows.push_back(std::move(row));
        }
        if (current < std::size(gSaveContext.inventory.dungeonKeys)) {
            auto row = Scalar("keys", ItemName(ITEM_KEY_SMALL), gSaveContext.inventory.dungeonKeys[current], "", {}, [current] {
                gSaveContext.ship.stats.dungeonKeys[current] = gSaveContext.inventory.dungeonKeys[current];
            });
            row.image = ItemImage(ITEM_KEY_SMALL);
            rows.push_back(std::move(row));
        } else rows.push_back(N::Action("no_keys", Text("no_small_keys"), [] { N::ReadCurrentDescription(); }));
        return rows;
    });
}

N::PagePtr QuestItemsPage(bool songs) {
    return N::MakePage(songs ? "save/songs" : "save/quest_items", Text(songs ? "songs" : "quest_items"), [songs] {
        std::vector<N::Row> rows;
        if (songs) {
            for (const auto& [quest, entry] : songMapping)
                rows.push_back(Flag(std::to_string(entry.id), SohUtils::GetQuestItemName(entry.id),
                    gSaveContext.inventory.questItems, uint32_t{1} << entry.id));
        } else {
            for (const auto& [quest, entry] : questMapping) {
                if (quest == QUEST_SKULL_TOKEN) continue;
                auto row = Flag(std::to_string(entry.id), SohUtils::GetQuestItemName(entry.id),
                    gSaveContext.inventory.questItems, uint32_t{1} << entry.id);
                row.image = Image(entry.texturePath);
                rows.push_back(std::move(row));
            }
        }
        return rows;
    });
}
}

N::PagePtr EquipmentPage() {
    return N::MakePage("save/equipment", Text("equipment"), [] {
        std::vector<N::Row> rows{N::Link("owned", Text("equipment"), OwnedEquipmentPage)};
        rows.push_back(UpgradeItems("bullet_bag", UPG_BULLET_BAG,
                                   {ITEM_NONE, ITEM_BULLET_BAG_30, ITEM_BULLET_BAG_40, ITEM_BULLET_BAG_50}));
        rows.push_back(UpgradeItems("quiver", UPG_QUIVER, {ITEM_NONE, ITEM_QUIVER_30, ITEM_QUIVER_40, ITEM_QUIVER_50}));
        rows.push_back(UpgradeItems("bomb_bag", UPG_BOMB_BAG, {ITEM_NONE, ITEM_BOMB_BAG_20, ITEM_BOMB_BAG_30, ITEM_BOMB_BAG_40}));
        rows.push_back(UpgradeItems("scale", UPG_SCALE, {ITEM_NONE, ITEM_SCALE_SILVER, ITEM_SCALE_GOLDEN}));
        rows.push_back(UpgradeItems("strength", UPG_STRENGTH, {ITEM_NONE, ITEM_BRACELET, ITEM_GAUNTLETS_SILVER, ITEM_GAUNTLETS_GOLD}));
        std::map<int, std::string> wallets{{0, Text("wallet_child")}, {1, Text("wallet_adult")}, {2, Text("wallet_giant")}};
        if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_INCLUDE_TYCOON_WALLET))
            wallets[3] = Text("wallet_tycoon");
        rows.push_back(Upgrade("wallet", UPG_WALLET, wallets));
        rows.push_back(Upgrade("stick_capacity", UPG_STICKS, {{0, N::Text("none")}, {1, "10"}, {2, "20"}, {3, "30"}}));
        rows.push_back(Upgrade("nut_capacity", UPG_NUTS, {{0, N::Text("none")}, {1, "20"}, {2, "30"}, {3, "40"}}));
        if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOMBCHU_BAG) == RO_BOMBCHU_BAG_PROGRESSIVE)
            rows.push_back(N::Choice("bombchu", Text("bombchu_capacity"), gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel,
                {{0, N::Text("none")}, {1, "20"}, {2, "30"}, {3, "50"}}, [](int value) {
                    gSaveContext.ship.quest.data.randomizer.bombchuUpgradeLevel = value;
                    INV_CONTENT(ITEM_BOMBCHU) = value > 0 ? ITEM_BOMBCHU : ITEM_NONE;
                }));
        return rows;
    });
}

N::PagePtr QuestPage() {
    return N::MakePage("save/quest", Text("quest_status"), [] {
        std::vector<N::Row> rows{
            N::Link("items", Text("quest_items"), [] { return QuestItemsPage(false); }),
            N::Link("songs", Text("songs"), [] { return QuestItemsPage(true); }),
            N::Link("dungeon", Text("dungeon_items"), DungeonPage)};
        rows.push_back(Scalar("tokens", Text("gs_count"), gSaveContext.inventory.gsTokens, Text("gs_count_help")));
        rows.push_back(Flag("gs", Text("gs_unlocked"), gSaveContext.inventory.questItems, uint32_t{1} << QUEST_SKULL_TOKEN,
                            Text("gs_unlocked_help")));
        rows.push_back(N::Choice("poh", Text("poh_count"), (gSaveContext.inventory.questItems >> 28) & 15,
            {{0, "0"}, {1, "1"}, {2, "2"}, {3, "3"}}, [](int value) {
                gSaveContext.inventory.questItems = (gSaveContext.inventory.questItems & 0x0FFFFFFF) |
                                                   (static_cast<uint32_t>(value) << 28);
            }, Text("poh_count_help")));
        return rows;
    });
}
}
