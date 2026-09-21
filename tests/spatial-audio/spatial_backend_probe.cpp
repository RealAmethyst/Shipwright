#include "ship/audio/SpatialAudioPlayer.h"
#include <iostream>
#include <array>
#include <chrono>
#include <thread>
#include <string_view>
int main(int argc, char** argv) {
    Ship::AudioSettings settings{32000, 1024, 1680, audioSpatial714};
    Ship::SpatialAudioPlayer output(settings);
    if (!output.Init()) return 1;
    if (output.Buffered() != 0 || output.GetNumOutputChannels() != 12) return 2;
    if (argc == 2 && std::string_view(argv[1]) == "--silent") {
        std::array<int16_t, 320 * 12> silence{};
        for (int i = 0; i < 100; ++i) {
            output.Play(reinterpret_cast<const uint8_t*>(silence.data()), sizeof(silence));
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::cout << "Silent 7.1.4 stream rendered " << output.GetRenderedFrames() << " frames.\n";
        if (output.GetRenderedFrames() < 24000) return 3;
    } else {
        // No Play call: activate the stream, without starting it.
        std::cout << "Production spatial backend initialized and stopped without playback.\n";
    }
    return 0;
}
