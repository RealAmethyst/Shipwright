#include "SohModals.h"
#include "SohGui.hpp"
#include "soh/NativeOptions/NativeOptions.h"
#include <deque>
#include <mutex>

namespace SohGui {
namespace {
struct Modal {
    std::string title, message, button1, button2;
    std::function<void()> callback1, callback2;
    bool active = false;
    bool dismissed = false;
};
std::deque<std::shared_ptr<Modal>> modals;
std::mutex modalMutex;

void Remove(const std::shared_ptr<Modal>& modal) {
    std::lock_guard lock(modalMutex);
    auto it = std::find(modals.begin(), modals.end(), modal);
    if (it != modals.end()) modals.erase(it);
}
}

void RegisterPopup(std::string title, std::string message, std::string button1, std::string button2,
                   std::function<void()> callback1, std::function<void()> callback2) {
    std::lock_guard lock(modalMutex);
    modals.push_back(std::make_shared<Modal>(Modal{std::move(title), std::move(message),
        std::move(button1), std::move(button2), std::move(callback1), std::move(callback2)}));
}

size_t PopupsQueued() {
    std::lock_guard lock(modalMutex);
    return modals.size();
}

bool DismissPopup(std::string title) {
    std::lock_guard lock(modalMutex);
    if (modals.empty() || modals.front()->title != title) return false;
    modals.front()->dismissed = true;
    return true;
}

bool UpdateNativePopups() {
    namespace N = NativeOptions;
    std::shared_ptr<Modal> modal;
    bool dismissed;
    {
        std::lock_guard lock(modalMutex);
        if (modals.empty()) return false;
        modal = modals.front();
        dismissed = modal->dismissed;
    }
    const auto id = "port_popup/" + modal->title;
    if (dismissed) {
        if (modal->active && N::GetModel().CurrentPage() && N::GetModel().CurrentPage()->id == id)
            N::GetModel().Back();
        else if (!modal->active) Remove(modal);
        return false;
    }
    if (modal->active) return false;
    modal->active = true;
    auto page = N::MakePage(id, modal->title, [modal] {
        std::vector<N::Row> rows{N::Action("first", modal->button1, [modal] {
            N::GetModel().Back(modal->callback1);
        })};
        if (!modal->button2.empty())
            rows.push_back(N::Action("second", modal->button2, [modal] {
                N::GetModel().Back(modal->callback2);
            }));
        return rows;
    }, modal->message);
    page->popup = true;
    page->onClose = [modal] { Remove(modal); };
    NativeOptions_Open();
    if (N::GetModel().IsOpen()) N::GetModel().Push(page);
    else N::GetModel().Open(page);
    return true;
}

void DrawSetupPopups() {
    std::shared_ptr<Modal> modal;
    bool dismissed;
    {
        std::lock_guard lock(modalMutex);
        if (modals.empty()) return;
        modal = modals.front();
        dismissed = modal->dismissed;
    }
    if (dismissed) {
        Remove(modal);
        return;
    }
    if (!ImGui::IsPopupOpen(modal->title.c_str())) ImGui::OpenPopup(modal->title.c_str());
    if (!ImGui::BeginPopupModal(modal->title.c_str(), nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) return;
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40);
    ImGui::TextUnformatted(modal->message.c_str());
    ImGui::PopTextWrapPos();
    std::function<void()> callback;
    bool selected = false;
    if (ImGui::Button(modal->button1.c_str())) {
        selected = true;
        callback = modal->callback1;
    }
    if (!modal->button2.empty()) {
        ImGui::SameLine();
        if (ImGui::Button(modal->button2.c_str())) {
            selected = true;
            callback = modal->callback2;
        }
    }
    if (selected) ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
    if (selected) {
        Remove(modal);
        if (callback) callback();
    }
}

void ClearNativePopups() {
    std::lock_guard lock(modalMutex);
    modals.clear();
}
}
