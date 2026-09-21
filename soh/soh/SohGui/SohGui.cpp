//
//  SohGui.cpp
//  soh
//
//  Created by David Chavez on 24.08.22.
//

#include "SohGui.hpp"
#include "soh/NativeOptions/NativeOptions.h"

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <libultraship/libultraship.h>

#ifdef __APPLE__
#include <fast/backends/gfx_metal.h>
#endif

#ifdef __SWITCH__
#include <port/switch/SwitchImpl.h>
#endif
#include "include/global.h"
#include "include/z64audio.h"
#include "soh/SaveManager.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/Presets/Presets.h"
#include "soh/resource/type/Skeleton.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/cosmetics/authenticGfxPatches.h"
#include "soh/Enhancements/debugger/MessageViewer.h"
#include "soh/Notification/Notification.h"
#include "soh/Enhancements/TimeDisplay/TimeDisplay.h"
#include "soh/Enhancements/mod_menu.h"
#include "soh/Network/Anchor/Anchor.h"

namespace SohGui {

// MARK: - Properties
static const char* bunnyHoodOptions[3] = { "Disabled", "Faster Run & Longer Jump", "Faster Run" };

static const inline std::vector<std::pair<const char*, const char*>> audioBackends = {
#ifdef _WIN32
    { "wasapi", "Windows Audio Session API" },
#endif
#if defined(__linux)
    { "pulse", "PulseAudio" },
#endif
#ifdef __APPLE__
    { "coreaudio", "Core Audio" },
#endif
    { "sdl", "SDL Audio" }
};

// MARK: - Helpers

std::string GetWindowButtonText(const char* text, bool menuOpen) {
    char buttonText[100] = "";
    if (menuOpen) {
        strcat(buttonText, ICON_FA_CHEVRON_RIGHT " ");
    }
    strcat(buttonText, text);
    if (!menuOpen) {
        strcat(buttonText, "  ");
    }
    return buttonText;
}

// MARK: - Delegates


std::shared_ptr<SohMenu> mSohMenu;
std::shared_ptr<InputViewer> mInputViewer;
std::shared_ptr<CheckTracker::CheckTrackerWindow> mCheckTrackerWindow;
std::shared_ptr<EntranceTracker::EntranceTrackerWindow> mEntranceTrackerWindow;
std::shared_ptr<ItemTrackerWindow> mItemTrackerWindow;
std::shared_ptr<TimeSplitWindow> mTimeSplitWindow;
std::shared_ptr<Notification::Window> mNotificationWindow;
std::shared_ptr<TimeDisplayWindow> mTimeDisplayWindow;
std::shared_ptr<AnchorRoomWindow> mAnchorRoomWindow;

UIWidgets::Colors GetMenuThemeColor() {
    return mSohMenu->GetMenuThemeColor();
}

std::shared_ptr<SohMenu> GetSohMenu() {
    return mSohMenu;
}

void SetupMenu() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    // Opening a native screen is transient, including after a crash or exit.
    CVarClear(CVAR_WINDOW("Menu"));
    gui->SaveConsoleVariablesNextFrame();
    mSohMenu = std::make_shared<SohMenu>("", "Port Menu");
    gui->SetMenu(mSohMenu);

}

void SetupMenuElements() {
    mSohMenu->AddMenuElements();
    NativeOptions::Init();
}

void SetupGuiElements() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();

    InitializeConsolePages();

    InitializeGraphicsDebugger();

    InitializePerformanceStats();

    InitializeMods();
    InitializeAudioEditor();
    mInputViewer = std::make_shared<InputViewer>(CVAR_WINDOW("InputViewer"), "Input Viewer");
    gui->AddGuiWindow(mInputViewer);
    InitializeCosmeticsEditor();
    InitializeActorViewer();
    InitializeCollisionViewer();
    InitializeSaveEditor();
    InitializeHookDebugger();
    InitializeDisplayListViewer();
    InitializeValueViewer();
    InitializeMessageViewer();
    InitializeGameplayStats();
    mCheckTrackerWindow = std::make_shared<CheckTracker::CheckTrackerWindow>(CVAR_WINDOW("CheckTracker"),
                                                                             "Check Tracker", ImVec2(400, 540));
    gui->AddGuiWindow(mCheckTrackerWindow);
    mEntranceTrackerWindow = std::make_shared<EntranceTracker::EntranceTrackerWindow>(
        CVAR_WINDOW("EntranceTracker"), "Entrance Tracker", ImVec2(500, 750));
    gui->AddGuiWindow(mEntranceTrackerWindow);
    mItemTrackerWindow =
        std::make_shared<ItemTrackerWindow>(CVAR_WINDOW("ItemTracker"), "Item Tracker", ImVec2(350, 600));
    gui->AddGuiWindow(mItemTrackerWindow);
    mTimeSplitWindow = std::make_shared<TimeSplitWindow>(CVAR_WINDOW("TimeSplits"), "Time Splits", ImVec2(450, 660));
    gui->AddGuiWindow(mTimeSplitWindow);
    RegisterPlandomizerPage();
    mNotificationWindow = std::make_shared<Notification::Window>(CVAR_WINDOW("Notifications"), "Notifications Window");
    gui->AddGuiWindow(mNotificationWindow);
    mNotificationWindow->Show();
    mTimeDisplayWindow = std::make_shared<TimeDisplayWindow>(CVAR_WINDOW("TimeDisplayEnabled"), "Additional Timers");
    gui->AddGuiWindow(mTimeDisplayWindow);
    mAnchorRoomWindow = std::make_shared<AnchorRoomWindow>(CVAR_WINDOW("AnchorRoom"), "Anchor Room");
    gui->AddGuiWindow(mAnchorRoomWindow);
}

void Destroy() {
    auto gui = Ship::Context::GetInstance()->GetWindow()->GetGui();
    gui->RemoveAllGuiWindows();

    mNotificationWindow = nullptr;
    ClearNativePopups();
    mItemTrackerWindow = nullptr;
    mEntranceTrackerWindow = nullptr;
    mCheckTrackerWindow = nullptr;
    mInputViewer = nullptr;
    mTimeSplitWindow = nullptr;
    mTimeDisplayWindow = nullptr;
    mAnchorRoomWindow = nullptr;
}

void ShowRandomizerSettingsMenu() {
    NativeOptions::RequestPage("Randomizer Settings");
}

void ShowEscMenu() {
    mSohMenu->Show();
}
} // namespace SohGui
