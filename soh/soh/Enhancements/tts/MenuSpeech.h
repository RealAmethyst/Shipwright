#pragma once

#include <functional>
#include <string>

namespace SpeechText {

// Screen lifetime is owned by the native view tracker. This state only controls
// the initial queue, focused-item deduplication and the opening control hints.
class MenuSpeech {
  public:
    using Speaker = std::function<void(const std::string&, bool)>;

    void Enter(const std::string& title, bool opening, const std::string& hints, const Speaker& speak) {
        previousItem.clear();
        queueItem = opening && !title.empty();
        pendingHints = opening ? hints : "";
        if (queueItem) speak(title, true);
    }

    void Item(const std::string& text, const Speaker& speak) {
        if (text.empty() || text == previousItem) return;
        speak(text, !queueItem);
        queueItem = false;
        previousItem = text;
        if (!pendingHints.empty()) {
            speak(pendingHints, false);
            pendingHints.clear();
        }
    }

    // Returning from an overlay restores focus without reopening the parent.
    void Resume() {
        previousItem.clear();
        queueItem = false;
        pendingHints.clear();
    }

  private:
    std::string previousItem;
    std::string pendingHints;
    bool queueItem = false;
};

} // namespace SpeechText
