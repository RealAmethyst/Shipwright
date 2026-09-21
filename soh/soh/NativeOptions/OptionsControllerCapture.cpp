#include "OptionsControllers.h"
#include "OptionsCaptureGate.h"
#include <ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h>
#include <SDL.h>
#include <cmath>

namespace NativeOptions {
namespace {
struct Capture {
    CaptureGate gate{SDL_GetTicks()};
    std::function<bool()> poll;
    bool cancel = false;
};
std::shared_ptr<Capture> capture;

void ClearRawCapture() {
    auto deck = Ship::Context::GetInstance()->GetControlDeck();
    for (uint8_t port = 0; port < 4; ++port)
        deck->GetControllerByPort(port)->CancelMappingCapture();
}

bool RawInputsReleased() {
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_GamepadStart; ++key)
        if (ImGui::IsKeyDown(static_cast<ImGuiKey>(key))) return false;
    for (bool down : ImGui::GetIO().MouseDown)
        if (down) return false;
    const auto wheel = Ship::WheelHandler::GetInstance()->GetDirections();
    if (wheel.x != Ship::LUS_WHEEL_NONE || wheel.y != Ship::LUS_WHEEL_NONE) return false;
    const auto devices = Ship::Context::GetInstance()->GetControlDeck()->GetConnectedPhysicalDeviceManager();
    for (const auto& [instance, name] : devices->GetConnectedSDLGamepadNames()) {
        auto gamepad = SDL_GameControllerFromInstanceID(instance);
        if (!gamepad) continue;
        for (int button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
            if (SDL_GameControllerGetButton(gamepad, static_cast<SDL_GameControllerButton>(button))) return false;
        for (int axis = SDL_CONTROLLER_AXIS_LEFTX; axis < SDL_CONTROLLER_AXIS_MAX; ++axis)
            if (std::abs(static_cast<int>(SDL_GameControllerGetAxis(gamepad, static_cast<SDL_GameControllerAxis>(axis)))) > 10000)
                return false;
    }
    return true;
}
}

void BeginControllerCapture(std::function<bool()> poll) {
    ClearRawCapture();
    capture = std::make_shared<Capture>();
    capture->poll = std::move(poll);
    auto page = MakePage("controller/capture", Text("bind_capture"), [] {
        return std::vector<Row>{Action("waiting", Text("bind_waiting"), {})};
    }, Text("bind_capture_help"));
    page->popup = true;
    page->hints = Text("bind_capture_hint");
    page->footer = Text("bind_capture_hint");
    page->onClose = [] { capture.reset(); ClearRawCapture(); };
    GetModel().Push(std::move(page));
}

bool ControllerCaptureActive() {
    return capture != nullptr;
}

bool CancelControllerCapture() {
    if (!capture) return false;
    capture->cancel = true;
    return true;
}

void UpdateControllerCapture() {
    const auto active = capture;
    if (!active) return;
    const auto step = active->gate.Update(SDL_GetTicks(), RawInputsReleased(), active->cancel);
    if (step == CaptureGate::Step::Wait || step == CaptureGate::Step::Arm) ClearRawCapture();
    if (step == CaptureGate::Step::Cancel ||
        ((step == CaptureGate::Step::Arm || step == CaptureGate::Step::Poll) && active->poll())) {
        GetModel().PlayFeedback(step == CaptureGate::Step::Cancel ? Feedback::Back : Feedback::Confirm);
        GetModel().Back();
        GetModel().Announce(Text(step == CaptureGate::Step::Cancel ? "bind_cancelled" : "bind_assigned"), false);
    }
}
}
