#include "OptionsControllers.h"
#include <fast/Fast3dWindow.h>
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/cvar_prefixes.h"

namespace NativeOptions {
namespace {
using namespace UIWidgets;
WidgetInfo dpadOcarina, freeLook, mouseControl, mouseAutoCapture, rightStickOcarina, dpadPause, dpadText;

void RegisterInputEditorWidgets() {
    dpadOcarina = { .name = "Dpad Ocarina Playback", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    dpadOcarina.CVar(CVAR_SETTING("CustomOcarina.Dpad")).Options(CheckboxOptions());
    SohGui::GetSohMenu()->AddSearchWidget({ dpadOcarina, "Settings", "Controls", "Ocarina Controls", "" });

    freeLook = { .name = "Free Look", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    freeLook.CVar(CVAR_SETTING("FreeLook.Enabled"))
        .Options(
            CheckboxOptions()
                .Tooltip(
                    "Enables free look camera control\nNote: You must remap C buttons off of the right stick in the "
                    "controller config menu, and map the camera stick to the right stick.\n"
                    "Doesn't work in areas were the game locks the camera.\n"
                    "Scene reload may be necessary to enable."));
    SohGui::GetSohMenu()->AddSearchWidget({ freeLook, "Settings", "Controls", "Camera Controls" });

    mouseControl = { .name = "Enable Mouse Controls", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    mouseControl.CVar(CVAR_SETTING("EnableMouse"))
        .Callback([](WidgetInfo& info) {
            bool enabled =
                CVarGetInteger(CVAR_SETTING("EnableMouse"), 0) && CVarGetInteger(CVAR_SETTING("AutoCaptureMouse"), 1);
            auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
            wnd->SetAutoCaptureMouse(enabled);
        })
        .Options(
            CheckboxOptions()
                .Tooltip("Allows for using the mouse to control the camera (must enable Free Look), "
                         "aim with the shield, and perform quickspin attacks (quickly rotate the mouse then press B)\n"
                         "Press F2 to toggle mouse capture manually."));
    SohGui::GetSohMenu()->AddSearchWidget({ mouseControl, "Settings", "Controls", "Camera Controls" });

    mouseAutoCapture = { .name = "Auto Capture Mouse Input", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    mouseAutoCapture.CVar(CVAR_SETTING("AutoCaptureMouse"))
        .Callback([](WidgetInfo& info) {
            bool enabled =
                CVarGetInteger(CVAR_SETTING("EnableMouse"), 0) && CVarGetInteger(CVAR_SETTING("AutoCaptureMouse"), 1);
            auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
            wnd->SetAutoCaptureMouse(enabled);
        })
        .Options(CheckboxOptions()
                     .Tooltip("When Mouse Controls are enabled, this toggles whether the program will automatically "
                              "hide the cursor "
                              "and capture mouse input when closing the menu."));
    SohGui::GetSohMenu()->AddSearchWidget({ mouseAutoCapture, "Settings", "Controls", "Camera Controls" });

    rightStickOcarina = { .name = "Right Stick Ocarina Playback", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    rightStickOcarina.CVar(CVAR_SETTING("CustomOcarina.RightStick")).Options(CheckboxOptions());
    SohGui::GetSohMenu()->AddSearchWidget({ rightStickOcarina, "Settings", "Controls", "Ocarina Controls" });

    dpadPause = { .name = "D-pad Support on Pause Screen", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    dpadPause.CVar(CVAR_SETTING("DPadOnPause"))
        .Options(CheckboxOptions()
                     .Tooltip("Navigate Pause with the D-pad\nIf used with \"D-pad as Equip Items\", you must hold "
                              "C-Up to equip instead of navigate"));
    SohGui::GetSohMenu()->AddSearchWidget({ dpadPause, "Settings", "Controls", "Dpad Controls" });

    dpadText = { .name = "D-pad Support in Text Boxes", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    dpadText.CVar(CVAR_SETTING("DpadInText"))
        .Options(CheckboxOptions()
                     .Tooltip("Navigate choices in text boxes, shop item selection, and the file select / name entry "
                              "screens with the D-pad"));
    SohGui::GetSohMenu()->AddSearchWidget({ dpadText, "Settings", "Controls", "Dpad Controls" });
    RegisterPage("Configure Controller", [] {
        return ControllerBindingsPage(BeginControllerCapture,
            {&freeLook, &mouseControl, &mouseAutoCapture, &rightStickOcarina, &dpadOcarina, &dpadPause, &dpadText});
    }, Text("bind_title"));
}

static RegisterMenuInitFunc menuInitFunc(RegisterInputEditorWidgets);
}
}
