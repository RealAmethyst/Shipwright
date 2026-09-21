#include "hookDebugger.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <libultraship/libultraship.h>
#include <version>

namespace {
namespace N = NativeOptions;
static std::map<std::string, std::map<HOOK_ID, HookInfo>*> hookData;

void AppendHooks(std::vector<N::Row>& rows, const std::string& name, bool includeName) {
    const auto found = hookData.find(name);
    if (found == hookData.end() || !found->second) return;
    for (const auto& [id, info] : *found->second) {
        const auto label = (includeName ? name + ", " : "") + N::Text("hook_id") + " " + std::to_string(id);
        auto row = N::Action(name + "/" + std::to_string(id), label, [] { N::ReadCurrentDescription(); });
        const std::map<int, std::string> types = {{HOOK_TYPE_NORMAL, N::Text("hook_normal")}, {HOOK_TYPE_ID, "ID"},
            {HOOK_TYPE_PTR, "Ptr"}, {HOOK_TYPE_FILTER, N::Text("filter")}};
        const auto type = types.find(info.registering.type);
        row.value = (type == types.end() ? N::Text("unknown") : type->second) + ", " + N::Text("hook_calls") + " " + std::to_string(info.calls);
        row.description = info.registering.valid ?
            fmt::format("{}({}:{}), {}", info.registering.file, info.registering.line, info.registering.column, info.registering.function) :
            N::Text("unavailable");
        rows.push_back(row);
    }
}

N::PagePtr HooksPage() {
    auto filter = std::make_shared<std::string>();
    return N::MakePage("advanced/hooks", N::Text("hook_debugger"), [=] {
        std::vector<N::Row> rows{
            N::String("filter", N::Text("filter"), *filter, [=](std::string next) { *filter = std::move(next); }),
            N::Link("all", N::Text("hook_all"), [] {
                return N::MakePage("advanced/hooks/all", N::Text("hook_all"), [] {
                    std::vector<N::Row> rows;
                    for (const auto& [name, data] : hookData) AppendHooks(rows, name, true);
                    return rows;
                });
            })
        };
        ImGuiTextFilter search(filter->c_str());
        for (const auto& [name, data] : hookData) {
            if (!search.PassFilter(name.c_str())) continue;
            auto row = N::Link(name, name, [name] {
                return N::MakePage("advanced/hooks/" + name, name, [=] {
                    std::vector<N::Row> rows;
                    AppendHooks(rows, name, false);
                    return rows;
                });
            });
            row.value = N::Text("hook_total") + " " + std::to_string(data->size());
            rows.push_back(row);
        }
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_lockout"));
        return rows;
    }
#ifndef __cpp_lib_source_location
    , N::Text("hook_source_unavailable")
#endif
    );
}
} // namespace

void InitializeHookDebugger() {
#define DEFINE_HOOK(name, _) hookData.emplace(#name, GameInteractor::Instance->GetHookData<GameInteractor::name>());
#include "../game-interactor/GameInteractor_HookTable.h"
#undef DEFINE_HOOK
    N::RegisterPage("Hook Debugger", HooksPage);
}
