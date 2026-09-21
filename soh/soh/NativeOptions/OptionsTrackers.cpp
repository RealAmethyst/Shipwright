#include "OptionsTrackers.h"
#include "soh/Enhancements/randomizer/randomizerTypes.h"
#include <libultraship/libultraship.h>

namespace NativeOptions {
void AppendTrackerDisplayRows(std::vector<Row>& rows, const std::string& prefix, int defaultWindow,
                              const std::string& displayKey, const std::map<int, std::string>& modes,
                              const std::map<int, std::string>& buttons, std::function<void()> changed) {
    if (CVarGetInteger((prefix + "WindowType").c_str(), defaultWindow) != TRACKER_WINDOW_FLOATING) return;
    rows.push_back(CVarToggle(Text("tracker_dragging"), prefix + "Draggable", true, "", changed));
    rows.push_back(CVarToggle(Text("tracker_paused"), prefix + "ShowOnlyPaused", false, "", changed));
    rows.push_back(CVarChoice(Text("tracker_display"), prefix + displayKey, TRACKER_DISPLAY_ALWAYS, modes, "", changed));
    if (CVarGetInteger((prefix + displayKey).c_str(), TRACKER_DISPLAY_ALWAYS) == TRACKER_DISPLAY_COMBO_BUTTON) {
        rows.push_back(CVarChoice(Text("tracker_button1"), prefix + "ComboButton1", TRACKER_COMBO_BUTTON_L, buttons, "", changed));
        rows.push_back(CVarChoice(Text("tracker_button2"), prefix + "ComboButton2", TRACKER_COMBO_BUTTON_R, buttons, "", changed));
    }
}
}
