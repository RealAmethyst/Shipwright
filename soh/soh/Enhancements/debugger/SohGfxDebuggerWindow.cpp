#include "SohGfxDebuggerWindow.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/OTRGlobals.h"
#include <libultraship/window/gui/GfxDebuggerWindow.h>
#include <fast/debug/GfxDebugger.h>

namespace {
namespace N = NativeOptions;
using Path = std::vector<const Fast::F3DGfx*>;

std::shared_ptr<LUS::GfxDebuggerWindow> Backend() {
    return std::dynamic_pointer_cast<LUS::GfxDebuggerWindow>(
        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("GfxDebuggerWindow"));
}

std::shared_ptr<Fast::GfxDebugger> Debugger() {
    return Ship::Context::GetInstance()->GetGfxDebugger();
}

N::PagePtr CommandsPage(const Fast::F3DGfx* commands, Path parent) {
    std::vector<LUS::GfxDebuggerWindow::Command> nodes;
    Backend()->ReadCommands(commands, nodes);
    const auto capture = Debugger()->GetDisplayList();
    return N::MakePage("gfx/commands/" + std::to_string(reinterpret_cast<uintptr_t>(commands)),
                       N::Text("gfx_display_list"), [nodes, parent, capture] {
        std::vector<N::Row> rows;
        if (!Debugger()->IsDebugging() || Debugger()->GetDisplayList() != capture) return rows;
        for (const auto& node : nodes) {
            auto path = parent;
            path.push_back(node.address);
            auto row = N::Link(std::to_string(node.index), node.text, [node, path, capture] {
                Debugger()->SetBreakPoint(path);
                return N::MakePage("gfx/command", node.text, [node, path, capture] {
                    std::vector<N::Row> actions;
                    if (!Debugger()->IsDebugging() || Debugger()->GetDisplayList() != capture) return actions;
                    actions.push_back(N::Action("break", N::Text("gfx_breakpoint"), [path] {
                        Debugger()->SetBreakPoint(path);
                    }));
                    if (node.child && path.size() < 64)
                        actions.push_back(N::Link("child", N::Text("gfx_child"), [node, path] {
                            return CommandsPage(node.child, path);
                        }));
                    actions.push_back(N::Action("copy", N::Text("copy_text"), [node] {
                        ImGui::SetClipboardText(node.text.c_str());
                    }));
                    actions.push_back(N::Action("address", N::Text("copy_address"), [node] {
                        ImGui::SetClipboardText(fmt::format("0x{:x}", reinterpret_cast<uintptr_t>(node.address)).c_str());
                    }));
                    return actions;
                }, fmt::format("0x{:x}: {}", reinterpret_cast<uintptr_t>(node.address), node.index));
            });
            if (Debugger()->HasBreakPoint(path)) row.value = N::Text("gfx_breakpoint_marker");
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

N::PagePtr StatePage() {
    const auto state = Backend()->ReadState();
    return N::MakePage("gfx/state", N::Text("gfx_state"), [state] {
        std::vector<N::Row> rows;
        rows.push_back(N::Link("stack", N::Text("gfx_stack"), [state] {
            return N::MakePage("gfx/stack", N::Text("gfx_stack"), [state] {
                std::vector<N::Row> lines;
                for (size_t i = 0; i < state.stack.size(); ++i)
                    lines.push_back(N::Action(std::to_string(i), state.stack[i], [] { N::ReadCurrentDescription(); }));
                return lines;
            });
        }));
        const char* textureKeys[] = {"gfx_loaded_texture_0", "gfx_loaded_texture_1", "gfx_texture_to_load"};
        for (size_t i = 0; i < state.textures.size(); ++i) {
            const auto info = state.textures[i];
            const auto title = N::Text(textureKeys[i]);
            auto row = N::Link(info.name, title, [info, title] {
                auto page = N::MakePage("gfx/texture", title, [] {
                    return std::vector<N::Row>{N::Action("close", N::Text("close"), [] { N::GetModel().Back(); })};
                }, fmt::format("{} x {}, {}", info.width, info.height, info.type));
                page->popup = true;
                if (info.available) page->documentImage = {info.name, info.width, info.height, 0, 0, true};
                return page;
            }, fmt::format("{} x {}, {}", info.width, info.height, info.type));
            if (info.available) row.image = {info.name, info.width, info.height, 0, 0, true};
            rows.push_back(std::move(row));
        }
        const char* colorKeys[] = {"gfx_env_color", "gfx_prim_color", "gfx_fog_color", "gfx_fill_color", "gfx_grayscale_color"};
        for (size_t i = 0; i < state.colors.size(); ++i) {
            const auto color = state.colors[i];
            auto row = N::Action(colorKeys[i], N::Text(colorKeys[i]), [] { N::ReadCurrentDescription(); });
            row.value = fmt::format("R {}, G {}, B {}, A {}", color[0], color[1], color[2], color[3]);
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

N::PagePtr GraphicsPage() {
    return N::MakePage("advanced/gfx", N::Text("gfx_debugger"), [] {
        std::vector<N::Row> rows;
        if (!Debugger()->IsDebugging()) {
            rows.push_back(N::Action("debug", N::Text("gfx_debug"), [] {
                Debugger()->RequestDebugging();
                N::GetModel().Close();
            }));
        } else {
            rows.push_back(N::Action("resume", N::Text("gfx_resume"), [] {
                Debugger()->ResumeGame();
                N::GetModel().Close();
            }));
            rows.push_back(N::Link("commands", N::Text("gfx_display_list"), [] {
                return CommandsPage(Debugger()->GetDisplayList(), {});
            }));
            rows.push_back(N::Link("state", N::Text("gfx_state"), StatePage));
            rows.push_back(N::Link("preview", N::Text("gfx_preview"), [] {
                auto page = N::MakePage("gfx/preview", N::Text("gfx_preview"), [] {
                    return std::vector<N::Row>{N::Action("back", N::Text("back"), [] { N::GetModel().Back(); })};
                }, N::Text("gfx_preview_help"));
                page->gamePreview = true;
                return page;
            }));
        }
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
        return rows;
    });
}
}

void InitializeGraphicsDebugger() {
    N::RegisterPage("GfxDebugger##SoH", GraphicsPage, N::Text("gfx_debugger"));
    Backend()->SetOpenHandler([] {
        if (!CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::RequestPage("GfxDebugger##SoH");
    });
}
