#pragma once

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace NativeOptions {
struct OverlayGeometry {
    ImVec2 position;
    ImVec2 size;
    bool docked = false;
};

inline OverlayGeometry ReadOverlayGeometry(const char* name, ImVec2 defaultSize = {}) {
    const auto* viewport = ImGui::GetMainViewport();
    if (const auto* window = ImGui::FindWindowByName(name))
        return {{window->Pos.x - viewport->Pos.x, window->Pos.y - viewport->Pos.y}, window->SizeFull,
                window->DockId != 0};
    if (const auto* settings = ImGui::FindWindowSettingsByID(ImHashStr(name))) {
        const ImVec2 origin = settings->ViewportId ? ImVec2(settings->ViewportPos.x, settings->ViewportPos.y) : viewport->Pos;
        return {{settings->Pos.x + origin.x - viewport->Pos.x, settings->Pos.y + origin.y - viewport->Pos.y},
                {settings->Size.x > 0 ? static_cast<float>(settings->Size.x) : defaultSize.x,
                 settings->Size.y > 0 ? static_cast<float>(settings->Size.y) : defaultSize.y}, settings->DockId != 0};
    }
    return {{60, 60}, defaultSize, false};
}

inline void WriteOverlayGeometry(const char* name, ImVec2 position, ImVec2 size, bool undock = false) {
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(size.x) || !std::isfinite(size.y)) return;
    position.x = std::clamp(position.x, -32768.0f, 32767.0f);
    position.y = std::clamp(position.y, -32768.0f, 32767.0f);
    size.x = std::clamp(size.x, 0.0f, 32767.0f);
    size.y = std::clamp(size.y, 0.0f, 32767.0f);
    const auto* viewport = ImGui::GetMainViewport();
    if (auto* window = ImGui::FindWindowByName(name)) {
        if (undock && window->DockId) ImGui::DockContextProcessUndockWindow(ImGui::GetCurrentContext(), window);
        ImGui::SetWindowPos(window, {viewport->Pos.x + position.x, viewport->Pos.y + position.y}, ImGuiCond_Always);
        if (size.x > 0 && size.y > 0) ImGui::SetWindowSize(window, size, ImGuiCond_Always);
    } else {
        auto* settings = ImGui::FindWindowSettingsByID(ImHashStr(name));
        if (!settings) settings = ImGui::CreateNewWindowSettings(name);
        settings->Pos = ImVec2ih(position);
        settings->Size = ImVec2ih(size);
        settings->ViewportId = viewport->ID;
        settings->ViewportPos = ImVec2ih(viewport->Pos);
        settings->WantDelete = false;
        if (undock) settings->DockId = 0;
    }
    ImGui::MarkIniSettingsDirty();
}
}
