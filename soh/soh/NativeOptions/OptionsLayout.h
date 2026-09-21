#pragma once

#include "OptionsModel.h"
#include <cstdint>

namespace NativeOptions {

struct Color {
    uint8_t r, g, b, a;
};

enum class Primitive { Rectangle, Glyph, Panel, Image };

struct DrawCommand {
    Primitive primitive;
    float x, y, width, height;
    Color color;
    uint16_t texture = 0;
    RowImage image;
};

using GlyphWidth = std::function<float(uint8_t)>;
using EncodeText = std::function<std::string(const std::string&)>;
std::vector<std::string> WrapText(const std::string& source, float available, float scale,
                                 const GlyphWidth& width, const EncodeText& encode);

// Geometry is expressed in the game's 320-by-240 coordinate system.
std::vector<DrawCommand> TitleLayout(const std::string& start, const std::string& options, const std::string& hints,
                                     int selected, int alpha, const GlyphWidth& width, const EncodeText& encode);
std::vector<DrawCommand> Layout(const Model& model, const std::string& footer, const GlyphWidth& width,
                                const EncodeText& encode);

} // namespace NativeOptions
