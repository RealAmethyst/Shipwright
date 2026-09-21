#include "SpeechSynthesizer.h"
#include <spdlog/spdlog.h>
#ifdef SOH_PRISM
#include <prism.h>
#endif

SpeechSynthesizer::~SpeechSynthesizer() {
    Uninitialize();
}

bool SpeechSynthesizer::Init() {
#ifdef SOH_PRISM
    if (mBackend != nullptr) {
        return true;
    }
    auto config = prism_config_init();
    mContext = prism_init(&config);
    if (mContext != nullptr) {
        // create_best initializes the backend and prefers available screen readers.
        mBackend = prism_registry_create_best(mContext);
    }
    if (mBackend == nullptr) {
        SPDLOG_ERROR("[Speech] Prism could not initialize a speech backend");
        Uninitialize();
        return false;
    }
    SPDLOG_INFO("[Speech] Prism backend: {}", prism_backend_name(mBackend));
    return true;
#else
    return false;
#endif
}

void SpeechSynthesizer::Uninitialize() {
#ifdef SOH_PRISM
    prism_backend_free(mBackend);
    mBackend = nullptr;
    if (mContext != nullptr) {
        prism_shutdown(mContext);
        mContext = nullptr;
    }
#endif
}

bool SpeechSynthesizer::IsInitialized() const {
    return mBackend != nullptr;
}

std::string SpeechSynthesizer::PrepareText(const char* text) {
    std::string result;
    if (text == nullptr) {
        return result;
    }
    // Message controls and glyphs are decoded by Message_TTS_Decode first.
    // Preserve UTF-8 and punctuation; Prism validates UTF-8 at its API boundary.
    bool space = false;
    for (const unsigned char c : std::string(text)) {
        if (c <= ' ') {
            space = !result.empty();
        } else {
            if (space) {
                result += ' ';
                space = false;
            }
            result += static_cast<char>(c);
        }
    }
    return result;
}

void SpeechSynthesizer::Speak(const char* text, const char* language, bool interrupt) {
    const auto prepared = PrepareText(text);
    if (prepared.empty() || !IsInitialized()) {
        return;
    }
#ifdef SOH_PRISM
    // The screen reader owns voice and language preferences.
    const auto result = prism_backend_speak(mBackend, prepared.c_str(), interrupt);
    if (result != PRISM_OK) {
        SPDLOG_ERROR("[Speech] Prism speak failed: {}", prism_error_string(result));
    }
#endif
}

void SpeechSynthesizer::Stop() {
#ifdef SOH_PRISM
    if (mBackend != nullptr) {
        const auto result = prism_backend_stop(mBackend);
        if (result != PRISM_OK && result != PRISM_ERROR_NOT_SPEAKING) {
            SPDLOG_WARN("[Speech] Prism stop failed: {}", prism_error_string(result));
        }
    }
#endif
}
