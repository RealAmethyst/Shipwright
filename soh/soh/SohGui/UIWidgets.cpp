#include "UIWidgets.hpp"
#include "soh/OTRGlobals.h"

namespace UIWidgets {

// Automatically adds newlines to break up text longer than a specified number of characters
// Manually included newlines will still be respected and reset the line length
// If line is midword when it hits the limit, text should break at the last encountered space
std::string WrappedText(const char* text, unsigned int charactersPerLine) {
    std::string newText(text);
    const size_t tipLength = newText.length();
    int lastSpace = -1;
    int currentLineLength = 0;
    for (unsigned int currentCharacter = 0; currentCharacter < tipLength; currentCharacter++) {
        if (newText[currentCharacter] == '\n') {
            currentLineLength = 0;
            lastSpace = -1;
            continue;
        } else if (newText[currentCharacter] == ' ') {
            lastSpace = currentCharacter;
        }

        if ((currentLineLength >= charactersPerLine) && (lastSpace >= 0)) {
            newText[lastSpace] = '\n';
            currentLineLength = currentCharacter - lastSpace - 1;
            lastSpace = -1;
        }
        currentLineLength++;
    }

    return newText;
}

std::string WrappedText(const std::string& text, unsigned int charactersPerLine) {
    return WrappedText(text.c_str(), charactersPerLine);
}

void PaddedSeparator(bool padTop, bool padBottom, float extraVerticalTopPadding, float extraVerticalBottomPadding) {
    if (padTop) {
        Spacer(extraVerticalTopPadding);
    }
    ImGui::Separator();
    if (padBottom) {
        Spacer(extraVerticalBottomPadding);
    }
}

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", WrappedText(text).c_str());
    }
}

void Spacer(float height) {
    ImGui::Dummy(ImVec2(0.0f, height));
}

std::map<std::string, int32_t> buttonMap = {
    { "A", BTN_A },
    { "B", BTN_B },
    { "Z", BTN_Z },
    { "START", BTN_START },
    { "D-Up", BTN_DUP },
    { "D-Down", BTN_DDOWN },
    { "D-Left", BTN_DLEFT },
    { "D-Right", BTN_DRIGHT },
    { "L", BTN_L },
    { "R", BTN_R },
    { "C-Up", BTN_CUP },
    { "C-Down", BTN_CDOWN },
    { "C-Left", BTN_CLEFT },
    { "C-Right", BTN_CRIGHT },
    { "Modifier 1", BTN_CUSTOM_MODIFIER1 },
    { "Modifier 2", BTN_CUSTOM_MODIFIER2 },
};

} // namespace UIWidgets

ImVec4 GetRandomValue(uint64_t* state) {
    ImVec4 NewColor;
    NewColor.x = (float)ShipUtils::RandomDouble(state);
    NewColor.y = (float)ShipUtils::RandomDouble(state);
    NewColor.z = (float)ShipUtils::RandomDouble(state);
    return NewColor;
}

ImVec4 VecFromRGBA8(Color_RGBA8 color) {
    ImVec4 vec = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
    return vec;
}
