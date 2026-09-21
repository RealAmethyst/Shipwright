#include "Anchor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include <libultraship/libultraship.h>
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/util.h"

namespace SohGui {
extern std::shared_ptr<SohMenu> mSohMenu;
extern std::shared_ptr<AnchorRoomWindow> mAnchorRoomWindow;
} // namespace SohGui

#ifdef ENABLE_REMOTE_CONTROL
void RegisterAnchorMenu() {
    WidgetPath path = { "Network", "Anchor", SECTION_COLUMN_1 };
    SohGui::mSohMenu->AddWidget(path, "AnchorMainMenu", WIDGET_CUSTOM)
        .NativePage([] { return NativeOptions::RegisteredPage("Network/Anchor/AnchorMainMenu"); }, NativeOptions::Text("connection_settings"));
    path.column = SECTION_COLUMN_2;
    SohGui::mSohMenu->AddWidget(path, "AnchorAdminMenu", WIDGET_CUSTOM)
        .NativePage([] { return NativeOptions::RegisteredPage("Network/Anchor/AnchorAdminMenu"); }, NativeOptions::Text("room_settings"));
    SohGui::mSohMenu->AddWidget(path, "AnchorInstructionsMenu", WIDGET_CUSTOM)
        .NativePage([] { return NativeOptions::RegisteredPage("Network/Anchor/AnchorInstructionsMenu"); }, NativeOptions::Text("usage_instructions"));
}

static RegisterMenuInitFunc menuInitFunc(RegisterAnchorMenu);
#endif
