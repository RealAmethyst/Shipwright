#pragma once

#include "soh/NativeOptions/NativeOptions.h"
#include <charconv>
#include <limits>
#include <type_traits>

namespace NativeSaveEditor {
namespace N = NativeOptions;
using Guard = std::function<bool()>;

inline std::string Text(const char* key) {
    return N::Text(std::string("save_") + key);
}

template <class T> N::Row Scalar(std::string id, std::string label, T& field, std::string description = "",
                                 Guard guard = {}, std::function<void()> changed = {}) {
    auto* pointer = &field;
    auto set = [pointer, guard, changed](T value) {
        if (guard && !guard()) return;
        *pointer = value;
        if (changed) changed();
    };
    if constexpr (std::is_floating_point_v<T>) {
        return N::Decimal(std::move(id), std::move(label), field, -std::numeric_limits<float>::max(),
                          std::numeric_limits<float>::max(), 1, set, std::move(description));
    } else if constexpr (std::numeric_limits<T>::max() <= INT32_MAX) {
        return N::Integer(std::move(id), std::move(label), field, std::numeric_limits<T>::lowest(),
                          std::numeric_limits<T>::max(), 1, set, std::move(description));
    } else {
        static_assert(std::is_unsigned_v<T> && sizeof(T) <= sizeof(uint32_t));
        auto validate = [](const std::string& text) {
            T value{};
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? std::string{} :
                N::Text("number_range") + " 0 - " + std::to_string(std::numeric_limits<T>::max());
        };
        auto row = N::String(std::move(id), std::move(label), std::to_string(field), [set, validate](std::string text) {
            T value{};
            if (validate(text).empty()) {
                std::from_chars(text.data(), text.data() + text.size(), value);
                set(value);
            }
        }, std::move(description), 16, validate, true);
        const T value = field;
        row.adjustable = true;
        row.adjust = [set, value](int direction) {
            if (direction > 0 && value < std::numeric_limits<T>::max()) set(value + 1);
            else if (direction < 0 && value > 0) set(value - 1);
        };
        return row;
    }
}

template <class T> N::Row Flag(std::string id, std::string label, T& field, uint32_t mask,
                               std::string description = "", Guard guard = {}) {
    auto* pointer = &field;
    return N::Toggle(std::move(id), std::move(label), (static_cast<uint32_t>(field) & mask) != 0,
        [pointer, mask, guard](bool enabled) {
            if (guard && !guard()) return;
            const auto current = static_cast<uint32_t>(*pointer);
            *pointer = static_cast<T>(enabled ? current | mask : current & ~mask);
        }, std::move(description));
}

template <class T> N::PagePtr Bits(std::string title, T& field, bool bulk = false,
                                   Guard guard = {}, std::vector<std::string> names = {}) {
    auto* pointer = &field;
    return N::MakePage("save/bits/" + title, title, [pointer, title, bulk, guard, names] {
        std::vector<N::Row> rows;
        if (guard && !guard()) return rows;
        if (bulk) {
            rows.push_back(N::Action("set", Text("set_all"), [pointer, guard] {
                if (!guard || guard()) *pointer = static_cast<T>(~std::make_unsigned_t<T>{0});
            }));
            rows.push_back(N::Action("clear", Text("clear_all"), [pointer, guard] {
                if (!guard || guard()) *pointer = 0;
            }));
        }
        for (int bit = 0; bit < sizeof(T) * 8; ++bit)
            rows.push_back(Flag(std::to_string(bit), bit < names.size() ? names[bit] : std::to_string(bit),
                               *pointer, uint32_t{1} << bit, "", guard));
        return rows;
    });
}

N::PagePtr InfoPage();
std::string ItemName(int item);
N::RowImage Image(const std::string& path);
N::RowImage ItemImage(int item);
N::PagePtr InventoryPage();
N::PagePtr EquipmentPage();
N::PagePtr QuestPage();
N::PagePtr FlagsPage();
N::PagePtr PlayerPage();
N::PagePtr PlayerFlagsPage();
void SceneChanged();
Guard CurrentSceneGuard();
}
