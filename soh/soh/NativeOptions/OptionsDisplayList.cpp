#include "OptionsDisplayList.h"

#include <algorithm>
#include <cmath>

namespace NativeOptions {

namespace {
// A textured primitive emits three state commands, seven texture-load commands
// and three rectangle commands. Check each macro's write before appending it.
class Packet {
  public:
    Gfx* Next() { return &commands.at(used++); }
    void AppendTo(std::vector<Gfx>& list) {
        list.insert(list.end(), commands.begin(), commands.begin() + used);
        used = 0;
    }

  private:
    std::array<Gfx, 13> commands{};
    size_t used = 0;
};
} // namespace

void DisplayLists::BeginFrame(uint32_t poolIndex) {
    current = poolIndex & 1;
    frames[current].used = 0;
}

std::span<const Gfx> DisplayLists::Build(const std::vector<DrawCommand>& commands, const TextureLookup& glyph,
                                        const TextureLookup& panel) {
    auto& frame = frames[current];
    if (frame.used == frame.lists.size()) frame.lists.emplace_back();
    auto& list = frame.lists[frame.used++];
    list.clear();
    Packet packet;
    gDPSetCycleType(packet.Next(), G_CYC_1CYCLE);
    gDPSetRenderMode(packet.Next(), G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetTexturePersp(packet.Next(), G_TP_NONE);
    gDPSetTextureFilter(packet.Next(), G_TF_BILERP);
    packet.AppendTo(list);
    for (const auto& command : commands) {
        gDPPipeSync(packet.Next());
        gDPSetPrimColor(packet.Next(), 0, 0, command.color.r, command.color.g, command.color.b, command.color.a);
        if (command.primitive == Primitive::Rectangle) {
            gDPSetCombineMode(packet.Next(), G_CC_PRIMITIVE, G_CC_PRIMITIVE);
            const int left = std::max(0, static_cast<int>(command.x));
            const int top = std::max(0, static_cast<int>(command.y));
            const int right = std::min(320, static_cast<int>(std::ceil(command.x + command.width)));
            const int bottom = std::min(240, static_cast<int>(std::ceil(command.y + command.height)));
            if (left < right && top < bottom)
                gDPFillRectangle(packet.Next(), left, top, right - 1, bottom - 1);
            packet.AppendTo(list);
            continue;
        }
        gDPSetCombineMode(packet.Next(), G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
        int sourceWidth = 16;
        int sourceHeight = 16;
        if (command.primitive == Primitive::Glyph) {
            gDPLoadTextureBlock_4b(packet.Next(), glyph(command.texture), G_IM_FMT_I, 16, 16, 0,
                                  G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP,
                                  G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        } else if (command.primitive == Primitive::Image) {
            sourceWidth = command.image.width;
            sourceHeight = command.image.height;
            if (command.image.guiTexture || (command.image.size != G_IM_SIZ_32b && command.image.size != G_IM_SIZ_16b)) {
                packet.AppendTo(list);
                continue;
            }
            if (command.image.size == G_IM_SIZ_32b) {
                gDPLoadTextureBlock(packet.Next(), command.image.resource, command.image.format, G_IM_SIZ_32b,
                    sourceWidth, sourceHeight, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            } else {
                gDPLoadTextureBlock(packet.Next(), command.image.resource, command.image.format, G_IM_SIZ_16b,
                    sourceWidth, sourceHeight, 0, G_TX_CLAMP, G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
            }
        } else {
            sourceWidth = command.texture % 4 == 3 ? 48 : 64;
            sourceHeight = 32;
            gDPLoadTextureBlock(packet.Next(), panel(command.texture), G_IM_FMT_IA, G_IM_SIZ_16b,
                                sourceWidth, sourceHeight, 0, G_TX_NOMIRROR | G_TX_CLAMP,
                                G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD, G_TX_NOLOD);
        }
        gSPTextureRectangle(packet.Next(), static_cast<int>(command.x * 4), static_cast<int>(command.y * 4),
                            static_cast<int>((command.x + command.width) * 4),
                            static_cast<int>((command.y + command.height) * 4), G_TX_RENDERTILE, 0, 0,
                            static_cast<int>(1024 * sourceWidth / command.width),
                            static_cast<int>(1024 * sourceHeight / command.height));
        packet.AppendTo(list);
    }
    gSPEndDisplayList(packet.Next());
    packet.AppendTo(list);
    return list;
}

} // namespace NativeOptions
