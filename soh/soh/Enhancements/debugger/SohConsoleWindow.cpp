#include "SohConsoleWindow.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/OTRGlobals.h"
#include <ship/window/gui/ConsoleWindow.h>

namespace {
namespace N = NativeOptions;
std::string command;
std::string filter;
int level = spdlog::level::trace;

std::shared_ptr<Ship::ConsoleWindow> Backend() {
    return std::dynamic_pointer_cast<Ship::ConsoleWindow>(
        Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow("Console"));
}

N::PagePtr OutputPage() {
    return N::MakePage("console/output", N::Text("console_output"), [] {
        std::vector<N::Row> rows;
        auto backend = Backend();
        const auto channel = backend->GetCurrentChannel();
        rows.push_back(N::Choice("channel", N::Text("console_channel"), channel == "Logs",
            {{0, "Console"}, {1, "Logs"}}, [backend](int value) { backend->SetCurrentChannel(value ? "Logs" : "Console"); }));
        rows.push_back(N::String("filter", N::Text("filter"), filter, [](std::string value) { filter = std::move(value); }));
        if (channel != "Console") {
            std::map<int, std::string> levels;
            for (int i = spdlog::level::trace; i <= spdlog::level::off; ++i) {
                const auto name = spdlog::level::to_string_view(static_cast<spdlog::level::level_enum>(i));
                levels[i] = std::string(name.data(), name.size());
            }
            rows.push_back(N::Choice("level", N::Text("console_level"), level, levels, [](int value) { level = value; }));
        }
        rows.push_back(N::Action("clear", N::Text("clear"), [backend, channel] { backend->ClearLogs(channel); }));
        const auto lines = backend->GetLines(channel);
        for (size_t i = 0; i < lines.size(); ++i) {
            const auto& line = lines[i];
            if ((!filter.empty() && line.Text.find(filter) == std::string::npos) ||
                (channel != "Console" && line.Priority < level)) continue;
            rows.push_back(N::Link("line/" + std::to_string(i), line.Text, [line] {
                return N::MakePage("console/line", N::Text("console_output"), [line] {
                    return std::vector<N::Row>{
                        N::Action("read", N::Text("read_details"), [] { N::ReadCurrentDescription(); }, line.Text),
                        N::Action("copy", N::Text("copy_text"), [line] { ImGui::SetClipboardText(line.Text.c_str()); })};
                }, line.Text);
            }));
        }
        return rows;
    });
}

N::PagePtr CommandsPage(bool history) {
    return N::MakePage(history ? "console/history" : "console/commands",
                       N::Text(history ? "console_history" : "console_commands"), [history] {
        std::vector<N::Row> rows;
        auto add = [&rows](std::string id, std::string text, std::string description) {
            rows.push_back(N::Action(std::move(id), text, [text] {
                N::GetModel().Back([text] { command = text; });
            }, std::move(description)));
        };
        if (history) {
            const auto commands = Backend()->GetHistory();
            for (size_t i = commands.size(); i > 0; --i)
                add("history/" + std::to_string(i - 1), commands[i - 1], "");
        } else {
            rows.push_back(N::String("filter", N::Text("filter"), filter,
                                    [](std::string value) { filter = std::move(value); }));
            auto console = Ship::Context::GetInstance()->GetConsole();
            for (const auto& [name, entry] : console->GetCommands()) {
                if (!filter.empty() && name.find(filter) == std::string::npos &&
                    entry.Description.find(filter) == std::string::npos) continue;
                const auto usage = console->BuildUsage(entry);
                add("command/" + name, name, entry.Description +
                    (usage == "None" ? "" : "\n" + name + " " + usage));
            }
        }
        return rows;
    });
}

N::PagePtr ConsolePage() {
    return N::MakePage("advanced/console", N::Text("console"), [] {
        std::vector<N::Row> rows;
        rows.push_back(N::String("command", N::Text("console_command"), command,
            [](std::string value) { command = std::move(value); }, "", 4096));
        auto submit = N::Action("submit", N::Text("console_submit"), [] {
            Backend()->Dispatch(command);
            command.clear();
            N::GetModel().Push(OutputPage());
        });
        submit.enabled = !command.empty() && command.front() != ' ';
        rows.push_back(std::move(submit));
        rows.push_back(N::Link("commands", N::Text("console_commands"), [] { return CommandsPage(false); }));
        rows.push_back(N::Link("history", N::Text("console_history"), [] { return CommandsPage(true); }));
        rows.push_back(N::Link("output", N::Text("console_output"), OutputPage));
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_disabled"));
        return rows;
    });
}
}

void InitializeConsolePages() {
    N::RegisterPage("Console##SoH", ConsolePage, N::Text("console"));
    Backend()->SetOpenHandler([] {
        if (!CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::RequestPage("Console##SoH");
    });
}
