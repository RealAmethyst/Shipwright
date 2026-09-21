#include "NativeOptions.h"
#include "OptionsOverlayGeometry.h"

namespace NativeOptions {
PagePtr OverlayLayoutPage(std::string window, bool resize, float defaultWidth, float defaultHeight) {
    return MakePage("layout/" + window, Text(resize ? "overlay_layout" : "overlay_position"), [=] {
        const auto layout = ReadOverlayGeometry(window.c_str(), {defaultWidth, defaultHeight});
        std::vector<Row> rows;
        if (layout.docked) {
            rows.push_back(Action("undock", Text("overlay_undock"), [=] {
                const auto current = ReadOverlayGeometry(window.c_str(), {defaultWidth, defaultHeight});
                WriteOverlayGeometry(window.c_str(), current.position, current.size, true);
                GetModel().Refresh();
            }, Text("overlay_docked")));
            return rows;
        }
        for (int field = 0; field < (resize ? 4 : 2); ++field) {
            const char* key = field == 0 ? "overlay_x" : field == 1 ? "overlay_y" : field == 2 ? "overlay_width" : "overlay_height";
            const float values[] = {layout.position.x, layout.position.y, layout.size.x, layout.size.y};
            rows.push_back(Integer(key, Text(key), static_cast<int>(values[field]), field < 2 ? -32768 : 32, 32767, 1,
                [=](int value) {
                    auto current = ReadOverlayGeometry(window.c_str(), {defaultWidth, defaultHeight});
                    (field == 0 ? current.position.x : field == 1 ? current.position.y : field == 2 ? current.size.x : current.size.y) = value;
                    WriteOverlayGeometry(window.c_str(), current.position, current.size);
                }, Text(field < 2 ? "overlay_position_help" : "overlay_size_help")));
        }
        auto center = Action("center", Text("overlay_center"), [=] {
            const auto current = ReadOverlayGeometry(window.c_str(), {defaultWidth, defaultHeight});
            const auto* viewport = ImGui::GetMainViewport();
            const ImVec2 position{std::max(0.0f, (viewport->Size.x - current.size.x) / 2),
                                  std::max(0.0f, (viewport->Size.y - current.size.y) / 2)};
            WriteOverlayGeometry(window.c_str(), position, current.size);
            GetModel().Refresh();
        });
        center.enabled = layout.size.x > 0 && layout.size.y > 0;
        if (!center.enabled) center.disabledReason = Text("overlay_size_unknown");
        rows.push_back(std::move(center));
        return rows;
    }, window);
}

Row OverlayLayout(std::string window, bool resize, float defaultWidth, float defaultHeight) {
    return Link("layout/" + window, Text(resize ? "overlay_layout" : "overlay_position"), [=] {
        return OverlayLayoutPage(window, resize, defaultWidth, defaultHeight);
    });
}

void ApplyOverlayPreset(std::string name, float x, float y, float width, float height) {
    const auto* viewport = ImGui::GetMainViewport();
    WriteOverlayGeometry(name.c_str(), {x - viewport->Pos.x, y - viewport->Pos.y}, {width, height});
}
}
