#pragma once
#include "NativeOptions.h"
#include <libultraship/libultraship.h>

namespace NativeOptions {
using CaptureRequest = std::function<void(std::function<bool()>)>;
struct ControllerWidgets {
    WidgetInfo* freeLook;
    WidgetInfo* mouse;
    WidgetInfo* autoCapture;
    WidgetInfo* rightStickOcarina;
    WidgetInfo* dpadOcarina;
    WidgetInfo* dpadPause;
    WidgetInfo* dpadText;
};
PagePtr ControllerBindingsPage(CaptureRequest capture, ControllerWidgets widgets);
PagePtr ControllerRumblePage(uint8_t port, CaptureRequest capture);
PagePtr ControllerLEDPage(uint8_t port, CaptureRequest capture);
PagePtr ControllerGyroPage(uint8_t port, CaptureRequest capture);
PagePtr ControllerStickResponsePage(std::shared_ptr<Ship::ControllerStick> stick);
PagePtr ControllerDevicesPage(uint8_t port);
PagePtr ControllerCameraPage(WidgetInfo& mouse, WidgetInfo& autoCapture, WidgetInfo& freeLook);
PagePtr ControllerDpadPage(WidgetInfo& pause, WidgetInfo& text);
void UpdateControllerPreview();
bool TestingControllerRumble();
void BeginControllerCapture(std::function<bool()> poll);
bool ControllerCaptureActive();
bool CancelControllerCapture();
void UpdateControllerCapture();
}
