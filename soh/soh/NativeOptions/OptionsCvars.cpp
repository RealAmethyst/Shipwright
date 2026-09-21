#include "NativeOptions.h"
#include <libultraship/libultraship.h>
#include "soh/ShipInit.hpp"

namespace NativeOptions {

void ChangedCVar(const std::string& cvar) {
    SaveSettings();
    ShipInit::Init(cvar.c_str());
}

Row CVarToggle(std::string label, std::string cvar, bool defaultValue, std::string description,
               std::function<void()> changed) {
    return Toggle(cvar, label, CVarGetInteger(cvar.c_str(), defaultValue), [=](bool value) {
        CVarSetInteger(cvar.c_str(), value);
        ChangedCVar(cvar);
        if (changed) changed();
    }, description);
}

Row CVarInteger(std::string label, std::string cvar, int defaultValue, int minimum, int maximum, int step,
                std::string description, std::function<void()> changed) {
    return Integer(cvar, label, CVarGetInteger(cvar.c_str(), defaultValue), minimum, maximum, step, [=](int value) {
        CVarSetInteger(cvar.c_str(), value);
        ChangedCVar(cvar);
        if (changed) changed();
    }, description);
}

Row CVarDecimal(std::string label, std::string cvar, float defaultValue, float minimum, float maximum, float step,
                std::string description, bool percentage, std::function<void()> changed) {
    return Decimal(cvar, label, CVarGetFloat(cvar.c_str(), defaultValue), minimum, maximum, step, [=](float value) {
        CVarSetFloat(cvar.c_str(), value);
        ChangedCVar(cvar);
        if (changed) changed();
    }, description, percentage);
}

Row CVarChoice(std::string label, std::string cvar, int defaultValue, std::map<int, std::string> choices,
               std::string description, std::function<void()> changed) {
    return Choice(cvar, label, CVarGetInteger(cvar.c_str(), defaultValue), choices, [=](int value) {
        CVarSetInteger(cvar.c_str(), value);
        ChangedCVar(cvar);
        if (changed) changed();
    }, description);
}

Row CVarString(std::string label, std::string cvar, std::string defaultValue, std::string description) {
    return String(cvar, label, CVarGetString(cvar.c_str(), defaultValue.c_str()), [=](std::string value) {
        CVarSetString(cvar.c_str(), value.c_str());
        ChangedCVar(cvar);
    }, description);
}

void Disable(std::vector<Row>& rows, const std::string& reason) {
    for (auto& row : rows) {
        row.enabled = false;
        row.disabledReason = reason;
    }
}

} // namespace NativeOptions
