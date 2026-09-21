#ifndef SOH_SPEECH_TEXT_H
#define SOH_SPEECH_TEXT_H

#include "../speechsynthesizer/SpeechSynthesizer.h"
#include <string>
#include <vector>

namespace SpeechText {
inline std::string WithPosition(const std::string& text, std::string format, int position, int total) {
    if (text.empty() || position < 1 || position > total)
        return "";
    const auto first = format.find("$0");
    if (first == std::string::npos)
        return "";
    format.replace(first, 2, std::to_string(position));
    const auto last = format.find("$1");
    if (last == std::string::npos)
        return "";
    format.replace(last, 2, std::to_string(total));
    return text + ", " + format;
}

struct Dialog {
    std::string body;
    std::vector<std::string> choices;
};

// Only the game decoder introduces newlines for choices; ordinary line wraps become spaces.
inline Dialog SplitDialog(const std::string& decoded, int choiceCount) {
    Dialog dialog;
    const auto choicesStart = decoded.find('\n');
    dialog.body = SpeechSynthesizer::PrepareText(decoded.substr(0, choicesStart).c_str());
    if (choicesStart == std::string::npos || (choiceCount != 2 && choiceCount != 3))
        return dialog;
    size_t begin = choicesStart + 1;
    while (begin < decoded.size()) {
        const auto end = decoded.find('\n', begin);
        const auto choice = SpeechSynthesizer::PrepareText(decoded.substr(begin, end - begin).c_str());
        if (!choice.empty())
            dialog.choices.push_back(choice);
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    if (dialog.choices.size() != choiceCount)
        dialog.choices.clear();
    return dialog;
}
} // namespace SpeechText

#endif
