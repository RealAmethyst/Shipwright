#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace SpeechText {
inline std::string SavedLatinName(std::span<const uint8_t> data, bool pal) {
    std::string result;
    for (uint8_t c : data) {
        if (c < 10) result += char('0' + c);
        else if (pal && c < 0x24) result += char('A' + c - 10);
        else if (pal && c < 0x3e) result += char('a' + c - 0x24);
        else if (!pal && c >= 0xab && c < 0xc5) result += char('A' + c - 0xab);
        else if (!pal && c >= 0xc5 && c < 0xdf) result += char('a' + c - 0xc5);
        else if (c == (pal ? 0x3e : 0xdf)) result += ' ';
        else if (c == (pal ? 0x3f : 0xe4)) result += '-';
        else if (c == (pal ? 0x40 : 0xea)) result += '.';
        else return "";
    }
    const auto end = result.find_last_not_of(' ');
    return end == std::string::npos ? "" : result.substr(0, end + 1);
}

// Caption-only subset of Message_Decode's raw western message format. Unlike
// decoded dialogue, raw sign text still contains the saved-name substitution.
// Reject unsupported controls instead of treating their parameters as letters.
inline std::string SignCaption(std::string_view raw, const std::string& name, bool firstLine,
                               const std::function<std::string(uint8_t)>& glyph) {
    std::string result;
    if (raw.size() > 4096) return "";
    for (size_t i = 0; i < raw.size(); ++i) {
        const uint8_t c = static_cast<uint8_t>(raw[i]);
        if (c == 0x02 || c == 0x09 || (c == 0x01 && firstLine)) return result;
        if (c == 0x01) { result += ' '; continue; }
        if (c == 0x05 || c == 0x06) { if (++i >= raw.size()) return ""; continue; }
        if (c == 0x08) continue;
        if (c == 0x0f) { if (name.empty()) return ""; result += name; continue; }
        if (c < 0x20 || c == 0x7f) return "";
        if (c < 0x80) result += char(c);
        else {
            const auto decoded = glyph(c);
            if (decoded.empty()) return "";
            result += decoded;
        }
    }
    return "";
}
}
