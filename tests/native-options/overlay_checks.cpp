#include "OptionsOverlayGeometry.h"
#include "OptionsTableLayout.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace NativeOptions;

static void Check(bool condition, const char* reason) {
    if (!condition) throw std::runtime_error(reason);
}

static void DrawSplitTable(ImGuiID expectedId) {
    ImGui::NewFrame();
    ImGui::SetNextWindowSize({450, 660});
    ImGui::Begin("Time Splits");
    ImGui::BeginChild("SplitTable");
    Check(ImGui::GetID("Splits") == expectedId, "table identity must match the renderer's actual child ID");
    if (ImGui::BeginTable("Splits", 5, ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable)) {
        for (int i = 0; i < 5; ++i) ImGui::TableSetupColumn(std::to_string(i).c_str());
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("Sample");
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::End();
    ImGui::EndFrame();
}

int main() {
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = {1280, 720};
    io.DeltaTime = 1.0f / 60;
    io.Fonts->Build();
    auto* viewport = ImGui::GetMainViewport();
    viewport->Pos = {100, 200};

    WriteOverlayGeometry("Unopened tracker", {42, 84}, {400, 540});
    auto layout = ReadOverlayGeometry("Unopened tracker");
    Check(layout.position.x == 42 && layout.position.y == 84 && layout.size.y == 540, "unopened layout");
    auto* settings = ImGui::FindWindowSettingsByID(ImHashStr("Unopened tracker"));
    Check(settings && settings->ViewportPos.x == 100 && settings->Pos.y == 84, "viewport-relative persistence");
    settings->DockId = 123;
    WriteOverlayGeometry("Unopened tracker", {43, 85}, {401, 541});
    Check(ReadOverlayGeometry("Unopened tracker").docked, "preserve docking");
    WriteOverlayGeometry("Unopened tracker", {43, 85}, {401, 541}, true);
    Check(!ReadOverlayGeometry("Unopened tracker").docked, "explicit undocking");
    WriteOverlayGeometry("Unopened tracker", {std::numeric_limits<float>::quiet_NaN(), 0}, {10, 20});
    Check(ReadOverlayGeometry("Unopened tracker").position.x == 43, "reject invalid geometry");

    ImGui::NewFrame();
    ImGui::Begin("Unopened tracker");
    ImGui::End();
    ImGui::EndFrame();
    layout = ReadOverlayGeometry("Unopened tracker");
    Check(layout.size.x == 401 && layout.size.y == 541, "first opening loads configured size");
    WriteOverlayGeometry("Unopened tracker", {120, 90}, {450, 300});
    layout = ReadOverlayGeometry("Unopened tracker");
    Check(layout.position.x == 120 && layout.position.y == 90 && layout.size.x == 450, "live window update");
    WriteOverlayGeometry("Another tracker", {10, 20}, {200, 250});
    const auto tableId = ChildTableId("Time Splits", "SplitTable", "Splits");
    auto columns = ReadTableLayout(tableId, 5);
    Check(columns[4].order == 4 && columns[4].visible, "unopened table defaults");
    columns[0].visible = false;
    std::swap(columns[0].order, columns[4].order);
    Check(WriteTableLayout(tableId, columns), "write unopened table layout");
    DrawSplitTable(tableId);
    auto* table = ImGui::TableFindByID(tableId);
    Check(table && !table->Columns[0].IsUserEnabled && table->Columns[4].DisplayOrder == 0,
          "renderer must consume unopened column visibility and order");
    columns = ReadTableLayout(tableId, 5);
    columns[1].visible = false;
    std::swap(columns[1].order, columns[3].order);
    Check(WriteTableLayout(tableId, columns), "write live table layout");
    Check(!ReadTableLayout(tableId, 5)[1].visible, "pending live changes must be readable before rendering");
    DrawSplitTable(tableId);
    Check(!table->Columns[1].IsUserEnabled && table->Columns[3].DisplayOrder == 1,
          "live table must load native column changes");
    auto invalid = columns;
    invalid[1].order = invalid[2].order;
    Check(!WriteTableLayout(tableId, invalid), "reject duplicate column positions");
    for (auto& column : invalid) column.visible = false;
    Check(!WriteTableLayout(tableId, invalid), "reject hiding every column");
    const std::string saved = ImGui::SaveIniSettingsToMemory();
    Check(saved.find("[Window][Another tracker]") != std::string::npos, "unopened window saved");
    ImGui::DestroyContext();

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::LoadIniSettingsFromMemory(saved.c_str(), saved.size());
    layout = ReadOverlayGeometry("Unopened tracker");
    Check(layout.position.x == 120 && layout.position.y == 90 && layout.size.x == 450, "layout restart persistence");
    Check(ReadOverlayGeometry("Another tracker").size.y == 250, "other window preserved");
    columns = ReadTableLayout(tableId, 5);
    Check(!columns[0].visible && !columns[1].visible && columns[3].order == 1 && columns[4].order == 0,
          "native column edits must persist in the existing ImGui settings");
    ImGui::DestroyContext();
    std::cout << "Overlay layout, existing-window updates, and persistence checks passed.\n";
}
