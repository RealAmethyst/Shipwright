#include "MessageViewer.h"

#include "soh/NativeOptions/NativeOptions.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <charconv>
#include <optional>
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "soh/OTRGlobals.h"

#include <textures/message_static/message_static.h>

#include "../custom-message/CustomMessageManager.h"
#include "functions.h"
#include "macros.h"
#include "soh/cvar_prefixes.h"
#include "message_data_static.h"
#include "variables.h"
#include "soh/util.h"

extern "C" u8 sMessageHasSetSfx;

extern "C" MessageTableEntry* sNesMessageEntryTablePtr;
extern "C" MessageTableEntry* sGerMessageEntryTablePtr;
extern "C" MessageTableEntry* sFraMessageEntryTablePtr;
extern "C" MessageTableEntry* sJpnMessageEntryTablePtr;
extern "C" MessageTableEntry* sStaffMessageEntryTablePtr;

static MessageTableEntry* FindMessageEntry(uint16_t textId, uint8_t language) {
    auto* table = language == LANGUAGE_GER ? sGerMessageEntryTablePtr :
                  language == LANGUAGE_FRA ? sFraMessageEntryTablePtr :
                  language == LANGUAGE_JPN ? sJpnMessageEntryTablePtr : sNesMessageEntryTablePtr;
    if (!table) table = sNesMessageEntryTablePtr;
    if (!table) return nullptr;
    for (auto* entry = table; entry->textId != 0xFFFF; ++entry)
        if (entry->textId == textId) return entry;
    return nullptr;
}

namespace {
namespace N = NativeOptions;
constexpr const char* messageViewerTable = "MessageViewer";
static std::string tableId, textId, customText;
static int idBase = 16;
static int language = LANGUAGE_ENG;

bool ParseTextId(uint16_t& id) {
    unsigned int value = 0;
    const auto result = std::from_chars(textId.data(), textId.data() + textId.size(), value, idBase);
    if (result.ec != std::errc{} || result.ptr != textId.data() + textId.size() || value > UINT16_MAX) return false;
    id = static_cast<uint16_t>(value);
    return true;
}

N::PagePtr MessageViewerPage() {
    return N::MakePage("advanced/messages", N::Text("message_viewer"), [] {
        SohGui::SohMenu::UpdateLanguageMap(SohGui::languages);
        std::map<int, std::string> languages(SohGui::languages.begin(), SohGui::languages.end());
        std::vector<N::Row> rows{
            N::String("table", N::Text("message_table"), tableId, [](std::string value) { tableId = std::move(value); },
                      N::Text("message_table_description"), 1023, [](const std::string& value) {
                return std::all_of(value.begin(), value.end(), [](unsigned char c) {
                    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
                }) ? "" : N::Text("message_table_invalid");
            }),
            N::Choice("base", N::Text("message_id_format"), idBase,
                      {{16, N::Text("message_hexadecimal")}, {10, N::Text("message_decimal")}},
                      [](int value) { idBase = value; textId.clear(); }),
            N::String("id", N::Text("message_id"), textId, [](std::string value) { textId = std::move(value); },
                      N::Text(idBase == 16 ? "message_hex_description" : "message_decimal_description"), 5),
            N::Choice("language", N::Text("language"), language, std::move(languages), [](int value) { language = value; })
        };
        uint16_t parsed = 0;
        const bool validId = ParseTextId(parsed);
        auto existing = N::Action("existing", N::Text("message_display_existing"), [parsed] {
            if (MessageDebug_StartTextBox(tableId.c_str(), parsed, language)) N::GetModel().Close();
            else N::Message(N::Text("message_viewer"), N::Text("message_preview_failed"));
        }, N::Text("message_display_description"));
        existing.enabled = validId && gPlayState && GET_PLAYER(gPlayState);
        if (!existing.enabled) existing.disabledReason = validId ? N::Text("save_required") : N::Text("message_id_invalid");
        rows.push_back(existing);
        rows.push_back(N::String("custom", N::Text("message_custom"), customText, [](std::string value) { customText = std::move(value); },
                                 N::Text("message_custom_description"), 1023, {}, false, true));
        auto custom = N::Action("display_custom", N::Text("message_display_custom"), [] {
            auto text = customText;
            std::erase(text, '\n');
            std::erase(text, '\r');
            if (MessageDebug_DisplayCustomMessage(text.c_str())) N::GetModel().Close();
            else N::Message(N::Text("message_viewer"), N::Text("message_preview_failed"));
        }, N::Text("message_display_description"));
        custom.enabled = gPlayState && GET_PLAYER(gPlayState);
        if (!custom.enabled) custom.disabledReason = N::Text("save_required");
        rows.push_back(custom);
        if (CVarGetInteger(CVAR_SETTING("DisableChanges"), 0)) N::Disable(rows, N::Text("race_lockout"));
        return rows;
    });
}
} // namespace

void InitializeMessageViewer() {
    CustomMessageManager::Instance->AddCustomMessageTable(messageViewerTable);
    NativeOptions::RegisterPage("Message Viewer", MessageViewerPage);
}

static const char* msgStaticTbl[] = {
    gDefaultMessageBackgroundTex,
    gSignMessageBackgroundTex,
    gNoteStaffMessageBackgroundTex,
    gFadingMessageBackgroundTex,
    gMessageContinueTriangleTex,
    gMessageEndSquareTex,
    gMessageArrowTex,
};

bool MessageDebug_StartTextBox(const char* tableId, uint16_t textId, uint8_t language) {
    PlayState* play = gPlayState;
    if (!play || !GET_PLAYER(play)) {
        SPDLOG_ERROR("Message preview requires a loaded player");
        return false;
    }
    if (!tableId) return false;
    const auto* entry = strlen(tableId) == 0 ? FindMessageEntry(textId, language) : nullptr;
    if (strlen(tableId) == 0 && (!entry || entry->msgSize > sizeof(play->msgCtx.font.msgBuf))) {
        SPDLOG_ERROR("Message preview could not find a valid message: {}", textId);
        return false;
    }
    std::optional<CustomMessage> customEntry;
    std::string customText;
    if (*tableId) {
        try {
            customEntry.emplace(CustomMessageManager::Instance->RetrieveMessage(tableId, textId));
            customText = customEntry->GetForLanguage(language);
        } catch (const MessageNotFoundException&) {
            SPDLOG_ERROR("Message preview could not find custom message {} in {}", textId, tableId);
            return false;
        }
        if (customText.size() >= sizeof(play->msgCtx.font.msgBuf)) {
            SPDLOG_ERROR("Message preview exceeds the message buffer: {} bytes", customText.size());
            return false;
        }
    }
    static int16_t messageStaticIndices[] = { 0, 1, 3, 2 };
    const auto player = GET_PLAYER(gPlayState);
    player->actor.flags |= ACTOR_FLAG_TALK;
    MessageContext* msgCtx = &play->msgCtx;
    msgCtx->ocarinaAction = 0xFFFF;
    Font* font = &msgCtx->font;
    sMessageHasSetSfx = 0;
    for (u32 i = 0; i < FONT_CHAR_TEX_SIZE * 120; i += FONT_CHAR_TEX_SIZE) {
        gSPInvalidateTexCache(play->state.gfxCtx->polyOpa.p++, reinterpret_cast<uintptr_t>(&font->charTexBuf[i]));
    }
    R_TEXT_CHAR_SCALE = 75;
    R_TEXT_LINE_SPACING = 12;
    R_TEXT_INIT_XPOS = 65;
    if (language == LANGUAGE_JPN) {
        R_TEXT_CHAR_SCALE = 88;
        R_TEXT_LINE_SPACING = 18;
        R_TEXT_INIT_XPOS = 65;
    }
    char* buffer = font->msgBuf;
    msgCtx->textId = textId;
    if (strlen(tableId) == 0) {
        font->charTexBuf[0] = entry->typePos;
        font->msgOffset = reinterpret_cast<uintptr_t>(entry->segment);
        font->msgLength = entry->msgSize;
        msgCtx->msgLength = static_cast<int32_t>(font->msgLength);
        const uintptr_t src = font->msgOffset;
        memcpy(font->msgBuf, reinterpret_cast<void const*>(src), font->msgLength);
    } else {
        constexpr int maxBufferSize = sizeof(font->msgBuf);
        font->charTexBuf[0] = (customEntry->GetTextBoxType() << 4) | customEntry->GetTextBoxPosition();
        font->msgLength =
            SohUtils::CopyStringToCharBuffer(buffer, customText, maxBufferSize);
        msgCtx->msgLength = static_cast<int32_t>(font->msgLength);
    }
    msgCtx->textBoxProperties = font->charTexBuf[0];
    msgCtx->textBoxType = msgCtx->textBoxProperties >> 4;
    msgCtx->textBoxPos = msgCtx->textBoxProperties & 0xF;
    const int16_t textBoxType = msgCtx->textBoxType;
    // "Text Box Type"
    osSyncPrintf("吹き出し種類＝%d\n", msgCtx->textBoxType);
    if (textBoxType < TEXTBOX_TYPE_NONE_BOTTOM) {
        const char* textureName = msgStaticTbl[messageStaticIndices[textBoxType]];
        memcpy(msgCtx->textboxSegment, textureName, strlen(textureName) + 1);
        if (textBoxType == TEXTBOX_TYPE_BLACK) {
            msgCtx->textboxColorRed = 0;
            msgCtx->textboxColorGreen = 0;
            msgCtx->textboxColorBlue = 0;
        } else if (textBoxType == TEXTBOX_TYPE_WOODEN) {
            msgCtx->textboxColorRed = 70;
            msgCtx->textboxColorGreen = 50;
            msgCtx->textboxColorBlue = 30;
        } else if (textBoxType == TEXTBOX_TYPE_BLUE) {
            msgCtx->textboxColorRed = 0;
            msgCtx->textboxColorGreen = 10;
            msgCtx->textboxColorBlue = 50;
        } else {
            msgCtx->textboxColorRed = 255;
            msgCtx->textboxColorGreen = 0;
            msgCtx->textboxColorBlue = 0;
        }
        if (textBoxType == TEXTBOX_TYPE_WOODEN) {
            msgCtx->textboxColorAlphaTarget = 230;
        } else if (textBoxType == TEXTBOX_TYPE_OCARINA) {
            msgCtx->textboxColorAlphaTarget = 180;
        } else {
            msgCtx->textboxColorAlphaTarget = 170;
        }
        msgCtx->textboxColorAlphaCurrent = 0;
    }
    msgCtx->choiceNum = msgCtx->textUnskippable = msgCtx->textboxEndType = 0;
    msgCtx->msgBufPos = msgCtx->unk_E3D0 = msgCtx->textDrawPos = 0;
    msgCtx->talkActor = &player->actor;
    msgCtx->msgMode = MSGMODE_TEXT_START;
    msgCtx->stateTimer = 0;
    msgCtx->textDelayTimer = 0;
    msgCtx->ocarinaMode = OCARINA_MODE_00;
    return true;
}

bool MessageDebug_DisplayCustomMessage(const char* customMessage) {
    CustomMessageManager::Instance->ClearMessageTable(messageViewerTable);
    CustomMessageManager::Instance->CreateMessage(messageViewerTable, 0,
                                                  CustomMessage(customMessage, customMessage, customMessage));
    return MessageDebug_StartTextBox(messageViewerTable, 0, 0);
}
