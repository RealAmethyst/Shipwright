#include "OptionsControllers.h"
#include "soh/cvar_prefixes.h"

namespace NativeOptions {

PagePtr ControllerDpadPage(WidgetInfo& pause, WidgetInfo& text) {
    return MakePage("controller/dpad-controls", Text("dpad_options"), [&pause, &text] {
        std::vector<Row> rows;
        AppendWidget(rows, pause, "pause");
        AppendWidget(rows, text, "text");
        auto hold = CVarToggle(Text("dpad_hold"), CVAR_SETTING("DpadHoldChange"), true, Text("dpad_hold_help"));
        hold.enabled = CVarGetInteger(CVAR_SETTING("DPadOnPause"), 0) || CVarGetInteger(CVAR_SETTING("DpadInText"), 0);
        rows.push_back(std::move(hold));
        return rows;
    });
}

PagePtr ControllerCameraPage(WidgetInfo& mouse, WidgetInfo& autoCapture, WidgetInfo& freeLook) {
    return MakePage("controller/camera", Text("camera_controls"), [&mouse, &autoCapture, &freeLook] {
        std::vector<Row> rows;
        AppendWidget(rows, mouse, "mouse");
        AppendWidget(rows, autoCapture, "mouse_capture");
        rows.push_back(Link("first_person", Text("first_person_camera"), [] {
            return MakePage("controller/first-person", Text("first_person_camera"), [] {
                std::vector<Row> result;
                result.push_back(CVarToggle(Text("right_stick_aim"), CVAR_SETTING("Controls.RightStickAim"), false, Text("right_stick_aim_help")));
                auto move = CVarToggle(Text("first_person_move"), CVAR_SETTING("MoveInFirstPerson"), false, Text("first_person_move_help"));
                move.enabled = CVarGetInteger(CVAR_SETTING("Controls.RightStickAim"), 0);
                if (!move.enabled) move.disabledReason = Text("first_person_move_disabled");
                result.push_back(std::move(move));
                result.push_back(CVarToggle(Text("aim_invert_x"), CVAR_SETTING("Controls.InvertAimingXAxis"), false, Text("aim_invert_x_help")));
                result.push_back(CVarToggle(Text("aim_invert_y"), CVAR_SETTING("Controls.InvertAimingYAxis"), true, Text("aim_invert_y_help")));
                result.push_back(CVarToggle(Text("shield_invert_x"), CVAR_SETTING("Controls.InvertShieldAimingXAxis"), true, Text("shield_invert_x_help")));
                result.push_back(CVarToggle(Text("shield_invert_y"), CVAR_SETTING("Controls.InvertShieldAimingYAxis"), false, Text("shield_invert_y_help")));
                result.push_back(CVarToggle(Text("z_aim_invert_y"), CVAR_SETTING("Controls.InvertZAimingYAxis"), true, Text("z_aim_invert_y_help")));
                result.push_back(CVarToggle(Text("first_person_auto_center"), CVAR_SETTING("DisableFirstPersonAutoCenterView"), false, Text("first_person_auto_center_help")));
                result.push_back(CVarToggle(Text("first_person_sensitivity"), CVAR_SETTING("FirstPersonCameraSensitivity.Enabled"), false, "", [] {
                    if (!CVarGetInteger(CVAR_SETTING("FirstPersonCameraSensitivity.Enabled"), 0)) {
                        CVarClear(CVAR_SETTING("FirstPersonCameraSensitivity.X"));
                        CVarClear(CVAR_SETTING("FirstPersonCameraSensitivity.Y"));
                        SaveSettings();
                    }
                }));
                if (CVarGetInteger(CVAR_SETTING("FirstPersonCameraSensitivity.Enabled"), 0)) {
                    result.push_back(CVarDecimal(Text("first_person_sensitivity_x"), CVAR_SETTING("FirstPersonCameraSensitivity.X"), 1, 0.01f, 5, 0.01f, "", true));
                    result.push_back(CVarDecimal(Text("first_person_sensitivity_y"), CVAR_SETTING("FirstPersonCameraSensitivity.Y"), 1, 0.01f, 5, 0.01f, "", true));
                }
                return result;
            });
        }));
        rows.push_back(Link("third_person", Text("third_person_camera"), [&freeLook] {
            return MakePage("controller/third-person", Text("third_person_camera"), [&freeLook] {
                std::vector<Row> result;
                AppendWidget(result, freeLook, "free_look");
                result.push_back(CVarToggle(Text("camera_invert_x"), CVAR_SETTING("FreeLook.InvertXAxis"), false, Text("camera_invert_x_help")));
                result.push_back(CVarToggle(Text("camera_invert_y"), CVAR_SETTING("FreeLook.InvertYAxis"), true, Text("camera_invert_y_help")));
                result.push_back(CVarDecimal(Text("third_person_sensitivity_x"), CVAR_SETTING("FreeLook.CameraSensitivity.X"), 1, 0.01f, 5, 0.01f, "", true));
                result.push_back(CVarDecimal(Text("third_person_sensitivity_y"), CVAR_SETTING("FreeLook.CameraSensitivity.Y"), 1, 0.01f, 5, 0.01f, "", true));
                result.push_back(CVarInteger(Text("camera_distance"), CVAR_SETTING("FreeLook.MaxCameraDistance"), 185, 100, 900));
                result.push_back(CVarInteger(Text("camera_transition"), CVAR_SETTING("FreeLook.TransitionSpeed"), 25, 0, 900));
                return result;
            });
        }));
        return rows;
    });
}
}
