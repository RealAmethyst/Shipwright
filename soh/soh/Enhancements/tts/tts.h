#ifndef SOH_TTS_H
#define SOH_TTS_H

void InitTTSBank();
void TTSResumePauseMenu();
void TTSResumeFileSelect();
void TTSSpeakLocalized(const char* key, const char* argument = nullptr, bool interrupt = true);

#endif
