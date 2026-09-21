#ifndef MENU_H
#define MENU_H

#include <libultraship/libultraship.h>
#include <fast/backends/gfx_rendering_api.h>
#include "MenuTypes.h"

namespace Ship {
class Menu : public GuiWindow {
  public:
    Menu(const std::string& cVar, const std::string& name);
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
    void Draw() override;
    UIWidgets::Colors GetMenuThemeColor();

    void AddMenuEntry(std::string entryName, const char* entryCvar);
    void AddSearchWidget(SearchWidget widget);
    std::unordered_map<uint32_t, disabledInfo>& GetDisabledMap();
    std::unordered_map<std::string, MainMenuEntry>& GetEntries();
    const std::vector<std::string>& GetEntryOrder() const;
    std::vector<SearchWidget>& GetExtraWidgets();
    void RefreshDisabledState();

  protected:
    std::unordered_map<std::string, MainMenuEntry> menuEntries;
    std::vector<std::string> menuOrder;
    std::vector<SearchWidget> extraSearchWidgets;
    std::unordered_map<uint32_t, disabledInfo> disabledMap;
};
} // namespace Ship

#endif // MENU_H
