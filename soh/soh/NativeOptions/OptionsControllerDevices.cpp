#include "OptionsControllers.h"
#include "soh/OTRGlobals.h"
#include "z64.h"
#include "soh/cvar_prefixes.h"
#include <SDL.h>

namespace NativeOptions {
namespace {
std::shared_ptr<Ship::ControllerRumbleMapping> rumblePreview;
uint32_t rumbleUntil = 0;

void StopRumble() {
    if (rumblePreview)
        rumblePreview->StopRumble();
    rumblePreview.reset();
}

Row Percent(std::string id, std::string label, int value, int maximum, std::function<void(int)> set) {
    auto row = Integer(id, label, value, 0, maximum, 1, set);
    row.value += "%";
    return row;
}

PagePtr RGBPage(std::string title, std::function<Color_RGB8()> get, std::function<void(Color_RGB8)> set) {
    return MakePage("controller/color", title, [=] {
        const auto color = get();
        std::vector<Row> rows;
        const uint8_t values[] = {color.r, color.g, color.b};
        const char* keys[] = {"red", "green", "blue"};
        for (int i = 0; i < 3; ++i)
            rows.push_back(Integer(keys[i], Text(keys[i]), values[i], 0, 255, 1, [=](int value) {
                auto next = get();
                if (i == 0) next.r = value;
                if (i == 1) next.g = value;
                if (i == 2) next.b = value;
                set(next);
            }));
        return rows;
    });
}
}

bool TestingControllerRumble() {
    return rumblePreview != nullptr;
}

void UpdateControllerPreview() {
    if (!rumblePreview)
        return;
    if (static_cast<int32_t>(SDL_GetTicks() - rumbleUntil) >= 0 || !GetModel().IsOpen())
        StopRumble();
    else
        rumblePreview->StartRumble();
}

PagePtr ControllerRumblePage(uint8_t port, CaptureRequest capture) {
    auto rumble = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetRumble();
    auto page = MakePage("controller/rumble", Text("rumble"), [=] {
        std::vector<Row> rows;
        const auto mappings = rumble->GetAllRumbleMappings();
        // Stable ordering is also the spoken ordering when a device is added or removed.
        const std::map<std::string, std::shared_ptr<Ship::ControllerRumbleMapping>> ordered(mappings.begin(), mappings.end());
        for (const auto& [id, mapping] : ordered)
            rows.push_back(Link(id, mapping->GetPhysicalDeviceName(), [=] {
                auto device = MakePage(id, mapping->GetPhysicalDeviceName(), [=] {
                    std::vector<Row> result;
                    result.push_back(Action("test", Text(rumblePreview == mapping ? "stop" : "test"), [=] {
                        const bool wasTesting = rumblePreview == mapping;
                        StopRumble();
                        if (!wasTesting) {
                            rumblePreview = mapping;
                            rumbleUntil = SDL_GetTicks() + 1000;
                        }
                    }));
                    result.push_back(Percent("small", Text("small_motor"), mapping->GetHighFrequencyIntensityPercentage(), 100,
                        [=](int value) { mapping->SetHighFrequencyIntensity(value); mapping->SaveToConfig(); }));
                    result.push_back(Percent("large", Text("large_motor"), mapping->GetLowFrequencyIntensityPercentage(), 100,
                        [=](int value) { mapping->SetLowFrequencyIntensity(value); mapping->SaveToConfig(); }));
                    result.push_back(Action("reset_small", Text("reset_small_motor"), [=] { mapping->ResetHighFrequencyIntensityToDefault(); }));
                    result.push_back(Action("reset_large", Text("reset_large_motor"), [=] { mapping->ResetLowFrequencyIntensityToDefault(); }));
                    result.push_back(Action("remove", Text("remove"), [=] { StopRumble(); rumble->ClearRumbleMapping(id); GetModel().Back(); }));
                    return result;
                });
                device->onClose = StopRumble;
                return device;
            }));
        rows.push_back(Action("add", Text("add_rumble"), [=] { capture([=] { return rumble->AddRumbleMappingFromRawPress(); }); }));
        return rows;
    });
    page->onClose = StopRumble;
    return page;
}

PagePtr ControllerGyroPage(uint8_t port, CaptureRequest capture) {
    auto gyro = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetGyro();
    return MakePage("controller/gyro", Text("gyro"), [=] {
        std::vector<Row> rows;
        const auto mapping = gyro->GetGyroMapping();
        if (!mapping) {
            rows.push_back(Action("add", Text("add_gyro"), [=] { capture([=] { return gyro->SetGyroMappingFromRawPress(); }); }));
            return rows;
        }
        float pitch = 0, yaw = 0;
        mapping->UpdatePad(pitch, yaw);
        auto preview = Action("preview", mapping->GetPhysicalDeviceName(), {});
        preview.value = Text("pitch") + " " + std::to_string(pitch) + ", " + Text("yaw") + " " + std::to_string(yaw);
        rows.push_back(std::move(preview));
        rows.push_back(Percent("sensitivity", Text("sensitivity"), mapping->GetSensitivityPercent(), 100,
            [=](int value) { mapping->SetSensitivity(value); mapping->SaveToConfig(); }));
        rows.push_back(Action("reset", Text("reset_sensitivity"), [=] { mapping->ResetSensitivityToDefault(); }));
        rows.push_back(Action("calibrate", Text("recalibrate"), [=] { mapping->Recalibrate(); mapping->SaveToConfig(); }));
        rows.push_back(Action("remove", Text("remove"), [=] { gyro->ClearGyroMapping(); }));
        return rows;
    });
}

PagePtr ControllerLEDPage(uint8_t port, CaptureRequest capture) {
    auto led = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetLED();
    return MakePage("controller/led", Text("leds"), [=] {
        std::vector<Row> rows;
        const auto mappings = led->GetAllLEDMappings();
        const std::map<std::string, std::shared_ptr<Ship::ControllerLEDMapping>> ordered(mappings.begin(), mappings.end());
        for (const auto& [id, mapping] : ordered)
            rows.push_back(Link(id, mapping->GetPhysicalDeviceName(), [=] {
                return MakePage(id, mapping->GetPhysicalDeviceName(), [=] {
                    std::vector<Row> result;
                    result.push_back(Choice("source", Text("led_color"), mapping->GetColorSource(),
                        {{LED_COLOR_SOURCE_OFF, Text("off")}, {LED_COLOR_SOURCE_SET, Text("set")}, {LED_COLOR_SOURCE_GAME, Text("game")}},
                        [=](int value) { mapping->SetColorSource(value); }));
                    if (mapping->GetColorSource() == LED_COLOR_SOURCE_SET)
                        result.push_back(Link("color", Text("color"), [=] {
                            return RGBPage(Text("led_color"), [=] { return mapping->GetSavedColor(); },
                                           [=](Color_RGB8 value) { mapping->SetSavedColor(value); });
                        }));
                    if (mapping->GetColorSource() == LED_COLOR_SOURCE_GAME) {
                        result.push_back(CVarChoice(Text("source"), CVAR_SETTING("LEDColorSource"), LED_SOURCE_TUNIC_ORIGINAL,
                            {{LED_SOURCE_TUNIC_ORIGINAL, Text("led_tunic_original")}, {LED_SOURCE_TUNIC_COSMETICS, Text("led_tunic_cosmetics")},
                             {LED_SOURCE_HEALTH, Text("led_health")}, {LED_SOURCE_NAVI_ORIGINAL, Text("led_navi_original")},
                             {LED_SOURCE_NAVI_COSMETICS, Text("led_navi_cosmetics")}, {LED_SOURCE_CUSTOM, Text("custom")}}, Text("led_source_help")));
                        if (CVarGetInteger(CVAR_SETTING("LEDColorSource"), LED_SOURCE_TUNIC_ORIGINAL) == LED_SOURCE_CUSTOM)
                            result.push_back(Link("custom_color", Text("custom_color"), [] {
                                return RGBPage(Text("custom_color"), [] { return CVarGetColor24(CVAR_SETTING("LEDPort1Color"), {255, 255, 255}); },
                                    [](Color_RGB8 value) { CVarSetColor24(CVAR_SETTING("LEDPort1Color"), value); ChangedCVar(CVAR_SETTING("LEDPort1Color")); });
                            }));
                        result.push_back(CVarDecimal(Text("brightness"), CVAR_SETTING("LEDBrightness"), 1, 0, 1, 0.01f, Text("led_brightness_help"), true));
                        auto critical = CVarToggle(Text("led_critical"), CVAR_SETTING("LEDCriticalOverride"), true, Text("led_critical_help"));
                        if (CVarGetInteger(CVAR_SETTING("LEDColorSource"), LED_SOURCE_TUNIC_ORIGINAL) == LED_SOURCE_HEALTH) {
                            critical.enabled = false;
                            critical.disabledReason = Text("led_critical_redundant");
                        }
                        result.push_back(std::move(critical));
                    }
                    result.push_back(Action("remove", Text("remove"), [=] { led->ClearLEDMapping(id); GetModel().Back(); }));
                    return result;
                });
            }));
        rows.push_back(Action("add", Text("add_led"), [=] { capture([=] { return led->AddLEDMappingFromRawPress(); }); }));
        return rows;
    });
}

PagePtr ControllerStickResponsePage(std::shared_ptr<Ship::ControllerStick> stick) {
    return MakePage("controller/stick-response", Text("analog_options"), [=] {
        int8_t x = 0, y = 0;
        stick->Process(x, y);
        auto preview = Action("preview", Text("analog_preview"), {});
        preview.value = "X " + std::to_string(x) + ", Y " + std::to_string(y);
        auto angle = Integer("notch", Text("notch_angle"), stick->GetNotchSnapAngle(), 0, 45, 1,
            [=](int value) { stick->SetNotchSnapAngle(value); });
        angle.value += " " + Text("degrees");
        return std::vector<Row>{preview,
            Percent("sensitivity", Text("sensitivity"), stick->GetSensitivityPercentage(), 200, [=](int value) { stick->SetSensitivity(value); }),
            Action("reset_sensitivity", Text("reset_sensitivity"), [=] { stick->ResetSensitivityToDefault(); }),
            Percent("deadzone", Text("deadzone"), stick->GetDeadzonePercentage(), 100, [=](int value) { stick->SetDeadzone(value); }),
            Action("reset_deadzone", Text("reset_deadzone"), [=] { stick->ResetDeadzoneToDefault(); }), angle,
            Action("reset_notch", Text("reset_notch"), [=] { stick->ResetNotchSnapAngleToDefault(); })};
    });
}

PagePtr ControllerDevicesPage(uint8_t port) {
    return MakePage("controller/devices", Text("devices"), [=] {
        std::vector<Row> rows{
            Action("keyboard", Text("keyboard"), [] { ReadCurrentDescription(); }, Text("bind_keyboard_mouse_help")),
            Action("mouse", Text("mouse"), [] { ReadCurrentDescription(); }, Text("bind_keyboard_mouse_help"))};
        auto manager = Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager();
        for (const auto& [instance, name] : manager->GetConnectedSDLGamepadNames())
            rows.push_back(Toggle(std::to_string(instance), name, !manager->PortIsIgnoringInstanceId(port, instance), [=](bool enabled) {
                if (enabled) manager->UnignoreInstanceIdForPort(port, instance);
                else manager->IgnoreInstanceIdForPort(port, instance);
            }));
        return rows;
    });
}
}
