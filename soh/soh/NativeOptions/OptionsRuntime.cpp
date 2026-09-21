#include "NativeOptions.h"

#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/Enhancements/speechsynthesizer/SpeechSynthesizer.h"
#include "soh/Enhancements/tts/SpeechText.h"
#include "soh/Enhancements/tts/tts.h"
#include "soh/SaveManager.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "OptionsControllers.h"
#include <ship/resource/type/Json.h>
#include <nlohmann/json.hpp>
#include <SDL.h>
#include <deque>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "global.h"
}

namespace NativeOptions {
static std::map<std::string, nlohmann::json> textBanks;
static bool initialized = false;
static int lastDirection = 0;
static uint32_t nextRepeat = 0;
static bool awaitRelease = false;
static bool suppressGameInput = false;
static bool keyboardConfirmHeld = false;
static std::deque<std::function<void()>> keyboardInput;
static std::string requestedPage;

void RequestPage(std::string name) {
    requestedPage = std::move(name);
    NativeOptions_Open();
}

static std::string ResourceText(const std::string& bank, const std::string& key) {
    auto& strings = textBanks[bank];
    if (!strings.is_object()) {
        auto data = std::make_shared<Ship::ResourceInitData>();
        data->Format = RESOURCE_FORMAT_BINARY;
        data->Type = static_cast<uint32_t>(Ship::ResourceType::Json);
        data->ResourceVersion = 0;
        auto resource = std::dynamic_pointer_cast<Ship::Json>(Ship::Context::GetInstance()->GetResourceManager()->LoadResource(
            "accessibility/texts/" + bank + ".json", true, data));
        strings = resource && resource->Data.is_object() ? resource->Data : nlohmann::json::object();
    }
    auto it = strings.find(key);
    return it != strings.end() && it->is_string() ? it->get<std::string>() : "";
}

std::string Text(const std::string& key) {
    return ResourceText("options_eng", key);
}

std::string OriginalOptionText(const std::string& key) {
    const char* suffix = gSaveContext.language == LANGUAGE_GER ? "ger" :
                         gSaveContext.language == LANGUAGE_FRA ? "fra" : "eng";
    return ResourceText(std::string("filechoose_") + suffix, key);
}

std::string OriginalItemText(int item) {
    const char* suffix = gSaveContext.language == LANGUAGE_GER ? "ger" :
                         gSaveContext.language == LANGUAGE_FRA ? "fra" : "eng";
    auto text = ResourceText(std::string("kaleidoscope_") + suffix, std::to_string(item));
    if (text.starts_with("$0 ")) text.erase(0, 3);
    return text;
}

Model& GetModel() {
    static Model model([](const std::string& text, size_t position, size_t count) {
        return SpeechText::WithPosition(text, Text("position"), static_cast<int>(position), static_cast<int>(count));
    }, [](Feedback feedback) {
        u16 sound;
        switch (feedback) {
            case Feedback::Cursor: sound = NA_SE_SY_FSEL_CURSOR; break;
            case Feedback::Confirm: sound = NA_SE_SY_FSEL_DECIDE_L; break;
            case Feedback::Back: sound = NA_SE_SY_FSEL_CLOSE; break;
            case Feedback::Error: sound = NA_SE_SY_FSEL_ERROR; break;
        }
        Audio_PlaySoundGeneral(sound, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale,
                               &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    });
    return model;
}

static void OpenDetails() {
    const auto depth = GetModel().Depth();
    ReadCurrentDescription();
    if (GetModel().Depth() > depth) GetModel().PlayFeedback(Feedback::Confirm);
}

void Init() {
    if (initialized)
        return;
    RegisterNetworkPages();
    RegisterPage("Randomizer Settings", [] { return BuildRoot("randomizer"); }, Text("randomizer"));
    initialized = true;
}

void CollectKeyboardInput() {
    keyboardConfirmHeld = false;
    if (!initialized || !SohGui::GetSohMenu()->IsVisible()) {
        keyboardInput.clear();
        return;
    }
    if (ControllerCaptureActive()) {
        keyboardInput.clear();
        return;
    }
    for (const auto& [key, direction] : {
             std::pair{ImGuiKey_UpArrow, -1}, {ImGuiKey_DownArrow, 1},
             {ImGuiKey_LeftArrow, -2}, {ImGuiKey_RightArrow, 2}}) {
        if (ImGui::IsKeyPressed(key))
            keyboardInput.push_back([direction] {
                GetModel().Navigate(direction == -1 ? Navigation::Up : direction == 1 ? Navigation::Down :
                                    direction == -2 ? Navigation::Left : Navigation::Right);
            });
    }
    keyboardConfirmHeld = ImGui::IsKeyDown(ImGuiKey_Enter);
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false))
        keyboardInput.push_back([] { GetModel().Navigate(Navigation::Confirm); });
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false))
        keyboardInput.push_back([] { OpenDetails(); });
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
        keyboardInput.push_back([] {
            if (GetModel().IsEditingText()) GetModel().Backspace();
            else GetModel().Navigate(Navigation::Back);
        });
    if (GetModel().IsEditingText()) {
        const auto& io = ImGui::GetIO();
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V, false)) {
            const char* clipboard = ImGui::GetClipboardText();
            const std::string text = clipboard ? clipboard : "";
            keyboardInput.push_back([text] { GetModel().InputText(text); });
        } else if (!io.KeyCtrl && !io.KeyAlt) {
            for (auto character : io.InputQueueCharacters) {
                char buffer[5]{};
                ImTextCharToUtf8(buffer, character);
                const std::string text = buffer;
                keyboardInput.push_back([text] { GetModel().InputText(text); });
            }
        }
    }
}

static bool Neutral(const Input& input) {
    return input.cur.button == 0 && std::abs(input.cur.stick_x) < 20 && std::abs(input.cur.stick_y) < 20;
}

static void ClearInput(GameState* gameState) {
    for (auto& input : gameState->input)
        std::memset(&input, 0, sizeof(input));
}

static void SpeakPending() {
    auto pending = GetModel().TakeSpeech();
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || !SpeechSynthesizer::Instance)
        return;
    for (const auto& utterance : pending)
        SpeechSynthesizer::Instance->Speak(utterance.text.c_str(), "en-US", utterance.interrupt);
}

PagePtr PauseTabPage() {
    auto page = MakePage("pause/options", Text("title"), [] {
        auto row = Action("open", Text("pause_open_options"), [] { NativeOptions_Open(); });
        row.description = Text("pause_options_help");
        return std::vector<Row>{std::move(row)};
    });
    page->hints = Text(CVarGetInteger(CVAR_ENHANCEMENT("NGCKaleidoSwitcher"), 0) ? "pause_options_l_hints" : "pause_options_z_hints");
    page->footer = page->hints;
    return page;
}

void SpeakPauseTab() {
    Model model([](const std::string& text, size_t position, size_t count) {
        return SpeechText::WithPosition(text, Text("position"), static_cast<int>(position), static_cast<int>(count));
    });
    model.Open(PauseTabPage());
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || !SpeechSynthesizer::Instance) return;
    for (const auto& utterance : model.TakeSpeech())
        SpeechSynthesizer::Instance->Speak(utterance.text.c_str(), "en-US", utterance.interrupt);
}

} // namespace NativeOptions

extern "C" int NativeOptions_IsOpen() {
    return NativeOptions::GetModel().IsOpen();
}

extern "C" int NativeOptions_AdvanceFrame(GameState* gameState) {
    if (!gPlayState || gameState != &gPlayState->state || !gPlayState->frameAdvCtx.enabled ||
        !CVarGetInteger(CVAR_DEVELOPER_TOOLS("FrameAdvanceTick"), 0))
        return false;
    NativeOptions::ClearInput(gameState);
    return true;
}

extern "C" void NativeOptions_Open() {
    auto menu = SohGui::GetSohMenu();
    if (menu)
        menu->Show();
}

extern "C" void NativeOptions_SpeakTitleItem(const char* startCaption, int selected, int opening) {
    using namespace NativeOptions;
    if (!CVarGetInteger(CVAR_SETTING("A11yTTS"), 1) || !SpeechSynthesizer::Instance)
        return;
    const auto text = SpeechText::WithPosition(selected ? NativeOptions::Text("title") : startCaption,
                                               NativeOptions::Text("position"), selected + 1, 2);
    SpeechSynthesizer::Instance->Speak(text.c_str(), "en-US", !opening);
    if (opening)
        SpeechSynthesizer::Instance->Speak(NativeOptions::Text("title_navigation").c_str(), "en-US", false);
}

extern "C" int NativeOptions_Update(GameState* gameState) {
    using namespace NativeOptions;
    if (!initialized)
        return false;
    JoinRandoGenerationThread();
    UpdateControllerPreview();
    auto menu = SohGui::GetSohMenu();
    auto& model = GetModel();
    if (!ControllerCaptureActive() && SohGui::UpdateNativePopups()) {
        awaitRelease = true;
        keyboardInput.clear();
    }
    const bool wasOpen = model.IsOpen();
    if (!menu->IsVisible()) {
        if (wasOpen) {
            model.PlayFeedback(Feedback::Back);
            model.Close();
            TTSResumePauseMenu();
            TTSResumeFileSelect();
            CVarSave();
            suppressGameInput = true;
            keyboardInput.clear();
        }
        if (suppressGameInput) {
            const bool released = Neutral(gameState->input[0]);
            ClearInput(gameState);
            suppressGameInput = !released;
        }
        return false;
    }
    UpdateNetworkWhilePaused();
    menu->RefreshDisabledState();
    if (!wasOpen) {
        SaveManager::Instance->EnsureGlobalLoaded();
        model.Open(BuildRoot());
        model.PlayFeedback(Feedback::Confirm);
        awaitRelease = true;
        lastDirection = 0;
        nextRepeat = SDL_GetTicks() + 300;
    } else if (ControllerCaptureActive()) {
        UpdateControllerCapture();
        awaitRelease = true;
        keyboardInput.clear();
        keyboardConfirmHeld = false;
        lastDirection = 0;
    } else {
        model.Refresh();
        while (!keyboardInput.empty() && model.IsOpen() && !ControllerCaptureActive()) {
            auto action = std::move(keyboardInput.front());
            keyboardInput.pop_front();
            action();
        }
        const auto& input = gameState->input[0];
        if (ControllerCaptureActive())
            awaitRelease = true;
        else if (awaitRelease && Neutral(input))
            awaitRelease = false;
        int direction = 0;
        if (awaitRelease) direction = 0;
        else if ((input.cur.button & BTN_DUP) || input.cur.stick_y > 40) direction = -1;
        else if ((input.cur.button & BTN_DDOWN) || input.cur.stick_y < -40) direction = 1;
        else if ((input.cur.button & BTN_DLEFT) || input.cur.stick_x < -40) direction = -2;
        else if ((input.cur.button & BTN_DRIGHT) || input.cur.stick_x > 40) direction = 2;
        if (direction && (direction != lastDirection || static_cast<int32_t>(SDL_GetTicks() - nextRepeat) >= 0)) {
            model.Navigate(direction == -1 ? Navigation::Up : direction == 1 ? Navigation::Down :
                           direction == -2 ? Navigation::Left : Navigation::Right);
            nextRepeat = SDL_GetTicks() + (direction != lastDirection ? 350 : 120);
        }
        lastDirection = direction;
        if (!awaitRelease && (input.press.button & BTN_B))
            model.Navigate(Navigation::Back);
        else if (!awaitRelease && (input.press.button & BTN_A))
            model.Navigate(Navigation::Confirm);
        else if (!awaitRelease && (input.press.button & BTN_R))
            OpenDetails();
        if (!awaitRelease && ((input.cur.button & BTN_A) || keyboardConfirmHeld) && !model.Rows().empty()) {
            const auto row = model.Rows()[model.Selection()];
            if (row.enabled && row.held) row.held();
        }
    }
    if (!requestedPage.empty() && !ControllerCaptureActive()) {
        auto name = std::move(requestedPage);
        requestedPage.clear();
        if (auto page = RegisteredPage(name)) model.Push(std::move(page));
        awaitRelease = true;
        keyboardInput.clear();
    }
    SpeakPending();
    if (!model.IsOpen()) {
        TTSResumePauseMenu();
        TTSResumeFileSelect();
        menu->Hide();
        CVarSave();
        suppressGameInput = true;
        ClearInput(gameState);
        keyboardInput.clear();
        return false;
    }
    return true;
}
