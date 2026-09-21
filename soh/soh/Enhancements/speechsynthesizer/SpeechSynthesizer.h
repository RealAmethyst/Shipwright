#ifndef SOH_SPEECH_SYNTHESIZER_H
#define SOH_SPEECH_SYNTHESIZER_H

#include <string>

struct PrismContext;
struct PrismBackend;

class SpeechSynthesizer {
  public:
    static SpeechSynthesizer* Instance;
    ~SpeechSynthesizer();
    bool Init();
    void Uninitialize();
    void Speak(const char* text, const char* language, bool interrupt = true);
    void Stop();
    bool IsInitialized() const;
    static std::string PrepareText(const char* text);

  private:
    PrismContext* mContext = nullptr;
    PrismBackend* mBackend = nullptr;
};

#endif
