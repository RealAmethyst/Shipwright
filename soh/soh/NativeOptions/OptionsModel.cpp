#include "OptionsModel.h"
#include "OptionsFormatting.h"

#include <algorithm>
#include <utility>

namespace NativeOptions {

Model::Model(PositionFormatter positionFormatter, FeedbackPlayer feedbackPlayer)
    : formatPosition(std::move(positionFormatter)), playFeedback(std::move(feedbackPlayer)) {
}

void Model::PlayFeedback(Feedback feedback) {
    if (playFeedback) playFeedback(feedback);
}

void Model::Navigate(Navigation action) {
    if (!IsOpen()) return;
    if (action == Navigation::Back) {
        PlayFeedback(Feedback::Back);
        Back();
        return;
    }
    if (rows.empty()) return;
    const auto row = rows[Selection()];
    switch (action) {
        case Navigation::Up:
        case Navigation::Down:
            if (rows.size() > 1) PlayFeedback(Feedback::Cursor);
            Move(action == Navigation::Up ? -1 : 1);
            break;
        case Navigation::Left:
        case Navigation::Right:
            if (!row.enabled) PlayFeedback(Feedback::Error);
            else if (row.adjust) PlayFeedback(Feedback::Cursor);
            Adjust(action == Navigation::Left ? -1 : 1);
            break;
        case Navigation::Confirm:
            if (!row.enabled) PlayFeedback(Feedback::Error);
            else if (row.activate || row.adjust) PlayFeedback(Feedback::Confirm);
            Activate();
            break;
        case Navigation::Back:
            break;
    }
}

void Model::Open(std::shared_ptr<Page> page) {
    Close();
    speech.clear();
    Push(std::move(page));
}

void Model::Push(std::shared_ptr<Page> page) {
    if (!page || !page->rows)
        return;
    page->title = PrepareOptionsText(page->title);
    page->description = PrepareOptionsText(page->description);
    page->hints = PrepareOptionsText(page->hints);
    const auto initialFocus = page->initialFocus;
    frames.push_back({ std::move(page), initialFocus });
    rows.clear();
    Introduce(true);
    Refresh();
}

void Model::Introduce(bool opening) {
    auto& frame = frames.back();
    frame.firstItemPending = true;
    frame.queueFirstItem = false;
    frame.hintsPending = opening;
    // Returning to a child page announces its section; the root title belongs to the opening only.
    if ((opening || frames.size() > 1) && !frame.page->title.empty()) {
        Announce(frame.page->title);
        frame.queueFirstItem = true;
    }
    if (opening && !frame.page->description.empty()) {
        Announce(frame.page->description, !frame.queueFirstItem);
        frame.queueFirstItem = true;
    }
}

void Model::Back(std::function<void()> apply) {
    if (frames.empty())
        return;
    auto onClose = frames.back().page->onClose;
    frames.pop_back();
    rows.clear();
    if (onClose)
        onClose();
    auto parent = frames.empty() ? nullptr : frames.back().page;
    if (apply)
        apply();
    if (!frames.empty() && frames.back().page == parent) {
        Introduce(false);
        Refresh();
    }
}

void Model::Close() {
    while (!frames.empty()) {
        auto onClose = frames.back().page->onClose;
        frames.pop_back();
        if (onClose)
            onClose();
    }
    rows.clear();
    speech.clear();
}

void Model::Refresh() {
    if (frames.empty())
        return;
    auto page = frames.back().page;
    auto next = page->rows();
    for (auto& row : next) {
        row.label = PrepareOptionsText(row.label);
        row.value = PrepareOptionsText(row.value);
        row.description = PrepareOptionsText(row.description);
        row.disabledReason = PrepareOptionsText(row.disabledReason);
    }
    // Providers may schedule a child screen. Do not install the parent's snapshot into it.
    if (frames.empty() || frames.back().page != page)
        return;
    auto& frame = frames.back();
    const auto oldId = frame.selectedId;
    rows = std::move(next);
    if (rows.empty()) {
        frame.selection = 0;
        return;
    }
    auto selected = std::find_if(rows.begin(), rows.end(), [&](const Row& row) { return row.id == oldId; });
    frame.selection = selected != rows.end() ? static_cast<size_t>(selected - rows.begin())
                                             : std::min(frame.selection, rows.size() - 1);
    frame.selectedId = rows[frame.selection].id;
    if (frame.firstItemPending) {
        SpeakItem(!frame.queueFirstItem);
        frame.firstItemPending = false;
        frame.queueFirstItem = false;
        if (frame.hintsPending && !frame.page->hints.empty())
            Announce(frame.page->hints, false);
        frame.hintsPending = false;
    } else if (!oldId.empty() && oldId != frame.selectedId) {
        SpeakItem(true);
    }
}

void Model::SpeakItem(bool interrupt) {
    if (frames.empty() || rows.empty())
        return;
    const auto& frame = frames.back();
    const auto& row = rows[frame.selection];
    if (row.label.empty())
        return;
    std::string text = row.label;
    for (const auto* field : { &row.value, &row.description, &row.disabledReason }) {
        if (!field->empty())
            text += ", " + *field;
    }
    if (!(frame.page->popup && rows.size() == 1))
        text = formatPosition(text, frame.selection + 1, rows.size());
    Announce(text, interrupt);
}

void Model::Move(int delta) {
    if (frames.empty() || rows.empty() || delta == 0)
        return;
    auto& frame = frames.back();
    const auto count = static_cast<int>(rows.size());
    frame.selection = static_cast<size_t>((static_cast<int>(frame.selection) + delta % count + count) % count);
    frame.selectedId = rows[frame.selection].id;
    SpeakItem(true);
}

void Model::Focus(const std::string& id) {
    if (frames.empty())
        return;
    auto it = std::find_if(rows.begin(), rows.end(), [&](const Row& row) { return row.id == id; });
    if (it != rows.end()) {
        frames.back().selection = static_cast<size_t>(it - rows.begin());
        frames.back().selectedId = id;
    }
}

void Model::Activate() {
    if (frames.empty() || rows.empty())
        return;
    const auto row = rows[frames.back().selection];
    if (!row.enabled) {
        SpeakItem(true);
        return;
    }
    auto page = frames.back().page;
    const auto pending = speech.size();
    if (row.activate)
        row.activate();
    else if (row.adjust)
        row.adjust(1);
    Refresh();
    if (!frames.empty() && frames.back().page == page && speech.size() == pending)
        SpeakItem(true);
}

void Model::Adjust(int direction) {
    if (frames.empty() || rows.empty() || direction == 0)
        return;
    const auto row = rows[frames.back().selection];
    if (row.enabled && row.adjust)
        row.adjust(direction < 0 ? -1 : 1);
    Refresh();
    SpeakItem(true);
}

void Model::Announce(const std::string& text, bool interrupt) {
    if (!text.empty())
        speech.push_back({ PrepareOptionsText(text), interrupt });
}

bool Model::IsEditingText() const {
    return std::any_of(frames.begin(), frames.end(), [](const Frame& frame) { return !!frame.page->onText; });
}

void Model::InputText(const std::string& text) {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->page->onText) {
            auto accept = it->page->onText;
            accept(text);
            Refresh();
            return;
        }
    }
}

void Model::Backspace() {
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
        if (it->page->onBackspace) {
            auto erase = it->page->onBackspace;
            erase();
            Refresh();
            return;
        }
    }
}

bool Model::IsOpen() const {
    return !frames.empty();
}

size_t Model::Depth() const {
    return frames.size();
}

const Page* Model::CurrentPage() const {
    return frames.empty() ? nullptr : frames.back().page.get();
}

const std::vector<Row>& Model::Rows() const {
    return rows;
}

size_t Model::Selection() const {
    return frames.empty() ? 0 : frames.back().selection;
}

std::vector<Utterance> Model::TakeSpeech() {
    auto pending = std::move(speech);
    speech.clear();
    return pending;
}

} // namespace NativeOptions
