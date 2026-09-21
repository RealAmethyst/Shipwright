#include "debugSaveEditor.h"
#include "NativeSaveEditor.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/cvar_prefixes.h"
#include <libultraship/bridge.h>

extern "C" {
#include "global.h"
}

// Maps entries in the GS flag array to the area name it represents
std::vector<const char*> gsMapping = {
    "Deku Tree",
    "Dodongo's Cavern",
    "Inside Jabu-Jabu's Belly",
    "Forest Temple",
    "Fire Temple",
    "Water Temple",
    "Spirit Temple",
    "Shadow Temple",
    "Bottom of the Well",
    "Ice Cavern",
    "Hyrule Field",
    "Lon Lon Ranch",
    "Kokiri Forest",
    "Lost Woods, Sacred Forest Meadow",
    "Castle Town and Ganon's Castle",
    "Death Mountain Trail, Goron City",
    "Kakariko Village",
    "Zora Fountain, River",
    "Lake Hylia",
    "Gerudo Valley",
    "Gerudo Fortress",
    "Desert Colossus, Haunted Wasteland",
};

// Modification of gAmmoItems that replaces ITEM_NONE with the item in inventory slot it represents
u8 gAllAmmoItems[] = {
    ITEM_STICK,     ITEM_NUT,          ITEM_BOMB,    ITEM_BOW,      ITEM_ARROW_FIRE, ITEM_DINS_FIRE,
    ITEM_SLINGSHOT, ITEM_OCARINA_TIME, ITEM_BOMBCHU, ITEM_LONGSHOT, ITEM_ARROW_ICE,  ITEM_FARORES_WIND,
    ITEM_BOOMERANG, ITEM_LENS,         ITEM_BEAN,    ITEM_HAMMER,
};

char z2ASCII(int code) {
    int ret;
    if (code < 10) { // Digits
        ret = code + 0x30;
    } else if (code >= 10 && code < 36) { // Uppercase letters
        ret = code + 0x37;
    } else if (code >= 36 && code < 62) { // Lowercase letters
        ret = code + 0x3D;
    } else if (code == 62) { // Space
        ret = code - 0x1E;
    } else if (code == 63 || code == 64) { // _ and .
        ret = code - 0x12;
    } else {
        ret = code;
    }
    return char(ret);
}

std::string decodeNTSCPlayerNameChar(int code) {
    const std::string charmap[] = {
        "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8",  "9",  // 10
        "あ", "い", "う", "え", "お", "か", "き", "く", "け", "こ", // 20
        "さ", "し", "す", "せ", "そ", "た", "ち", "つ", "て", "と", // 30
        "な", "に", "ぬ", "ね", "の", "は", "ひ", "ふ", "へ", "ほ", // 40
        "ま", "み", "む", "め", "も", "や", "ゆ", "よ", "ら", "り", // 50
        "る", "れ", "ろ", "わ", "を", "ん", "ぁ", "ぃ", "ぅ", "ぇ", // 60
        "ぉ", "っ", "ゃ", "ゅ", "ょ", "が", "ぎ", "ぐ", "げ", "ご", // 70
        "ざ", "じ", "ず", "ぜ", "ぞ", "だ", "ぢ", "づ", "で", "ど", // 80
        "ば", "び", "ぶ", "べ", "ぼ", "ぱ", "ぴ", "ぷ", "ぺ", "ぽ", // 90
        "ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", // 100
        "サ", "シ", "ス", "セ", "ソ", "タ", "チ", "ツ", "テ", "ト", // 110
        "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ", // 120
        "マ", "ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ", "ラ", "リ", // 130
        "ル", "レ", "ロ", "ワ", "ヲ", "ン", "ァ", "ィ", "ゥ", "ェ", // 140
        "ォ", "ッ", "ャ", "ュ", "ョ", "ガ", "ギ", "グ", "ゲ", "ゴ", // 150
        "ザ", "ジ", "ズ", "ゼ", "ゾ", "ダ", "ヂ", "ヅ", "デ", "ド", // 160
        "バ", "ビ", "ブ", "ベ", "ボ", "パ", "ピ", "プ", "ペ", "ポ", // 170
        "ヴ",
    };
    std::string ret;

    if (code < 171) { // Digits and Japanese
        ret = charmap[code];
    } else if (code >= 171 && code < 197) { // Uppercase letters
        ret.assign(1, (char)(code - 171 + 65));
    } else if (code >= 197 && code < 223) { // Lowercase letters
        ret.assign(1, (char)(code - 197 + 97));
    } else if (code == 223) { // Space
        ret = " ";
    } else if (code == 228) { // -
        ret = "-";
    } else if (code == 234) { // .
        ret = ".";
    } else {
        ret = "?";
    }

    return ret;
}

void InitializeSaveEditor() {
    namespace N = NativeOptions;
    N::RegisterPage("Save Editor", [] {
        return N::MakePage("save", NativeSaveEditor::Text("editor"), [] {
            using namespace NativeSaveEditor;
            std::vector<N::Row> rows{
                N::Link("info", Text("info"), InfoPage),
                N::Link("inventory", Text("inventory"), InventoryPage),
                N::Link("flags", Text("flags"), FlagsPage),
                N::Link("equipment", Text("equipment"), EquipmentPage),
                N::Link("quest", Text("quest_status"), QuestPage),
                N::Link("player", Text("player"), PlayerPage)};
            rows.back().enabled = gPlayState && GET_PLAYER(gPlayState);
            if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
            return rows;
        });
    }, NativeSaveEditor::Text("editor"));
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t) {
        NativeSaveEditor::SceneChanged();
    });
}
