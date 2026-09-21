#include "Menu.h"

namespace Ship {
Menu::Menu(const std::string& cVar, const std::string& name) : GuiWindow(cVar, name) {
}

void Menu::InitElement() {
}

void Menu::UpdateElement() {
}

void Menu::Draw() {
}

void Menu::DrawElement() {
}

UIWidgets::Colors Menu::GetMenuThemeColor() {
    return UIWidgets::Colors::LightBlue;
}

void Menu::AddMenuEntry(std::string entryName, const char* entryCvar) {
    menuEntries.emplace(entryName, MainMenuEntry{ entryName, entryCvar });
    menuOrder.push_back(entryName);
}

void Menu::AddSearchWidget(SearchWidget widget) {
    extraSearchWidgets.push_back(widget);
}

std::unordered_map<uint32_t, disabledInfo>& Menu::GetDisabledMap() {
    return disabledMap;
}

std::unordered_map<std::string, MainMenuEntry>& Menu::GetEntries() {
    return menuEntries;
}

const std::vector<std::string>& Menu::GetEntryOrder() const {
    return menuOrder;
}

std::vector<SearchWidget>& Menu::GetExtraWidgets() {
    return extraSearchWidgets;
}

void Menu::RefreshDisabledState() {
    for (auto& [reason, info] : disabledMap) {
        info.active = info.evaluation(info);
    }
}

} // namespace Ship
