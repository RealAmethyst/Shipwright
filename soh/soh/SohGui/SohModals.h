#pragma once
#include <cstddef>
#include <functional>
#include <string>
namespace SohGui {
void RegisterPopup(std::string title, std::string message, std::string button1 = "OK", std::string button2 = "",
                   std::function<void()> button1callback = nullptr, std::function<void()> button2callback = nullptr);
size_t PopupsQueued();
bool DismissPopup(std::string title);
// Called from the game-state update, before native Options input is processed.
bool UpdateNativePopups();
// ROM setup cannot use game fonts or panels before extraction finishes.
void DrawSetupPopups();
void ClearNativePopups();
}
