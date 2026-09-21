#include "OptionsLayout.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace NativeOptions {
namespace {
constexpr Color white = { 238, 244, 255, 255 };
constexpr Color gold = { 255, 222, 119, 255 };
constexpr Color muted = { 169, 189, 217, 255 };
constexpr Color shadow = { 0, 4, 15, 220 };

class Canvas {
  public:
    std::vector<DrawCommand> commands;
    GlyphWidth width;
    EncodeText encode;

    float Measure(const std::string& text, float scale) const {
        float result = 0;
        for (uint8_t glyph : text)
            result += width(glyph) * scale;
        return result;
    }

    void Rect(float x, float y, float w, float h, Color color) {
        commands.push_back({ Primitive::Rectangle, x, y, w, h, color });
    }

    void Text(const std::string& source, float x, float y, float available, float scale, Color color,
              bool alignRight = false) {
        auto text = encode(source);
        std::replace(text.begin(), text.end(), '\n', ' ');
        if (Measure(text, scale) > available) {
            while (!text.empty() && Measure(text + "...", scale) > available)
                text.pop_back();
            text += "...";
        }
        if (alignRight)
            x += available - Measure(text, scale);
        for (uint8_t glyph : text) {
            if (glyph != ' ') {
                commands.push_back({ Primitive::Glyph, x + 0.7f, y + 0.7f, 16 * scale, 16 * scale, shadow, glyph });
                commands.push_back({ Primitive::Glyph, x, y, 16 * scale, 16 * scale, color, glyph });
            }
            x += width(glyph) * scale;
        }
    }

    void Paragraph(const std::string& source, float x, float y, float available, float scale, int lines, Color color) {
        std::istringstream words(source);
        std::string word, line;
        int count = 0;
        while (words >> word) {
            const auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && Measure(encode(candidate), scale) > available) {
                if (count == lines - 1) {
                    Text(line + "...", x, y, available, scale, color);
                    return;
                }
                Text(line, x, y, available, scale, color);
                ++count;
                y += 11;
                line = word;
            } else {
                line = candidate;
            }
        }
        if (!line.empty() && count < lines)
            Text(line, x, y, available, scale, color);
    }
};
} // namespace

std::vector<std::string> WrapText(const std::string& source, float available, float scale,
                                 const GlyphWidth& width, const EncodeText& encode) {
    const auto measure = [&](const std::string& text) {
        float total = 0;
        for (uint8_t glyph : encode(text)) total += width(glyph) * scale;
        return total;
    };
    std::vector<std::string> lines;
    std::istringstream paragraphs(source);
    std::string paragraph;
    while (std::getline(paragraphs, paragraph)) {
        std::istringstream words(paragraph);
        std::string word, line;
        while (words >> word) {
            if (!line.empty() && measure(line + " " + word) > available) {
                lines.push_back(line);
                line.clear();
            }
            while (measure(word) > available) {
                size_t end = 0;
                for (size_t next = 1; next <= word.size(); ++next) {
                    if (next < word.size() && (static_cast<unsigned char>(word[next]) & 0xc0) == 0x80)
                        continue;
                    if (end && measure(word.substr(0, next)) > available)
                        break;
                    end = next;
                }
                lines.push_back(word.substr(0, end));
                word.erase(0, end);
            }
            if (!word.empty()) line += (line.empty() ? "" : " ") + word;
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<DrawCommand> TitleLayout(const std::string& start, const std::string& options, const std::string& hints,
                                     int selected, int alpha, const GlyphWidth& width, const EncodeText& encode) {
    Canvas canvas{{}, width, encode};
    const std::string labels[] = {start, options, hints};
    const auto opacity = static_cast<uint8_t>(std::clamp(alpha, 0, 255));
    for (int row = 0; row < 3; ++row) {
        const float scale = row == 2 ? 0.5f : 0.65f;
        const float length = std::min(284.0f, canvas.Measure(encode(labels[row]), scale));
        const float left = 160 - length / 2;
        const float top = row == 2 ? 222 : 171 + row * 14;
        if (row == selected)
            canvas.Rect(left - 10, top - 1, length + 18, 12, {8, 22, 51, static_cast<uint8_t>(opacity * 3 / 4)});
        auto color = row == selected ? gold : white;
        color.a = opacity;
        const auto first = canvas.commands.size();
        canvas.Text(labels[row], left, top, length, scale, color);
        for (size_t i = first; i < canvas.commands.size(); ++i)
            canvas.commands[i].color.a = std::min(canvas.commands[i].color.a, opacity);
    }
    return canvas.commands;
}

std::vector<DrawCommand> Layout(const Model& model, const std::string& footer, const GlyphWidth& width,
                                const EncodeText& encode) {
    Canvas canvas{ {}, width, encode };
    const auto* page = model.CurrentPage();
    if (!page)
        return {};
    canvas.Rect(-160, 0, 640, 240, { 3, 8, 21, 255 });
    // The twenty tiles are the original file-selection window, four columns by five rows.
    for (int i = 0; i < 20; ++i) {
        const int column = i % 4;
        const int row = i / 4;
        const float left = std::round(8 + column * 81.06667f);
        const float right = column == 3 ? 312 : std::round(8 + (column + 1) * 81.06667f);
        const float top = std::round(8 + row * 44.8f);
        const float bottom = std::round(8 + (row + 1) * 44.8f);
        canvas.commands.push_back({ Primitive::Panel, left, top, right - left, bottom - top,
                                    { 70, 109, 194, 255 }, static_cast<uint16_t>(i) });
    }
    canvas.Rect(19, 17, 282, 206, { 4, 15, 42, 155 });
    canvas.Text(page->title, 26, 20, 226, 0.85f, gold);
    const auto& rows = model.Rows();
    const auto selection = model.Selection();
    if (!rows.empty() && (!page->popup || rows.size() > 1))
        canvas.Text(std::to_string(selection + 1) + "/" + std::to_string(rows.size()), 255, 24, 37, 0.55f, muted, true);
    canvas.Rect(26, 38, 268, 1, { 130, 162, 211, 150 });
    if (!page->documentLines.empty() || page->popup || page->documentImage.resource) {
        const auto lines = page->documentLines.empty() ? WrapText(page->description, 260, 0.60f, width, encode)
                                                       : page->documentLines;
        const size_t maxLines = page->documentImage.resource ? 2 : 13;
        for (size_t i = 0; i < std::min(maxLines, lines.size()); ++i)
            canvas.Text(lines[i] + (i == 12 && lines.size() > 13 ? "..." : ""), 28, 44 + i * 11, 260, 0.60f, white);
        if (page->documentImage.resource && page->documentImage.width > 0 && page->documentImage.height > 0) {
            const auto& image = page->documentImage;
            const float scale = std::min(128.0f / image.width, 110.0f / image.height);
            canvas.commands.push_back({Primitive::Image, 160 - image.width * scale / 2, 70,
                image.width * scale, image.height * scale, {255, 255, 255, 255}, 0, image});
        }
        for (size_t i = 0; i < std::min<size_t>(2, rows.size()); ++i) {
            const float y = 190 + i * 13;
            if (i == selection)
                canvas.Rect(24, y - 1, 272, 12, {57, 72, 92, 245});
            canvas.Text(rows[i].label, 30, y, 256, 0.60f, !rows[i].enabled ? muted : i == selection ? gold : white);
        }
        canvas.Text(page->footer.empty() ? footer : page->footer, 29, 217, 261, 0.54f, gold);
        return canvas.commands;
    }
    constexpr size_t visibleRows = 6;
    const size_t first = (selection / visibleRows) * visibleRows;
    for (size_t i = first; i < std::min(rows.size(), first + visibleRows); ++i) {
        const auto& row = rows[i];
        const float y = 45.0f + static_cast<float>(i - first) * 21.0f;
        const bool focused = i == selection;
        if (focused) {
            canvas.Rect(23, y - 2, 274, 20, { 57, 72, 92, 245 });
            canvas.Rect(23, y - 2, 2, 20, gold);
            canvas.Rect(25, y + 17, 272, 0.7f, { 227, 193, 104, 150 });
        }
        const auto color = !row.enabled ? muted : focused ? gold : white;
        const float inset = row.image.resource ? 23 : 0;
        if (row.image.resource)
            canvas.commands.push_back({ Primitive::Image, 29, y - 1, 19, 19, {255, 255, 255, 255}, 0, row.image });
        if (row.value.empty()) {
            canvas.Text(row.label, 30 + inset, y + 2, 252 - inset, 0.65f, color);
            if (row.activate)
                canvas.Text(">", 287, y + 2, 8, 0.65f, color);
        } else {
            canvas.Text(row.label, 30 + inset, y, 256 - inset, 0.60f, color);
            canvas.Text(row.value, 38 + inset, y + 10, 247 - inset, 0.55f, focused ? white : muted, true);
        }
    }
    canvas.Rect(26, 173, 268, 1, { 130, 162, 211, 150 });
    std::string description = page->description;
    if (!rows.empty()) {
        const auto& row = rows[selection];
        description = row.label;
        if (!row.description.empty())
            description += ": " + row.description;
        if (!row.disabledReason.empty())
            description += " " + row.disabledReason;
    }
    canvas.Paragraph(description, 28, 179, 260, 0.57f, 3, white);
    canvas.Text(page->footer.empty() ? footer : page->footer, 29, 217, 261, 0.54f, gold);
    return canvas.commands;
}

} // namespace NativeOptions
