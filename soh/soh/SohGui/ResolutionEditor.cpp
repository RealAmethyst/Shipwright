#include "ResolutionEditor.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/SohGui/SohMenu.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/OTRGlobals.h"
#include <fast/Fast3dWindow.h>
#include <fast/interpreter.h>
#include <algorithm>

namespace SohGui {
namespace {
using namespace NativeOptions;
constexpr float aspectX[] = { 0, 16, 4, 16, 5, 16, 21 };
constexpr float aspectY[] = { 0, 9, 3, 9, 3, 10, 9 };
constexpr int pixelCounts[] = { 480, 240, 480, 720, 960, 1200, 1440, 1080, 2160 };
bool showHorizontalResolution = false;
std::weak_ptr<Fast::Interpreter> interpreter;

float Aspect(bool x) {
    const int preset = std::clamp(CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", 3), 0, 6);
    return x ? CVarGetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", aspectX[preset])
             : CVarGetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", aspectY[preset]);
}

int Vertical() {
    const int preset = std::clamp(CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.PixelCount", 0), 0, 8);
    return std::clamp(CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalPixelCount", pixelCounts[preset]), 240, 4320);
}

void SetAspect(float x, float y) {
    CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", x);
    CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", y);
    CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", 1);
    SaveSettings();
}

PagePtr IntegerScaling() {
    return MakePage("resolution/scaling", NativeOptions::Text("integer_scaling"), [] {
        const bool advanced = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0);
        const bool fixed = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0);
        const bool perfect = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0);
        const bool automatic = CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.FitAutomatically", 1);
        int bounds = 1;
        if (auto renderer = interpreter.lock()) {
            const auto viewport = renderer->mGameWindowViewport;
            const auto dimensions = renderer->mCurDimensions;
            if (dimensions.width && dimensions.height)
                bounds = std::min(viewport.width / dimensions.width, viewport.height / dimensions.height);
        }
        const int maximum = std::max(6, bounds + CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.ExceedBoundsBy", 0));
        std::vector<Row> rows = {
            CVarInteger(NativeOptions::Text("integer_scale_factor"), CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.Factor", 1, 1, maximum,
                        1, NativeOptions::Text("integer_scale_description")),
            CVarToggle(NativeOptions::Text("automatic_scale"), CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.FitAutomatically", true,
                       NativeOptions::Text("automatic_scale_description")),
            CVarToggle(NativeOptions::Text("never_exceed_bounds"), CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.NeverExceedBounds", true,
                       NativeOptions::Text("never_exceed_description"), [] {
                           CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.ExceedBoundsBy", 0);
                           SaveSettings();
                       }),
            CVarToggle(NativeOptions::Text("exceed_by_one"), CVAR_PREFIX_ADVANCED_RESOLUTION ".IntegerScale.ExceedBoundsBy", false,
                       NativeOptions::Text("scroll_warning")),
        };
        if (!advanced || !fixed || !perfect) {
            Disable(rows, NativeOptions::Text("requires_pixel_perfect"));
        } else if (automatic) {
            rows[0].enabled = rows[2].enabled = false;
            rows[0].disabledReason = rows[2].disabledReason = NativeOptions::Text("automatic_scale");
            rows[0].value = std::to_string(std::max(1, bounds));
        }
        auto mode = CVarToggle(NativeOptions::Text("pixel_perfect"), CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", false,
                              NativeOptions::Text("pixel_perfect_description"));
        if (!advanced || !fixed) {
            mode.enabled = false;
            mode.disabledReason = NativeOptions::Text("requires_fixed_resolution");
        }
        rows.insert(rows.begin(), std::move(mode));
        return rows;
    });
}

PagePtr ResolutionPage() {
    return MakePage("resolution", NativeOptions::Text("advanced_graphics"), [] {
        std::vector<Row> rows;
        if (auto renderer = interpreter.lock()) {
            auto viewport = renderer->mGameWindowViewport;
            auto dimensions = renderer->mCurDimensions;
            auto output = Action("viewport", NativeOptions::Text("viewport_dimensions"), {});
            output.value = std::to_string(viewport.width) + " x " + std::to_string(viewport.height);
            rows.push_back(std::move(output));
            auto internal = Action("internal", NativeOptions::Text("internal_resolution"), {});
            internal.value = std::to_string(dimensions.width) + " x " + std::to_string(dimensions.height);
            rows.push_back(std::move(internal));
        }
        rows.push_back(CVarToggle(NativeOptions::Text("advanced_enabled"), CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled"));
        if (CVarGetInteger(CVAR_LOW_RES_MODE, 0)) {
            rows.push_back(Action("n64", NativeOptions::Text("disable_n64"), [] {
                CVarSetInteger(CVAR_LOW_RES_MODE, 0);
                ChangedCVar(CVAR_LOW_RES_MODE);
            }, NativeOptions::Text("n64_override")));
        } else if (IsDroppingFrames()) {
            rows.push_back(Action("fps", NativeOptions::Text("frame_drops"), {}));
        }
        std::vector<Row> settings;
        const int aspect = std::clamp(CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", 3), 0, 6);
        settings.push_back(Choice("aspect", NativeOptions::Text("aspect_ratio"), aspect,
            {{0, NativeOptions::Text("off")}, {1, NativeOptions::Text("custom")}, {2, NativeOptions::Text("aspect_original")}, {3, NativeOptions::Text("aspect_wide")},
             {4, NativeOptions::Text("aspect_3ds")}, {5, "16:10 (8:5)"}, {6, NativeOptions::Text("aspect_ultrawide")}}, [](int value) {
                if (value != 1) {
                    CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioX", aspectX[value]);
                    CVarSetFloat(CVAR_PREFIX_ADVANCED_RESOLUTION ".AspectRatioY", aspectY[value]);
                }
                CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.AspectRatio", value);
                SaveSettings();
            }));
        if (aspect == 1 && !showHorizontalResolution) {
            settings.push_back(Decimal("x", "X", Aspect(true), .1f, 32, .001f,
                                        [](float value) { SetAspect(value, Aspect(false)); }));
            settings.push_back(Decimal("y", "Y", Aspect(false), .1f, 24, .001f,
                                        [](float value) { SetAspect(Aspect(true), value); }));
        }
        settings.push_back(CVarToggle(NativeOptions::Text("fixed_resolution"), CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle",
                                      false, NativeOptions::Text("fixed_resolution_description"), [] {
            if (!CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalResolutionToggle", 0)) {
                CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0);
                SaveSettings();
            }
        }));
        settings.push_back(Choice("pixels", NativeOptions::Text("pixel_presets"),
            std::clamp(CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.PixelCount", 0), 0, 8),
            {{0, NativeOptions::Text("custom")}, {1, NativeOptions::Text("pixels_native")}, {2, "2x (480p)"}, {3, "3x (720p)"}, {4, "4x (960p)"},
             {5, "5x (1200p)"}, {6, "6x (1440p)"}, {7, NativeOptions::Text("pixels_hd")}, {8, "4K (2160p)"}}, [](int value) {
                CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.PixelCount", value);
                if (value) CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalPixelCount", pixelCounts[value]);
                SaveSettings();
            }));
        if (showHorizontalResolution) {
            if (Aspect(true) > 0 && Aspect(false) > 0) {
                const int horizontal = static_cast<int>(Vertical() * Aspect(true) / Aspect(false));
                settings.push_back(Integer("horizontal", NativeOptions::Text("horizontal_pixels"), horizontal, 320, 32768, 8,
                                            [](int value) { SetAspect(static_cast<float>(value), static_cast<float>(Vertical())); }));
            } else {
                settings.push_back(Action("resolve", NativeOptions::Text("resolve_aspect"), [] { SetAspect(4, 3); }, NativeOptions::Text("requires_aspect")));
            }
        }
        settings.push_back(Integer("vertical", NativeOptions::Text("vertical_pixels"), Vertical(), 240, 4320, 8, [](int value) {
            if (showHorizontalResolution && Aspect(false) > 0)
                SetAspect(Vertical() * Aspect(true) / Aspect(false), static_cast<float>(value));
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".VerticalPixelCount", value);
            CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".UIComboItem.PixelCount", 0);
            SaveSettings();
        }));
        settings.push_back(Toggle("horizontal-field", NativeOptions::Text("horizontal_field"), showHorizontalResolution,
                                  [](bool value) { showHorizontalResolution = value; }));
        settings.push_back(Link("scaling", NativeOptions::Text("integer_scaling"), IntegerScaling));
#if defined(__SWITCH__) || defined(__WIIU__)
        auto correction = CVarToggle(NativeOptions::Text("ignore_aspect"), CVAR_PREFIX_ADVANCED_RESOLUTION ".IgnoreAspectCorrection");
        correction.enabled = !CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".PixelPerfectMode", 0);
        settings.push_back(std::move(correction));
#else
        if (CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IgnoreAspectCorrection", 0)) {
            settings.push_back(Action("correction", NativeOptions::Text("restore_aspect"), [] {
                CVarSetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".IgnoreAspectCorrection", 0);
                SaveSettings();
            }));
        }
#endif
        if (!CVarGetInteger(CVAR_PREFIX_ADVANCED_RESOLUTION ".Enabled", 0))
            Disable(settings, NativeOptions::Text("requires_advanced_resolution"));
        rows.insert(rows.end(), settings.begin(), settings.end());
        return rows;
    });
}
} // namespace

void RegisterResolutionWidgets() {
    auto window = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    interpreter = window->GetInterpreterWeak();
    WidgetPath path = { "Settings", "Graphics", SECTION_COLUMN_2 };
    SohGui::GetSohMenu()->AddWidget(path, "Advanced Graphics Options", WIDGET_CUSTOM)
        .NativePage(ResolutionPage, NativeOptions::Text("advanced_graphics"))
        .RaceDisable(false);
}

bool IsDroppingFrames() {
    const short target = OTRGlobals::Instance->GetInterpolationFPS();
    return ImGui::GetIO().Framerate < target - (target / 20.0f + 4.1f);
}

static RegisterMenuInitFunc menuInitFunc(RegisterResolutionWidgets);
} // namespace SohGui
