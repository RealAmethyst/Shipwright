#pragma once

#include "NativeOptions.h"

namespace NativeOptions {
template <typename Map> std::map<int, std::string> TrackerChoices(const Map& choices) {
    return {choices.begin(), choices.end()};
}
void AppendTrackerDisplayRows(std::vector<Row>& rows, const std::string& prefix, int defaultWindow,
                              const std::string& displayKey, const std::map<int, std::string>& modes,
                              const std::map<int, std::string>& buttons, std::function<void()> changed = {});
}
