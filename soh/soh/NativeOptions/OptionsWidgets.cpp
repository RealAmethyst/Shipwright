#include "NativeOptions.h"
#include "OptionsFormatting.h"

#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/ShipInit.hpp"
#include "soh/ShipUtils.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace NativeOptions {

static std::map<std::string, PageFactory> pageFactories;
static std::map<std::string, std::string> pageTitles;

void RegisterPage(std::string name, PageFactory factory, std::string title) {
    if (!title.empty())
        pageTitles[name] = std::move(title);
    if (!pageFactories.emplace(std::move(name), std::move(factory)).second)
        throw std::logic_error("Duplicate native Options page");
}

PagePtr RegisteredPage(const std::string& name) {
    const auto it = pageFactories.find(name);
    if (it == pageFactories.end()) {
        SPDLOG_ERROR("Native Options page is missing: {}", name);
        return nullptr;
    }
    return it->second();
}

void SaveSettings() {
    Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
}

static std::string Caption(std::string name) {
    name = name.substr(0, name.find("##"));
    return name;
}

static void Changed(WidgetInfo& widget) {
    if (widget.cVar && *widget.cVar) {
        SaveSettings();
        ShipInit::Init(widget.cVar);
    }
    if (widget.callback)
        widget.callback(widget);
}

static bool CanWrite(WidgetInfo& widget) {
    SohGui::GetSohMenu()->RefreshDisabledState();
    if (widget.preFunc) {
        widget.ResetDisables();
        widget.preFunc(widget);
    }
    std::string reason;
    if (widget.raceDisable && CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) {
        reason = Text("race_lockout");
    } else if (widget.isHidden || !widget.options) {
        reason = Text("unavailable");
    } else {
        if (widget.options->disabled)
            reason = widget.options->disabledTooltip && *widget.options->disabledTooltip
                ? widget.options->disabledTooltip : Text("unavailable");
        for (auto disable : widget.activeDisables)
            reason += std::string(reason.empty() ? "" : " ") + SohGui::GetSohMenu()->GetDisabledMap().at(disable).reason;
    }
    if (reason.empty()) return true;
    Message(Text("unavailable"), reason);
    return false;
}

static PagePtr ColorPage(std::string base, std::string id, std::string label,
                         std::shared_ptr<UIWidgets::ColorPickerOptions> options, std::function<void()> changed,
                         std::function<bool()> canWrite = [] { return true; }) {
    return MakePage(id, label, [=] {
        std::vector<Row> rows;
        auto color = CVarGetColor((base + ".Value").c_str(), options->defaultValue);
        const bool locked = CVarGetInteger((base + ".Locked").c_str(), 0);
        const char* keys[] = { "red", "green", "blue", "alpha" };
        const uint8_t components[] = { color.r, color.g, color.b, color.a };
        for (int i = 0; i < (options->useAlpha ? 4 : 3); ++i) {
            auto row = Integer(keys[i], Text(keys[i]), components[i], 0, 255, 1, [=](int value) {
                if (!canWrite() || CVarGetInteger((base + ".Locked").c_str(), 0)) return;
                auto next = CVarGetColor((base + ".Value").c_str(), options->defaultValue);
                switch (i) {
                    case 0: next.r = value; break;
                    case 1: next.g = value; break;
                    case 2: next.b = value; break;
                    case 3: next.a = value; break;
                }
                CVarSetColor((base + ".Value").c_str(), next);
                ShipInit::Init((base + ".Value").c_str());
                changed();
            });
            row.enabled = !locked;
            if (locked)
                row.disabledReason = Text("locked");
            rows.push_back(std::move(row));
        }
        if (options->showReset) {
            auto row = Action("reset", Text("reset"), [=] {
                if (!canWrite() || CVarGetInteger((base + ".Locked").c_str(), 0)) return;
                for (const auto* suffix : { ".R", ".G", ".B", ".A", ".Type" })
                    CVarClear((base + suffix).c_str());
                CVarClearBlock((base + ".Value").c_str());
                ShipInit::Init((base + ".Value").c_str());
                changed();
            });
            row.enabled = !locked;
            rows.push_back(std::move(row));
        }
        if (options->showRandom) {
            auto row = Action("random", Text("random"), [=] {
                if (!canWrite() || CVarGetInteger((base + ".Locked").c_str(), 0)) return;
                auto next = CVarGetColor((base + ".Value").c_str(), options->defaultValue);
                next.r = ShipUtils::next32() % 256;
                next.g = ShipUtils::next32() % 256;
                next.b = ShipUtils::next32() % 256;
                CVarSetColor((base + ".Value").c_str(), next);
                CVarSetInteger((base + ".Rainbow").c_str(), 0);
                ShipInit::Init((base + ".Rainbow").c_str());
                ShipInit::Init((base + ".Value").c_str());
                changed();
            });
            row.enabled = !locked;
            rows.push_back(std::move(row));
        }
        if (options->showRainbow) {
            auto row = Toggle("rainbow", Text("rainbow"), CVarGetInteger((base + ".Rainbow").c_str(), 0),
                              [=](bool value) {
                                  if (!canWrite() || CVarGetInteger((base + ".Locked").c_str(), 0)) return;
                                  CVarSetInteger((base + ".Rainbow").c_str(), value);
                                  ShipInit::Init((base + ".Rainbow").c_str());
                                  SaveSettings();
                              });
            row.enabled = !locked;
            rows.push_back(std::move(row));
        }
        if (options->showLock)
            rows.push_back(Toggle("lock", Text("lock"), locked, [=](bool value) {
                if (!canWrite()) return;
                CVarSetInteger((base + ".Locked").c_str(), value);
                SaveSettings();
            }));
        return rows;
    });
}

Row CVarColor(std::string label, std::string cvar, std::array<uint8_t, 4> defaultValue, bool useAlpha,
              bool rainbow, bool lock) {
    auto options = std::make_shared<UIWidgets::ColorPickerOptions>();
    options->DefaultValue({defaultValue[0], defaultValue[1], defaultValue[2], defaultValue[3]})
        .UseAlpha(useAlpha).ShowReset().ShowRandom().ShowRainbow(rainbow).ShowLock(lock);
    return Link(cvar, label, [=] {
        return ColorPage(cvar, cvar + "/color", label, options, [=] { ChangedCVar(cvar); });
    });
}

void AppendWidget(std::vector<Row>& rows, WidgetInfo& widget, const std::string& id) {
    if (!widget.options)
        return;
    if (widget.preFunc) {
        widget.ResetDisables();
        widget.preFunc(widget);
    }
    if (widget.isHidden)
        return;
    Row row;
    const bool overlayScale = widget.cVar && std::string(widget.cVar) == CVAR_SETTING("ImGuiScale");
    const auto label = overlayScale ? Text("overlay_scaling") : Caption(widget.name);
    const std::string description = overlayScale ? Text("overlay_scaling_help") :
                                    widget.options->tooltip ? widget.options->tooltip : "";
    auto setInt = [&widget](int value) {
        if (!CanWrite(widget)) return;
        CVarSetInteger(widget.cVar, value);
        Changed(widget);
    };
    switch (widget.type) {
        case WIDGET_CVAR_CHECKBOX:
        case WIDGET_CHECKBOX: {
            auto options = std::static_pointer_cast<UIWidgets::CheckboxOptions>(widget.options);
            auto pointer = std::get_if<bool*>(&widget.valuePointer);
            if (widget.type == WIDGET_CHECKBOX && (!pointer || !*pointer))
                return;
            const bool value = widget.type == WIDGET_CVAR_CHECKBOX ? CVarGetInteger(widget.cVar, options->defaultValue) : **pointer;
            row = Toggle(id, label, value, [=, &widget](bool next) {
                if (!CanWrite(widget)) return;
                if (widget.type == WIDGET_CVAR_CHECKBOX)
                    CVarSetInteger(widget.cVar, next);
                else
                    **pointer = next;
                Changed(widget);
            }, description);
            break;
        }
        case WIDGET_CVAR_COMBOBOX:
        case WIDGET_COMBOBOX: {
            auto options = std::static_pointer_cast<UIWidgets::ComboboxOptions>(widget.options);
            auto pointer = std::get_if<int32_t*>(&widget.valuePointer);
            if (widget.type == WIDGET_COMBOBOX && (!pointer || !*pointer))
                return;
            std::map<int, std::string> choices;
            for (const auto& [key, name] : options->comboMap)
                if (name)
                    choices.emplace(key, Caption(name));
            const int value = widget.type == WIDGET_CVAR_COMBOBOX ? CVarGetInteger(widget.cVar, options->defaultIndex) : **pointer;
            row = Choice(id, label, value, choices, [=, &widget](int next) {
                if (!CanWrite(widget) || !options->comboMap.contains(next)) return;
                if (widget.type == WIDGET_CVAR_COMBOBOX)
                    CVarSetInteger(widget.cVar, next);
                else
                    **pointer = next;
                Changed(widget);
            }, description);
            break;
        }
        case WIDGET_CVAR_SLIDER_INT:
        case WIDGET_SLIDER_INT: {
            auto options = std::static_pointer_cast<UIWidgets::IntSliderOptions>(widget.options);
            auto pointer = std::get_if<int32_t*>(&widget.valuePointer);
            if (widget.type == WIDGET_SLIDER_INT && (!pointer || !*pointer))
                return;
            const int value = widget.type == WIDGET_CVAR_SLIDER_INT ? CVarGetInteger(widget.cVar, options->defaultValue) : **pointer;
            if (!widget.nativeChoices.empty()) {
                row = Choice(id, label, value, widget.nativeChoices, [=, &widget](int next) {
                    if (!CanWrite(widget) || !widget.nativeChoices.contains(next)) return;
                    if (widget.type == WIDGET_CVAR_SLIDER_INT)
                        CVarSetInteger(widget.cVar, next);
                    else
                        **pointer = next;
                    Changed(widget);
                }, description);
                break;
            }
            row = Integer(id, NumberLabel(label), value, options->min, options->max, options->step, [=, &widget](int next) {
                if (!CanWrite(widget) || next < options->min || next > options->max) return;
                if (widget.type == WIDGET_CVAR_SLIDER_INT)
                    CVarSetInteger(widget.cVar, next);
                else
                    **pointer = next;
                Changed(widget);
            }, description);
            const auto formatted = NumberValue(label, options->format ? options->format : "", value, true);
            if (!formatted.empty())
                row.value = formatted;
            break;
        }
        case WIDGET_CVAR_SLIDER_FLOAT:
        case WIDGET_SLIDER_FLOAT: {
            auto options = std::static_pointer_cast<UIWidgets::FloatSliderOptions>(widget.options);
            auto pointer = std::get_if<float*>(&widget.valuePointer);
            if (widget.type == WIDGET_SLIDER_FLOAT && (!pointer || !*pointer))
                return;
            const float value = widget.type == WIDGET_CVAR_SLIDER_FLOAT ? CVarGetFloat(widget.cVar, options->defaultValue) : **pointer;
            row = Decimal(id, NumberLabel(label), value, options->min, options->max, options->step, [=, &widget](float next) {
                if (!CanWrite(widget) || next < options->min || next > options->max) return;
                if (widget.type == WIDGET_CVAR_SLIDER_FLOAT)
                    CVarSetFloat(widget.cVar, next);
                else
                    **pointer = next;
                Changed(widget);
            }, description, options->isPercentage);
            const auto formatted = NumberValue(label, options->format ? options->format : "",
                                               options->isPercentage ? value * 100.0 : value, false);
            if (!formatted.empty())
                row.value = formatted;
            break;
        }
        case WIDGET_CVAR_BTN_SELECTOR: {
            auto options = std::static_pointer_cast<UIWidgets::BtnSelectorOptions>(widget.options);
            row = Link(id, label, [=, &widget] {
                return MakePage(id, label, [=, &widget] {
                    std::vector<Row> buttons;
                    for (const auto& [name, mask] : UIWidgets::buttonMap) {
                        if (!mask)
                            continue;
                        const int value = CVarGetInteger(widget.cVar, options->defaultValue);
                        buttons.push_back(Toggle(std::to_string(mask), name, (value & mask) != 0, [=](bool on) {
                            setInt(on ? value | mask : value & ~mask);
                        }));
                    }
                    buttons.push_back(Action("reset", Text("reset"), [=] { setInt(options->defaultValue); }));
                    return buttons;
                });
            }, description);
            for (const auto& [name, mask] : UIWidgets::buttonMap)
                if (mask && (CVarGetInteger(widget.cVar, options->defaultValue) & mask))
                    row.value += (row.value.empty() ? "" : " + ") + name;
            if (row.value.empty())
                row.value = Text("none");
            break;
        }
        case WIDGET_CVAR_COLOR_PICKER:
            row = Link(id, label, [=, &widget] {
                return ColorPage(widget.cVar, id, label,
                                 std::static_pointer_cast<UIWidgets::ColorPickerOptions>(widget.options),
                                 [&widget] { Changed(widget); }, [&widget] { return CanWrite(widget); });
            }, description);
            break;
        case WIDGET_BUTTON:
            row = Action(id, label, [&widget] { if (CanWrite(widget) && widget.callback) widget.callback(widget); }, description);
            break;
        case WIDGET_AUDIO_BACKEND: {
            auto audio = Ship::Context::GetInstance()->GetAudio();
            std::map<int, std::string> choices;
            for (auto backend : *audio->GetAvailableAudioBackends())
                if (audioBackendsMap.contains(backend))
                    choices.emplace(static_cast<int>(backend), audioBackendsMap.at(backend));
            row = Choice(id, label, static_cast<int>(audio->GetCurrentAudioBackend()), choices, [=, &widget](int value) {
                if (!CanWrite(widget)) return;
                const auto available = audio->GetAvailableAudioBackends();
                if (std::find(available->begin(), available->end(), static_cast<Ship::AudioBackend>(value)) == available->end()) return;
                audio->SetCurrentAudioBackend(static_cast<Ship::AudioBackend>(value));
            }, description);
            row.enabled = choices.size() > 1;
            if (!row.enabled) row.disabledReason = Text("audio_backend_only");
            break;
        }
        case WIDGET_VIDEO_BACKEND: {
            auto ctx = Ship::Context::GetInstance();
            auto window = ctx->GetWindow();
            std::map<int, std::string> choices;
            for (auto backend : *window->GetAvailableWindowBackends())
                choices.emplace(static_cast<int>(backend), windowBackendsMap.at(backend));
            const int current = ctx->GetConfig()->GetInt("Window.Backend.Id", static_cast<int>(window->GetWindowBackend()));
            row = Choice(id, label, current, choices, [=, &widget](int value) {
                if (!CanWrite(widget) || !choices.contains(value) || !window->IsAvailableWindowBackend(value)) return;
                ctx->GetConfig()->SetInt("Window.Backend.Id", value);
                ctx->GetConfig()->SetString("Window.Backend.Name", choices.at(value));
                ctx->GetConfig()->Save();
            }, description);
            row.enabled = choices.size() > 1;
            if (!row.enabled) row.disabledReason = Text("video_backend_only");
            break;
        }
        case WIDGET_WINDOW_BUTTON:
            if (!std::static_pointer_cast<UIWidgets::WindowButtonOptions>(widget.options)->embedWindow) {
                auto window = Ship::Context::GetInstance()->GetWindow()->GetGui()->GetGuiWindow(widget.windowName);
                if (!window)
                    throw std::logic_error("Missing Options overlay: " + std::string(widget.windowName));
                row = Toggle(id, Caption(widget.windowName), window->IsVisible(), [=, &widget](bool visible) {
                    if (!CanWrite(widget)) return;
                    if (visible) window->Show(); else window->Hide();
                    SaveSettings();
                }, description);
            } else {
                row = Link(id, pageTitles.contains(widget.windowName) ? pageTitles.at(widget.windowName) : Caption(widget.windowName),
                           [=] { return RegisteredPage(widget.windowName); });
            }
            break;
        case WIDGET_CUSTOM:
            row = widget.nativePage ? Link(id, widget.nativeTitle, widget.nativePage, description)
                                    : Link(id, pageTitles.contains(id) ? pageTitles.at(id) : label,
                                           [=] { return RegisteredPage(id); }, description);
            break;
        case WIDGET_TEXT:
            row = Action(id, label, [=] { Message(label, description); }, description);
            break;
        case WIDGET_SEPARATOR:
        case WIDGET_SEPARATOR_TEXT:
        case WIDGET_SEARCH:
            return;
        default:
            throw std::logic_error("Native Options has an unsupported widget: " + widget.name);
    }
    if (widget.options->disabled) {
        row.enabled = false;
        row.disabledReason = widget.options->disabledTooltip ? widget.options->disabledTooltip : "";
    }
    for (auto reason : widget.activeDisables) {
        row.enabled = false;
        if (!row.disabledReason.empty())
            row.disabledReason += " ";
        row.disabledReason += SohGui::GetSohMenu()->GetDisabledMap().at(reason).reason;
    }
    if (widget.raceDisable && CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) {
        row.enabled = false;
        row.disabledReason = Text("race_lockout");
    }
    rows.push_back(std::move(row));
}

} // namespace NativeOptions
