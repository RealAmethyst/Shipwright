#include <vector>
#include <fstream>

#include <ship/Context.h>
#include "TimeSplits.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/NativeOptions/OptionsFileIO.h"
#include "soh/NativeOptions/OptionsTableLayout.h"
#include <filesystem>
#include <stdexcept>
#include "soh/Enhancements/gameplaystats.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh_assets.h"
#include <soh/SohGui/SohGui.hpp>
#include "soh/SohGui/UIWidgets.hpp"

extern "C" {
#include "z64item.h"
#include "macros.h"
extern SaveContext gSaveContext;
extern PlayState* gPlayState;
}

using namespace UIWidgets;

// ImVec4 Colors
#define COLOR_WHITE ImVec4(1.00f, 1.00f, 1.00f, 1.00f)
#define COLOR_LIGHT_RED ImVec4(1.0f, 0.05f, 0.0f, 1.0f)
#define COLOR_RED ImVec4(1.00f, 0.00f, 0.00f, 1.00f)
#define COLOR_LIGHT_GREEN ImVec4(0.52f, 1.0f, 0.23f, 1.0f)
#define COLOR_GREEN ImVec4(0.10f, 1.00f, 0.10f, 1.00f)
#define COLOR_BLUE ImVec4(0.00f, 0.33f, 1.00f, 1.00f)
#define COLOR_PURPLE ImVec4(0.54f, 0.19f, 0.89f, 1.00f)
#define COLOR_YELLOW ImVec4(1.00f, 1.00f, 0.00f, 1.00f)
#define COLOR_ORANGE ImVec4(1.00f, 0.67f, 0.11f, 1.00f)
#define COLOR_LIGHT_BLUE ImVec4(0.00f, 0.88f, 1.00f, 1.00f)
#define COLOR_GREY ImVec4(0.78f, 0.78f, 0.78f, 1.00f)

using json = nlohmann::json;

static uint32_t splitBestTimeDisplay;
static ImVec4 splitTimeColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
static ImVec4 activeSplitHighlight = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

std::vector<SplitObject> splitList;

std::vector<SplitObject> splitObjectList = {
    // clang-format off
    { SPLIT_TYPE_ITEM,      ITEM_STICK,                           "Deku Stick",                       "ITEM_STICK",                   COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_NUT,                             "Deku Nut",                         "ITEM_NUT",                     COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BOMB,                            "Bomb",                             "ITEM_BOMB",                    COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BOW,                             "Fairy Bow",                        "ITEM_BOW",                     COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_ARROW_FIRE,                      "Fire Arrow",                       "ITEM_ARROW_FIRE",              COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_DINS_FIRE,                       "Din's Fire",                       "ITEM_DINS_FIRE",               COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_SLINGSHOT,                       "Fairy Slingshot",                  "ITEM_SLINGSHOT",               COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_OCARINA_FAIRY,                   "Fairy Ocarina",                    "ITEM_OCARINA_FAIRY",           COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_OCARINA_TIME,                    "Ocarina of Time",                  "ITEM_OCARINA_TIME",            COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BOMBCHU,                         "Bombchu",                          "ITEM_BOMBCHU",                 COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_HOOKSHOT,                        "Hookshot",                         "ITEM_HOOKSHOT",                COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_LONGSHOT,                        "Longshot",                         "ITEM_LONGSHOT",                COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_ARROW_ICE,                       "Ice Arrow",                        "ITEM_ARROW_ICE",               COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_FARORES_WIND,                    "Farore's Wind",                    "ITEM_FARORES_WIND",            COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BOOMERANG,                       "Boomerang",                        "ITEM_BOOMERANG",               COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_LENS,                            "Lens of Truth",                    "ITEM_LENS",                    COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BEAN,                            "Magic Bean",                       "ITEM_BEAN",                    COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_HAMMER,                          "Megaton Hammer",                   "ITEM_HAMMER",                  COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_ARROW_LIGHT,                     "Light Arrow",                      "ITEM_ARROW_LIGHT",             COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_NAYRUS_LOVE,                     "Nayru's Love",                     "ITEM_NAYRUS_LOVE",             COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BOTTLE,                          "Empty Bottle",                     "ITEM_BOTTLE",                  COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_POTION_RED,                      "Red Potion",                       "ITEM_POTION_RED",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_POTION_GREEN,                    "Green Potion",                     "ITEM_POTION_GREEN",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_POTION_BLUE,                     "Blue Potion",                      "ITEM_POTION_BLUE",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_FAIRY,                           "Bottled Fairy",                    "ITEM_FAIRY",                   COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_FISH,                            "Fish",                             "ITEM_FISH",                    COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MILK_BOTTLE,                     "Milk",                             "ITEM_MILK_BOTTLE",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_LETTER_RUTO,                     "Ruto's Letter",                    "ITEM_LETTER_RUTO",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BLUE_FIRE,                       "Blue Fire",                        "ITEM_BLUE_FIRE",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BUG,                             "Bug",                              "ITEM_BUG",                     COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BIG_POE,                         "Big Poe",                          "ITEM_BIG_POE",                 COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_POE,                             "Poe",                              "ITEM_POE",                     COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_WEIRD_EGG,                       "Weird Egg",                        "ITEM_WEIRD_EGG",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_CHICKEN,                         "Chicken",                          "ITEM_CHICKEN",                 COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_LETTER_ZELDA,                    "Zelda's Letter",                   "ITEM_LETTER_ZELDA",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_KEATON,                     "Keaton Mask",                      "ITEM_MASK_KEATON",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_SKULL,                      "Skull Mask",                       "ITEM_MASK_SKULL",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_SPOOKY,                     "Spooky Mask",                      "ITEM_MASK_SPOOKY",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_BUNNY,                      "Bunny Hood",                       "ITEM_MASK_BUNNY",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_GORON,                      "Goron Mask",                       "ITEM_MASK_GORON",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_ZORA,                       "Zora Mask",                        "ITEM_MASK_ZORA",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_GERUDO,                     "Gerudo Mask",                      "ITEM_MASK_GERUDO",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_MASK_TRUTH,                      "Mask of Truth",                    "ITEM_MASK_TRUTH",              COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_POCKET_EGG,                      "Pocket Egg",                       "ITEM_POCKET_EGG",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_POCKET_CUCCO,                    "Pocket Cucco",                     "ITEM_POCKET_CUCCO",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_COJIRO,                          "Cojiro",                           "ITEM_COJIRO",                  COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_ODD_MUSHROOM,                    "Odd Mushroom",                     "ITEM_ODD_MUSHROOM",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_ODD_POTION,                      "Odd Potion",                       "ITEM_ODD_POTION",              COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_SAW,                             "Poacher's Saw",                    "ITEM_SAW",                     COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_SWORD_BROKEN,                    "Goron's Sword (Broken)",           "ITEM_SWORD_BROKEN",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_PRESCRIPTION,                    "Prescription",                     "ITEM_PRESCRIPTION",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_FROG,                            "Eyeball Frog",                     "ITEM_FROG",                    COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_EYEDROPS,                        "Eye Drops",                        "ITEM_EYEDROPS",                COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_CLAIM_CHECK,                     "Claim Check",                      "ITEM_CLAIM_CHECK",             COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SWORD_KOKIRI,                    "Kokiri Sword",                     "ITEM_SWORD_KOKIRI",            COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SWORD_MASTER,                    "Master Sword",                     "ITEM_SWORD_MASTER",            COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SWORD_BGS,                       "Giant's Knife & Biggoron's Sword", "ITEM_SWORD_BGS",               COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SHIELD_DEKU,                     "Deku Shield",                      "ITEM_SHIELD_DEKU",             COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SHIELD_HYLIAN,                   "Hylian Shield",                    "ITEM_SHIELD_HYLIAN",           COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_SHIELD_MIRROR,                   "Mirror Shield",                    "ITEM_SHIELD_MIRROR",           COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_TUNIC_GORON,                     "Goron Tunic",                      "ITEM_TUNIC_GORON",             COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_TUNIC_ZORA,                      "Zora Tunic",                       "ITEM_TUNIC_ZORA",              COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_BOOTS_IRON,                      "Iron Boots",                       "ITEM_BOOTS_IRON",              COLOR_WHITE },
    { SPLIT_TYPE_EQUIPMENT, ITEM_BOOTS_HOVER,                     "Hover Boots",                      "ITEM_BOOTS_HOVER",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BULLET_BAG_30,                   "Bullet Bag (30)",                  "ITEM_BULLET_BAG_30",           COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BULLET_BAG_40,                   "Bullet Bag (40)",                  "ITEM_BULLET_BAG_40",           COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BULLET_BAG_50,                   "Bullet Bag (50)",                  "ITEM_BULLET_BAG_50",           COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_QUIVER_30,                       "Quiver (30)",                      "ITEM_QUIVER_30",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_QUIVER_40,                       "Big Quiver (40)",                  "ITEM_QUIVER_40",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_QUIVER_50,                       "Biggest Quiver (50)",              "ITEM_QUIVER_50",               COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BOMB_BAG_20,                     "Bomb Bag (20)",                    "ITEM_BOMB_BAG_20",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BOMB_BAG_30,                     "Big Bomb Bag (30)",                "ITEM_BOMB_BAG_30",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_BOMB_BAG_40,                     "Biggest Bomb Bag (40)",            "ITEM_BOMB_BAG_40",             COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_BRACELET,                        "Goron's Bracelet",                 "ITEM_BRACELET",                COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_GAUNTLETS_SILVER,                "Silver Gauntlets",                 "ITEM_GAUNTLETS_SILVER",        COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_GAUNTLETS_GOLD,                  "Golden Gauntlets",                 "ITEM_GAUNTLETS_GOLD",          COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_SCALE_SILVER,                    "Silver Scale",                     "ITEM_SCALE_SILVER",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_SCALE_GOLDEN,                    "Golden Scale",                     "ITEM_SCALE_GOLDEN",            COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_SWORD_KNIFE,                     "Giant's Knife (Broken)",           "ITEM_SWORD_KNIFE",             COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_WALLET_ADULT,                    "Adult's Wallet",                   "ITEM_WALLET_ADULT",            COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_WALLET_GIANT,                    "Giant's Wallet",                   "ITEM_WALLET_GIANT",            COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_FISHING_POLE,                    "Fishing Pole",                     "ITEM_FISHING_POLE",            COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_MINUET,                     "Minuet of Forest",                 "QUEST_SONG_MINUET",            COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_BOLERO,                     "Bolero of Fire",                   "QUEST_SONG_BOLERO",            COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_SERENADE,                   "Serenade of Water",                "QUEST_SONG_SERENADE",          COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_REQUIEM,                    "Requiem of Spirit",                "QUEST_SONG_REQUIEM",           COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_NOCTURNE,                   "Nocturne of Shadow",               "QUEST_SONG_NOCTURNE",          COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_PRELUDE,                    "Prelude of Light",                 "QUEST_SONG_PRELUDE",           COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_LULLABY,                    "Zelda's Lullaby",                  "QUEST_SONG_LULLABY",           COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_EPONA,                      "Epona's Song",                     "QUEST_SONG_EPONA",             COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_SARIA,                      "Saria's Song",                     "QUEST_SONG_SARIA",             COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_SUN,                        "Sun's Song",                       "QUEST_SONG_SUN",               COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_TIME,                       "Song of Time",                     "QUEST_SONG_TIME",              COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SONG_STORMS,                     "Song of Storms",                   "QUEST_SONG_STORMS",            COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_FOREST,                "Forest Medallion",                 "QUEST_MEDALLION_FOREST",       COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_FIRE,                  "Fire Medallion",                   "QUEST_MEDALLION_FIRE",         COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_WATER,                 "Water Medallion",                  "QUEST_MEDALLION_WATER",        COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_SPIRIT,                "Spirit Medallion",                 "QUEST_MEDALLION_SPIRIT",       COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_SHADOW,                "Shadow Medallion",                 "QUEST_MEDALLION_SHADOW",       COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_MEDALLION_LIGHT,                 "Light Medallion",                  "QUEST_MEDALLION_LIGHT",        COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_KOKIRI_EMERALD,                  "Kokiri's Emerald",                 "QUEST_KOKIRI_EMERALD",         COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_GORON_RUBY,                      "Goron's Ruby",                     "QUEST_GORON_RUBY",             COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_ZORA_SAPPHIRE,                   "Zora's Sapphire",                  "QUEST_ZORA_SAPPHIRE",          COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_STONE_OF_AGONY,                  "Stone of Agony",                   "QUEST_STONE_OF_AGONY",         COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_GERUDO_CARD,                     "Gerudo's Card",                    "QUEST_GERUDO_CARD",            COLOR_WHITE },
    { SPLIT_TYPE_QUEST,     ITEM_SKULL_TOKEN,                     "Skulltula Token",                  "QUEST_SKULL_TOKEN",            COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_SINGLE_MAGIC,                    "Magic Meter",                      "ITEM_MAGIC_SMALL",             COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_DOUBLE_MAGIC,                    "Double Magic",                     "ITEM_MAGIC_LARGE",             COLOR_WHITE },
    { SPLIT_TYPE_ITEM,      ITEM_DOUBLE_DEFENSE,                  "Double Defense",                   "ITEM_HEART_CONTAINER",         COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_STICK_UPGRADE_20,                "Deku Stick Upgrade (20)",          "ITEM_STICK",                   COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_STICK_UPGRADE_30,                "Deku Stick Upgrade (30)",          "ITEM_STICK",                   COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_NUT_UPGRADE_30,                  "Deku Nut Upgrade (30)",            "ITEM_NUT",                     COLOR_WHITE },
    { SPLIT_TYPE_UPGRADE,   ITEM_NUT_UPGRADE_40,                  "Deku Nut Upgrade (40)",            "ITEM_NUT",                     COLOR_WHITE },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_GOMA,                      "Queen Gohma",                      "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_LIGHT_GREEN },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_DODONGO,                   "King Dodongo",                     "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_LIGHT_RED },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_VA,                        "Barinade",                         "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_LIGHT_BLUE },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_GANONDROF,                 "Phantom Ganon",                    "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_GREEN },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_FD2,                       "Volvagia",                         "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_RED },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_MO,                        "Morpha",                           "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_BLUE },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_SST,                       "Bongo Bongo",                      "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_PURPLE },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_TW,                        "Twinrova",                         "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_ORANGE },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_GANON,                     "Ganondorf",                        "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_GREY },
    { SPLIT_TYPE_BOSS,      ACTOR_BOSS_GANON2,                    "Ganon",                            "SPECIAL_TRIFORCE_PIECE_WHITE", COLOR_YELLOW },
    { SPLIT_TYPE_ENTRANCE,  SCENE_DEKU_TREE,                      "Enter Deku Tree",                  "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_DODONGOS_CAVERN,                "Enter Dodongos Cavern",            "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_JABU_JABU,                      "Enter Jabu Jabu's Belly",          "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_FOREST_TEMPLE,                  "Enter Forest Temple",              "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_FIRE_TEMPLE,                    "Enter Fire Temple",                "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_WATER_TEMPLE,                   "Enter Water Temple",               "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_SPIRIT_TEMPLE,                  "Enter Spirit Temple",              "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_SHADOW_TEMPLE,                  "Enter Shadow Temple",              "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_BOTTOM_OF_THE_WELL,             "Enter Bottom of the Well",         "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_ICE_CAVERN,                     "Enter Ice Cavern",                 "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_GANONS_TOWER,                   "Enter Ganons Tower",               "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_GERUDO_TRAINING_GROUND,         "Enter Gerudo Training Ground",    "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_THIEVES_HIDEOUT,                "Enter Thieves Hideout",            "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_INSIDE_GANONS_CASTLE,           "Enter Ganons Castle",              "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_GANONS_TOWER_COLLAPSE_INTERIOR, "Enter Tower Collapse Interior",    "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_ENTRANCE,  SCENE_INSIDE_GANONS_CASTLE_COLLAPSE,  "Enter Ganons Castle Collapse",     "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_MISC,      SCENE_ZORAS_RIVER,                    "Lost Woods Escape",                "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_MISC,      SCENE_LOST_WOODS,                     "Forest Escape",                    "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },
    { SPLIT_TYPE_MISC,      SCENE_KAKARIKO_VILLAGE,               "Watchtower Death",                 "SPECIAL_SPLIT_ENTRANCE",       COLOR_WHITE },

    // clang-format on
};

std::map<uint32_t, std::vector<uint32_t>> popupList = {
    // clang-format off
    { ITEM_STICK,           { ITEM_STICK, ITEM_STICK_UPGRADE_20, ITEM_STICK_UPGRADE_30 } },
    { ITEM_NUT,             { ITEM_NUT, ITEM_NUT_UPGRADE_30, ITEM_NUT_UPGRADE_40 } },
    { ITEM_BOMB,            { ITEM_BOMB_BAG_20, ITEM_BOMB_BAG_30, ITEM_BOMB_BAG_40 } },
    { ITEM_BOW,             { ITEM_QUIVER_30, ITEM_QUIVER_40, ITEM_QUIVER_50 } },
    { ITEM_SLINGSHOT,       { ITEM_BULLET_BAG_30, ITEM_BULLET_BAG_40, ITEM_BULLET_BAG_50 } },
    { ITEM_OCARINA_FAIRY,   { ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME } },
    { ITEM_HOOKSHOT,        { ITEM_HOOKSHOT, ITEM_LONGSHOT } },
    { ITEM_BOTTLE,          { ITEM_BOTTLE, ITEM_POTION_RED, ITEM_POTION_GREEN, ITEM_POTION_BLUE,
                              ITEM_FAIRY, ITEM_FISH, ITEM_MILK_BOTTLE, ITEM_LETTER_RUTO,
                              ITEM_BLUE_FIRE, ITEM_BUG, ITEM_BIG_POE, ITEM_POE } },
    { ITEM_WEIRD_EGG,       { ITEM_WEIRD_EGG, ITEM_CHICKEN, ITEM_LETTER_ZELDA, ITEM_MASK_KEATON,
                              ITEM_MASK_SKULL, ITEM_MASK_SPOOKY, ITEM_MASK_BUNNY, ITEM_MASK_GORON,
                              ITEM_MASK_ZORA, ITEM_MASK_GERUDO, ITEM_MASK_TRUTH } },
    { ITEM_POCKET_EGG,      { ITEM_POCKET_EGG, ITEM_POCKET_CUCCO, ITEM_COJIRO, ITEM_ODD_MUSHROOM,
                              ITEM_ODD_POTION, ITEM_SAW, ITEM_SWORD_BROKEN, ITEM_PRESCRIPTION,
                              ITEM_FROG, ITEM_EYEDROPS, ITEM_CLAIM_CHECK } },
    { ITEM_BRACELET,        { ITEM_BRACELET, ITEM_GAUNTLETS_SILVER, ITEM_GAUNTLETS_GOLD } },
    { ITEM_SCALE_SILVER,    { ITEM_SCALE_SILVER, ITEM_SCALE_GOLDEN } },
    { ITEM_WALLET_ADULT,    { ITEM_WALLET_ADULT, ITEM_WALLET_GIANT } },
    { ITEM_SINGLE_MAGIC,    { ITEM_SINGLE_MAGIC, ITEM_DOUBLE_MAGIC } },
    { ITEM_SKULL_TOKEN,     { } }

    // clang-format on
};

std::string formatTimestampTimeSplit(uint32_t value) {
    uint32_t sec = value / 10;
    uint32_t hh = sec / 3600;
    uint32_t mm = (sec - hh * 3600) / 60;
    uint32_t ss = sec - hh * 3600 - mm * 60;
    uint32_t ds = value % 10;
    return fmt::format("{}:{:0>2}:{:0>2}.{}", hh, mm, ss, ds);
}

nlohmann::json ImVec4_to_json(const ImVec4& vec) {
    return nlohmann::json{ { "x", vec.x }, { "y", vec.y }, { "z", vec.z }, { "w", vec.w } };
}

nlohmann::json SplitObject_to_json(const SplitObject& split) {
    return nlohmann::json{ { "splitType", split.splitType },
                           { "splitID", split.splitID },
                           { "splitName", split.splitName },
                           { "splitImage", split.splitImage },
                           { "splitTint", ImVec4_to_json(split.splitTint) },
                           { "splitTimeCurrent", split.splitTimeCurrent },
                           { "splitTimeBest", split.splitTimeBest },
                           { "splitTimePreviousBest", split.splitTimePreviousBest },
                           { "splitTimeStatus", SPLIT_STATUS_INACTIVE },
                           { "splitSkullTokenCount", split.splitSkullTokenCount } };
}

SplitObject json_to_SplitObject(const nlohmann::json& jsonSplit) {
    const auto number = [&jsonSplit](const char* key) {
        const auto& value = jsonSplit.at(key);
        if (!value.is_number_integer() || (value.is_number_unsigned() ? value.get<uint64_t>() > UINT32_MAX :
            value.get<int64_t>() < 0 || value.get<int64_t>() > UINT32_MAX))
            throw std::runtime_error(std::string("Invalid split field: ") + key);
        return value.get<uint32_t>();
    };
    SplitObject split{};
    split.splitType = number("splitType");
    split.splitID = number("splitID");
    split.splitTimeCurrent = number("splitTimeCurrent");
    split.splitTimeBest = number("splitTimeBest");
    split.splitTimePreviousBest = number("splitTimePreviousBest");
    split.splitTimeStatus = SPLIT_STATUS_INACTIVE;
    split.splitSkullTokenCount = number("splitSkullTokenCount");
    return split;
}

void TimeSplitsUpdateSplitStatus() {
    uint32_t index = 0;
    for (auto& data : splitList) {
        if (data.splitTimeStatus == SPLIT_STATUS_INACTIVE || data.splitTimeStatus == SPLIT_STATUS_ACTIVE) {
            data.splitTimeStatus = SPLIT_STATUS_ACTIVE;
            break;
        }
        index++;
    }
    for (size_t i = index; i < splitList.size(); i++) {
        if (splitList[i].splitTimeStatus != SPLIT_STATUS_ACTIVE &&
            splitList[i].splitTimeStatus != SPLIT_STATUS_COLLECTED) {
            splitList[i].splitTimeStatus = SPLIT_STATUS_INACTIVE;
        }
    }
}

void TimeSplitCompleteSplits() {
    gSaveContext.ship.stats.itemTimestamp[TIMESTAMP_DEFEAT_GANON] = GAMEPLAYSTAT_TOTAL_TIME;
    gSaveContext.ship.stats.gameComplete = true;
}

void TimeSplitsSkipSplit(uint32_t index) {
    if (index >= splitList.size() || !GameInteractor::IsSaveLoaded()) return;
    splitList[index].splitTimeStatus = SPLIT_STATUS_SKIPPED;
    if (index + 1 == splitList.size()) {
        TimeSplitCompleteSplits();
    } else {
        TimeSplitsUpdateSplitStatus();
    }
}

void TimeSplitsItemSplitEvent(uint32_t type, u8 item) {
    uint32_t index = 0;
    if (type <= SPLIT_TYPE_QUEST) {
        if (item == ITEM_NUTS_5 || item == ITEM_NUTS_10) {
            item = ITEM_NUT;
        } else if (item == ITEM_STICKS_5 || item == ITEM_STICKS_10) {
            item = ITEM_STICK;
        }
    }
    if (type == SPLIT_TYPE_ENTRANCE) {
        if ((item == SCENE_ZORAS_RIVER && gSaveContext.entranceIndex == ENTR_ZORAS_RIVER_UNDERWATER_SHORTCUT) ||
            (item == SCENE_LOST_WOODS && (gSaveContext.entranceIndex == ENTR_LOST_WOODS_BRIDGE_EAST_EXIT ||
                                          gSaveContext.entranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT))) {
            type = SPLIT_TYPE_MISC;
        }
    }

    for (auto& split : splitList) {
        if (split.splitType == type) {
            if (item == split.splitID &&
                (type != SPLIT_TYPE_QUEST || item != ITEM_SKULL_TOKEN ||
                 split.splitSkullTokenCount == gSaveContext.inventory.gsTokens)) {
                if (split.splitTimeStatus == SPLIT_STATUS_ACTIVE) {
                    split.splitTimeCurrent = GAMEPLAYSTAT_TOTAL_TIME;
                    split.splitTimeStatus = SPLIT_STATUS_COLLECTED;
                    if (split.splitTimeBest > GAMEPLAYSTAT_TOTAL_TIME || split.splitTimeBest == 0) {
                        split.splitTimeBest = GAMEPLAYSTAT_TOTAL_TIME;
                    }
                    if (split.splitTimePreviousBest == 0) {
                        split.splitTimePreviousBest = GAMEPLAYSTAT_TOTAL_TIME;
                    }
                    if (index == splitList.size() - 1) {
                        TimeSplitCompleteSplits();
                    } else {
                        splitList[index + 1].splitTimeStatus = SPLIT_STATUS_ACTIVE;
                    }
                }
            }
        }
        index++;
    }
}

void TimeSplitsSplitBestTimeDisplay(SplitObject split) {
    activeSplitHighlight = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (split.splitTimeStatus == SPLIT_STATUS_ACTIVE) {
        if (GAMEPLAYSTAT_TOTAL_TIME > split.splitTimePreviousBest) {
            splitTimeColor = COLOR_RED;
            splitBestTimeDisplay = (GAMEPLAYSTAT_TOTAL_TIME - split.splitTimePreviousBest);
        }
        if (GAMEPLAYSTAT_TOTAL_TIME == split.splitTimePreviousBest) {
            splitTimeColor = COLOR_WHITE;
            splitBestTimeDisplay = GAMEPLAYSTAT_TOTAL_TIME;
        }
        if (GAMEPLAYSTAT_TOTAL_TIME < split.splitTimePreviousBest) {
            splitTimeColor = COLOR_GREEN;
            splitBestTimeDisplay = (split.splitTimePreviousBest - GAMEPLAYSTAT_TOTAL_TIME);
        }
        activeSplitHighlight = COLOR_LIGHT_BLUE;
    }
    if (split.splitTimeStatus == SPLIT_STATUS_INACTIVE) {
        splitTimeColor = COLOR_WHITE;
        splitBestTimeDisplay = split.splitTimeBest;
    }
    if (split.splitTimeStatus == SPLIT_STATUS_COLLECTED) {
        if (split.splitTimeCurrent > split.splitTimePreviousBest) {
            splitTimeColor = COLOR_RED;
            splitBestTimeDisplay = (split.splitTimeCurrent - split.splitTimePreviousBest);
        }
        if (split.splitTimeCurrent == split.splitTimePreviousBest) {
            splitTimeColor = COLOR_WHITE;
            splitBestTimeDisplay = split.splitTimeCurrent;
        }
        if (split.splitTimeCurrent < split.splitTimePreviousBest) {
            splitTimeColor = COLOR_GREEN;
            splitBestTimeDisplay = (split.splitTimePreviousBest - split.splitTimeCurrent);
        }
    }
}

void TimeSplitsDrawSplitsList() {
    ImGui::BeginChild("SplitTable");
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 0));
    if (ImGui::BeginTable("Splits", 5, ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable)) {
        ImGui::TableSetupColumn("Item Image", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel,
                                34.0f);
        ImGui::TableSetupColumn("Item Name");
        ImGui::TableSetupColumn("Current Time");
        ImGui::TableSetupColumn("+/-");
        ImGui::TableSetupColumn("Prev. Best");
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int column = 1; column < 5; ++column) {
            if (ImGui::TableSetColumnIndex(column)) ImGui::TextUnformatted(ImGui::TableGetColumnName(column));
        }
        ImGui::TableSetColumnIndex(4);

        for (auto& split : splitList) {
            ImGui::TableNextColumn();
            TimeSplitsSplitBestTimeDisplay(split);

            ImGui::PushID(split.splitID);
            if (split.splitTimeStatus == SPLIT_STATUS_ACTIVE) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(47, 79, 90, 255));
            }
            ImGui::Image(Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(split.splitImage),
                         ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), split.splitTint, ImVec4(0, 0, 0, 0));
            ImGui::TableNextColumn();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 5.0f));
            ImGui::AlignTextToFramePadding();
            ImGui::TextWrapped("%s", split.splitName.c_str());
            ImGui::TableNextColumn();
            // Current Time
            ImGui::Text("%s", (split.splitTimeStatus == SPLIT_STATUS_ACTIVE)
                                  ? formatTimestampTimeSplit(GAMEPLAYSTAT_TOTAL_TIME).c_str()
                              : (split.splitTimeStatus == SPLIT_STATUS_COLLECTED)
                                  ? formatTimestampTimeSplit(split.splitTimeCurrent).c_str()
                                  : "--:--:-");
            ImGui::TableNextColumn();
            // +/- Difference
            ImGui::TextColored(splitTimeColor, "%s", formatTimestampTimeSplit(splitBestTimeDisplay).c_str());
            ImGui::TableNextColumn();
            // Previous Best
            ImGui::Text("%s", (split.splitTimePreviousBest != 0)
                                  ? formatTimestampTimeSplit(split.splitTimePreviousBest).c_str()
                                  : "--:--:-");
            ImGui::PopID();
            ImGui::PopStyleVar(1);

        }


        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

namespace {
namespace N = NativeOptions;

N::PagePtr SplitColumnsPage() {
    return N::MakePage("splits/columns", N::Text("splits_columns"), [] {
        const auto id = N::ChildTableId("Time Splits", "SplitTable", "Splits");
        const auto columns = N::ReadTableLayout(id, 5);
        const char* keys[] = {"splits_column_image", "splits_column_name", "splits_column_current",
                              "splits_column_difference", "splits_column_best"};
        std::vector<N::Row> rows;
        for (int order = 0; order < 5; ++order) {
            const auto it = std::find_if(columns.begin(), columns.end(), [=](auto c) { return c.order == order; });
            const int index = static_cast<int>(it - columns.begin());
            auto row = N::Toggle(std::to_string(index), N::Text(keys[index]), it->visible, [=](bool visible) {
                auto next = N::ReadTableLayout(id, 5);
                next[index].visible = visible;
                if (!N::WriteTableLayout(id, next)) N::GetModel().Announce(N::Text("splits_column_required"));
            }, N::Text("splits_columns_help"));
            row.adjust = [=](int direction) {
                auto next = N::ReadTableLayout(id, 5);
                const int target = next[index].order + direction;
                if (target < 0 || target >= 5) return;
                const auto other = std::find_if(next.begin(), next.end(), [=](auto c) { return c.order == target; });
                std::swap(next[index].order, other->order);
                N::WriteTableLayout(id, next);
            };
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

json ReadSplitLists() {
    const auto path = Ship::Context::GetPathRelativeToAppDirectory("timesplitdata.json");
    if (!std::filesystem::exists(path)) return json::object();
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Could not open time split lists");
    json document;
    input >> document;
    if (!document.is_object()) throw std::runtime_error("Time split lists must be an object");
    return document;
}

void SplitFileError(const std::exception& error) {
    SPDLOG_ERROR("Time split list operation failed: {}", error.what());
    N::Message(N::Text("time_splits"), N::Text("splits_file_error") + "\n" + error.what());
}

void WriteSplitLists(const json& document) {
    N::ReplaceFileWithBackup(Ship::Context::GetPathRelativeToAppDirectory("timesplitdata.json"), document.dump(4));
}

void SaveSplitList(const std::string& name) {
    try {
        auto document = ReadSplitLists();
        auto list = json::array();
        for (const auto& split : splitList) list.push_back(SplitObject_to_json(split));
        document[name] = std::move(list);
        WriteSplitLists(document);
        N::Message(N::Text("time_splits"), N::Text("splits_saved") + " " + name);
    } catch (const std::exception& error) { SplitFileError(error); }
}

void LoadSplitList(const std::string& name) {
    try {
        const auto document = ReadSplitLists();
        const auto& list = document.at(name);
        if (!list.is_array()) throw std::runtime_error("Invalid time split list");
        std::vector<SplitObject> next;
        for (const auto& entry : list) {
            auto split = json_to_SplitObject(entry);
            const auto known = std::find_if(splitObjectList.begin(), splitObjectList.end(), [&](const SplitObject& candidate) {
                return candidate.splitID == split.splitID && candidate.splitType == split.splitType;
            });
            if (known == splitObjectList.end() || split.splitSkullTokenCount > 100)
                throw std::runtime_error("Invalid split type, item, or token count");
            // Use the current resource definition; saved files own the times and token target.
            split.splitImage = known->splitImage;
            split.splitTint = known->splitTint;
            split.splitName = known->splitName;
            if (split.splitID == ITEM_SKULL_TOKEN && split.splitType == SPLIT_TYPE_QUEST)
                split.splitName += " (" + std::to_string(split.splitSkullTokenCount) + ")";
            split.splitTimeStatus = SPLIT_STATUS_INACTIVE;
            next.push_back(std::move(split));
        }
        if (!next.empty()) next.front().splitTimeStatus = SPLIT_STATUS_ACTIVE;
        splitList = std::move(next);
        N::Message(N::Text("time_splits"), N::Text("splits_loaded") + " " + name);
    } catch (const std::exception& error) { SplitFileError(error); }
}

void AddSplit(SplitObject split) {
    split.splitTimeStatus = splitList.empty() ? SPLIT_STATUS_ACTIVE : SPLIT_STATUS_INACTIVE;
    splitList.push_back(std::move(split));
    N::GetModel().Announce(N::Text("splits_added") + " " + splitList.back().splitName);
}

N::PagePtr AddSplitPage(uint32_t type, const std::string& title) {
    return N::MakePage("splits/add/" + std::to_string(type), title, [=] {
        std::vector<N::Row> rows;
        for (const auto& split : splitObjectList) {
            if (split.splitType != type) continue;
            rows.push_back(N::Action(std::to_string(split.splitID), split.splitName, [split] {
                if (split.splitID == ITEM_SKULL_TOKEN && split.splitType == SPLIT_TYPE_QUEST) {
                    auto count = std::make_shared<int>(0);
                    N::GetModel().Push(N::MakePage("splits/tokens", split.splitName, [=] {
                        return std::vector<N::Row>{
                            N::Integer("count", N::Text("splits_tokens"), *count, 0, 100, 1, [=](int value) { *count = value; }),
                            N::Action("add", N::Text("splits_set_tokens"), [=] {
                                auto copy = split;
                                copy.splitSkullTokenCount = *count;
                                copy.splitName += " (" + std::to_string(*count) + ")";
                                N::GetModel().Back();
                                AddSplit(copy);
                            })
                        };
                    }));
                } else if (split.splitType < SPLIT_TYPE_BOSS && popupList.contains(split.splitID)) {
                    const auto variants = popupList.at(split.splitID);
                    N::GetModel().Push(N::MakePage("splits/variant", split.splitName, [=] {
                        std::vector<N::Row> choices;
                        for (auto id : variants) {
                            const auto found = std::find_if(splitObjectList.begin(), splitObjectList.end(),
                                                           [=](const SplitObject& candidate) { return candidate.splitID == id; });
                            if (found == splitObjectList.end()) continue;
                            const auto item = *found;
                            choices.push_back(N::Action(std::to_string(id), item.splitName, [=] {
                                N::GetModel().Back();
                                AddSplit(item);
                            }));
                        }
                        return choices;
                    }));
                } else {
                    AddSplit(split);
                }
            }));
        }
        return rows;
    });
}

N::PagePtr ManageSplitPage(size_t index) {
    auto position = std::make_shared<size_t>(index);
    return N::MakePage("splits/manage/" + std::to_string(index), splitList[index].splitName, [=] {
        if (*position >= splitList.size()) return std::vector<N::Row>{};
        const auto& split = splitList[*position];
        auto status = N::Action("status", N::Text("splits_status"), [] { N::ReadCurrentDescription(); });
        const char* statuses[] = {"splits_active", "splits_inactive", "splits_collected", "splits_skipped"};
        status.value = split.splitTimeStatus <= SPLIT_STATUS_SKIPPED ? N::Text(statuses[split.splitTimeStatus]) : "";
        status.description = N::Text("splits_current_time") + ": " +
            formatTimestampTimeSplit(split.splitTimeStatus == SPLIT_STATUS_ACTIVE ? GAMEPLAYSTAT_TOTAL_TIME : split.splitTimeCurrent) +
            "\n" + N::Text("splits_best") + ": " + formatTimestampTimeSplit(split.splitTimeBest) +
            "\n" + N::Text("splits_previous_best") + ": " + formatTimestampTimeSplit(split.splitTimePreviousBest);
        auto move = [=](int direction) {
            const size_t next = direction < 0 ? *position - 1 : *position + 1;
            if (next >= splitList.size()) return;
            std::swap(splitList[*position], splitList[next]);
            *position = next;
            // The first unfinished split becomes active after a reorder, matching the original editor.
            for (auto& entry : splitList)
                if (entry.splitTimeStatus == SPLIT_STATUS_ACTIVE) entry.splitTimeStatus = SPLIT_STATUS_INACTIVE;
            TimeSplitsUpdateSplitStatus();
        };
        auto up = N::Action("up", N::Text("move_up"), [=] { move(-1); });
        up.enabled = *position > 0;
        auto down = N::Action("down", N::Text("move_down"), [=] { move(1); });
        down.enabled = *position + 1 < splitList.size();
        auto skip = N::Action("skip", N::Text("splits_skip"), [=] { TimeSplitsSkipSplit(static_cast<uint32_t>(*position)); });
        skip.enabled = GameInteractor::IsSaveLoaded();
        if (!skip.enabled) skip.disabledReason = N::Text("save_required");
        return std::vector<N::Row>{status, up, down, skip,
            N::Action("remove", N::Text("remove"), [=] {
                N::Confirm(N::Text("remove"), split.splitName, N::Text("remove"), [=] {
                    splitList.erase(splitList.begin() + *position);
                    TimeSplitsUpdateSplitStatus();
                    N::GetModel().Back();
                });
            })
        };
    });
}

N::PagePtr SplitsPage() {
    return N::MakePage("splits/current", N::Text("splits_current"), [] {
        std::vector<N::Row> rows;
        for (size_t index = 0; index < splitList.size(); ++index) {
            auto row = N::Link(std::to_string(index), splitList[index].splitName, [=] { return ManageSplitPage(index); });
            row.value = formatTimestampTimeSplit(splitList[index].splitTimeCurrent);
            rows.push_back(row);
        }
        return rows;
    });
}

N::PagePtr SavedSplitsPage() {
    auto names = std::make_shared<std::vector<std::string>>();
    auto failure = std::make_shared<std::string>();
    auto refresh = [=] {
        names->clear();
        failure->clear();
        try {
            const auto document = ReadSplitLists();
            for (const auto& entry : document.items()) names->push_back(entry.key());
        } catch (const std::exception& error) {
            SPDLOG_ERROR("Time split list operation failed: {}", error.what());
            *failure = N::Text("splits_file_error") + "\n" + error.what();
        }
    };
    refresh();
    return N::MakePage("splits/files", N::Text("splits_lists"), [=] {
        std::vector<N::Row> rows{N::Action("refresh", N::Text("refresh"), refresh)};
        if (!failure->empty())
            rows.push_back(N::Action("error", N::Text("splits_file_error"), [] { N::ReadCurrentDescription(); }, *failure));
        rows.push_back(N::String("create", N::Text("splits_create"), "", [=](std::string name) {
            try {
                if (ReadSplitLists().contains(name))
                    N::Confirm(N::Text("splits_save"), N::Text("splits_replace") + " " + name, N::Text("save"),
                               [=] { SaveSplitList(name); refresh(); });
                else { SaveSplitList(name); refresh(); }
            } catch (const std::exception& error) { SplitFileError(error); }
        }, N::Text("splits_create_description"), 24, [](const std::string& name) {
            return name.find_first_not_of(" \t\r\n") == std::string::npos ? N::Text("name_required") : "";
        }));
        for (const auto& name : *names) {
            rows.push_back(N::Link(name, name, [=] {
                return N::MakePage("splits/file/" + name, name, [=] {
                    return std::vector<N::Row>{
                        N::Action("load", N::Text("splits_load"), [=] {
                            N::Confirm(N::Text("splits_load"), N::Text("splits_load_description"), N::Text("load"),
                                       [=] { LoadSplitList(name); });
                        }),
                        N::Action("save", N::Text("splits_save"), [=] { SaveSplitList(name); }),
                        N::Action("delete", N::Text("splits_delete"), [=] {
                            N::Confirm(N::Text("splits_delete"), name, N::Text("delete"), [=] {
                                try {
                                    auto document = ReadSplitLists();
                                    document.erase(name);
                                    WriteSplitLists(document);
                                    N::GetModel().Back();
                                    refresh();
                                } catch (const std::exception& error) { SplitFileError(error); }
                            });
                        })
                    };
                });
            }));
        }
        return rows;
    });
}

N::PagePtr TimeSplitsPage() {
    return N::MakePage("splits", N::Text("time_splits"), [] {
        return std::vector<N::Row>{
            N::CVarToggle(N::Text("splits_overlay"), CVAR_WINDOW("TimeSplits")),
            N::Link("current", N::Text("splits_current"), SplitsPage),
            N::Link("add", N::Text("splits_add"), [] {
                return N::MakePage("splits/add", N::Text("splits_add"), [] {
                    std::vector<N::Row> rows;
                    for (const auto& [type, key] : {
                            std::pair{SPLIT_TYPE_EQUIPMENT, "tracker_equipment"}, {SPLIT_TYPE_ITEM, "tracker_inventory"},
                            {SPLIT_TYPE_QUEST, "splits_quest"}, {SPLIT_TYPE_ENTRANCE, "splits_entrances"},
                            {SPLIT_TYPE_BOSS, "splits_bosses"}, {SPLIT_TYPE_MISC, "splits_misc"} }) {
                        const auto title = N::Text(key);
                        rows.push_back(N::Link(key, title, [=] { return AddSplitPage(type, title); }));
                    }
                    return rows;
                });
            }),
            N::Link("files", N::Text("splits_lists"), SavedSplitsPage),
            N::Action("attempt", N::Text("splits_attempt"), [] {
                N::Confirm(N::Text("splits_attempt"), N::Text("splits_attempt_description"), N::Text("confirm"), [] {
                    for (auto& split : splitList) {
                        split.splitTimeStatus = SPLIT_STATUS_INACTIVE;
                        split.splitTimeCurrent = 0;
                    }
                    if (!splitList.empty()) splitList.front().splitTimeStatus = SPLIT_STATUS_ACTIVE;
                });
            }),
            N::Action("update", N::Text("splits_update"), [] {
                for (auto& split : splitList)
                    if (split.splitTimeBest != 0 && (split.splitTimePreviousBest == 0 || split.splitTimeBest < split.splitTimePreviousBest))
                        split.splitTimePreviousBest = split.splitTimeBest;
                N::GetModel().Announce(N::Text("splits_updated"));
            }),
            N::Link("window", N::Text("splits_window"), [] {
                return N::MakePage("splits/window", N::Text("splits_window"), [] {
                    return std::vector<N::Row>{
                        N::OverlayLayout("Time Splits", true, 450, 660),
                        N::Link("columns", N::Text("splits_columns"), SplitColumnsPage),
                        N::CVarColor(N::Text("background_color"), CVAR_ENHANCEMENT("TimeSplits.WindowColor"), {0, 0, 0, 255}),
                        N::CVarDecimal(N::Text("splits_scale"), CVAR_ENHANCEMENT("TimeSplits.WindowScale"), 1, 1, 3, 0.1f)
                    };
                });
            })
        };
    });
}
} // namespace

void TimeSplitWindow::Draw() {
    const Color_RGBA8 color = CVarGetColor(CVAR_ENHANCEMENT("TimeSplits.WindowColor.Value"), {0, 0, 0, 255});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, VecFromRGBA8(color));
    GuiWindow::Draw();
    ImGui::PopStyleColor();
}

void TimeSplitWindow::DrawElement() {
    ImGui::SetWindowFontScale(CVarGetFloat(CVAR_ENHANCEMENT("TimeSplits.WindowScale"), 1.0f));
    TimeSplitsDrawSplitsList();
}

void TimeSplitWindow::InitElement() {
    NativeOptions::RegisterPage("Time Splits", TimeSplitsPage);

    Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadGuiTexture("SPECIAL_TRIFORCE_PIECE_WHITE",
                                                                        gWTriforcePieceTex, ImVec4(1, 1, 1, 1));
    Ship::Context::GetInstance()->GetWindow()->GetGui()->LoadGuiTexture("SPECIAL_SPLIT_ENTRANCE", gSplitEntranceTex,
                                                                        ImVec4(1, 1, 1, 1));

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnTimestamp>([](u8 item) {
        if (item != ITEM_SKULL_TOKEN) {
            uint32_t tempType = SPLIT_TYPE_ITEM;
            for (auto& data : splitList) {
                if (data.splitID == item) {
                    tempType = data.splitType;
                    break;
                }
            }
            TimeSplitsItemSplitEvent(tempType, item);
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>([](GetItemEntry itemEntry) {
        GetItemEntry testItem = itemEntry;
        if (itemEntry.itemId == ITEM_SKULL_TOKEN || itemEntry.itemId == ITEM_BOTTLE || itemEntry.itemId == ITEM_POE ||
            itemEntry.itemId == ITEM_BIG_POE) {
            uint32_t tempType = SPLIT_TYPE_ITEM;
            for (auto& data : splitList) {
                if (data.splitID == itemEntry.itemId) {
                    tempType = data.splitType;
                    break;
                }
            }
            TimeSplitsItemSplitEvent(tempType, itemEntry.itemId);
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerBottleUpdate>(
        [](int16_t contents) { TimeSplitsItemSplitEvent(SPLIT_TYPE_UPGRADE, contents); });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBossDefeat>([](void* refActor) {
        Actor* bossActor = (Actor*)refActor;
        TimeSplitsItemSplitEvent(SPLIT_TYPE_BOSS, bossActor->id);
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (gPlayState->sceneNum != SCENE_KAKARIKO_VILLAGE) {
            TimeSplitsItemSplitEvent(SPLIT_TYPE_ENTRANCE, sceneNum);
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerHealthChange>([](int16_t amount) {
        if (gPlayState->sceneNum == SCENE_KAKARIKO_VILLAGE) {
            Player* player = GET_PLAYER(gPlayState);
            if (player->fallDistance > 500 && gSaveContext.health <= 0) {
                TimeSplitsItemSplitEvent(SPLIT_TYPE_MISC, gPlayState->sceneNum);
            }
        }
    });
}
