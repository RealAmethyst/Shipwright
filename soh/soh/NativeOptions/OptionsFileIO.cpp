#include "OptionsFileIO.h"

#include <chrono>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace NativeOptions {
std::filesystem::path ReplaceFileWithBackup(const std::filesystem::path& path, const std::string& contents) {
    namespace fs = std::filesystem;
    const auto stamp = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    fs::path staging;
    for (unsigned i = 0; i < 1000; ++i) {
        auto candidate = path;
        candidate += ".saving-" + stamp + "-" + std::to_string(i);
        if (fs::create_directory(candidate)) {
            staging = candidate;
            break;
        }
    }
    if (staging.empty())
        throw std::runtime_error("Could not reserve a save staging directory");
    const auto temporary = staging / "contents";
    fs::path backup;
    try {
        std::ofstream output(temporary, std::ios::binary);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.flush();
        output.close();
        if (fs::exists(path)) {
            backup = staging;
            backup += ".backup";
            fs::copy_file(path, backup);
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw fs::filesystem_error("Could not replace file", temporary, path,
                                       std::error_code(GetLastError(), std::system_category()));
#else
        fs::rename(temporary, path);
#endif
    } catch (...) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        fs::remove(staging, ignored);
        throw;
    }
    std::error_code ignored;
    fs::remove(staging, ignored);
    return backup;
}
}
