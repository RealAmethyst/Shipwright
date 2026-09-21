#include "NativeSaveEditor.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/util.h"
#include <fast/resource/type/Texture.h>
#include <soh_assets.h>

extern "C" {
#include "global.h"
extern u8 gAmmoItems[];
}
extern u8 gAllAmmoItems[];

namespace NativeSaveEditor {
std::string ItemName(int item) {
    if (item == ITEM_NONE) return N::Text("none");
    if (item == ITEM_ROCS_FEATHER) return N::OriginalItemText(item);
    return item >= 0 && itemMapping.contains(item) ? SohUtils::GetItemName(item) : "";
}

N::RowImage Image(const std::string& path) {
    static std::map<std::string, N::RowImage> images;
    if (path.empty()) return {};
    if (const auto found = images.find(path); found != images.end()) return found->second;
    auto resource = std::dynamic_pointer_cast<Fast::Texture>(
        Ship::Context::GetInstance()->GetResourceManager()->LoadResource(path));
    if (!resource || !resource->Width || !resource->Height ||
        (resource->Type != Fast::TextureType::RGBA32bpp && resource->Type != Fast::TextureType::RGBA16bpp)) return {};
    const auto entry = images.emplace(path, N::RowImage{}).first;
    entry->second = {entry->first.c_str(), resource->Width, resource->Height, G_IM_FMT_RGBA,
                    resource->Type == Fast::TextureType::RGBA32bpp ? G_IM_SIZ_32b : G_IM_SIZ_16b};
    return entry->second;
}

N::RowImage ItemImage(int item) {
    if (item == ITEM_ROCS_FEATHER) return Image(gRocsFeatherTex);
    const auto found = itemMapping.find(item);
    return found == itemMapping.end() ? N::RowImage{} : Image(found->second.texturePath);
}

namespace {
bool restrictToValid = true;

N::PagePtr AmmoPage() {
    return N::MakePage("save/ammo", Text("ammo"), [] {
        std::vector<N::Row> rows;
        for (int index = 0; index < 16; ++index) {
            const int item = restrictToValid ? gAmmoItems[index] : gAllAmmoItems[index];
            if (item == ITEM_NONE) continue;
            auto row = Scalar(std::to_string(index), ItemName(item), AMMO(item));
            row.image = ItemImage(item);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

N::PagePtr TradePage() {
    return N::MakePage("save/trade", Text("adult_trade_items"), [] {
        std::vector<N::Row> rows;
        for (int item = ITEM_POCKET_EGG; item <= ITEM_CLAIM_CHECK; ++item) {
            auto row = N::Action(std::to_string(item), ItemName(item), [] { N::ReadCurrentDescription(); });
            row.image = ItemImage(item);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}
}

N::PagePtr InventoryPage() {
    return N::MakePage("save/inventory", Text("inventory"), [] {
        std::vector<N::Row> rows{N::Toggle("restrict", Text("restrict_valid"), restrictToValid,
            [](bool value) { restrictToValid = value; }, Text("restrict_valid_help"))};
        rows.push_back(N::Link("ammo", Text("ammo"), AmmoPage));
        if (IS_RANDO && OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE))
            rows.push_back(N::Link("trade", Text("adult_trade_items"), TradePage));
        for (int slot = 0; slot < std::size(gSaveContext.inventory.items); ++slot) {
            std::map<int, std::string> choices{{ITEM_NONE, N::Text("none")}};
            if (restrictToValid) {
                const int targetSlot = slot >= SLOT_BOTTLE_1 && slot <= SLOT_BOTTLE_4 ? SLOT_BOTTLE_1 : slot;
                for (int candidate = 0; candidate < 56; ++candidate)
                    if (gItemSlots[candidate] == targetSlot && itemMapping.contains(candidate))
                        choices[candidate] = ItemName(candidate);
            } else {
                for (const auto& [item, entry] : itemMapping) choices[item] = ItemName(item);
            }
            auto row = N::Choice("slot/" + std::to_string(slot), Text("slot") + " " + std::to_string(slot + 1),
                gSaveContext.inventory.items[slot], choices,
                [slot](int value) { gSaveContext.inventory.items[slot] = value; });
            row.value = ItemName(gSaveContext.inventory.items[slot]);
            row.image = ItemImage(gSaveContext.inventory.items[slot]);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}
}
