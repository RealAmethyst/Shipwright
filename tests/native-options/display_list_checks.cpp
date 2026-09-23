#include "NativeOptions.h"
#include "OptionsDisplayList.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace NativeOptions;

namespace {
void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Equal(std::span<const Gfx> left, std::span<const Gfx> right) {
    return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](const Gfx& a, const Gfx& b) {
        return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1;
    });
}

int Opcode(const Gfx& command) {
    return (command.words.w0 >> 24) & 0xff;
}
} // namespace

void DisplayListChecks() {
    // These are resource identifiers only: emitting commands must not load a
    // texture, open a window, initialize speech or start the game.
    const char glyphResource[] = "__OTR__test/glyph";
    const char panelResource[] = "__OTR__test/panel";
    const TextureLookup glyph = [&](uint16_t) { return glyphResource; };
    const TextureLookup panel = [&](uint16_t) { return panelResource; };
    const GlyphWidth width = [](uint8_t) { return 8.0f; };
    const EncodeText encode = [](const std::string& text) { return text; };
    DisplayLists lists;
    lists.BeginFrame(0);

    auto& model = GetModel();
    model.Open(MakePage("options", Text("title"), [] {
        std::vector<Row> rows;
        for (const char* key : {"search", "accessibility", "audio", "display", "controls", "gameplay", "cosmetics",
                                "randomizer", "trackers", "network", "system", "advanced"})
            rows.push_back(Action(key, Text(key), [] {}));
        rows.front().description = Text("search_help");
        return rows;
    }));
    const auto root = lists.Build(Layout(model, Text("hints"), width, encode), glyph, panel);
    Check(root.size() > 0x800, "The root regression fixture must exceed the game's 2048-command overlay capacity");
    Check(Opcode(root.back()) == G_ENDDL, "The owned list must return to its calling overlay");
    const std::vector<Gfx> savedRoot(root.begin(), root.end());

    // The title, pause tab and full menu can all submit lists within a frame.
    // Appending more lists must preserve pointers already linked by the game.
    const auto title = lists.Build(TitleLayout("PRESS START", Text("title"), Text("title_navigation"), 1, 255,
                                               width, encode), glyph, panel);
    const std::vector<Gfx> savedTitle(title.begin(), title.end());
    auto details = MakePage("details", Text("search"), [] {
        return std::vector<Row>{Action("close", Text("close"), [] {})};
    });
    details->popup = true;
    details->documentLines.assign(13, std::string(100, 'i'));
    model.Push(details);
    const auto dense = lists.Build(Layout(model, Text("hints"), width, encode), glyph, panel);
    Check(dense.size() > root.size(), "Dense details must exercise a larger display list than the root");
    Check(Equal(root, savedRoot) && Equal(title, savedTitle), "Building another list must not invalidate earlier lists");

    struct Overlay {
        uint64_t before = 0x12345678;
        std::array<Gfx, 2> commands{};
        uint64_t after = 0x87654321;
    } overlay;
    auto* cursor = overlay.commands.data();
    __gSPDisplayList(cursor++, root.data());
    __gSPDisplayList(cursor++, dense.data());
    Check(cursor == overlay.commands.data() + 2 && overlay.before == 0x12345678 && overlay.after == 0x87654321,
          "Large native menus must require only one overlay link each and preserve the guard words");
    Check(Opcode(overlay.commands[0]) == G_DL && overlay.commands[0].words.w1 == reinterpret_cast<uintptr_t>(root.data()),
          "The overlay must point to the owned command storage");

    const std::vector<Gfx> savedDense(dense.begin(), dense.end());
    auto navigation = MakePage("navigation", "Navigation", [] {
        std::vector<Row> rows;
        for (int i = 0; i < 12; ++i) {
            auto row = Action(std::to_string(i), "Nearby target", [] {});
            row.description = "150 game units away, northeast";
            rows.push_back(row);
        }
        return rows;
    });
    navigation->section = "Ladders and crawlspaces";
    model.Open(navigation);
    const auto navigationList = lists.Build(Layout(model, "Left/right: category. A: navigate. B: close.", width, encode), glyph, panel);
    Check(!navigationList.empty() && Opcode(navigationList.back()) == G_ENDDL, "Navigation must emit a bounded display list");
    Check(Equal(root, savedRoot) && Equal(dense, savedDense), "Navigation invalidated existing game display lists");
    lists.BeginFrame(1);
    for (int i = 0; i < 8; ++i) lists.Build({}, glyph, panel);
    Check(Equal(root, savedRoot) && Equal(dense, savedDense), "Resetting the other graphics pool must retain this frame");
    // Captured frames skip BeginFrame, so repeated consumption sees intact data.
    for (int i = 0; i < 8; ++i)
        Check(Equal(dense, savedDense), "Interpolation and paused captures must retain their display lists");
    lists.BeginFrame(2);

    const Color color{255, 255, 255, 255};
    const auto single = lists.Build({{Primitive::Glyph, 10, 20, 16, 16, color, 65}}, glyph, panel);
    const int glyphOpcodes[] = {G_RDPPIPESYNC, G_SETPRIMCOLOR, G_SETCOMBINE, G_SETTIMG, G_SETTILE,
        G_RDPLOADSYNC, G_LOADBLOCK, G_RDPPIPESYNC, G_SETTILE, G_SETTILESIZE, G_TEXRECT, G_RDPHALF_1, G_RDPHALF_2};
    Check(single.size() == 4 + std::size(glyphOpcodes) + 1, "The glyph emitter must produce a complete bounded packet");
    for (size_t i = 0; i < std::size(glyphOpcodes); ++i)
        Check(Opcode(single[4 + i]) == glyphOpcodes[i], "Glyph command order must match the real GBI texture protocol");
    Check(single[7].words.w1 == reinterpret_cast<uintptr_t>(glyphResource), "Glyph loading must preserve the resource pointer");

    for (const int size : {G_IM_SIZ_16b, G_IM_SIZ_32b}) {
        RowImage image{panelResource, 32, 32, G_IM_FMT_RGBA, size};
        const auto textured = lists.Build({{Primitive::Image, 10, 20, 32, 32, color, 0, image}}, glyph, panel);
        Check(textured.size() == 18 && textured[7].words.w1 == reinterpret_cast<uintptr_t>(panelResource),
              "Both image formats must fit a bounded packet and retain the source texture");
        image.guiTexture = true;
        const auto skipped = lists.Build({{Primitive::Image, 10, 20, 32, 32, color, 0, image}}, glyph, panel);
        Check(skipped.size() == 8 && Opcode(skipped.back()) == G_ENDDL, "GUI-only images must leave a valid list");
    }
    const auto clipped = lists.Build({{Primitive::Rectangle, -160, 0, 640, 240, color}}, glyph, panel);
    Check(clipped.size() == 9 && Opcode(clipped[7]) == G_FILLRECT, "Backdrop rectangles must use a bounded packet");
    Check((clipped[7].words.w0 & 0xffffff) == ((319 * 4) << 12 | (239 * 4)),
          "Backdrop clipping must keep the original 320-by-240 bounds");
    std::cout << "Display-list regression: root " << savedRoot.size() << ", details " << savedDense.size()
              << " commands; overlay links 2; guards and frame lifetimes passed\n";
    model.Close();
    model.TakeSpeech();
}
