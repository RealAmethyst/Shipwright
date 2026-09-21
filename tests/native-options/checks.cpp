#include "NativeOptions.h"
#include "OptionsLayout.h"
#include "OptionsFormatting.h"
#include "OptionsFileIO.h"
#include "OptionsCaptureGate.h"
#include "OptionsPauseNavigation.h"
#include <ship/window/gui/IconsFontAwesome4.h>
#include "../../soh/soh/Enhancements/debugger/NativeSaveEditor.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <deque>

using namespace NativeOptions;

namespace NativeOptions {
static nlohmann::json resources;
std::string Text(const std::string& key) {
    if (!resources.is_object()) {
        std::ifstream file(OPTIONS_TEXT_PATH);
        file >> resources;
    }
    return resources.value(key, "");
}
Model& GetModel() {
    static Model model([](const std::string& text, size_t position, size_t count) {
        auto format = Text("position");
        format.replace(format.find("$0"), 2, std::to_string(position));
        format.replace(format.find("$1"), 2, std::to_string(count));
        return text + ", " + format;
    });
    return model;
}
} // namespace NativeOptions

static void Check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

static void NavigationChecks() {
    auto& model = GetModel();
    bool ready = false;
    bool showFirst = true;
    int calls = 0;
    auto page = MakePage("root", "Options", [&] {
        std::vector<Row> rows;
        if (!ready)
            return rows;
        if (showFirst)
            rows.push_back(Action("first", "First", [&] { ++calls; }));
        rows.push_back(Action("second", "Second", [&] { ++calls; }));
        return rows;
    });
    model.Open(page);
    auto speech = model.TakeSpeech();
    Check(speech.size() == 1 && speech[0].text == "Options" && speech[0].interrupt, "Opening must announce its title");
    model.Refresh();
    Check(model.TakeSpeech().empty(), "A delayed row must not repeat its title");
    ready = true;
    model.Refresh();
    speech = model.TakeSpeech();
    Check(speech.size() == 2 && !speech[0].interrupt && !speech[1].interrupt, "Delayed first item and hints must queue");
    Check(speech[0].text == "First, 1 of 2", "First item position must match the navigable snapshot");
    model.Move(1);
    speech = model.TakeSpeech();
    Check(speech.size() == 1 && speech[0].text == "Second, 2 of 2" && speech[0].interrupt, "Movement must interrupt once");
    showFirst = false;
    model.Refresh();
    Check(model.Rows()[model.Selection()].id == "second" && model.Selection() == 0, "Conditional rows must retain identity");
    model.Activate();
    Check(calls == 1, "Actions must run exactly once");
    model.TakeSpeech();
    auto child = MakePage("child", "Audio", [] { return std::vector<Row>{ Action("volume", "Volume", {}) }; });
    model.Push(child);
    model.TakeSpeech();
    model.Back();
    speech = model.TakeSpeech();
    Check(speech.size() == 1 && speech[0].text == "Second, 1 of 1", "Returning must not repeat root title or hints");
    model.Close();
    model.Open(page);
    speech = model.TakeSpeech();
    Check(speech.front().text == "Options", "A real close permits title on reopening");
    Message("Notice", "Saved");
    speech = model.TakeSpeech();
    Check(speech.size() == 4 && speech[2].text == "OK", "One-choice popup must omit position");
    model.Back();
    model.TakeSpeech();
    int selected = 17;
    auto choose = MakePage("choice-root", "Display", [&] {
        return std::vector<Row>{ Choice("renderer", "Renderer", selected, {{ 2, "Two" }, { 17, "Seventeen" }},
                                       [&](int value) { selected = value; }) };
    });
    model.Open(choose);
    model.TakeSpeech();
    model.Activate();
    speech = model.TakeSpeech();
    Check(model.Selection() == 1 && speech[1].text.find("Seventeen") == 0, "Choice must open and speak the selected value");
    model.Move(-1);
    model.Activate();
    Check(selected == 2 && model.Depth() == 1, "Choice must write its actual key and return");
    model.TakeSpeech();
    model.Adjust(-1);
    Check(selected == 17, "Sparse choices must wrap through real keys");
    model.Close();
    model.TakeSpeech();
}

static void InputTransitionChecks() {
    using Step = CaptureGate::Step;
    CaptureGate capture(100);
    Check(capture.Update(101, false, false) == Step::Wait, "Capture must wait for the activating input to release");
    Check(capture.Update(150, true, false) == Step::Arm, "The release frame must clear stale presses and arm event capture");
    Check(capture.Update(151, false, false) == Step::Poll, "The next press must reach the mapping factory");
    Check(capture.Update(152, false, true) == Step::Cancel, "Escape must cancel an armed capture");
    CaptureGate held(100);
    Check(held.Update(10099, false, false) == Step::Wait, "Holding input must not bypass release gating");
    Check(held.Update(10100, false, false) == Step::Cancel, "Ten seconds must cancel even while input remains held");
    CaptureGate wrapped(UINT32_MAX - 5000);
    Check(wrapped.Update(UINT32_MAX - 1, true, false) == Step::Arm, "Clock rollover must not expire capture early");
    Check(wrapped.Update(4998, false, false) == Step::Poll, "Capture must remain armed across clock rollover");
    Check(wrapped.Update(4999, false, false) == Step::Cancel, "The timeout must survive clock rollover");
    Check(NativeOptions_PauseRoute(3, true, false, 0, 3) == NATIVE_PAUSE_ENTER_OPTIONS,
          "Equipment right must enter Options");
    Check(NativeOptions_PauseRoute(0, false, false, 0, 3) == NATIVE_PAUSE_ENTER_OPTIONS,
          "Items left must enter Options");
    Check(NativeOptions_PauseRoute(3, false, true, 0, 3) == NATIVE_PAUSE_RETURN &&
          NativeOptions_PauseRoute(0, true, true, 0, 3) == NATIVE_PAUSE_RETURN,
          "Reversing direction must restore the preceding native page");
    Check(NativeOptions_PauseRoute(3, true, true, 0, 3) == NATIVE_PAUSE_ROTATE &&
          NativeOptions_PauseRoute(0, false, true, 0, 3) == NATIVE_PAUSE_ROTATE,
          "Continuing past Options must use the native wrap rotation");
    for (int page = 0; page < 4; ++page) {
        if (page < 3) Check(NativeOptions_PauseRoute(page, true, false, 0, 3) == NATIVE_PAUSE_ROTATE,
                            "Interior right transitions must retain native rotation");
        if (page > 0) Check(NativeOptions_PauseRoute(page, false, false, 0, 3) == NATIVE_PAUSE_ROTATE,
                            "Interior left transitions must retain native rotation");
    }
}

static void ControlChecks() {
    int value = 9;
    auto integer = Integer("i", "Number", value, 0, 10, 5, [&](int next) { value = next; });
    integer.adjust(1);
    Check(value == 10, "Integer adjustment must clamp");
    float fraction = 0.99f;
    auto decimal = Decimal("f", "Volume", fraction, 0, 1, 0.1f, [&](float next) { fraction = next; }, "", true);
    decimal.adjust(1);
    Check(fraction == 1.0f && decimal.value == "99%", "Percentage formatting and clamping must agree");
    int writes = 0;
    auto disabled = Action("disabled", "Unavailable", [&] { ++writes; });
    disabled.enabled = false;
    disabled.disabledReason = "Save Not Loaded";
    GetModel().Open(MakePage("test", "Options", [=] { return std::vector<Row>{disabled}; }));
    GetModel().Activate();
    Check(writes == 0, "Disabled actions must not execute");
    GetModel().Close();
    GetModel().TakeSpeech();
}

static void TextEntryChecks() {
    auto& model = GetModel();
    std::string saved = "Original";
    auto page = MakePage("text-root", "Options", [&] {
        return std::vector<Row>{ String("name", "Name", saved, [&](std::string value) { saved = value; }, "", 16) };
    });
    model.Open(page);
    model.Activate();
    Check(model.IsEditingText(), "Text editing must capture typing");
    model.Focus("clear");
    model.Activate();
    model.TakeSpeech();
    model.InputText("Caf\xC3\xA9");
    auto speech = model.TakeSpeech();
    Check(speech.size() == 1 && speech[0].text == "Caf\xC3\xA9", "Typing must announce the accepted text once");
    model.Backspace();
    model.Focus("done");
    model.Activate();
    Check(saved == "Caf" && !model.IsEditingText(), "Backspace must remove a whole UTF-8 code point");
    model.Activate();
    model.InputText(" discarded");
    model.Back();
    Check(saved == "Caf", "Back must cancel pending text edits");
    model.Activate();
    model.Focus("lowercase");
    model.Activate();
    Check(model.IsEditingText(), "Character picker must retain the editor's keyboard capture");
    model.InputText("e");
    model.Back();
    model.Focus("done");
    model.Activate();
    Check(saved == "Cafe", "Typing in a child character picker must update its owning editor");
    model.Close();
    model.TakeSpeech();
}

static void NumberEntryChecks() {
    Check(NumberLabel("Master Volume: %d %%") == "Master Volume", "Numeric editor titles must omit format syntax");
    Check(NumberValue("Master Volume: %d %%", "", 75, true) == "75 %", "Label units must survive");
    Check(NumberValue("FPS", "Original (%d)", 20, true) == "Original (20)", "Formatted value captions must survive");
    Check(NumberValue("Required", "50%", 4, true) == "50%", "Randomizer captions are literal, not indices");
    Check(NumberValue("Delay", "%+d frames", 2, true) == "+2 frames", "Signed values and units must survive");
    Check(NumberValue("Speed", "%.0f%%", 125, false) == "125%", "Percentage conversions must not duplicate the unit");
    auto& model = GetModel();
    int value = 5;
    model.Open(MakePage("numbers", "Options", [&] {
        return std::vector<Row>{Integer("value", "Value", value, -10, 10, 1, [&](int next) { value = next; })};
    }));
    model.Activate();
    for (const auto* invalid : {"11", "9999999999999", "1junk", "1.5", ""}) {
        model.Focus("clear");
        model.Activate();
        model.InputText(invalid);
        model.Focus("done");
        model.Activate();
        Check(model.IsEditingText() && value == 5, "Invalid numeric text must never write or close the editor");
    }
    model.Focus("clear");
    model.Activate();
    model.InputText("-8");
    model.Focus("done");
    model.Activate();
    Check(value == -8 && !model.IsEditingText(), "Valid signed numeric entry must apply exactly");
    model.Close();
    model.TakeSpeech();
}

static void DescriptionChecks() {
    auto width = [](uint8_t) { return 1.0f; };
    auto encode = [](const std::string& text) { return text; };
    auto lines = WrapText("One two three.\n\nFour", 7, 1, width, encode);
    Check(lines == std::vector<std::string>{"One two", "three.", "", "Four"}, "Details must preserve paragraphs and every word");
    lines = WrapText("abcdefghijk", 4, 1, width, encode);
    Check(lines == std::vector<std::string>{"abcd", "efgh", "ijk"}, "Long filenames must wrap without losing characters");
}

static void EditorResultChecks() {
    auto& model = GetModel();
    std::string value;
    model.Open(MakePage("root", "Options", [&] {
        return std::vector<Row>{String("notes", "Notes", value, [&](std::string next) {
            value = next;
            Message("Saved", value);
        }, "", 100, {}, false, true)};
    }));
    model.Activate();
    model.InputText("First line\nSecond line");
    model.Focus("done");
    model.Activate();
    Check(value == "First line\nSecond line", "Multiline notes must retain line breaks");
    Check(model.Depth() == 2 && model.CurrentPage()->title == "Saved",
          "Committing an editor must preserve a result dialog opened by its callback");
    model.Activate();
    Check(model.Depth() == 1 && model.Rows().front().value == value, "Dismissing the result must return to the updated row");
    model.Close();
    Check(model.TakeSpeech().empty(), "Closing Options must discard pending announcements");
}

static void FileChecks() {
    namespace fs = std::filesystem;
    const auto directory = fs::temp_directory_path() /
        ("ship-native-options-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Check(fs::create_directory(directory), "File checks must use a new private directory");
    const auto path = directory / "spoiler.json";
    auto read = [](const fs::path& file) {
        std::ifstream input(file, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    try {
        Check(ReplaceFileWithBackup(path, "original").empty(), "New files do not have a previous version");
        const auto first = ReplaceFileWithBackup(path, "edited");
        Check(read(path) == "edited" && read(first) == "original", "Saving must preserve the original file");
        const auto second = ReplaceFileWithBackup(path, "edited again");
        Check(first != second && read(first) == "original" && read(second) == "edited",
              "Repeated saves must preserve both backups");
        bool failed = false;
        try { ReplaceFileWithBackup(directory / "missing" / "spoiler.json", "bad"); }
        catch (const std::exception&) { failed = true; }
        Check(failed && read(path) == "edited again", "Save failures must be reported without changing other files");
        size_t count = 0;
        for (const auto& entry : fs::directory_iterator(directory)) {
            Check(entry.is_regular_file(), "Successful saves must remove their staging directory");
            ++count;
        }
        Check(count == 3, "File checks should leave only the destination and two backups");
    } catch (...) {
        fs::remove_all(directory);
        throw;
    }
    fs::remove_all(directory);
}

static void UnsignedEditorChecks() {
    auto& model = GetModel();
    uint32_t flags = 3;
    bool live = true;
    model.Open(MakePage("save", "Save Editor", [&] {
        return std::vector<Row>{NativeSaveEditor::Scalar("flags", "Flags", flags, "", [&] { return live; })};
    }));
    model.Activate();
    model.Focus("clear");
    model.Activate();
    model.InputText("4294967296");
    model.Focus("done");
    model.Activate();
    Check(model.IsEditingText() && flags == 3, "Overflowing unsigned save values must be rejected");
    model.Focus("clear");
    model.Activate();
    model.InputText("4294967295");
    model.Focus("done");
    model.Activate();
    Check(flags == UINT32_MAX, "Unsigned save fields must retain all 32 bits");
    model.Adjust(1);
    Check(flags == UINT32_MAX, "Increment at the unsigned limit must not wrap");
    live = false;
    model.Adjust(-1);
    Check(flags == UINT32_MAX, "A stale player or scene guard must prevent writes");
    model.Close();
}

static void GlyphChecks() {
    Check(PrepareOptionsText("Left Stick " ICON_FA_ARROW_LEFT) == "Left Stick Left",
          "Physical stick directions must retain their meaning");
    Check(PrepareOptionsText("D-Pad " ICON_FA_ARROW_DOWN) == "D-Pad Down",
          "Physical D-pad directions must retain their meaning");
    Check(PrepareOptionsText(ICON_FA_BARS) == "Menu", "The SDL Start glyph must have a readable caption");
    Check(PrepareOptionsText("Axis 6 " ICON_FA_MINUS) == "Axis 6 Minus", "Generic axis signs must remain distinct");
    Check(PrepareOptionsText("50%\nA + B") == "50%\nA + B", "Literal values and combinations must survive preparation");
    Check(PrepareOptionsText(ICON_FA_EXCLAMATION_TRIANGLE " WARNING").find(ICON_FA_EXCLAMATION_TRIANGLE) == std::string::npos,
          "Decorative warning icons must not reach the game font or speech");
}

static void FeedbackChecks() {
    std::vector<Feedback> sounds;
    Model model([](const std::string& text, size_t position, size_t count) {
        return text + ", " + std::to_string(position) + " of " + std::to_string(count);
    }, [&](Feedback sound) { sounds.push_back(sound); });
    bool fullscreen = false;
    int writes = 0;
    auto child = MakePage("child", "Choice", [&] {
        return std::vector<Row>{Action("accept", "Accept", [&] { model.Back(); })};
    });
    auto root = MakePage("root", "Options", [&] {
        auto locked = Action("locked", "Unavailable", [&] { ++writes; });
        locked.enabled = false;
        return std::vector<Row>{
            Toggle("fullscreen", "Toggle Fullscreen", fullscreen, [&](bool next) { fullscreen = next; ++writes; },
                   "Toggles Fullscreen On/Off."),
            Action("child", "Choice", [&] { model.Push(child); }), locked,
            Action("info", "Information", {})};
    });
    model.Open(root);
    auto speech = model.TakeSpeech();
    Check(sounds.empty(), "Constructing or refreshing a page must not play an input sound");
    Check(speech[1].text == "Toggle Fullscreen, Off, Toggles Fullscreen On/Off., 1 of 4",
          "A live toggle must announce its value before the description and position");
    model.Navigate(Navigation::Confirm);
    speech = model.TakeSpeech();
    Check(fullscreen && writes == 1 && model.Depth() == 1 && sounds == std::vector{Feedback::Confirm},
          "Confirm must toggle in place, play one sound and write once");
    Check(speech.size() == 1 && speech[0].text == "Toggle Fullscreen, On, Toggles Fullscreen On/Off., 1 of 4",
          "The new live value must be spoken immediately after confirm");
    sounds.clear();
    model.Navigate(Navigation::Left);
    speech = model.TakeSpeech();
    Check(!fullscreen && writes == 2 && sounds == std::vector{Feedback::Cursor} && speech.size() == 1 &&
              speech[0].text == "Toggle Fullscreen, Off, Toggles Fullscreen On/Off., 1 of 4",
          "Directional adjustment must speak the updated toggle without entering a child page");
    fullscreen = true;
    model.Refresh();
    Check(model.Rows()[0].value == "On" && model.TakeSpeech().empty(),
          "External window changes must update the row without automatic per-frame speech");
    model.Navigate(Navigation::Right);
    Check(!fullscreen && writes == 3, "An adjustment after an external change must use the current value");
    sounds.clear();
    model.Navigate(Navigation::Down);
    model.Navigate(Navigation::Confirm);
    Check(model.Depth() == 2 && sounds == std::vector{Feedback::Cursor, Feedback::Confirm},
          "Entering a child page must play one confirm after cursor movement");
    sounds.clear();
    model.Navigate(Navigation::Confirm);
    Check(model.Depth() == 1 && sounds == std::vector{Feedback::Confirm},
          "Accepting a choice that returns to its parent must not also play Back");
    sounds.clear();
    model.Navigate(Navigation::Confirm);
    model.Navigate(Navigation::Back);
    Check(model.Depth() == 1 && sounds == std::vector{Feedback::Confirm, Feedback::Back},
          "User Back must play its distinct sound exactly once");
    model.Focus("locked");
    sounds.clear();
    model.Navigate(Navigation::Confirm);
    Check(writes == 3 && sounds == std::vector{Feedback::Error}, "Disabled actions must not play successful confirmation");
    model.Focus("info");
    sounds.clear();
    model.Navigate(Navigation::Confirm);
    model.Navigate(Navigation::Right);
    Check(sounds.empty(), "An informational row must not claim an action or adjustment occurred");
    model.Navigate(Navigation::Back);
    Check(!model.IsOpen() && sounds == std::vector{Feedback::Back}, "Closing the root must play one Back sound");
    model.Navigate(Navigation::Back);
    Check(sounds.size() == 1, "Input after closing must not play another sound");
}

void DisplayListChecks();

int main(int argc, char** argv) {
    try {
        NavigationChecks();
        InputTransitionChecks();
        ControlChecks();
        TextEntryChecks();
        NumberEntryChecks();
        DescriptionChecks();
        EditorResultChecks();
        FileChecks();
        UnsignedEditorChecks();
        GlyphChecks();
        FeedbackChecks();
        DisplayListChecks();
        if (argc == 3 && std::string(argv[1]) == "--preview") {
            nlohmann::json request;
            std::cin >> request;
            std::vector<Row> rows;
            std::deque<std::string> imageNames;
            for (const auto& item : request.at("rows")) {
                Row row;
                row.id = item.at("id");
                row.label = item.at("label");
                row.value = item.value("value", "");
                row.description = item.value("description", "");
                row.enabled = item.value("enabled", true);
                if (item.contains("image")) {
                    imageNames.push_back(item.at("image").at("resource"));
                    row.image = {imageNames.back().c_str(), item.at("image").at("width"),
                                 item.at("image").at("height")};
                }
                rows.push_back(row);
            }
            auto page = MakePage("preview", request.at("title"), [=] { return rows; });
            page->initialFocus = request.value("focus", rows.empty() ? "" : rows.front().id);
            page->popup = request.value("popup", false);
            page->description = request.value("description", "");
            page->documentLines = request.value("documentLines", std::vector<std::string>{});
            page->footer = request.value("footer", "");
            if (request.contains("image")) {
                imageNames.push_back(request.at("image").at("resource"));
                page->documentImage = {imageNames.back().c_str(), request.at("image").at("width"),
                                      request.at("image").at("height")};
            }
            GetModel().Open(page);
            const auto widths = request.at("widths").get<std::vector<float>>();
            const GlyphWidth glyphWidth = [=](uint8_t glyph) {
                return glyph >= 32 && glyph - 32 < widths.size() ? widths[glyph - 32] : 0;
            };
            const EncodeText encode = [](const std::string& text) { return text; };
            const auto commands = request.value("titleMenu", false)
                ? TitleLayout(request.at("start"), Text("title"), Text("title_navigation"), request.value("selection", 0),
                              255, glyphWidth, encode)
                : Layout(GetModel(), Text("hints"), glyphWidth, encode);
            nlohmann::json result = nlohmann::json::array();
            for (const auto& command : commands) {
                Check(std::isfinite(command.x) && std::isfinite(command.y) && command.width > 0 && command.height > 0,
                      "Layout geometry must be finite and positive");
                result.push_back({{"type", static_cast<int>(command.primitive)}, {"x", command.x}, {"y", command.y},
                                  {"width", command.width}, {"height", command.height}, {"texture", command.texture},
                                  {"resource", command.image.resource ? command.image.resource : ""},
                                  {"color", {command.color.r, command.color.g, command.color.b, command.color.a}}});
            }
            std::ofstream output(argv[2]);
            output << result.dump(2);
        }
        std::cout << "Native Options checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
