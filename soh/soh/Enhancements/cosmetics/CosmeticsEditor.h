#pragma once
#include <libultraship/libultraship.h>

// Groups share reset and randomize actions in the native Cosmetics pages.
typedef enum {
    COSMETICS_GROUP_LINK,
    COSMETICS_GROUP_MIRRORSHIELD,
    COSMETICS_GROUP_SWORDS,
    COSMETICS_GROUP_GLOVES,
    COSMETICS_GROUP_EQUIPMENT,
    COSMETICS_GROUP_KEYRING,
    COSMETICS_GROUP_SMALL_KEYS,
    COSMETICS_GROUP_BOSS_KEYS,
    COSMETICS_GROUP_CONSUMABLE,
    COSMETICS_GROUP_HUD,
    COSMETICS_GROUP_KALEIDO,
    COSMETICS_GROUP_TITLE,
    COSMETICS_GROUP_NPC,
    COSMETICS_GROUP_WORLD,
    COSMETICS_GROUP_MAGIC,
    COSMETICS_GROUP_ARROWS,
    COSMETICS_GROUP_SPIN_ATTACK,
    COSMETICS_GROUP_TRAILS,
    COSMETICS_GROUP_NAVI,
    COSMETICS_GROUP_IVAN,
    COSMETICS_GROUP_MESSAGE,
    COSMETICS_GROUP_MAX,
} CosmeticGroup;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

Color_RGBA8 CosmeticsEditor_GetDefaultValue(const char* id);

#ifdef __cplusplus
}

void CosmeticsEditor_RandomizeAll();
void CosmeticsEditor_AutoRandomizeAll();
void CosmeticsEditor_RandomizeGroup(CosmeticGroup group);
void CosmeticsEditor_ResetAll();
void CosmeticsEditor_ResetGroup(CosmeticGroup group);
void ApplyOrResetCustomGfxPatches(bool manualChange = true);

void InitializeCosmeticsEditor();
void SetCosmeticMargins(bool enabled);
void ResetCosmeticPositions();
#endif //__cplusplus
