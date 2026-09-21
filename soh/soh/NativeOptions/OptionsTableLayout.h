#pragma once
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace NativeOptions {
struct TableColumnLayout {
    int order;
    bool visible;
};

inline ImGuiID ChildTableId(const char* window, const char* child, const char* table) {
    const auto childId = ImHashStr(child, 0, ImHashStr(window));
    char suffix[16];
    std::snprintf(suffix, sizeof(suffix), "_%08X", childId);
    const auto childName = std::string(window) + "/" + child + suffix;
    return ImHashStr(table, 0, ImHashStr(childName.c_str()));
}

inline std::vector<TableColumnLayout> ReadTableLayout(ImGuiID id, int count) {
    std::vector<TableColumnLayout> result;
    for (int i = 0; i < count; ++i) result.push_back({i, true});
    auto* table = ImGui::TableFindByID(id);
    auto* settings = ImGui::TableSettingsFindByID(id);
    if (table && table->ColumnsCount == count && table->Columns.size() == count &&
        !table->IsSettingsRequestLoad && !(settings && settings->WantApply)) {
        for (int i = 0; i < count; ++i)
            result[i] = {table->Columns[i].DisplayOrder, table->Columns[i].IsUserEnabledNextFrame};
    } else if (settings && settings->ColumnsCount == count) {
        for (int i = 0; i < count; ++i) {
            const auto& column = settings->GetColumnSettings()[i];
            if (column.Index < 0 || column.Index >= count) continue;
            if (settings->SaveFlags & ImGuiTableFlags_Reorderable) result[column.Index].order = column.DisplayOrder;
            if (settings->SaveFlags & ImGuiTableFlags_Hideable) result[column.Index].visible = column.IsEnabled != 0;
        }
    }
    std::vector<bool> seen(count);
    bool valid = true;
    for (const auto& column : result) {
        if (column.order < 0 || column.order >= count || seen[column.order]) { valid = false; break; }
        seen[column.order] = true;
    }
    if (!valid) for (int i = 0; i < count; ++i) result[i].order = i;
    return result;
}

inline bool WriteTableLayout(ImGuiID id, const std::vector<TableColumnLayout>& columns) {
    const int count = static_cast<int>(columns.size());
    if (!count || count > 64 || std::none_of(columns.begin(), columns.end(), [](auto c) { return c.visible; })) return false;
    std::vector<bool> seen(count);
    for (const auto& column : columns) {
        if (column.order < 0 || column.order >= count || seen[column.order]) return false;
        seen[column.order] = true;
    }
    auto* settings = ImGui::TableSettingsFindByID(id);
    if (settings && settings->ColumnsCountMax < count) { settings->ID = 0; settings = nullptr; }
    if (!settings) settings = ImGui::TableSettingsCreate(id, count);
    settings->ColumnsCount = static_cast<ImGuiTableColumnIdx>(count);
    settings->SaveFlags |= ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable;
    settings->WantApply = true;
    for (int i = 0; i < count; ++i) {
        auto& column = settings->GetColumnSettings()[i];
        column.Index = static_cast<ImGuiTableColumnIdx>(i);
        column.DisplayOrder = static_cast<ImGuiTableColumnIdx>(columns[i].order);
        column.IsEnabled = columns[i].visible;
    }
    if (auto* table = ImGui::TableFindByID(id)) {
        table->IsSettingsRequestLoad = true;
        table->IsSettingsDirty = false;
        table->SettingsOffset = ImGui::GetCurrentContext()->SettingsTables.offset_from_ptr(settings);
    }
    ImGui::MarkIniSettingsDirty();
    return true;
}
}
