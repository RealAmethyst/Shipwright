#include "CueMixer.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace SpatialAudio {
namespace {
// Time Stranger's entity gap, boundary fades and destructible beat at 32 kHz.
constexpr size_t LoopGap = 1920, FadeSamples = 64, DestructibleCycle = 12800;
uint32_t Read32(const std::vector<uint8_t>& bytes, size_t offset) {
    return bytes[offset] | (uint32_t(bytes[offset + 1]) << 8) |
           (uint32_t(bytes[offset + 2]) << 16) | (uint32_t(bytes[offset + 3]) << 24);
}
uint16_t Read16(const std::vector<uint8_t>& bytes, size_t offset) {
    return bytes[offset] | (uint16_t(bytes[offset + 1]) << 8);
}
}

bool CueMixer::Load(Cue cue, const std::string& path, std::string& error) {
    const size_t index = static_cast<size_t>(cue);
    if (index >= samples.size()) { error = "Unknown cue"; return false; }
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    const auto length = input.tellg();
    if (length < 44 || length > 32000 * 2 * 60) { error = "Missing or oversized cue WAV"; return false; }
    input.seekg(0);
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) { error = "Unreadable cue WAV"; return false; }
    if (Read32(bytes, 0) != 0x46464952 || Read32(bytes, 8) != 0x45564157 ||
        uint64_t(Read32(bytes, 4)) + 8 != bytes.size()) { error = "Invalid cue RIFF header"; return false; }
    bool format = false;
    size_t data = 0, dataLength = 0;
    for (size_t offset = 12; offset + 8 <= bytes.size();) {
        const uint32_t tag = Read32(bytes, offset), size = Read32(bytes, offset + 4);
        offset += 8;
        if (size > bytes.size() - offset) { error = "Truncated cue WAV chunk"; return false; }
        if (tag == 0x20746d66) {
            format = size >= 16 && Read16(bytes, offset) == 1 && Read16(bytes, offset + 2) == 1 &&
                     Read32(bytes, offset + 4) == 32000 && Read32(bytes, offset + 8) == 64000 &&
                     Read16(bytes, offset + 12) == 2 && Read16(bytes, offset + 14) == 16;
        } else if (tag == 0x61746164) { data = offset; dataLength = size; }
        offset += size + (size & 1);
    }
    if (!format || !data || !dataLength || dataLength % 2) { error = "Cue must be mono PCM16 at 32000 Hz"; return false; }
    auto& pcm = samples[index];
    pcm.resize(dataLength / 2);
    for (size_t f = 0; f < pcm.size(); ++f) pcm[f] = static_cast<int16_t>(Read16(bytes, data + f * 2)) / 32768.0f;
    return true;
}

bool CueMixer::Play(uint64_t identity, Cue cue) {
    const size_t index = static_cast<size_t>(cue);
    if (!identity || index >= samples.size() || samples[index].empty()) return false;
    Voice* voice = nullptr;
    for (auto& candidate : voices) if (candidate.identity == identity) { voice = &candidate; break; }
    if (!voice) for (auto& candidate : voices) if (!candidate.identity) { voice = &candidate; break; }
    if (!voice) return false;
    *voice = {identity, cue, 0, {}, 0};
    return true;
}

bool CueMixer::KeepPlaying(uint64_t identity, Cue cue) {
    if (!identity) return false;
    for (auto& voice : voices) if (voice.identity == identity && voice.cue == cue) {
        voice.repeat = true;
        return true;
    }
    if (!Play(identity, cue)) return false;
    for (auto& voice : voices) if (voice.identity == identity) voice.repeat = true;
    return true;
}

void CueMixer::Update(uint64_t identity, SpatialAudioSource source, float gain) {
    if (!source.identity || !std::isfinite(gain) || gain <= 0) { Stop(identity); return; }
    for (auto& voice : voices) if (voice.identity == identity) {
        voice.source = source;
        voice.gain = std::clamp(gain, 0.0f, 1.0f);
        return;
    }
}
void CueMixer::Stop(uint64_t identity) {
    for (auto& voice : voices) if (voice.identity == identity) voice = {};
}
void CueMixer::StopAll() { voices.fill({}); }
size_t CueMixer::ActiveVoices() const {
    return std::count_if(voices.begin(), voices.end(), [](const Voice& voice) { return voice.identity != 0; });
}
void CueMixer::Render(int frames, float gameGain, void* context, RenderCallback callback) {
    if (!callback || frames < 1 || frames > MaxFrames || !std::isfinite(gameGain)) return;
    std::array<float, MaxFrames> block;
    for (auto& voice : voices) {
        if (!voice.identity) continue;
        const auto& pcm = samples[static_cast<size_t>(voice.cue)];
        const bool wall = voice.repeat && voice.cue >= Cue::WallNorth && voice.cue <= Cue::WallWest;
        const size_t overlap = wall ? std::min(FadeSamples, pcm.size() / 2) : 0;
        const bool destructible = voice.repeat && voice.cue == Cue::Destructible;
        const size_t length = destructible ? std::min(pcm.size(), DestructibleCycle) : pcm.size();
        const size_t gap = destructible ? std::max(size_t{1}, DestructibleCycle - length) : LoopGap;
        const float gain = voice.gain * std::clamp(gameGain, 0.0f, 1.0f);
        block.fill(0);
        for (int f = 0; f < frames; ++f) {
            if (voice.gap) {
                if (--voice.gap == 0) voice.cursor = 0;
                continue;
            }
            if (voice.cursor >= length) {
                if (!voice.repeat) break;
                voice.cursor = overlap;
            }
            float sample = pcm[voice.cursor];
            if (overlap && voice.cursor >= length - overlap) {
                const size_t head = voice.cursor - (length - overlap);
                sample += (pcm[head] - sample) * (head + 1.0f) / overlap;
            } else if (!wall) {
                sample *= std::min(1.0f, voice.cursor / float(FadeSamples));
                sample *= std::min(1.0f, (length - voice.cursor) / float(FadeSamples));
            }
            block[f] = sample * gain;
            ++voice.cursor;
            if (voice.cursor == length && voice.repeat && !wall) voice.gap = gap;
        }
        callback(context, voice.source, block.data(), frames);
        if (voice.cursor == pcm.size() && !voice.repeat) voice = {};
    }
}
}
