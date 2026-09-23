#include "OptionsControllers.h"
#include "soh/OTRGlobals.h"
#include "soh/SohGui/MenuTypes.h"
#include "soh/cvar_prefixes.h"
#include "z64.h"
#ifndef __WIIU__
#include <ship/controller/controldevice/controller/mapping/sdl/SDLAxisDirectionToButtonMapping.h>
#endif

namespace NativeOptions {
namespace {
using Mappings = std::map<std::string, std::shared_ptr<Ship::ControllerInputMapping>>;
using MappingReader = std::function<Mappings()>;

void AppendThresholdRows(std::vector<Row>& rows, const std::shared_ptr<Ship::ControllerInputMapping>& mapping) {
#ifndef __WIIU__
    const auto axis = std::dynamic_pointer_cast<Ship::SDLAxisDirectionToButtonMapping>(mapping);
    if (!axis) return;
    const auto settings = Ship::Context::GetInstance()->GetControlDeck()->GetGlobalSDLDeviceSettings();
    const auto add = [&](const char* key, int value, std::function<void(int)> set) {
        auto row = Integer(key, Text(key), value, 0, 100, 1, [settings, set](int value) {
            set(value);
            settings->SaveToConfig();
        }, Text("bind_threshold_help"));
        row.value += "%";
        rows.push_back(std::move(row));
    };
    if (axis->AxisIsStick())
        add("bind_stick_threshold", settings->GetStickAxisThresholdPercentage(),
            [settings](int value) { settings->SetStickAxisThresholdPercentage(value); });
    if (axis->AxisIsTrigger())
        add("bind_trigger_threshold", settings->GetTriggerAxisThresholdPercentage(),
            [settings](int value) { settings->SetTriggerAxisThresholdPercentage(value); });
#endif
}

PagePtr MappingPage(std::string title, MappingReader get, std::function<bool(std::string)> bind,
                    std::function<void(std::string)> remove, CaptureRequest capture) {
    return MakePage("controller/mappings/" + title, title, [=] {
        std::vector<Row> rows;
        for (const auto& entry : get()) {
            const auto id = entry.first;
            const auto mapping = entry.second;
            if (!mapping) continue;
            auto row = Link(id, mapping->GetPhysicalInputName(), [=] {
                return MakePage("controller/mapping/" + id, mapping->GetPhysicalInputName(), [=] {
                    const auto mappings = get();
                    const auto current = mappings.find(id);
                    if (current == mappings.end() || !current->second) return std::vector<Row>{};
                    std::vector<Row> result{
                        Action("edit", Text("bind_edit"), [=] {
                            GetModel().Back();
                            capture([=] { return bind(id); });
                        }),
                        Action("remove", Text("remove"), [=] { remove(id); GetModel().Back(); })};
                    AppendThresholdRows(result, current->second);
                    return result;
                }, mapping->GetPhysicalDeviceName());
            }, mapping->GetPhysicalDeviceName());
            rows.push_back(std::move(row));
        }
        rows.push_back(Action("add", Text("bind_add"), [=] { capture([=] { return bind(""); }); }));
        return rows;
    });
}

Row ButtonBinding(uint8_t port, CONTROLLERBUTTONS_T mask, std::string caption, CaptureRequest capture) {
    auto button = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->GetButton(mask);
    auto row = Link("button/" + std::to_string(mask), caption, [=] {
        return MappingPage(caption, [button] {
            Mappings mappings;
            for (const auto& [id, mapping] : button->GetAllButtonMappings()) mappings[id] = mapping;
            return mappings;
        }, [button, mask](std::string id) { return button->AddOrEditButtonMappingFromRawPress(mask, id, true); },
           [button](std::string id) { button->ClearButtonMapping(id); }, capture);
    });
    const auto mappings = button->GetAllButtonMappings();
    const std::map<std::string, std::shared_ptr<Ship::ControllerButtonMapping>> ordered(mappings.begin(), mappings.end());
    for (const auto& [id, mapping] : ordered)
        row.value += (row.value.empty() ? "" : ", ") + mapping->GetPhysicalInputName();
    if (row.value.empty()) row.value = Text("none");
    return row;
}

PagePtr ButtonsPage(uint8_t port, int group, CaptureRequest capture) {
    const char* groupKey = group == 1 ? "bind_dpad" : group == 2 ? "bind_modifiers" : "buttons";
    return MakePage("controller/buttons/" + std::to_string(port) + "/" + std::to_string(group), Text(groupKey), [=] {
        std::vector<std::pair<CONTROLLERBUTTONS_T, const char*>> buttons;
        if (group == 1)
            buttons = {{BTN_DUP, "bind_d_up"}, {BTN_DDOWN, "bind_d_down"}, {BTN_DLEFT, "bind_d_left"}, {BTN_DRIGHT, "bind_d_right"}};
        else if (group == 2)
            buttons = {{BTN_CUSTOM_MODIFIER1, "bind_m1"}, {BTN_CUSTOM_MODIFIER2, "bind_m2"}};
        else {
            buttons = {{BTN_A, "bind_a"}, {BTN_B, "bind_b"}};
            if (port != 1 || CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0)) {
                buttons.push_back({BTN_START, "bind_start"});
                buttons.push_back({BTN_L, "bind_l"});
                buttons.push_back({BTN_R, "bind_r"});
            }
            buttons.insert(buttons.end(), {{BTN_Z, "bind_z"}, {BTN_CUP, "bind_c_up"}, {BTN_CDOWN, "bind_c_down"},
                                          {BTN_CLEFT, "bind_c_left"}, {BTN_CRIGHT, "bind_c_right"}});
            if (port == 0) buttons.push_back({BTN_AIM_CYCLE, "bind_aim_cycle"});
        }
        std::vector<Row> rows;
        for (const auto& [mask, key] : buttons) rows.push_back(ButtonBinding(port, mask, Text(key), capture));
        return rows;
    });
}

PagePtr StickPage(uint8_t port, bool right, CaptureRequest capture) {
    auto controller = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port);
    auto stick = right ? controller->GetRightStick() : controller->GetLeftStick();
    return MakePage("controller/stick/" + std::to_string(port) + (right ? "/right" : "/left"),
                    Text(right ? "right_stick" : "analog_stick"), [=] {
        std::vector<Row> rows;
        for (const auto& entry : {std::pair{Ship::UP, "glyph_up"}, {Ship::DOWN, "glyph_down"},
                                 {Ship::LEFT, "glyph_left"}, {Ship::RIGHT, "glyph_right"}}) {
            const auto direction = entry.first;
            const auto caption = Text(entry.second);
            auto row = Link(std::to_string(direction), caption, [=] {
                return MappingPage(caption, [=] {
                    Mappings mappings;
                    for (const auto& [id, mapping] : stick->GetAllAxisDirectionMappingByDirection(direction))
                        mappings[id] = mapping;
                    return mappings;
                }, [=](std::string id) { return stick->AddOrEditAxisDirectionMappingFromRawPress(direction, id, true); },
                   [=](std::string id) { stick->ClearAxisDirectionMapping(direction, id); }, capture);
            });
            const auto mappings = stick->GetAllAxisDirectionMappingByDirection(direction);
            const std::map<std::string, std::shared_ptr<Ship::ControllerAxisDirectionMapping>> ordered(mappings.begin(), mappings.end());
            for (const auto& [id, mapping] : ordered)
                row.value += (row.value.empty() ? "" : ", ") + mapping->GetPhysicalInputName();
            if (row.value.empty()) row.value = Text("none");
            rows.push_back(std::move(row));
        }
        rows.push_back(Link("response", Text("analog_options"), [=] { return ControllerStickResponsePage(stick); }));
        return rows;
    });
}

PagePtr OcarinaPage(CaptureRequest capture, ControllerWidgets widgets) {
    return MakePage("controller/ocarina", Text("bind_ocarina"), [=] {
        std::vector<Row> rows;
        AppendWidget(rows, *widgets.dpadOcarina, "dpad_ocarina");
        AppendWidget(rows, *widgets.rightStickOcarina, "stick_ocarina");
        rows.push_back(CVarToggle(Text("bind_custom_ocarina"), CVAR_SETTING("CustomOcarina.Enabled")));
        for (const auto& [mask, key] : std::vector<std::pair<CONTROLLERBUTTONS_T, const char*>>{
                {BTN_CUSTOM_OCARINA_NOTE_D4, "bind_note_d4"}, {BTN_CUSTOM_OCARINA_NOTE_F4, "bind_note_f4"},
                {BTN_CUSTOM_OCARINA_NOTE_A4, "bind_note_a4"}, {BTN_CUSTOM_OCARINA_NOTE_B4, "bind_note_b4"},
                {BTN_CUSTOM_OCARINA_NOTE_D5, "bind_note_d5"}, {BTN_CUSTOM_OCARINA_DISABLE_SONGS, "bind_disable_songs"},
                {BTN_CUSTOM_OCARINA_PITCH_UP, "bind_pitch_up"}, {BTN_CUSTOM_OCARINA_PITCH_DOWN, "bind_pitch_down"}}) {
            auto row = ButtonBinding(0, mask, Text(key), capture);
            row.enabled = CVarGetInteger(CVAR_SETTING("CustomOcarina.Enabled"), 0);
            if (!row.enabled) row.disabledReason = Text("unavailable");
            rows.push_back(std::move(row));
        }
        return rows;
    });
}

PagePtr DefaultsPage(uint8_t port) {
    return MakePage("controller/defaults", Text("bind_defaults"), [port] {
        std::vector<Row> rows;
        for (const auto& [type, key] : {std::pair{Ship::PhysicalDeviceType::Keyboard, "keyboard"},
                                       {Ship::PhysicalDeviceType::SDLGamepad, "bind_gamepad"}}) {
            const auto device = type;
            const auto caption = Text(key);
            rows.push_back(Action(std::to_string(static_cast<int>(type)), caption, [port, device, caption] {
                auto description = Text("bind_defaults_help");
                description.replace(description.find("$0"), 2, caption);
                description.replace(description.find("$1"), 2, std::to_string(port + 1));
                Confirm(Text("bind_defaults"), description, Text("bind_defaults"), [port, device] {
                    auto controller = Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port);
                    controller->ClearAllMappingsForDeviceType(device);
                    controller->AddDefaultMappings(device);
                });
            }));
        }
        return rows;
    });
}

PagePtr PortPage(uint8_t port, CaptureRequest capture, ControllerWidgets widgets) {
    const auto title = port == 0 ? Text("bind_link") : port == 1 ? Text("bind_ivan") : Text("port") + " " + std::to_string(port + 1);
    return MakePage("controller/port/" + std::to_string(port), title, [=] {
        std::vector<Row> rows{
            Action("clear", Text("bind_clear"), [port] {
                auto description = Text("bind_clear_help");
                description.replace(description.find("$0"), 2, std::to_string(port + 1));
                Confirm(Text("bind_clear"), description, Text("bind_clear"), [port] {
                    Ship::Context::GetInstance()->GetControlDeck()->GetControllerByPort(port)->ClearAllMappings();
                });
            }),
            Link("defaults", Text("bind_defaults"), [=] { return DefaultsPage(port); }),
            Link("devices", Text("devices"), [=] { return ControllerDevicesPage(port); }),
            Link("buttons", Text("buttons"), [=] { return ButtonsPage(port, 0, capture); }),
            Link("dpad", Text("bind_dpad"), [=] { return ButtonsPage(port, 1, capture); }),
            Link("stick", Text("analog_stick"), [=] { return StickPage(port, false, capture); })};
        if (port == 0) {
            rows.push_back(Link("right_stick", Text("right_stick"), [=] { return StickPage(port, true, capture); }));
            rows.push_back(Link("rumble", Text("rumble"), [=] { return ControllerRumblePage(port, capture); }));
            rows.push_back(Link("gyro", Text("gyro"), [=] { return ControllerGyroPage(port, capture); }));
            rows.push_back(Link("led", Text("leds"), [=] { return ControllerLEDPage(port, capture); }));
            rows.push_back(Link("modifiers", Text("bind_modifiers"), [=] { return ButtonsPage(port, 2, capture); }));
            rows.push_back(Link("ocarina", Text("bind_ocarina"), [=] { return OcarinaPage(capture, widgets); }));
            rows.push_back(Link("camera", Text("camera_controls"), [=] {
                return ControllerCameraPage(*widgets.mouse, *widgets.autoCapture, *widgets.freeLook);
            }));
            rows.push_back(Link("dpad_options", Text("dpad_options"), [=] {
                return ControllerDpadPage(*widgets.dpadPause, *widgets.dpadText);
            }));
        }
        return rows;
    });
}
}

PagePtr ControllerBindingsPage(CaptureRequest capture, ControllerWidgets widgets) {
    return MakePage("controller", Text("bind_title"), [=] {
        std::vector<Row> rows;
        for (uint8_t port = 0; port < (CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0) ? 4 : 2); ++port) {
            const auto title = port == 0 ? Text("bind_link") : port == 1 ? Text("bind_ivan") : Text("port") + " " + std::to_string(port + 1);
            rows.push_back(Link(std::to_string(port), title, [=] { return PortPage(port, capture, widgets); }));
        }
        return rows;
    });
}
}
