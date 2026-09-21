#pragma once

#ifdef __cplusplus
extern "C" {
#endif
struct GameState;
struct GraphicsContext;
int NativeOptions_Update(struct GameState* gameState);
void NativeOptions_Draw(struct GraphicsContext* gfxCtx);
void NativeOptions_BeginGraphicsFrame(struct GraphicsContext* gfxCtx);
void NativeOptions_Open(void);
int NativeOptions_IsOpen(void);
int NativeOptions_AdvanceFrame(struct GameState* gameState);
void NativeOptions_DrawTitleMenu(struct GraphicsContext* gfxCtx, const char* startCaption, int selected, int alpha);
void NativeOptions_SpeakTitleItem(const char* startCaption, int selected, int opening);
void NativeOptions_DrawPauseTab(struct GraphicsContext* gfxCtx);
#ifdef __cplusplus
}

#include "OptionsModel.h"
#include <array>
#include <cstdint>
#include <map>

struct WidgetInfo;

namespace NativeOptions {
using PagePtr = std::shared_ptr<Page>;
using RowsProvider = std::function<std::vector<Row>()>;
using PageFactory = std::function<PagePtr()>;
using TextValidator = std::function<std::string(const std::string&)>;

Model& GetModel();
std::string Text(const std::string& key);
std::string OriginalOptionText(const std::string& key);
std::string OriginalItemText(int item);
PagePtr MakePage(std::string id, std::string title, RowsProvider rows, std::string description = "");
Row Link(std::string id, std::string label, PageFactory page, std::string description = "");
Row Action(std::string id, std::string label, std::function<void()> action, std::string description = "");
Row Toggle(std::string id, std::string label, bool value, std::function<void(bool)> set,
           std::string description = "");
Row Integer(std::string id, std::string label, int value, int minimum, int maximum, int step,
            std::function<void(int)> set, std::string description = "");
Row Decimal(std::string id, std::string label, float value, float minimum, float maximum, float step,
            std::function<void(float)> set, std::string description = "", bool percentage = false);
Row Choice(std::string id, std::string label, int value, std::map<int, std::string> choices,
           std::function<void(int)> set, std::string description = "");
Row String(std::string id, std::string label, std::string value, std::function<void(std::string)> set,
           std::string description = "", size_t maxLength = 1024, TextValidator validate = {}, bool numeric = false,
           bool multiline = false);
void Confirm(std::string title, std::string description, std::string accept, std::function<void()> action);
void Message(std::string title, std::string description);
void SaveSettings();
void ChangedCVar(const std::string& cvar);
Row CVarToggle(std::string label, std::string cvar, bool defaultValue = false, std::string description = "",
               std::function<void()> changed = {});
Row CVarInteger(std::string label, std::string cvar, int defaultValue, int minimum, int maximum, int step = 1,
                std::string description = "", std::function<void()> changed = {});
Row CVarDecimal(std::string label, std::string cvar, float defaultValue, float minimum, float maximum, float step,
                std::string description = "", bool percentage = false, std::function<void()> changed = {});
Row CVarChoice(std::string label, std::string cvar, int defaultValue, std::map<int, std::string> choices,
               std::string description = "", std::function<void()> changed = {});
Row CVarString(std::string label, std::string cvar, std::string defaultValue = "", std::string description = "");
Row CVarColor(std::string label, std::string cvar, std::array<uint8_t, 4> defaultValue, bool useAlpha = true,
              bool rainbow = false, bool lock = false);
void Disable(std::vector<Row>& rows, const std::string& reason);
void RegisterNetworkPages();
void UpdateNetworkWhilePaused();
void AppendWidget(std::vector<Row>& rows, WidgetInfo& widget, const std::string& id);
void RegisterPage(std::string name, PageFactory factory, std::string title = "");
PagePtr RegisteredPage(const std::string& name);
void RequestPage(std::string name);
PagePtr BuildRoot(const std::string& category = "");
void Init();
void CollectKeyboardInput();
void DrawDebuggerOverlay(float x, float y, float width, float height);
void ReadCurrentDescription();
PagePtr CosmeticPositionsPage();
PagePtr PauseTabPage();
void SpeakPauseTab();
Row OverlayLayout(std::string window, bool resize = false, float defaultWidth = 0, float defaultHeight = 0);
PagePtr OverlayLayoutPage(std::string window, bool resize = false, float defaultWidth = 0, float defaultHeight = 0);
void ApplyOverlayPreset(std::string name, float x, float y, float width, float height);
} // namespace NativeOptions
#endif
