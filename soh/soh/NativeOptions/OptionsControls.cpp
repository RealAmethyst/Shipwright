#include "NativeOptions.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>
#include <charconv>

namespace NativeOptions {

PagePtr MakePage(std::string id, std::string title, RowsProvider rows, std::string description) {
    auto page = std::make_shared<Page>();
    page->id = std::move(id);
    page->title = std::move(title);
    page->description = std::move(description);
    page->hints = Text("navigation");
    page->rows = std::move(rows);
    return page;
}

Row Action(std::string id, std::string label, std::function<void()> action, std::string description) {
    Row row;
    row.id = std::move(id);
    row.label = std::move(label);
    row.description = std::move(description);
    row.activate = std::move(action);
    return row;
}

Row Link(std::string id, std::string label, PageFactory page, std::string description) {
    return Action(std::move(id), std::move(label), [page] { GetModel().Push(page()); }, std::move(description));
}

Row Toggle(std::string id, std::string label, bool value, std::function<void(bool)> set, std::string description) {
    auto row = Action(std::move(id), std::move(label), [=] { set(!value); }, std::move(description));
    row.value = Text(value ? "on" : "off");
    row.adjustable = true;
    row.adjust = [=](int) { set(!value); };
    return row;
}

Row Integer(std::string id, std::string label, int value, int minimum, int maximum, int step,
            std::function<void(int)> set, std::string description) {
    auto validate = [=](const std::string& text) {
        int parsed = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size() && parsed >= minimum && parsed <= maximum
            ? std::string{} : Text("number_range") + " " + std::to_string(minimum) + " - " + std::to_string(maximum);
    };
    Row row = String(id, label, std::to_string(value), [=](std::string text) {
        int parsed = 0;
        if (validate(text).empty()) {
            std::from_chars(text.data(), text.data() + text.size(), parsed);
            set(parsed);
        }
    }, description, 16, validate, true);
    row.value = std::to_string(value);
    row.adjustable = true;
    row.enabled = minimum <= maximum;
    row.adjust = [=](int direction) {
        if (minimum > maximum)
            return;
        const auto next = std::clamp(static_cast<long long>(value) + direction * static_cast<long long>(std::max(1, step)),
                                    static_cast<long long>(minimum), static_cast<long long>(maximum));
        if (next != value)
            set(static_cast<int>(next));
    };
    return row;
}

Row Decimal(std::string id, std::string label, float value, float minimum, float maximum, float step,
            std::function<void(float)> set, std::string description, bool percentage) {
    const float factor = percentage ? 100.0f : 1.0f;
    auto validate = [=](const std::string& text) {
        float parsed = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
        return result.ec == std::errc{} && result.ptr == text.data() + text.size() && std::isfinite(parsed) &&
               parsed >= minimum * factor && parsed <= maximum * factor
            ? std::string{} : Text("number_range") + " " + std::to_string(minimum * factor) + " - " + std::to_string(maximum * factor);
    };
    auto row = String(id, label, std::to_string(value * factor), [=](std::string text) {
        float parsed = 0;
        if (validate(text).empty()) {
            std::from_chars(text.data(), text.data() + text.size(), parsed);
            set(parsed / factor);
        }
    }, description, 32, validate, true);
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(percentage ? 0 : (step < 0.01f ? 3 : 2))
           << (percentage ? value * 100.0f : value);
    row.value = stream.str() + (percentage ? "%" : "");
    row.adjustable = true;
    row.enabled = std::isfinite(value) && minimum <= maximum && std::isfinite(minimum) && std::isfinite(maximum);
    row.adjust = [=](int direction) {
        if (!std::isfinite(value) || minimum > maximum)
            return;
        const float increment = step > 0 && std::isfinite(step) ? step : 0.01f;
        const float next = std::clamp(value + direction * increment, minimum, maximum);
        if (next != value)
            set(next);
    };
    return row;
}

Row Choice(std::string id, std::string label, int value, std::map<int, std::string> choices,
           std::function<void(int)> set, std::string description) {
    auto row = Link(id, label, [=] {
        auto page = MakePage(id + "/choices", label, [=] {
            std::vector<Row> result;
            for (const auto& [key, name] : choices) {
                auto item = Action(std::to_string(key), name, [=] {
                    GetModel().Back([=] {
                        if (key != value)
                            set(key);
                    });
                });
                if (key == value)
                    item.value = Text("selected");
                result.push_back(std::move(item));
            }
            return result;
        });
        page->initialFocus = std::to_string(value);
        return page;
    }, description);
    const auto current = choices.find(value);
    row.value = current == choices.end() ? Text("unavailable") : current->second;
    row.enabled = !choices.empty();
    row.adjustable = true;
    row.adjust = [=](int direction) {
        if (choices.empty())
            return;
        auto it = choices.find(value);
        if (it == choices.end()) {
            it = choices.begin();
        } else if (direction > 0) {
            if (++it == choices.end())
                it = choices.begin();
        } else {
            if (it == choices.begin())
                it = choices.end();
            --it;
        }
        if (it->first != value)
            set(it->first);
    };
    return row;
}

void Confirm(std::string title, std::string description, std::string accept, std::function<void()> action) {
    auto page = MakePage("confirmation", std::move(title), [=] {
        return std::vector<Row>{ Action("cancel", Text("cancel"), [] { GetModel().Back(); }),
                                 Action("accept", accept, [=] {
                                     GetModel().Back(action);
                                 }) };
    }, std::move(description));
    page->popup = true;
    GetModel().Push(page);
}

void Message(std::string title, std::string description) {
    auto page = MakePage("message", std::move(title), [] {
        return std::vector<Row>{ Action("ok", Text("ok"), [] { GetModel().Back(); }) };
    }, std::move(description));
    page->popup = true;
    GetModel().Push(page);
}

} // namespace NativeOptions
