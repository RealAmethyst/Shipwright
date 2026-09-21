#include <windows.h>
#include <mmdeviceapi.h>
#include <spatialaudioclient.h>
#include <wrl/client.h>
#include <iostream>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

static void Check(HRESULT result, const char* operation) {
    if (FAILED(result)) {
        std::cerr << operation << " failed: 0x" << std::hex << static_cast<unsigned long>(result) << '\n';
        throw std::runtime_error(operation);
    }
}

int main() {
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return 1;
    int result = 0;
    try {
        ComPtr<IMMDeviceEnumerator> enumerator;
        Check(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator)),
              "Create device enumerator");
        ComPtr<IMMDevice> device;
        Check(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device), "Get default output");
        ComPtr<ISpatialAudioClient> client;
        Check(device->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr,
                               reinterpret_cast<void**>(client.GetAddressOf())), "Activate spatial client");
        UINT32 dynamic = 0;
        AudioObjectType mask = AudioObjectType_None;
        Check(client->GetMaxDynamicObjectCount(&dynamic), "Query dynamic objects");
        Check(client->GetNativeStaticObjectTypeMask(&mask), "Query static channels");
        std::cout << "Available dynamic objects: " << dynamic << '\n';
        std::cout << "Static channel mask: 0x" << std::hex << static_cast<unsigned int>(mask) << std::dec << '\n';
        std::cout << "Height channels available: "
                  << ((mask & (AudioObjectType_TopFrontLeft | AudioObjectType_TopFrontRight |
                               AudioObjectType_TopBackLeft | AudioObjectType_TopBackRight)) != 0 ? "yes" : "no") << '\n';
        ComPtr<IAudioFormatEnumerator> formats;
        Check(client->GetSupportedAudioObjectFormatEnumerator(&formats), "Query spatial formats");
        UINT32 count = 0;
        Check(formats->GetCount(&count), "Count spatial formats");
        for (UINT32 i = 0; i < count; ++i) {
            WAVEFORMATEX* format = nullptr;
            Check(formats->GetFormat(i, &format), "Read spatial format");
            std::cout << "Object format: " << format->nSamplesPerSec << " Hz, " << format->nChannels
                      << " channels, " << format->wBitsPerSample << " bits, tag " << format->wFormatTag << '\n';
        }
        WAVEFORMATEX format{WAVE_FORMAT_IEEE_FLOAT, 1, 48000, 48000 * 4, 4, 32, 0};
        Check(client->IsAudioObjectFormatSupported(&format), "Check floating-point format");
        HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        const auto bed = static_cast<AudioObjectType>(0x1ffe); // FL through top-back-right, 7.1.4.
        SpatialAudioObjectRenderStreamActivationParams parameters{&format, bed, 0, 0,
            AudioCategory_GameEffects, event, nullptr};
        PROPVARIANT activation{};
        activation.vt = VT_BLOB;
        activation.blob.cbSize = sizeof(parameters);
        activation.blob.pBlobData = reinterpret_cast<BYTE*>(&parameters);
        ComPtr<ISpatialAudioObjectRenderStream> stream;
        const HRESULT activated = client->ActivateSpatialAudioStream(&activation, IID_PPV_ARGS(&stream));
        stream.Reset();
        CloseHandle(event);
        Check(activated, "Activate 7.1.4 static stream without starting it");
        std::cout << "7.1.4 static stream accepted. No stream was started and no audio was played.\n";
    } catch (const std::exception&) { result = 1; }
    CoUninitialize();
    return result;
}
