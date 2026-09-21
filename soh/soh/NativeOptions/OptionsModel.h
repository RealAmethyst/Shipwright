#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace NativeOptions {

struct Page;

struct RowImage {
    const char* resource = nullptr;
    int width = 0;
    int height = 0;
    int format = 0;
    int size = 0;
    bool guiTexture = false;
};

struct Row {
    std::string id;
    std::string label;
    std::string value;
    std::string description;
    std::string disabledReason;
    bool enabled = true;
    bool adjustable = false;
    std::function<void()> activate;
    std::function<void(int)> adjust;
    std::function<void()> held;
    RowImage image;
};

struct Page {
    std::string id;
    std::string title;
    std::string description;
    std::string hints;
    std::string footer;
    std::string initialFocus;
    bool popup = false;
    bool gamePreview = false;
    std::vector<std::string> documentLines;
    RowImage documentImage;
    std::function<std::vector<Row>()> rows;
    std::function<void()> onClose;
    std::function<void(const std::string&)> onText;
    std::function<void()> onBackspace;
};

struct Utterance {
    std::string text;
    bool interrupt;
};

enum class Navigation { Up, Down, Left, Right, Confirm, Back };
enum class Feedback { Cursor, Confirm, Back, Error };

// The same snapshot is used by the renderer, navigation and speech.
class Model {
  public:
    using PositionFormatter = std::function<std::string(const std::string&, size_t, size_t)>;
    using FeedbackPlayer = std::function<void(Feedback)>;
    explicit Model(PositionFormatter positionFormatter, FeedbackPlayer feedbackPlayer = {});
    void Navigate(Navigation action);
    void PlayFeedback(Feedback feedback);
    void Open(std::shared_ptr<Page> page);
    void Push(std::shared_ptr<Page> page);
    void Back(std::function<void()> apply = {});
    void Close();
    void Refresh();
    void Move(int delta);
    void Activate();
    void Adjust(int direction);
    void Focus(const std::string& id);
    bool IsEditingText() const;
    void InputText(const std::string& text);
    void Backspace();
    void Announce(const std::string& text, bool interrupt = true);
    bool IsOpen() const;
    size_t Depth() const;
    const Page* CurrentPage() const;
    const std::vector<Row>& Rows() const;
    size_t Selection() const;
    std::vector<Utterance> TakeSpeech();

  private:
    struct Frame {
        std::shared_ptr<Page> page;
        std::string selectedId;
        size_t selection = 0;
        bool firstItemPending = true;
        bool queueFirstItem = false;
        bool hintsPending = true;
    };
    std::vector<Frame> frames;
    std::vector<Row> rows;
    std::vector<Utterance> speech;
    PositionFormatter formatPosition;
    FeedbackPlayer playFeedback;
    void Introduce(bool opening);
    void SpeakItem(bool interrupt);
};

} // namespace NativeOptions
