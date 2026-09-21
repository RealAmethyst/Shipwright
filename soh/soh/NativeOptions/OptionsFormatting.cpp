#include "OptionsFormatting.h"
#include "NativeOptions.h"
#include <ship/window/gui/IconsFontAwesome4.h>

#include <cstdio>
#include <optional>

namespace NativeOptions {
std::string PrepareOptionsText(const std::string& text) {
    const std::pair<const char*, const char*> glyphs[] = {
        {ICON_FA_ARROW_LEFT, "glyph_left"}, {ICON_FA_ARROW_RIGHT, "glyph_right"},
        {ICON_FA_ARROW_UP, "glyph_up"}, {ICON_FA_ARROW_DOWN, "glyph_down"},
        {ICON_FA_PLUS, "glyph_plus"}, {ICON_FA_MINUS, "glyph_minus"}, {ICON_FA_BARS, "glyph_menu"},
        {ICON_FA_EXCLAMATION_TRIANGLE, ""}};
    std::string result;
    for (size_t i = 0; i < text.size();) {
        bool replaced = false;
        for (const auto& [glyph, key] : glyphs) {
            if (text.compare(i, 3, glyph) == 0) {
                result += *key ? Text(key) : " ";
                i += 3;
                replaced = true;
                break;
            }
        }
        if (replaced) continue;
        const auto c = static_cast<unsigned char>(text[i++]);
        if (c >= 32 && c != 127) result += static_cast<char>(c);
        else if (c == '\n') result += '\n';
        else if (c == '\t' || c == '\r') result += ' ';
    }
    return result;
}

namespace {
struct Conversion {
    size_t start;
    size_t end;
    char type;
};

std::optional<Conversion> FindNumber(const std::string& format) {
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%')
            continue;
        const auto start = i;
        if (i + 1 < format.size() && format[i + 1] == '%') {
            ++i;
            continue;
        }
        while (++i < format.size() && std::string("+- 0123456789.").find(format[i]) != std::string::npos) {}
        if (i < format.size() && std::string("dif").find(format[i]) != std::string::npos && i - start < 16)
            return Conversion{start, i + 1, format[i]};
    }
    return {};
}

std::string Unescape(std::string text) {
    for (size_t i = 0; (i = text.find("%%", i)) != std::string::npos; ++i)
        text.replace(i, 2, "%");
    return text;
}
}

std::string NumberLabel(const std::string& label) {
    auto result = label.substr(0, FindNumber(label).value_or(Conversion{label.size(), 0, 0}).start);
    while (!result.empty() && (result.back() == ' ' || result.back() == ':'))
        result.pop_back();
    return result;
}

std::string NumberValue(const std::string& label, const std::string& format, double value, bool integer) {
    const auto labelNumber = FindNumber(label);
    const std::string source = labelNumber ? label.substr(labelNumber->start) : format;
    const auto conversion = FindNumber(source);
    if (!conversion)
        return Unescape(source);
    // Only the verified numeric conversion reaches printf. Literal option captions, including
    // percentage signs and any other percent sequences, are never interpreted as arguments.
    const auto specifier = source.substr(conversion->start, conversion->end - conversion->start);
    char number[128];
    if (conversion->type == 'f')
        std::snprintf(number, sizeof(number), specifier.c_str(), value);
    else if (integer)
        std::snprintf(number, sizeof(number), specifier.c_str(), static_cast<int>(value));
    else
        return {};
    return Unescape(source.substr(0, conversion->start)) + number + Unescape(source.substr(conversion->end));
}
}
