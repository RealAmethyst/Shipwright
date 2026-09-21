#include "NativeOptions.h"

namespace NativeOptions {

Row String(std::string id, std::string label, std::string value, std::function<void(std::string)> set,
           std::string description, size_t maxLength, TextValidator validate, bool numeric, bool multiline) {
    auto row = Link(id, label, [=] {
        auto buffer = std::make_shared<std::string>(value);
        auto append = [=](const std::string& text) {
            if (text.empty() || buffer->size() + text.size() > maxLength)
                return;
            for (unsigned char character : text)
                if ((character < 32 && !(multiline && (character == '\n' || character == '\t' || character == '\r'))) || character == 127)
                    return;
            *buffer += text;
            GetModel().Announce(text);
        };
        auto erase = [=] {
            if (buffer->empty())
                return;
            size_t end = buffer->size() - 1;
            while (end > 0 && (static_cast<unsigned char>((*buffer)[end]) & 0xC0) == 0x80)
                --end;
            buffer->erase(end);
            GetModel().Announce(buffer->empty() ? Text("empty") : *buffer);
        };
        auto characters = [=](const std::string& title, const std::string& alphabet) {
            return MakePage(id + "/characters", title, [=] {
                std::vector<Row> rows;
                for (size_t i = 0; i < alphabet.size(); ++i) {
                    const auto character = alphabet.substr(i, 1);
                    rows.push_back(Action(std::to_string(i), character, [=] { append(character); }));
                }
                return rows;
            });
        };
        auto page = MakePage(id + "/edit", label, [=] {
            std::vector<Row> rows;
            auto preview = Action("value", Text("text_value"), [=] { GetModel().Announce(*buffer); });
            preview.value = buffer->empty() ? Text("empty") : *buffer;
            rows.push_back(std::move(preview));
            if (!numeric) {
                rows.push_back(Link("lowercase", Text("lowercase"), [=] {
                    return characters(Text("lowercase"), "abcdefghijklmnopqrstuvwxyz");
                }));
                rows.push_back(Link("uppercase", Text("uppercase"), [=] {
                    return characters(Text("uppercase"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
                }));
            }
            rows.push_back(Link("numbers", Text("numbers"), [=] {
                return characters(Text("numbers"), numeric ? "0123456789.-" : "0123456789.,:;!?-_/@\\#%&*+()[]{}=\"'$^~|<>");
            }));
            if (!numeric)
                rows.push_back(Action("space", Text("space"), [=] { append(" "); }));
            if (multiline)
                rows.push_back(Action("newline", Text("newline"), [=] { append("\n"); }));
            rows.push_back(Action("backspace", Text("backspace"), erase));
            rows.push_back(Action("clear", Text("clear"), [=] { buffer->clear(); }));
            auto done = Action("done", Text("done"), [=] {
                GetModel().Back([=] { set(*buffer); });
            });
            if (validate) {
                done.disabledReason = validate(*buffer);
                done.enabled = done.disabledReason.empty();
            }
            rows.push_back(std::move(done));
            rows.push_back(Action("cancel", Text("cancel"), [] { GetModel().Back(); }));
            return rows;
        }, description);
        page->hints = Text("keyboard_hint");
        page->onText = append;
        page->onBackspace = erase;
        return page;
    }, description);
    row.value = value.empty() ? Text("empty") : value;
    return row;
}

} // namespace NativeOptions
