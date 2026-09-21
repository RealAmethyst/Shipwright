#include "valueViewer.h"
#include "soh/NativeOptions/NativeOptions.h"
#include "soh/SohGui/SohGui.hpp"
#include "soh/OTRGlobals.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include <z64.h>
#include "variables.h"
#include "functions.h"
#include "macros.h"
#include "soh/cvar_prefixes.h"
extern PlayState* gPlayState;
void GfxPrint_SetColor(GfxPrint* printer, u32 r, u32 g, u32 b, u32 a);
void GfxPrint_SetPos(GfxPrint* printer, s32 x, s32 y);
s32 GfxPrint_Printf(GfxPrint* printer, const char* fmt, ...);
}

#define CVAR_NAME CVAR_DEVELOPER_TOOLS("ValueViewerEnablePrinting")
#define CVAR_DEFAULT 0
#define CVAR_VALUE CVarGetInteger(CVAR_NAME, CVAR_DEFAULT)

ImVec4 WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

// clang-format off
std::vector<ValueTableElement> valueTable = {
    { "Time",               "gSaveContext.dayTime",                 "TIME:",   TYPE_U16,   false, []() -> void* { return &gSaveContext.dayTime; },                      WHITE },
    { "Age",                "gSaveContext.linkAge",                 "AGE:",    TYPE_S32,   false, []() -> void* { return &gSaveContext.linkAge; },                      WHITE },
    { "Health",             "gSaveContext.health",                  "HP:",     TYPE_S16,   false, []() -> void* { return &gSaveContext.health; },                       WHITE },
    { "Navi Timer",         "gSaveContext.naviTimer",               "NAVI:",   TYPE_U16,   false, []() -> void* { return &gSaveContext.naviTimer; },                    WHITE },
    { "Scene ID",           "play->sceneNum",                       "SCENE:",  TYPE_S16,   true,  []() -> void* { return &gPlayState->sceneNum; },                      WHITE },
    { "Room ID",            "play->roomCtx.curRoom.num",            "ROOM:",   TYPE_S8,    true,  []() -> void* { return &gPlayState->roomCtx.curRoom.num; },           WHITE },
    { "Entrance ID",        "gSaveContext.entranceIndex",           "ENTR:",   TYPE_S32,   false, []() -> void* { return &gSaveContext.entranceIndex; },                WHITE },
    { "Cutscene ID",        "gSaveContext.cutsceneIndex",           "CUTS:",   TYPE_S32,   false, []() -> void* { return &gSaveContext.cutsceneIndex; },                WHITE },
    { "Link X",             "Player->actor.world.pos.x",            "X:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.x; }, WHITE },
    { "Link Y",             "Player->actor.world.pos.y",            "Y:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.y; }, WHITE },
    { "Link Z",             "Player->actor.world.pos.z",            "Z:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.pos.z; }, WHITE },
    { "Link Yaw",           "Player->actor.world.rot.y",            "ROT:",    TYPE_S16,   true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.world.rot.y; }, WHITE },
    { "Link Velocity",      "Player->linearVelocity",               "V:",      TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->linearVelocity; },    WHITE },
    { "Link X Velocity",    "Player->actor.velocity.x",             "XV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.x; },  WHITE },
    { "Link Y Velocity",    "Player->actor.velocity.y",             "YV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.y; },  WHITE },
    { "Link Z Velocity",    "Player->actor.velocity.z",             "ZV:",     TYPE_FLOAT, true,  []() -> void* { return &GET_PLAYER(gPlayState)->actor.velocity.z; },  WHITE },
    { "Text ID",            "play->msgCtx.textId",                  "TEXTID:", TYPE_U16,   true,  []() -> void* { return &gPlayState->msgCtx.textId; },                 WHITE },
    { "Analog Stick X",     "play->state.input->cur.stick_x",       "AX:",     TYPE_S8,    true,  []() -> void* { return &gPlayState->state.input->cur.stick_x; },      WHITE },
    { "Analog Stick Y",     "play->state.input->cur.stick_y",       "AY:",     TYPE_S8,    true,  []() -> void* { return &gPlayState->state.input->cur.stick_y; },      WHITE },
    { "getItemID",          "Player->getItemId",                    "ITEM:",   TYPE_S16,   true,  []() -> void* { return &GET_PLAYER(gPlayState)->getItemId; },         WHITE },
    { "getItemEntry",       "Player->getItemEntry",                 "IE:",     TYPE_S16,   true,  []() -> void* { return &GET_PLAYER(gPlayState)->getItemEntry.itemId; }, WHITE },
};
// clang-format on

extern "C" void ValueViewer_Draw(GfxPrint* printer) {
    for (size_t i = 0; i < valueTable.size(); i++) {
        ValueTableElement& element = valueTable[i];
        if (!element.isActive || !element.isPrinted || (element.requiresPlayState && (!gPlayState || !GET_PLAYER(gPlayState))))
            continue;
        GfxPrint_SetColor(printer, element.color.x * 255, element.color.y * 255, element.color.z * 255,
                          element.color.w * 255);
        GfxPrint_SetPos(printer, element.x, element.y);
        switch (element.type) {
            case TYPE_S8:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%d"), element.prefix.c_str(),
                                *(s8*)element.valueFn());
                break;
            case TYPE_U8:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%u"), element.prefix.c_str(),
                                *(u8*)element.valueFn());
                break;
            case TYPE_S16:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%d"), element.prefix.c_str(),
                                *(s16*)element.valueFn());
                break;
            case TYPE_U16:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%u"), element.prefix.c_str(),
                                *(u16*)element.valueFn());
                break;
            case TYPE_S32:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%d"), element.prefix.c_str(),
                                *(s32*)element.valueFn());
                break;
            case TYPE_U32:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s0x%x" : "%s%u"), element.prefix.c_str(),
                                *(u32*)element.valueFn());
                break;
            case TYPE_CHAR:
                GfxPrint_Printf(printer, "%s%c", element.prefix.c_str(), *(char*)element.valueFn());
                break;
            case TYPE_STRING:
                GfxPrint_Printf(printer, "%s%s", element.prefix.c_str(), (char*)element.valueFn());
                break;
            case TYPE_FLOAT:
                GfxPrint_Printf(printer, (element.typeFormat ? "%s%4.1f" : "%s%f"), element.prefix.c_str(),
                                *(float*)element.valueFn());
                break;
        }
    }
}

extern "C" void ValueViewer_SetupDraw() {
    OPEN_DISPS(gGameState->gfxCtx);

    Gfx* gfx;
    Gfx* polyOpa;
    GfxPrint printer;

    polyOpa = POLY_OPA_DISP;
    gfx = Graph_GfxPlusOne(polyOpa);
    gSPDisplayList(OVERLAY_DISP++, gfx);

    GfxPrint_Init(&printer);
    GfxPrint_Open(&printer, gfx);

    ValueViewer_Draw(&printer);

    gfx = GfxPrint_Close(&printer);
    GfxPrint_Destroy(&printer);

    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(polyOpa, gfx);
    POLY_OPA_DISP = gfx;

    CLOSE_DISPS(gGameState->gfxCtx);
}

void RegisterValueViewerHooks() {
    COND_HOOK(OnGameFrameUpdate, CVAR_VALUE, []() { ValueViewer_SetupDraw(); });
}

static RegisterShipInitFunc initFunc(RegisterValueViewerHooks, { CVAR_NAME });

namespace {
namespace N = NativeOptions;

std::string ValueText(const ValueTableElement& element) {
    if (element.requiresPlayState && (!gPlayState || !GET_PLAYER(gPlayState))) return N::Text("unavailable");
    const void* value = element.valueFn();
    if (!value) return N::Text("unavailable");
    auto integer = [&](auto number) { return element.typeFormat ? fmt::format("0x{:x}", number) : fmt::format("{}", number); };
    switch (element.type) {
        case TYPE_S8: return integer(*static_cast<const s8*>(value));
        case TYPE_U8: return integer(*static_cast<const u8*>(value));
        case TYPE_S16: return integer(*static_cast<const s16*>(value));
        case TYPE_U16: return integer(*static_cast<const u16*>(value));
        case TYPE_S32: return integer(*static_cast<const s32*>(value));
        case TYPE_U32: return integer(*static_cast<const u32*>(value));
        case TYPE_CHAR: return std::string(1, *static_cast<const char*>(value));
        case TYPE_STRING: return static_cast<const char*>(value);
        case TYPE_FLOAT:
            return element.typeFormat ? fmt::format("{:.1f}", *static_cast<const float*>(value)) :
                                        fmt::format("{:f}", *static_cast<const float*>(value));
    }
    return "";
}

N::PagePtr ValuePage(size_t index) {
    return N::MakePage("advanced/value/" + std::to_string(index), valueTable[index].name, [=] {
        auto& element = valueTable[index];
        auto value = N::Action("value", N::Text("value_current"), [] { N::ReadCurrentDescription(); }, element.path);
        value.value = ValueText(element);
        std::vector<N::Row> rows{value};
        if (element.type <= TYPE_U32 || element.type == TYPE_FLOAT)
            rows.push_back(N::Toggle("format", N::Text(element.type == TYPE_FLOAT ? "value_trim" : "value_hex"),
                                    element.typeFormat, [=](bool next) { valueTable[index].typeFormat = next; }));
        if (CVarGetInteger(CVAR_NAME, 0)) {
            rows.push_back(N::Toggle("print", N::Text("value_print"), element.isPrinted, [=](bool next) { valueTable[index].isPrinted = next; }));
            if (element.isPrinted) {
                rows.push_back(N::String("prefix", N::Text("value_prefix"), element.prefix,
                                        [=](std::string next) { valueTable[index].prefix = std::move(next); }, "", 9));
                rows.push_back(N::Integer("x", "X", static_cast<int>(element.x), 0, 44, 1, [=](int next) { valueTable[index].x = next; }));
                rows.push_back(N::Integer("y", "Y", static_cast<int>(element.y), 0, 29, 1, [=](int next) { valueTable[index].y = next; }));
                rows.push_back(N::Link("color", N::Text("color"), [=] {
                    return N::MakePage("advanced/value/color", N::Text("color"), [=] {
                        std::vector<N::Row> components;
                        const char* keys[] = {"red", "green", "blue"};
                        const auto& color = valueTable[index].color;
                        const float values[] = {color.x, color.y, color.z};
                        for (int component = 0; component < 3; ++component)
                            components.push_back(N::Decimal(keys[component], N::Text(keys[component]),
                                values[component], 0, 1, 0.01f, [=](float next) {
                                    auto& color = valueTable[index].color;
                                    if (component == 0) color.x = next;
                                    else if (component == 1) color.y = next;
                                    else color.z = next;
                                }, "", true));
                        return components;
                    });
                }));
            }
        }
        rows.push_back(N::Action("remove", N::Text("remove"), [=] {
            valueTable[index].isActive = valueTable[index].isPrinted = false;
            N::GetModel().Back();
        }));
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_lockout"));
        return rows;
    });
}

N::PagePtr ValueViewerPage() {
    return N::MakePage("advanced/values", N::Text("value_viewer"), [] {
        std::vector<N::Row> rows{
            N::CVarToggle(N::Text("value_printing"), CVAR_NAME),
            N::Link("add", N::Text("value_select"), [] {
                return N::MakePage("advanced/values/add", N::Text("value_select"), [] {
                    std::vector<N::Row> rows;
                    for (size_t index = 0; index < valueTable.size(); ++index) {
                        if (valueTable[index].isActive) continue;
                        rows.push_back(N::Action(std::to_string(index), valueTable[index].name, [=] {
                            valueTable[index].isActive = true;
                            N::GetModel().Back();
                        }, valueTable[index].path));
                    }
                    return rows;
                });
            })
        };
        for (size_t index = 0; index < valueTable.size(); ++index) {
            const auto& element = valueTable[index];
            if (!element.isActive) continue;
            auto row = N::Link(std::to_string(index), element.name, [=] { return ValuePage(index); }, element.path);
            row.value = ValueText(element);
            rows.push_back(row);
        }
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_lockout"));
        return rows;
    });
}
} // namespace

void InitializeValueViewer() {
    NativeOptions::RegisterPage("Value Viewer", ValueViewerPage);
}
