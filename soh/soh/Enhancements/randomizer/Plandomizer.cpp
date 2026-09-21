#include "Plandomizer.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/NativeOptions/OptionsFileIO.h"
#include "soh/Enhancements/randomizer/3drando/hints.hpp"
#include "soh/Enhancements/randomizer/Traps.h"
#include "soh/Enhancements/randomizer/rando_hash.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/OTRGlobals.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
namespace N = NativeOptions;
namespace fs = std::filesystem;

struct Check {
    std::string name;
    RandomizerCheckArea area;
    RandomizerGet reward;
    int price = -1;
    RandomizerGet model = RG_NONE;
    std::string trickName;
};
struct Hint {
    std::string name;
    std::string text;
    bool edited = false;
};

fs::path loadedPath;
nlohmann::json loadedDocument;
std::vector<int> seedHash;
std::vector<Check> originalChecks, checks;
std::vector<Hint> originalHints, hints;
std::map<RandomizerGet, int> rewardPool;
bool dirty = false;

const std::vector<RandomizerGet> infiniteItemList = {
    RG_GREEN_RUPEE, RG_BLUE_RUPEE,   RG_RED_RUPEE,     RG_PURPLE_RUPEE, RG_HUGE_RUPEE,     RG_ARROWS_5, RG_ARROWS_10,
    RG_ARROWS_30,   RG_DEKU_STICK_1, RG_DEKU_SEEDS_30, RG_DEKU_NUTS_5,  RG_DEKU_NUTS_10,   RG_BOMBS_5,  RG_BOMBS_10,
    RG_BOMBS_20,    RG_BOMBCHU_5,    RG_BOMBCHU_10,    RG_BOMBCHU_20,   RG_RECOVERY_HEART, RG_ICE_TRAP, RG_SOLD_OUT
};


const std::vector<RandomizerGet> trapModels = {
    RG_NONE,
    RG_KOKIRI_SWORD,
    RG_GIANTS_KNIFE,
    RG_BIGGORON_SWORD,
    RG_DEKU_SHIELD,
    RG_HYLIAN_SHIELD,
    RG_MIRROR_SHIELD,
    RG_GORON_TUNIC,
    RG_ZORA_TUNIC,
    RG_IRON_BOOTS,
    RG_HOVER_BOOTS,
    RG_BOOMERANG,
    RG_LENS_OF_TRUTH,
    RG_MEGATON_HAMMER,
    RG_STONE_OF_AGONY,
    RG_DINS_FIRE,
    RG_FARORES_WIND,
    RG_NAYRUS_LOVE,
    RG_FIRE_ARROWS,
    RG_ICE_ARROWS,
    RG_LIGHT_ARROWS,
    RG_GERUDO_MEMBERSHIP_CARD,
    RG_MAGIC_BEAN,
    RG_MAGIC_BEAN_PACK,
    RG_DOUBLE_DEFENSE,
    RG_WEIRD_EGG,
    RG_ZELDAS_LETTER,
    RG_POCKET_EGG,
    RG_COJIRO,
    RG_ODD_MUSHROOM,
    RG_ODD_POTION,
    RG_POACHERS_SAW,
    RG_BROKEN_SWORD,
    RG_PRESCRIPTION,
    RG_EYEBALL_FROG,
    RG_EYEDROPS,
    RG_CLAIM_CHECK,
    RG_GOLD_SKULLTULA_TOKEN,
    RG_PROGRESSIVE_HOOKSHOT,
    RG_PROGRESSIVE_STRENGTH,
    RG_PROGRESSIVE_BOMB_BAG,
    RG_PROGRESSIVE_BOW,
    RG_PROGRESSIVE_SLINGSHOT,
    RG_PROGRESSIVE_WALLET,
    RG_PROGRESSIVE_SCALE,
    RG_PROGRESSIVE_NUT_UPGRADE,
    RG_PROGRESSIVE_STICK_UPGRADE,
    RG_PROGRESSIVE_BOMBCHU_BAG,
    RG_PROGRESSIVE_MAGIC_METER,
    RG_MAGIC_SINGLE,
    RG_MAGIC_DOUBLE,
    RG_PROGRESSIVE_OCARINA,
    RG_PROGRESSIVE_GORONSWORD,
    RG_EMPTY_BOTTLE,
    RG_BOTTLE_WITH_MILK,
    RG_BOTTLE_WITH_RED_POTION,
    RG_BOTTLE_WITH_GREEN_POTION,
    RG_BOTTLE_WITH_BLUE_POTION,
    RG_BOTTLE_WITH_FAIRY,
    RG_BOTTLE_WITH_FISH,
    RG_BOTTLE_WITH_BLUE_FIRE,
    RG_BOTTLE_WITH_BUGS,
    RG_BOTTLE_WITH_POE,
    RG_RUTOS_LETTER,
    RG_BOTTLE_WITH_BIG_POE,
    RG_ZELDAS_LULLABY,
    RG_EPONAS_SONG,
    RG_SARIAS_SONG,
    RG_SUNS_SONG,
    RG_SONG_OF_TIME,
    RG_SONG_OF_STORMS,
    RG_MINUET_OF_FOREST,
    RG_BOLERO_OF_FIRE,
    RG_SERENADE_OF_WATER,
    RG_REQUIEM_OF_SPIRIT,
    RG_NOCTURNE_OF_SHADOW,
    RG_PRELUDE_OF_LIGHT,
    RG_DEKU_TREE_MAP,
    RG_DODONGOS_CAVERN_MAP,
    RG_JABU_JABUS_BELLY_MAP,
    RG_FOREST_TEMPLE_MAP,
    RG_FIRE_TEMPLE_MAP,
    RG_WATER_TEMPLE_MAP,
    RG_SPIRIT_TEMPLE_MAP,
    RG_SHADOW_TEMPLE_MAP,
    RG_BOTTOM_OF_THE_WELL_MAP,
    RG_ICE_CAVERN_MAP,
    RG_DEKU_TREE_COMPASS,
    RG_DODONGOS_CAVERN_COMPASS,
    RG_JABU_JABUS_BELLY_COMPASS,
    RG_FOREST_TEMPLE_COMPASS,
    RG_FIRE_TEMPLE_COMPASS,
    RG_WATER_TEMPLE_COMPASS,
    RG_SPIRIT_TEMPLE_COMPASS,
    RG_SHADOW_TEMPLE_COMPASS,
    RG_BOTTOM_OF_THE_WELL_COMPASS,
    RG_ICE_CAVERN_COMPASS,
    RG_FOREST_TEMPLE_BOSS_KEY,
    RG_FIRE_TEMPLE_BOSS_KEY,
    RG_WATER_TEMPLE_BOSS_KEY,
    RG_SPIRIT_TEMPLE_BOSS_KEY,
    RG_SHADOW_TEMPLE_BOSS_KEY,
    RG_GANONS_CASTLE_BOSS_KEY,
    RG_FOREST_TEMPLE_SMALL_KEY,
    RG_FIRE_TEMPLE_SMALL_KEY,
    RG_WATER_TEMPLE_SMALL_KEY,
    RG_SPIRIT_TEMPLE_SMALL_KEY,
    RG_SHADOW_TEMPLE_SMALL_KEY,
    RG_BOTTOM_OF_THE_WELL_SMALL_KEY,
    RG_GERUDO_TRAINING_GROUND_SMALL_KEY,
    RG_GERUDO_FORTRESS_SMALL_KEY,
    RG_GANONS_CASTLE_SMALL_KEY,
    RG_TREASURE_GAME_SMALL_KEY,
    RG_KOKIRI_EMERALD,
    RG_GORON_RUBY,
    RG_ZORA_SAPPHIRE,
    RG_FOREST_MEDALLION,
    RG_FIRE_MEDALLION,
    RG_WATER_MEDALLION,
    RG_SPIRIT_MEDALLION,
    RG_SHADOW_MEDALLION,
    RG_LIGHT_MEDALLION,
    RG_RECOVERY_HEART,
    RG_GREEN_RUPEE,
    RG_GREG_RUPEE,
    RG_BLUE_RUPEE,
    RG_RED_RUPEE,
    RG_PURPLE_RUPEE,
    RG_HUGE_RUPEE,
    RG_TREASURE_GAME_GREEN_RUPEE,
    RG_PIECE_OF_HEART,
    RG_HEART_CONTAINER,
    RG_MILK,
    RG_BOMBS_5,
    RG_BOMBS_10,
    RG_BOMBS_20,
    RG_BUY_BOMBS_525,
    RG_BUY_BOMBS_535,
    RG_BUY_BOMBS_10,
    RG_BUY_BOMBS_20,
    RG_BUY_BOMBS_30,
    RG_DEKU_NUTS_5,
    RG_DEKU_NUTS_10,
    RG_BUY_DEKU_NUTS_5,
    RG_BUY_DEKU_NUTS_10,
    RG_BOMBCHU_5,
    RG_BOMBCHU_10,
    RG_BOMBCHU_20,
    RG_BUY_BOMBCHUS_20,
    RG_ARROWS_5,
    RG_BUY_ARROWS_10,
    RG_ARROWS_10,
    RG_BUY_ARROWS_30,
    RG_ARROWS_30,
    RG_BUY_ARROWS_50,
    RG_TREASURE_GAME_HEART,
    RG_DEKU_SEEDS_30,
    RG_BUY_DEKU_SEEDS_30,
    RG_BUY_HEART,
    RG_FISHING_POLE,
    RG_SOLD_OUT,
    RG_TRIFORCE_PIECE,
    RG_SKELETON_KEY,
};
static std::map<RandomizerCheckArea, const char*> rcAreaNameMap = {
    { RCAREA_KOKIRI_FOREST, "Kokiri Forest" },
    { RCAREA_LOST_WOODS, "Lost Woods" },
    { RCAREA_SACRED_FOREST_MEADOW, "Sacred Forest Meadow" },
    { RCAREA_HYRULE_FIELD, "Hyrule Field" },
    { RCAREA_LAKE_HYLIA, "Lake Hylia" },
    { RCAREA_GERUDO_VALLEY, "Gerudo Valley" },
    { RCAREA_GERUDO_FORTRESS, "Gerudo Fortress" },
    { RCAREA_WASTELAND, "Haunted Wasteland" },
    { RCAREA_DESERT_COLOSSUS, "Desert Colossus" },
    { RCAREA_MARKET, "Hyrule Market" },
    { RCAREA_HYRULE_CASTLE, "Hyrule Castle" },
    { RCAREA_KAKARIKO_VILLAGE, "Kakariko Village" },
    { RCAREA_GRAVEYARD, "Graveyard" },
    { RCAREA_DEATH_MOUNTAIN_TRAIL, "Death Mountain Trail" },
    { RCAREA_GORON_CITY, "Goron City" },
    { RCAREA_DEATH_MOUNTAIN_CRATER, "Death Mountain Crater" },
    { RCAREA_ZORAS_RIVER, "Zora's River" },
    { RCAREA_ZORAS_DOMAIN, "Zora's Domain" },
    { RCAREA_ZORAS_FOUNTAIN, "Zora's Fountain" },
    { RCAREA_LON_LON_RANCH, "Lon Lon Ranch" },
    { RCAREA_DEKU_TREE, "Deku Tree" },
    { RCAREA_DODONGOS_CAVERN, "Dodongo's Cavern" },
    { RCAREA_JABU_JABUS_BELLY, "Jabu Jabu's Belly" },
    { RCAREA_FOREST_TEMPLE, "Forest Temple" },
    { RCAREA_FIRE_TEMPLE, "Fire Temple" },
    { RCAREA_WATER_TEMPLE, "Water Temple" },
    { RCAREA_SPIRIT_TEMPLE, "Spirit Temple" },
    { RCAREA_SHADOW_TEMPLE, "Shadow Temple" },
    { RCAREA_BOTTOM_OF_THE_WELL, "Bottom of the Well" },
    { RCAREA_ICE_CAVERN, "Ice Cavern" },
    { RCAREA_GERUDO_TRAINING_GROUND, "Gerudo Training Ground" },
    { RCAREA_GANONS_CASTLE, "Ganon's Castle" },
    { RCAREA_INVALID, "All" },
};

std::string ItemName(RandomizerGet id) {
    return Rando::StaticData::RetrieveItem(id).GetName().GetForLanguage(CVarGetInteger(CVAR_SETTING("Languages"), 0));
}

RandomizerGet ParseItem(const nlohmann::json& value) {
    const auto name = value.get<std::string>();
    const auto item = Rando::StaticData::itemNameToEnum.find(name);
    if (item == Rando::StaticData::itemNameToEnum.end())
        throw std::runtime_error("Unknown item: " + name);
    return item->second;
}

nlohmann::json ReadDocument(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Could not open spoiler log");
    nlohmann::json document;
    input >> document;
    return document;
}

void Load(const fs::path& path) {
    try {
        auto document = ReadDocument(path);
        const auto& hash = document.at("file_hash");
        if (!hash.is_array() || hash.size() != 5)
            throw std::runtime_error("A seed hash must contain five icons");
        std::vector<int> nextHash;
        for (const auto& icon : hash) {
            if (!icon.is_number_integer() || icon.get<int64_t>() < 0 ||
                icon.get<int64_t>() >= static_cast<int64_t>(gSeedTextures.size()))
                throw std::runtime_error("Invalid seed hash icon");
            nextHash.push_back(icon.get<int>());
        }
        const auto& locations = document.at("locations");
        if (!locations.is_object()) throw std::runtime_error("Invalid locations");
        std::vector<Check> nextChecks;
        for (const auto& [name, value] : locations.items()) {
            if (name == "Ganon" || name == "Completed Triforce") continue;
            const auto location = Rando::StaticData::locationNameToEnum.find(name);
            if (location == Rando::StaticData::locationNameToEnum.end())
                throw std::runtime_error("Unknown location: " + name);
            Check check{ name, Rando::StaticData::GetLocation(location->second)->GetArea(),
                         ParseItem(value.is_object() ? value.at("item") : value) };
            if (value.is_object()) {
                if (value.contains("price") && !value.at("price").is_null()) {
                    const auto& price = value.at("price");
                    if (!price.is_number_integer() || price.get<int64_t>() < 0 || price.get<int64_t>() > 999)
                        throw std::runtime_error("Invalid shop price");
                    check.price = price.get<int>();
                }
                if (check.reward == RG_ICE_TRAP) {
                    check.model = ParseItem(value.at("model"));
                    check.trickName = value.at("trickName").get<std::string>();
                }
            } else if (Rando::StaticData::RetrieveItem(check.reward).GetItemType() == ITEMTYPE_SHOP) {
                check.price = Rando::StaticData::RetrieveItem(check.reward).GetPrice();
            }
            nextChecks.push_back(std::move(check));
        }
        std::vector<Hint> nextHints;
        if (document.contains("Gossip Stone Hints")) {
            const auto& gossip = document.at("Gossip Stone Hints");
            if (!gossip.is_object()) throw std::runtime_error("Invalid gossip stone hints");
            for (const auto& [name, value] : gossip.items())
                nextHints.push_back({ name, value.at("message").get<std::string>() });
        }
        loadedPath = path;
        loadedDocument = std::move(document);
        seedHash = std::move(nextHash);
        checks = originalChecks = std::move(nextChecks);
        hints = originalHints = std::move(nextHints);
        rewardPool.clear();
        dirty = false;
        N::GetModel().Back();
        N::Message(N::Text("plando_title"), N::Text("plando_loaded") + "\n" + path.filename().string());
    } catch (const std::exception& error) {
        SPDLOG_ERROR("Plandomizer load failed for {}: {}", path.string(), error.what());
        N::Message(N::Text("plando_title"), N::Text("plando_load_failed") + "\n" + error.what());
    }
}

void Save() {
    try {
        if (loadedPath.empty() || seedHash.size() != 5)
            throw std::runtime_error("No spoiler log is loaded");
        if (ReadDocument(loadedPath) != loadedDocument) {
            N::Message(N::Text("plando_title"), N::Text("plando_file_changed"));
            return;
        }
        auto document = loadedDocument;
        document["file_hash"] = seedHash;
        for (const auto& hint : hints) {
            if (hint.edited)
                document["Gossip Stone Hints"][hint.name] = { { "type", "Hardcoded Message" }, { "message", hint.text } };
        }
        for (const auto& check : checks) {
            const auto& item = Rando::StaticData::RetrieveItem(check.reward);
            auto& entry = document["locations"][check.name];
            if (check.reward == RG_ICE_TRAP || check.price >= 0 || entry.is_object()) {
                if (!entry.is_object()) entry = nlohmann::json::object();
                entry["item"] = item.GetName().english;
                if (check.reward == RG_ICE_TRAP) {
                    entry["model"] = Rando::StaticData::RetrieveItem(check.model).GetName().english;
                    entry["trickName"] = check.trickName;
                } else {
                    entry.erase("model");
                    entry.erase("trickName");
                }
                if (check.price >= 0) entry["price"] = check.price;
                else entry.erase("price");
            } else {
                entry = item.GetName().english;
            }
        }
        const auto backup = N::ReplaceFileWithBackup(loadedPath, document.dump(4));
        loadedDocument = std::move(document);
        originalChecks = checks;
        for (auto& hint : hints) hint.edited = false;
        originalHints = hints;
        dirty = false;
        N::Message(N::Text("plando_title"), N::Text("plando_saved") + "\n" + loadedPath.filename().string() +
                   "\n" + N::Text("backup") + ": " + backup.filename().string());
    } catch (const std::exception& error) {
        SPDLOG_ERROR("Plandomizer save failed: {}", error.what());
        N::Message(N::Text("plando_title"), N::Text("plando_save_failed") + "\n" + error.what());
    }
}

bool Infinite(RandomizerGet item) {
    return std::find(infiniteItemList.begin(), infiniteItemList.end(), item) != infiniteItemList.end();
}

void ReturnReward(RandomizerGet item) {
    if (!Infinite(item)) ++rewardPool[item];
}

bool AssignReward(size_t index, RandomizerGet item) {
    if (index >= checks.size()) return false;
    if (!Infinite(item)) {
        const auto available = rewardPool.find(item);
        if (available == rewardPool.end() || available->second <= 0) return false;
        if (--available->second == 0) rewardPool.erase(available);
    }
    ReturnReward(checks[index].reward);
    checks[index].reward = item;
    dirty = true;
    return true;
}

std::string RandomHint() {
    return Rando::StaticData::hintTextTable[GetRandomJunkHint()].GetHintMessage().GetForCurrentLanguage(MF_ENCODE);
}

N::PagePtr FilesPage() {
    auto paths = std::make_shared<std::vector<fs::path>>();
    auto failure = std::make_shared<std::string>();
    auto refresh = [paths, failure] {
        paths->clear();
        failure->clear();
        std::error_code error;
        const auto directory = Ship::Context::GetPathRelativeToAppDirectory("Randomizer");
        if (!fs::exists(directory, error) && !error) return;
        fs::directory_iterator it(directory, error), end;
        while (!error && it != end) {
            if (it->is_regular_file(error) && it->path().extension() == ".json") paths->push_back(it->path());
            it.increment(error);
        }
        std::sort(paths->begin(), paths->end());
        if (error) {
            SPDLOG_ERROR("Plandomizer cannot list {}: {}", directory, error.message());
            *failure = N::Text("plando_list_failed") + "\n" + error.message();
        }
    };
    refresh();
    return N::MakePage("plando/files", N::Text("plando_load"), [=] {
        std::vector<N::Row> rows;
        rows.push_back(N::Action("refresh", N::Text("refresh"), refresh));
        if (!failure->empty())
            rows.push_back(N::Action("error", N::Text("plando_list_failed"), [] { N::ReadCurrentDescription(); }, *failure));
        for (const auto& path : *paths) {
            rows.push_back(N::Action(path.string(), path.stem().string(), [path] {
                if (dirty)
                    N::Confirm(N::Text("plando_load"), N::Text("plando_discard"), N::Text("load"),
                               [path] { Load(path); });
                else Load(path);
            }));
        }
        if (paths->empty() && failure->empty()) {
            auto empty = N::Action("empty", N::Text("plando_no_files"), {});
            empty.enabled = false;
            rows.push_back(empty);
        }
        return rows;
    });
}

std::string HashName(size_t index) {
    if (index >= gSeedTextures.size()) return "";
    for (size_t item = 0; item < 158; ++item) {
        if (gItemIcons[item] && std::strcmp(static_cast<const char*>(gItemIcons[item]), gSeedTextures[index].tex) == 0) {
            // The hash picker shows artwork only. These icons have no correct
            // caption in the pause-menu speech bank (114 is the heart-piece counter).
            if (item == ITEM_HEART_CONTAINER) return N::Text("hash_heart_container");
            if (item == ITEM_MAGIC_SMALL) return N::Text("hash_magic_small");
            if (item == ITEM_MAGIC_LARGE) return N::Text("hash_magic_large");
            if (item == ITEM_WALLET_ADULT) return N::Text("hash_adult_wallet");
            return N::OriginalItemText(static_cast<int>(item));
        }
    }
    return "";
}

N::RowImage HashImage(size_t index) {
    const auto& sprite = gSeedTextures[index];
    return {sprite.tex, sprite.width, sprite.height, sprite.im_fmt, sprite.im_siz};
}

N::PagePtr HashPage() {
    return N::MakePage("plando/hash", N::Text("plando_hash"), [] {
        std::vector<N::Row> rows;
        for (size_t slot = 0; slot < seedHash.size(); ++slot) {
            const auto label = N::Text("plando_icon") + " " + std::to_string(slot + 1);
            auto row = N::Link(std::to_string(slot), label, [=] {
                auto page = N::MakePage("plando/hash/" + std::to_string(slot), label, [=] {
                    std::vector<N::Row> icons;
                    for (size_t icon = 0; icon < gSeedTextures.size(); ++icon) {
                        auto item = N::Action(std::to_string(icon), HashName(icon), [=] {
                            seedHash[slot] = static_cast<int>(icon);
                            dirty = true;
                            N::GetModel().Back();
                        });
                        item.image = HashImage(icon);
                        if (seedHash[slot] == icon) item.value = N::Text("selected");
                        icons.push_back(item);
                    }
                    return icons;
                });
                page->initialFocus = std::to_string(seedHash[slot]);
                return page;
            });
            row.image = HashImage(seedHash[slot]);
            row.value = HashName(seedHash[slot]);
            row.adjustable = true;
            row.adjust = [=](int direction) {
                seedHash[slot] = (seedHash[slot] + direction + gSeedTextures.size()) % gSeedTextures.size();
                dirty = true;
            };
            rows.push_back(row);
        }
        return rows;
    });
}

N::PagePtr RewardsPage(size_t index, bool resources) {
    auto filter = std::make_shared<std::string>();
    return N::MakePage("plando/rewards", N::Text(resources ? "plando_resources" : "plando_rewards"), [=] {
        std::vector<N::Row> rows{N::String("search", N::Text("filter"), *filter, [=](std::string value) {
            *filter = std::move(value);
        })};
        std::vector<std::pair<RandomizerGet, int>> items;
        if (resources) {
            for (auto item : infiniteItemList) items.emplace_back(item, 0);
        } else {
            for (auto item : rewardPool) items.push_back(item);
            std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
                const auto first = Rando::StaticData::RetrieveItem(a.first).GetItemType();
                const auto second = Rando::StaticData::RetrieveItem(b.first).GetItemType();
                return first == second ? a.first < b.first : first < second;
            });
        }
        ImGuiTextFilter search(filter->c_str());
        for (const auto& [item, count] : items) {
            const auto name = ItemName(item);
            if (!search.PassFilter(name.c_str())) continue;
            auto row = N::Action(std::to_string(item), (count ? std::to_string(count) + " " : "") + name, [=] {
                if (AssignReward(index, item)) N::GetModel().Back();
            });
            rows.push_back(row);
        }
        return rows;
    });
}

N::PagePtr CheckPage(size_t index) {
    return N::MakePage("plando/check/" + std::to_string(index), checks[index].name, [=] {
        auto& check = checks[index];
        auto original = N::Action("original", N::Text("plando_original_reward"), [] { N::ReadCurrentDescription(); });
        original.value = ItemName(originalChecks[index].reward);
        auto current = N::Action("current", N::Text("plando_new_reward"), [] { N::ReadCurrentDescription(); });
        current.value = ItemName(check.reward);
        std::vector<N::Row> rows{ original, current,
            N::Link("resources", N::Text("plando_resources"), [=] { return RewardsPage(index, true); }),
            N::Link("rewards", N::Text("plando_rewards"), [=] { return RewardsPage(index, false); }) };
        if (check.price >= 0) {
            auto price = N::Integer("price", N::Text("plando_price"), check.price, 0, 999, 1, [=](int value) {
                checks[index].price = value;
                dirty = true;
            });
            price.value += " " + N::Text("rupees");
            rows.push_back(price);
        }
        if (check.reward == RG_ICE_TRAP) {
            std::map<int, std::string> models;
            for (auto item : trapModels) models[item] = ItemName(item);
            rows.push_back(N::Choice("model", N::Text("plando_model"), check.model, std::move(models), [=](int value) {
                checks[index].model = static_cast<RandomizerGet>(value);
                dirty = true;
            }));
            rows.push_back(N::String("name", N::Text("plando_trick_name"), check.trickName, [=](std::string value) {
                checks[index].trickName = std::move(value);
                dirty = true;
            }));
            auto random = N::Action("random", N::Text("plando_random_name"), [=] {
                checks[index].trickName = Rando::Traps::GetTrapName(checks[index].model)
                    .GetForLanguage(CVarGetInteger(CVAR_SETTING("Languages"), 0));
                dirty = true;
            });
            random.enabled = check.model != RG_NONE && check.model != RG_SOLD_OUT;
            if (!random.enabled) random.disabledReason = N::Text("plando_choose_model");
            rows.push_back(random);
        }
        return rows;
    });
}

N::PagePtr LocationsPage() {
    auto area = std::make_shared<int>(RCAREA_INVALID);
    auto filter = std::make_shared<std::string>();
    return N::MakePage("plando/locations", N::Text("locations"), [=] {
        std::map<int, std::string> areas;
        for (const auto& [id, name] : rcAreaNameMap) areas[id] = name;
        std::vector<N::Row> rows{
            N::Choice("area", N::Text("plando_area"), *area, std::move(areas), [=](int value) { *area = value; }),
            N::String("search", N::Text("filter"), *filter, [=](std::string value) { *filter = std::move(value); }),
            N::Action("empty", N::Text("plando_empty_rewards"), [] {
                N::Confirm(N::Text("plando_empty_rewards"), N::Text("plando_empty_explain"), N::Text("confirm"), [] {
                    for (auto& check : checks) {
                        ReturnReward(check.reward);
                        check.reward = RG_SOLD_OUT;
                    }
                    dirty = true;
                });
            })
        };
        ImGuiTextFilter search(filter->c_str());
        for (size_t index = 0; index < checks.size(); ++index) {
            const auto& check = checks[index];
            if ((*area != RCAREA_INVALID && *area != check.area) || !search.PassFilter(check.name.c_str())) continue;
            auto row = N::Link(std::to_string(index), check.name, [=] { return CheckPage(index); });
            row.value = ItemName(check.reward);
            rows.push_back(row);
        }
        return rows;
    });
}

N::PagePtr HintsPage() {
    return N::MakePage("plando/hints", N::Text("plando_hints"), [] {
        std::vector<N::Row> rows{
            N::Action("clear", N::Text("plando_clear_hints"), [] {
                N::Confirm(N::Text("plando_clear_hints"), N::Text("plando_bulk_hints"), N::Text("confirm"), [] {
                    for (auto& hint : hints) { hint.text.clear(); hint.edited = true; }
                    dirty = true;
                });
            }),
            N::Action("random", N::Text("plando_random_hints"), [] {
                N::Confirm(N::Text("plando_random_hints"), N::Text("plando_bulk_hints"), N::Text("confirm"), [] {
                    for (auto& hint : hints) { hint.text = RandomHint(); hint.edited = true; }
                    dirty = true;
                });
            })
        };
        for (size_t index = 0; index < hints.size(); ++index) {
            rows.push_back(N::Link(std::to_string(index), hints[index].name, [=] {
                return N::MakePage("plando/hint/" + std::to_string(index), hints[index].name, [=] {
                    auto original = N::Action("original", N::Text("plando_original_hint"), [] { N::ReadCurrentDescription(); });
                    original.description = CustomMessage(originalHints[index].text).GetEnglish(MF_CLEAN);
                    auto edit = N::String("edit", N::Text("plando_new_hint"), hints[index].text, [=](std::string value) {
                        hints[index].text = std::move(value);
                        hints[index].edited = dirty = true;
                    }, N::Text("plando_hint_syntax"), 16384);
                    return std::vector<N::Row>{ original, edit,
                        N::Action("random", N::Text("plando_random_hint"), [=] {
                            hints[index].text = RandomHint();
                            hints[index].edited = dirty = true;
                        }) };
                });
            }));
        }
        return rows;
    });
}

N::PagePtr PlandomizerPage() {
    return N::MakePage("plando", N::Text("plando_title"), [] {
        std::vector<N::Row> rows{N::Link("load", N::Text("plando_load"), FilesPage)};
        if (!loadedPath.empty()) {
            auto file = N::Action("file", N::Text("plando_loaded_file"), [] { N::ReadCurrentDescription(); });
            file.value = loadedPath.filename().string();
            file.description = N::Text(dirty ? "plando_unsaved" : "plando_no_changes");
            rows.push_back(file);
            rows.push_back(N::Action("save", N::Text("plando_save"), Save, N::Text("plando_save_explain")));
            rows.push_back(N::Link("hash", N::Text("plando_hash"), HashPage));
            rows.push_back(N::Link("hints", N::Text("plando_hints"), HintsPage));
            rows.push_back(N::Link("locations", N::Text("locations"), LocationsPage));
        }
        return rows;
    });
}
} // namespace

void RegisterPlandomizerPage() {
    NativeOptions::RegisterPage("Plandomizer Editor", PlandomizerPage, NativeOptions::Text("plando_title"));
}
