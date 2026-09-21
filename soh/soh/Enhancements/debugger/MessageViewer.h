#ifndef CUSTOMMESSAGEDEBUGGER_H
#define CUSTOMMESSAGEDEBUGGER_H
#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * \brief Pulls a message from the specified message table and kicks off the process of displaying that message
 * in a text box on screen.
 * \param tableId the tableId string for the table we want to pull from. Empty string for authentic/vanilla messages
 * \param textId The textId corresponding to the message to display. Invalid vanilla IDs are rejected.
 * \param language The Language to display on the screen.
 */
bool MessageDebug_StartTextBox(const char* tableId, uint16_t textId, uint8_t language);

/**
 * \brief
 * \param customMessage A string using Custom Message Syntax.
 */
bool MessageDebug_DisplayCustomMessage(const char* customMessage);
#ifdef __cplusplus
}

void InitializeMessageViewer();

#endif //__cplusplus
#endif // CUSTOMMESSAGEDEBUGGER_H
