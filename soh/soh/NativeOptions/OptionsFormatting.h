#pragma once

#include <string>

namespace NativeOptions {
std::string PrepareOptionsText(const std::string& text);
std::string NumberLabel(const std::string& label);
std::string NumberValue(const std::string& label, const std::string& format, double value, bool integer);
}
