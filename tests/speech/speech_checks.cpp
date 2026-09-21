#include "SpeechSynthesizer.h"
#include "../../soh/soh/Enhancements/tts/SpeechText.h"
#include "../../soh/soh/Enhancements/tts/MenuSpeech.h"
#include <prism.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

struct Utterance {
    std::string text;
    bool interrupt;
};
static std::vector<Utterance> utterances;
static bool backendAvailable = true;
static int contexts = 0, backends = 0, stops = 0;
SpeechSynthesizer* SpeechSynthesizer::Instance = nullptr;

extern "C" {
PrismConfig PRISM_CALL prism_config_init() {
    PrismConfig config{};
    config.version = PRISM_CONFIG_VERSION;
    return config;
}
PrismContext* PRISM_CALL prism_init(PrismConfig*) {
    ++contexts;
    return reinterpret_cast<PrismContext*>(1);
}
PrismBackend* PRISM_CALL prism_registry_create_best(PrismContext*) {
    if (!backendAvailable) return nullptr;
    ++backends;
    return reinterpret_cast<PrismBackend*>(2);
}
void PRISM_CALL prism_backend_free(PrismBackend* backend) {
    if (backend != nullptr) --backends;
}
void PRISM_CALL prism_shutdown(PrismContext*) { --contexts; }
const char* PRISM_CALL prism_backend_name(PrismBackend*) { return "test backend"; }
const char* PRISM_CALL prism_error_string(PrismError) { return "test error"; }
PrismError PRISM_CALL prism_backend_speak(PrismBackend*, const char* text, bool interrupt) {
    utterances.push_back({ text, interrupt });
    return PRISM_OK;
}
PrismError PRISM_CALL prism_backend_stop(PrismBackend*) { ++stops; return PRISM_OK; }
}

static void Check(bool result, const char* message) {
    if (!result) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

static void FileReturnChecks() {
    SpeechText::MenuSpeech menu;
    std::vector<Utterance> spoken;
    const auto speak = [&](const std::string& text, bool interrupt) { spoken.push_back({text, interrupt}); };
    const auto file = SpeechText::WithPosition("File 1", "$0 of $1", 1, 6);
    const auto options = SpeechText::WithPosition("Options", "$0 of $1", 6, 6);
    menu.Enter("Please select a file.", true, "A, Decide. B, Cancel.", speak);
    menu.Item("", speak);
    Check(spoken.size() == 1 && spoken[0].interrupt, "The file heading must wait for a readable first item");
    menu.Item(file, speak);
    Check(spoken.size() == 3 && spoken[1].text == file && !spoken[1].interrupt && !spoken[2].interrupt,
          "The delayed first item and controls must queue after the real opening heading");
    spoken.clear();
    menu.Item(options, speak);
    menu.Item(options, speak);
    Check(spoken.size() == 1 && spoken[0].text == "Options, 6 of 6" && spoken[0].interrupt,
          "Ordinary focus movement must speak the actual item and position only once");

    // Native Options suspends file selection without changing its view/cursor.
    // Closing it must invalidate item deduplication, not the parent's lifetime.
    spoken.clear();
    menu.Resume();
    menu.Item("", speak);
    Check(spoken.empty(), "A resumed view must wait for verified item text");
    menu.Item(options, speak);
    menu.Item(options, speak);
    Check(spoken.size() == 1 && spoken[0].text == "Options, 6 of 6" && spoken[0].interrupt,
          "Returning from Options must interrupt with the unchanged item, without heading or hints");
    spoken.clear();
    menu.Resume();
    menu.Item(options, speak);
    Check(spoken.size() == 1, "Each actual Options close must permit exactly one renewed item announcement");

    // Escape can cover other file-selection views and focused rows as well.
    menu.Item(file, speak);
    spoken.clear();
    menu.Resume();
    menu.Item(file, speak);
    Check(spoken.size() == 1 && spoken[0].text == file, "Return must announce current focus, not hardcoded Options");
    menu.Enter("Open this file?", true, "A, Decide. B, Cancel.", speak);
    const auto yes = SpeechText::WithPosition("Yes", "$0 of $1", 1, 2);
    menu.Item(yes, speak);
    spoken.clear();
    menu.Resume();
    menu.Item(yes, speak);
    Check(spoken.size() == 1 && spoken[0].text == yes && spoken[0].interrupt,
          "An overlay return to a native confirmation must retain its two-choice position");
    spoken.clear();
    menu.Enter("Please select a file.", false, "A, Decide. B, Cancel.", speak);
    menu.Item(file, speak);
    Check(spoken.size() == 1 && spoken[0].text == file && spoken[0].interrupt,
          "Returning from a native child must still omit the parent's heading and controls");
    spoken.clear();
    menu = {};
    menu.Enter("Please select a file.", true, "A, Decide. B, Cancel.", speak);
    menu.Item(file, speak);
    Check(spoken.size() == 3 && spoken[0].interrupt && !spoken[1].interrupt && !spoken[2].interrupt,
          "A real file-screen close and reopen must restore the complete opening sequence");
}

int main() {
    FileReturnChecks();
    Check(SpeechText::WithPosition("Yes", "$0 of $1", 1, 2) == "Yes, 1 of 2", "Choice index is missing");
    Check(SpeechText::WithPosition("Mono", "$0 von $1", 2, 3) == "Mono, 2 von 3", "Localized position failed");
    Check(SpeechText::WithPosition("Item", "$0 of $1", 1, 1) == "Item, 1 of 1", "Single ordinary item lost its index");
    Check(SpeechText::WithPosition("Item", "$0 of $1", 4, 3).empty(), "Invalid cursor must fail closed");
    Check(SpeechText::WithPosition("", "$0 of $1", 1, 2).empty(), "Unknown name must remain silent");
    Check(SpeechText::WithPosition("Item", "unknown", 1, 2).empty(), "Missing format must fail closed");
    auto dialog = SpeechText::SplitDialog("  Continue? \nYes\nNo", 2);
    Check(dialog.body == "Continue?" && dialog.choices.size() == 2 && dialog.choices[1] == "No",
          "Dialogue question and choices were not separated");
    dialog = SpeechText::SplitDialog("\nFirst\nSecond\nThird", 3);
    Check(dialog.body.empty() && dialog.choices.size() == 3, "Choices without a heading were lost");
    Check(SpeechText::SplitDialog("Question\nOnly one answer", 2).choices.empty(), "Incomplete choices must fail closed");
    Check(SpeechText::SplitDialog("Ordinary dialogue.", 0).body == "Ordinary dialogue.", "Ordinary dialogue changed");
    {
        SpeechSynthesizer speech;
        speech.Speak("Not initialized", "en-US");
        Check(utterances.empty(), "Uninitialized speech must be silent");
        Check(speech.Init() && speech.Init(), "Initialization must be idempotent");
        Check(contexts == 1 && backends == 1, "Repeated initialization leaked resources");
        speech.Speak("  Please select a file.\n", "en-US");
        speech.Speak("File 1, 1 of 6", "en-US", false);
        speech.Speak("A, Decide. B, Cancel.", "en-US", false);
        speech.Speak("File 2, 2 of 6", "en-US");
        Check(utterances.size() == 4 && utterances[0].interrupt && !utterances[1].interrupt &&
              !utterances[2].interrupt && utterances[3].interrupt, "Interrupt/queue intent was lost");
        Check(utterances[0].text == "Please select a file.", "Whitespace was not normalized");
        speech.Speak("\r\n\t", "en-US");
        speech.Speak(nullptr, "en-US");
        Check(utterances.size() == 4 && stops == 0, "Missing text must not cancel valid queued speech");
        speech.Speak("Épée\t öffnen", "fr-FR", false);
        Check(utterances.back().text == "Épée öffnen", "UTF-8 text was damaged");
        speech.Stop();
        Check(stops == 1, "Explicit stop was not forwarded");
        speech.Uninitialize();
        speech.Uninitialize();
        Check(contexts == 0 && backends == 0, "Shutdown leaked resources");
    }
    backendAvailable = false;
    SpeechSynthesizer failed;
    Check(!failed.Init() && contexts == 0 && backends == 0, "Failed initialization leaked its context");
    failed.Speak("Must remain silent", "en-US");
    Check(utterances.size() == 5, "Failed backend accepted speech");
    std::cout << "Speech bridge checks passed.\n";
}
