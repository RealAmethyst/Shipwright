#ifndef UIWidgets2_hpp
#define UIWidgets2_hpp

#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <libultraship/libultraship.h>
#include "soh/ShipUtils.h"
#include "soh/ShipInit.hpp"

namespace UIWidgets {

std::string WrappedText(const char* text, unsigned int charactersPerLine = 80);
std::string WrappedText(const std::string& text, unsigned int charactersPerLine = 80);
void PaddedSeparator(bool padTop = true, bool padBottom = true, float extraVerticalTopPadding = 0.0f,
                     float extraVerticalBottomPadding = 0.0f);
void Tooltip(const char* text);

// mostly in order for colors usable by the menu without custom text color
enum Colors {
    Red,
    DarkRed,
    Orange,
    Green,
    DarkGreen,
    LightBlue,
    Blue,
    DarkBlue,
    Indigo,
    Violet,
    Purple,
    Brown,
    Gray,
    DarkGray,
    // not suitable for menu theme use
    Pink,
    Yellow,
    Cyan,
    Black,
    LightGray,
    White,
    NoColor
};

const std::unordered_map<Colors, ImVec4> ColorValues = {
    { Colors::Pink, ImVec4(0.87f, 0.3f, 0.87f, 1.0f) },     { Colors::Red, ImVec4(0.55f, 0.0f, 0.0f, 1.0f) },
    { Colors::DarkRed, ImVec4(0.3f, 0.0f, 0.0f, 1.0f) },    { Colors::Orange, ImVec4(0.85f, 0.55f, 0.0f, 1.0f) },
    { Colors::Yellow, ImVec4(0.95f, 0.95f, 0.0f, 1.0f) },   { Colors::Green, ImVec4(0.0f, 0.55f, 0.0f, 1.0f) },
    { Colors::DarkGreen, ImVec4(0.0f, 0.3f, 0.0f, 1.0f) },  { Colors::Cyan, ImVec4(0.0f, 0.9f, 0.9f, 1.0f) },
    { Colors::LightBlue, ImVec4(0.0f, 0.24f, 0.8f, 1.0f) }, { Colors::Blue, ImVec4(0.08f, 0.03f, 0.65f, 1.0f) },
    { Colors::DarkBlue, ImVec4(0.03f, 0.0f, 0.5f, 1.0f) },  { Colors::Indigo, ImVec4(0.35f, 0.0f, 0.87f, 1.0f) },
    { Colors::Violet, ImVec4(0.5f, 0.0f, 0.9f, 1.0f) },     { Colors::Purple, ImVec4(0.31f, 0.0f, 0.67f, 1.0f) },
    { Colors::Brown, ImVec4(0.37f, 0.18f, 0.0f, 1.0f) },    { Colors::LightGray, ImVec4(0.75f, 0.75f, 0.75f, 1.0f) },
    { Colors::Gray, ImVec4(0.45f, 0.45f, 0.45f, 1.0f) },    { Colors::DarkGray, ImVec4(0.15f, 0.15f, 0.15f, 1.0f) },
    { Colors::Black, ImVec4(0.0f, 0.0f, 0.0f, 1.0f) },      { Colors::White, ImVec4(1.0f, 1.0f, 1.0f, 1.0f) },
    { Colors::NoColor, ImVec4(0.0f, 0.0f, 0.0f, 0.0f) },
};

namespace Sizes {
const ImVec2 Inline = ImVec2(0.0f, 0.0f);
const ImVec2 Fill = ImVec2(-1.0f, 0.0f);
} // namespace Sizes

enum LabelPositions {
    Near,
    Far,
    Above,
    None,
    Within,
};

enum ComponentAlignments {
    Left,
    Right,
};

struct WidgetOptions {
    const char* tooltip = "";
    bool disabled = false;
    const char* disabledTooltip = "";

    WidgetOptions& Tooltip(const char* tooltip_) {
        tooltip = tooltip_;
        return *this;
    }

    WidgetOptions& Disabled(bool disabled_) {
        disabled = disabled_;
        return *this;
    }

    WidgetOptions& DisabledTooltip(const char* disabledTooltip_) {
        disabledTooltip = disabledTooltip_;
        return *this;
    }
};

struct TextOptions : WidgetOptions {
    Colors color = Colors::NoColor;

    TextOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }
};

struct ButtonOptions : WidgetOptions {
    ImVec2 size = Sizes::Fill;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::Gray;

    ButtonOptions& Size(ImVec2 size_) {
        size = size_;
        return *this;
    }

    ButtonOptions& Padding(ImVec2 padding_) {
        padding = padding_;
        return *this;
    }

    ButtonOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    ButtonOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }
    ButtonOptions& DisabledTooltip(const char* disabledTooltip_) {
        WidgetOptions::disabledTooltip = disabledTooltip_;
        return *this;
    }
};

struct ColorPickerOptions : WidgetOptions {
    ImVec2 size = Sizes::Fill;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::Gray;
    Color_RGBA8 defaultValue = { 255, 255, 255, 255 };
    bool useAlpha, showReset, showRandom, showRainbow, showLock;

    ColorPickerOptions& Size(ImVec2 size_) {
        size = size_;
        return *this;
    }

    ColorPickerOptions& Padding(ImVec2 padding_) {
        padding = padding_;
        return *this;
    }

    ColorPickerOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    ColorPickerOptions& ShowReset(bool showReset_ = true) {
        showReset = showReset_;
        return *this;
    }

    ColorPickerOptions& ShowRandom(bool showRandom_ = true) {
        showRandom = showRandom_;
        return *this;
    }

    ColorPickerOptions& ShowRainbow(bool showRainbow_ = true) {
        showRainbow = showRainbow_;
        return *this;
    }

    ColorPickerOptions& ShowLock(bool showLock_ = true) {
        showLock = showLock_;
        return *this;
    }

    ColorPickerOptions& UseAlpha(bool useAlpha_ = true) {
        useAlpha = useAlpha_;
        return *this;
    }

    ColorPickerOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }

    ColorPickerOptions& DefaultValue(Color_RGBA8 defaultValue_) {
        defaultValue = defaultValue_;
        return *this;
    }
};

struct WindowButtonOptions : WidgetOptions {
    ImVec2 size = Sizes::Inline;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::Gray;
    bool showButton = true;
    bool embedWindow = true;

    WindowButtonOptions& Size(ImVec2 size_) {
        size = size_;
        return *this;
    }

    WindowButtonOptions& Padding(ImVec2 padding_) {
        padding = padding_;
        return *this;
    }

    WindowButtonOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    WindowButtonOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }

    WindowButtonOptions& ShowButton(bool showButton_) {
        showButton = showButton_;
        return *this;
    }

    WindowButtonOptions& EmbedWindow(bool embedWindow_) {
        embedWindow = embedWindow_;
        return *this;
    }
};

struct CheckboxOptions : WidgetOptions {
    bool defaultValue = false; // Only applicable to CVarCheckbox
    ComponentAlignments alignment = ComponentAlignments::Left;
    LabelPositions labelPosition = LabelPositions::Near;
    ImVec2 padding = ImVec2(10.0f, 8.0f);
    Colors color = Colors::LightBlue;

    CheckboxOptions& DefaultValue(bool defaultValue_) {
        defaultValue = defaultValue_;
        return *this;
    }

    CheckboxOptions& ComponentAlignment(ComponentAlignments alignment_) {
        alignment = alignment_;
        return *this;
    }

    CheckboxOptions& LabelPosition(LabelPositions labelPosition_) {
        labelPosition = labelPosition_;
        return *this;
    }

    CheckboxOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    CheckboxOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }

    CheckboxOptions& DisabledTooltip(const char* disabledTooltip_) {
        WidgetOptions::disabledTooltip = disabledTooltip_;
        return *this;
    }

    CheckboxOptions& Padding(ImVec2 padding_) {
        padding = padding_;
        return *this;
    }
};

struct ComboboxOptions : WidgetOptions {
    std::map<int32_t, const char*> comboMap = {};
    uint32_t defaultIndex = 0; // Only applicable to CVarCombobox
    ComponentAlignments alignment = ComponentAlignments::Left;
    LabelPositions labelPosition = LabelPositions::Above;
    ImGuiComboFlags flags = 0;
    Colors color = Colors::LightBlue;

    ComboboxOptions& ComboMap(std::map<int32_t, const char*> comboMap_) {
        comboMap = comboMap_;
        return *this;
    }

    ComboboxOptions& DefaultIndex(uint32_t defaultIndex_) {
        defaultIndex = defaultIndex_;
        return *this;
    }

    ComboboxOptions& ComponentAlignment(ComponentAlignments alignment_) {
        alignment = alignment_;
        return *this;
    }

    ComboboxOptions& LabelPosition(LabelPositions labelPosition_) {
        labelPosition = labelPosition_;
        return *this;
    }

    ComboboxOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    ComboboxOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }
};

struct IntSliderOptions : WidgetOptions {
    bool showButtons = true;
    const char* format = "%d";
    int32_t step = 1;
    int32_t min = 1;
    int32_t max = 10;
    int32_t defaultValue = 1;
    bool clamp = true;
    ComponentAlignments alignment = ComponentAlignments::Left;
    LabelPositions labelPosition = LabelPositions::Above;
    Colors color = Colors::Gray;
    ImGuiSliderFlags flags = 0;
    ImVec2 size = { 0, 0 };

    IntSliderOptions& ShowButtons(bool showButtons_) {
        showButtons = showButtons_;
        return *this;
    }

    IntSliderOptions& Format(const char* format_) {
        format = format_;
        return *this;
    }

    IntSliderOptions& Step(int32_t step_) {
        step = step_;
        return *this;
    }

    IntSliderOptions& Min(int32_t min_) {
        min = min_;
        return *this;
    }

    IntSliderOptions& Max(int32_t max_) {
        max = max_;
        return *this;
    }

    IntSliderOptions& DefaultValue(int32_t defaultValue_) {
        defaultValue = defaultValue_;
        return *this;
    }

    IntSliderOptions& ComponentAlignment(ComponentAlignments alignment_) {
        alignment = alignment_;
        return *this;
    }

    IntSliderOptions& LabelPosition(LabelPositions labelPosition_) {
        labelPosition = labelPosition_;
        return *this;
    }

    IntSliderOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    IntSliderOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }

    IntSliderOptions& Size(ImVec2 size_) {
        size = size_;
        return *this;
    }

    IntSliderOptions& Clamp(bool clamp_) {
        clamp = clamp_;
        return *this;
    }
};

struct FloatSliderOptions : WidgetOptions {
    bool showButtons = true;
    const char* format = "%f";
    float step = 0.01f;
    float min = 0.01f;
    float max = 10.0f;
    float defaultValue = 1.0f;
    bool clamp = true;
    bool isPercentage = false; // Multiplies visual value by 100
    ComponentAlignments alignment = ComponentAlignments::Left;
    LabelPositions labelPosition = LabelPositions::Above;
    Colors color = Colors::Gray;
    ImGuiSliderFlags flags = 0;
    ImVec2 size = { 0, 0 };

    FloatSliderOptions& ShowButtons(bool showButtons_) {
        showButtons = showButtons_;
        return *this;
    }

    FloatSliderOptions& Format(const char* format_) {
        format = format_;
        return *this;
    }

    FloatSliderOptions& Step(float step_) {
        step = step_;
        return *this;
    }

    FloatSliderOptions& Min(float min_) {
        min = min_;
        return *this;
    }

    FloatSliderOptions& Max(float max_) {
        max = max_;
        return *this;
    }

    FloatSliderOptions& DefaultValue(float defaultValue_) {
        defaultValue = defaultValue_;
        return *this;
    }

    FloatSliderOptions& ComponentAlignment(ComponentAlignments alignment_) {
        alignment = alignment_;
        return *this;
    }

    FloatSliderOptions& LabelPosition(LabelPositions labelPosition_) {
        labelPosition = labelPosition_;
        return *this;
    }

    FloatSliderOptions& IsPercentage(bool isPercentage_ = true) {
        isPercentage = isPercentage_;
        format = "%.0f%%";
        min = 0.0f;
        max = 1.0f;
        return *this;
    }

    FloatSliderOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    FloatSliderOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }

    FloatSliderOptions& Size(ImVec2 size_) {
        size = size_;
        return *this;
    }

    FloatSliderOptions& Clamp(bool clamp_) {
        clamp = clamp_;
        return *this;
    }
};

struct BtnSelectorOptions : WidgetOptions {
    s32 defaultValue = 0;
    ComponentAlignments alignment = ComponentAlignments::Left;
    LabelPositions labelPosition = LabelPositions::Above;
    Colors color = Colors::Gray;

    BtnSelectorOptions& DefaultValue(int32_t defaultValue_) {
        defaultValue = defaultValue_;
        return *this;
    }

    BtnSelectorOptions& ComponentAlignment(ComponentAlignments alignment_) {
        alignment = alignment_;
        return *this;
    }

    BtnSelectorOptions& LabelPosition(LabelPositions labelPosition_) {
        labelPosition = labelPosition_;
        return *this;
    }

    BtnSelectorOptions& Tooltip(const char* tooltip_) {
        WidgetOptions::tooltip = tooltip_;
        return *this;
    }

    BtnSelectorOptions& Color(Colors color_) {
        color = color_;
        return *this;
    }
};

void Spacer(float height = 0.0f);
extern std::map<std::string, int32_t> buttonMap;

} // namespace UIWidgets

ImVec4 GetRandomValue(uint64_t* state = nullptr);

ImVec4 VecFromRGBA8(Color_RGBA8 color);

#endif /* UIWidgets_hpp */
