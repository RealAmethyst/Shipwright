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

// Some native messages highlight a character's name as their only color-A span.
// Reject any other control or a second candidate rather than name the wrong NPC.
inline std::string HighlightedName(std::string_view raw) {
    if (raw.size() > 4096) return "";
    std::string result;
    for (size_t i = 0; i + 1 < raw.size(); ++i) {
        if (static_cast<uint8_t>(raw[i]) != 0x05 || raw[i + 1] != 'A') continue;
        if (!result.empty()) return "";
        i += 2;
        for (; i < raw.size(); ++i) {
            const auto c = static_cast<uint8_t>(raw[i]);
            if (c == 0x05 && i + 1 < raw.size() && raw[i + 1] == '@') { ++i; break; }
            if (result.size() >= 32 || !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                                         c == ' ' || c == '-' || c == '\'')) return "";
            result += static_cast<char>(c);
        }
        if (i >= raw.size()) return "";
        const auto first = result.find_first_not_of(' ');
        const auto last = result.find_last_not_of(' ');
        if (first == std::string::npos) return "";
        result = result.substr(first, last - first + 1);
    }
    return result;
}
}
