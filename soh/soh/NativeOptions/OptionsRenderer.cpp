#include "NativeOptions.h"
#include "OptionsLayout.h"
#include "OptionsDisplayList.h"
#include "soh/ShipUtils.h"
#include "soh/frame_interpolation.h"
#include "soh/Enhancements/custom-message/CustomMessageInterfaceAddon.h"

extern "C" {
#include "global.h"
#include "textures/title_static/title_static.h"
}

#include <algorithm>
#include <cmath>
#include <libultraship/libultraship.h>
#include <fast/resource/type/Texture.h>

namespace NativeOptions {
static const char* panels[] = {
        gFileSelWindow1Tex, gFileSelWindow2Tex, gFileSelWindow3Tex, gFileSelWindow4Tex,
        gFileSelWindow5Tex, gFileSelWindow6Tex, gFileSelWindow7Tex, gFileSelWindow8Tex,
        gFileSelWindow9Tex, gFileSelWindow10Tex, gFileSelWindow11Tex, gFileSelWindow12Tex,
        gFileSelWindow13Tex, gFileSelWindow14Tex, gFileSelWindow15Tex, gFileSelWindow16Tex,
        gFileSelWindow17Tex, gFileSelWindow18Tex, gFileSelWindow19Tex, gFileSelWindow20Tex,
    };
static std::string Encode(const std::string& text) {
    std::string copy = text;
    std::unique_ptr<const char[]> converted(Interface_ReplaceSpecialCharacters(copy.data()));
    return std::string(converted.get());
}

void ReadCurrentDescription() {
    const auto* current = GetModel().CurrentPage();
    if (!current || !current->documentLines.empty())
        return;
    std::string title = current->title;
    std::string body = current->description;
    if (!current->popup && !GetModel().Rows().empty()) {
        const auto& row = GetModel().Rows()[GetModel().Selection()];
        title = row.label;
        body = row.value;
        if (!row.description.empty()) body += (body.empty() ? "" : "\n") + row.description;
        if (!row.disabledReason.empty()) body += (body.empty() ? "" : "\n") + row.disabledReason;
    }
    if (body.empty()) body = title;
    const auto lines = WrapText(body, 260, 0.60f, Ship_GetCharFontWidth, Encode);
    const auto count = std::max<size_t>(1, (lines.size() + 12) / 13);
    auto index = std::make_shared<size_t>(0);
    auto page = std::make_shared<Page>();
    page->id = "details";
    page->title = title;
    page->popup = true;
    page->hints = Text("document_navigation");
    page->footer = Text("document_hints");
    std::weak_ptr<Page> weak = page;
    auto update = [=] {
        if (auto current = weak.lock()) {
            const auto first = *index * 13;
            const auto last = std::min(first + 13, lines.size());
            current->documentLines.assign(lines.begin() + first, lines.begin() + last);
            current->description.clear();
            if (count > 1) {
                current->description = Text("page_position");
                current->description.replace(current->description.find("$0"), 2, std::to_string(*index + 1));
                current->description.replace(current->description.find("$1"), 2, std::to_string(count));
            }
            for (const auto& line : current->documentLines)
                current->description += (current->description.empty() ? "" : "\n") + line;
        }
    };
    page->rows = [=] {
        if (count == 1)
            return std::vector<Row>{Action("close", Text("close"), [] { GetModel().Back(); })};
        auto turn = [=](int direction) {
            if ((direction < 0 && *index == 0) || (direction > 0 && *index + 1 == count)) return;
            *index += direction;
            update();
            if (auto current = weak.lock()) GetModel().Announce(current->description);
        };
        auto previous = Action("previous", Text("previous_page"), [=] { turn(-1); });
        previous.enabled = *index > 0;
        auto next = Action("next", Text("next_page"), [=] { turn(1); });
        next.enabled = *index + 1 < count;
        return std::vector<Row>{previous, next};
    };
    page->initialFocus = count > 1 ? "next" : "close";
    update();
    GetModel().Push(page);
}

void DrawDebuggerOverlay(float x, float y, float width, float height) {
    if (!GfxDebuggerIsDebugging() || !GetModel().IsOpen() || width <= 0 || height <= 0) return;
    if (GetModel().CurrentPage()->gamePreview) return;
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    auto* draw = ImGui::GetWindowDrawList();
    const float scale = std::min(width / 320, height / 240);
    const float left = x + (width - 320 * scale) / 2;
    const float top = y + (height - 240 * scale) / 2;
    draw->PushClipRect({x, y}, {x + width, y + height}, true);
    draw->AddRectFilled({x, y}, {x + width, y + height}, IM_COL32(3, 8, 21, 255));
    for (const auto& command : Layout(GetModel(), Text("hints"), Ship_GetCharFontWidth, Encode)) {
        const ImVec2 start{left + command.x * scale, top + command.y * scale};
        const ImVec2 end{start.x + command.width * scale, start.y + command.height * scale};
        const auto color = IM_COL32(command.color.r, command.color.g, command.color.b, command.color.a);
        if (command.primitive == Primitive::Rectangle) {
            draw->AddRectFilled(start, end, color);
            continue;
        }
        const char* resource = command.primitive == Primitive::Panel ? panels[command.texture] :
            command.primitive == Primitive::Glyph ? static_cast<const char*>(Ship_GetCharFontTexture(command.texture)) :
            command.image.resource;
        if (!resource) continue;
        const std::string name = command.primitive == Primitive::Image && command.image.guiTexture
                                     ? resource : std::string("NativeOptions/") + resource;
        if (!gui->HasTextureByName(name) && !(command.primitive == Primitive::Image && command.image.guiTexture)) {
            auto texture = std::dynamic_pointer_cast<Fast::Texture>(
                Ship::Context::GetInstance()->GetResourceManager()->LoadResource(resource, true));
            if (texture) gui->LoadGuiTexture(name, *texture, {1, 1, 1, 1});
        }
        if (gui->HasTextureByName(name))
            draw->AddImage(gui->GetTextureByName(name), start, end, {0, 0}, {1, 1}, color);
    }
    draw->PopClipRect();
}
} // namespace NativeOptions

static NativeOptions::DisplayLists displayLists;

extern "C" void NativeOptions_BeginGraphicsFrame(GraphicsContext* gfxCtx) {
    displayLists.BeginFrame(gfxCtx->gfxPoolIdx);
}

static void DrawCommands(GraphicsContext* gfxCtx, const std::vector<NativeOptions::DrawCommand>& commands) {
    const auto list = displayLists.Build(commands,
        [](uint16_t texture) { return Ship_GetCharFontTexture(static_cast<uint8_t>(texture)); },
        [](uint16_t texture) { return NativeOptions::panels[texture]; });
    Gfx_SetupDL_39Overlay(gfxCtx);
    OPEN_DISPS(gfxCtx);
    // This is owned command memory, not an archive resource identifier.
    __gSPDisplayList(OVERLAY_DISP++, list.data());
    CLOSE_DISPS(gfxCtx);
}

extern "C" void NativeOptions_DrawTitleMenu(GraphicsContext* gfxCtx, const char* startCaption, int selected, int alpha) {
    DrawCommands(gfxCtx, NativeOptions::TitleLayout(startCaption, NativeOptions::Text("title"),
        NativeOptions::Text("title_navigation"), selected, alpha, Ship_GetCharFontWidth, NativeOptions::Encode));
}

void NativeOptions::DrawModel(GraphicsContext* gfxCtx, const Model& model, const std::string& footer) {
    DrawCommands(gfxCtx, Layout(model, footer, Ship_GetCharFontWidth, Encode));
}

extern "C" void NativeOptions_Draw(GraphicsContext* gfxCtx) {
    const auto commands = NativeOptions::Layout(NativeOptions::GetModel(), NativeOptions::Text("hints"),
                                                Ship_GetCharFontWidth, NativeOptions::Encode);
    if (commands.empty()) return;
    Gfx_SetupFrame(gfxCtx, 3, 8, 21);
    DrawCommands(gfxCtx, commands);
}

extern "C" void NativeOptions_DrawPauseTab(GraphicsContext* gfxCtx) {
    NativeOptions::Model model([](const std::string& text, size_t, size_t) { return text; });
    model.Open(NativeOptions::PauseTabPage());
    DrawCommands(gfxCtx, NativeOptions::Layout(model, model.CurrentPage()->footer,
                                              Ship_GetCharFontWidth, NativeOptions::Encode));
}
